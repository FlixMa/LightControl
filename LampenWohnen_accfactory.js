var Accessory = require('../').Accessory;
var Service = require('../').Service;
var Characteristic = require('../').Characteristic;
var uuid = require('../').uuid;

const net = require('net');

// - - - - - - - - Wohnzimmerlampen - - - - - - - - \\

var sock = new net.Socket();
sock.setEncoding('utf8');
sock.on("connect", function() {
    // when connection is established
    console.log("Socket for lights opened.");
});

sock.on("data", function (data) {
    // when data is received
    console.log("Socket for lights received data.");

    try {
    	var command = data;

	    var lines = data.split("\n");
	    if (lines.length > 1) {

		    command = lines[lines.length - 1];

		    if (command === "") {
		    	command = lines[lines.length - 2];
		    }
	    }
	    
	    var state = JSON.parse(command);
	    console.log("-> ", state);

	    if (state.hasOwnProperty("identifier")) {
	        // get the right accessory
	        updateAccessory(
	            state.identifier,
	            state.isPoweredOn || false,
	            state.brightness || 0
	        );
	    }

    } catch(err) {
    	console.log("Error while parsing arduino state:", err);
    }


});

sock.on("error", function (error) {

    console.log("Socket for lights issued error:", error);

});

sock.on("close", function () {
    // Reinitiate the connection.

    console.log("Socket for lights closed.");

    setTimeout(10000, connect);

});


function connect() {
	sock.connect(12345, "192.168.228.238");	
};
connect();

function updateAccessory(identifier, isPoweredOn, brightness) {

    var acc = accessories.find(function (accessory) {
        return accessory.FAKE_ACCESSORY.arduinoIdentifier == identifier;
    });
    if (acc != undefined) {
        var light = acc.FAKE_ACCESSORY;

        // update its values
        light.isPoweredOn = isPoweredOn;
        light.brightness = brightness;

        // broadcast the update
        acc.getService(Service.Lightbulb)
            .setCharacteristic(Characteristic.On, light.isPoweredOn);
        acc.getService(Service.Lightbulb)
            .setCharacteristic(Characteristic.Brightness, light.brightness);

        console.log("Updated Light with ID ", identifier);
    }
}

function setBrightness(identifier, brightness) {
    if (identifier == undefined || brightness == undefined) {
        console.log(
            "WARNING: While setting brightness of arduino lights:\
            identifier or brightness is undefined!"
        );
        return;
    }
    var data = {
        "identifier": identifier,
        "brightness": brightness
    };

    var json = JSON.stringify(data);

    sock.write(json);
}

function setPower(identifier, power) {
	if (identifier == undefined || power == undefined) {
        console.log(
            "WARNING: While setting power of arduino lights:\
            identifier or power is undefined!"
        );
        return;
    }
    var data = {
        "identifier": identifier,
        "power": power
    };

    var json = JSON.stringify(data);

    sock.write(json);
}

function fakeLight(displayName, mac, arduinoIdentifier) {

    var FAKE_ACCESSORY = {

        //GENERAL ATTRIBUTES
        displayName : "Stehlampe " + displayName,   //Name to be displayed in the UI
        username    : mac,                          //Identifies this device - has to be consistent over reboots - has to be unique
        pincode     : "031-45-154",                 //Pincode to enter when pairing


        //DEVICE INFORMATION

        manufacturer    : "Maaßis",
        model           : "Rev-1",
        serialNumber    : "04D375F45E7",

        // Arduino
        arduinoIdentifier: arduinoIdentifier,

        //PHYSICAL ATTRIBUTES - override those

        brightness : 0,
        isPoweredOn: false

    }

    var acc = exports.accessory = new Accessory(FAKE_ACCESSORY.displayName, uuid.generate("dev.maass28."+ FAKE_ACCESSORY.username));

    acc.FAKE_ACCESSORY = FAKE_ACCESSORY;

    //MARK: - GENERAL SETUP

    // Add properties for publishing (not neccessary if using Bridge)
    acc.username    = FAKE_ACCESSORY.username;
    acc.pincode     = FAKE_ACCESSORY.pincode;

    // Set device information
    acc.getService(Service.AccessoryInformation)
        .setCharacteristic(Characteristic.Manufacturer, FAKE_ACCESSORY.manufacturer)
        .setCharacteristic(Characteristic.Model,        FAKE_ACCESSORY.model)
        .setCharacteristic(Characteristic.SerialNumber, FAKE_ACCESSORY.serialNumber);

    // listen for the "identify" event for this Accessory
    acc.on('identify', function(paired, callback) {
        console.log("Identify the Light!");
        FAKE_ACCESSORY.identify();
        callback(); // success
    });




    //MARK: - SERVICE AND CHARATERISTICS SETUP


    //STEP BY STEP - MANUAL
    // 1. Have a look at the available Services and Characteristics in "lib/gen/HomeKitTypes.js"
    // 2. Search in the "Services"-Section for the Service, your device should simulate (eg Lightbulb)
    //      - There is a list of required Characteristics -> you have to implement them (eg On)
    //      - And then there is a list of Characteristics which are optional            (eg Brightness)
    // - - - - -
    // 2.1. Add the Service to your accessory (addService)
    // 2.2. Add the requiered Characteristics (getCharacteristic) and implement those methods (eg "set" and "get")
    // 2.3. Probably you'll add some of the optional ones. Do so by using setCharacteristic and implement its method
    // - - - - -
    // 3. You're ready to go! Have fun :)


    // Lightbulb is on

    acc.addService(Service.Lightbulb)
        .getCharacteristic(Characteristic.On)
        .on('get', function (callback) {
            var value = FAKE_ACCESSORY.isPoweredOn;

            console.log("The light is " + value ? "on": "off" + ".");
            callback(null, value);
        });

    acc.getService(Service.Lightbulb)
        .getCharacteristic(Characteristic.On)
        .on('set', function (value, callback) {
            console.log("Turning the light " + value ? "on": "off" + ".");

            setPower(FAKE_ACCESSORY.arduinoIdentifier, value ? 1 : -1);

            callback();
        });

    // Brightness
    acc.getService(Service.Lightbulb)
        .getCharacteristic(Characteristic.Brightness)
        .on('get', function (callback) {
            var value = FAKE_ACCESSORY.brightness;
            console.log("The light is at " + value + "% brightness.");
            callback(null, value);
        });

    acc.getService(Service.Lightbulb)
        .getCharacteristic(Characteristic.Brightness)
        .on('set', function (value, callback) {
            console.log("Setting brightness to " + value + "%.");

            setBrightness(FAKE_ACCESSORY.arduinoIdentifier, value);

            callback();
        });

    return acc;
}


var accessories = [
    fakeLight("alle",   "C0:A0:CA:54:F2:E0",    0),
    fakeLight("Papa",   "C1:A1:CB:54:F2:E1",    1),
    fakeLight("Mama",   "C2:A2:CC:54:F2:E2",    2),
    fakeLight("Tisch",  "C3:A3:CD:54:F2:E3",    3)
];

module.exports = accessories;
