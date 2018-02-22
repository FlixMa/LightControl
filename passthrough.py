import serial
import socket
import threading
from time import sleep

arduino = serial.Serial()
arduino.port = "/dev/tty.usbmodem1411"
arduino.baudrate = 115200
arduino.bytesize = serial.EIGHTBITS
arduino.parity = serial.PARITY_NONE
arduino.stopbits = serial.STOPBITS_ONE
arduino.timeout = 1


HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 12345              # Arbitrary non-privileged port
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.settimeout(5.0)
server.bind((HOST, PORT))
server.listen(1)

nodejs = None

def normalizeJSON(jsonString):
    if type(jsonString) == bytes:
        jsonString = jsonString.decode()

    # strip all carriage returns
    jsonString.replace("\r", "")

    index = jsonString.find("\n")
    if index == -1:
        # a newline is vital for the arduino
        jsonString += "\n"
    else:
        # trim the string to the first newline (arduino only looks for this part)
        jsonString[:jsonString.find("\n") + 1]

    return jsonString

def readFromArduino():
    data = arduino.readline()
    if data:
        return data.decode()
    return None

def writeToArduino(jsonString):
    if not arduino.isOpen():
        return False

    jsonString = normalizeJSON(jsonString)
    # lets encode the string in bytes and write it via serial connection
    arduino.write(jsonString.encode())
    return True

def readFromNode():
    data = nodejs.recv(2048)
    if data:
        return data
    return None

def writeToNode(jsonString):
    jsonString = normalizeJSON(jsonString)
    nodejs.sendall(jsonString.encode())


########################################################################

arduinoToNode = None
nodeToArduino = None


def connectToArduino(stopEvent):
    while not stopEvent.is_set() and not arduino.isOpen():
        try:
            arduino.open()
            break
        except serial.SerialException:
            sleep(0.2)
    print("Successfully connected to Arduino:", readFromArduino())

def listenForNode(stopEvent):
    global nodejs
    print("Listening for NodeJS Client.")

    while not stopEvent.is_set():
        try:
            nodejs, addr = server.accept()
            print("Client connected from", addr)
        except socket.timeout:
            pass

def fromArduinoToNode(stopEvent):
    connectToArduino(stopEvent)

    while not stopEvent.is_set():
        try:

            jsonString = readFromArduino()
            if jsonString is not None:
                print("A ->", jsonString)
                writeToNode(jsonString)

        except serial.SerialException:
            print("Arduino Connection Reset.")
            arduino.close()
            connectToArduino(stopEvent)

    arduino.close()
    print("Connection to Arduino closed.")

def fromNodeToArduino(stopEvent):
    listenForNode(stopEvent)

    while not stopEvent.is_set():
        jsonString = readFromNode()
        if jsonString is not None:
            writeToArduino(jsonString)
        else:
            listenForNode(stopEvent)

    if nodejs is not None:
        nodejs.close()
        print("Connection to NodeJS closed.")

def main():

    stopEvent = threading.Event()

    arduinoToNode = threading.Thread(target=fromArduinoToNode, args=(stopEvent,))
    nodeToArduino = threading.Thread(target=fromNodeToArduino, args=(stopEvent,))

    arduinoToNode.start()
    nodeToArduino.start()

    try:
        while True:
            sleep(0.1)
    except KeyboardInterrupt:
        print("\nAttempting to close Passthrough Server!")
        stopEvent.set()
        print("Waiting for Arduino to close.")
        arduinoToNode.join()
        print("Waiting for NodeJS to close.")
        nodeToArduino.join()

        print("\nBye :)")


########################################################################
if __name__ == "__main__":
    main()
