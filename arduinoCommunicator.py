import serial
import threading

from time import sleep
from random import randint
import json

comm = serial.Serial(
    port="/dev/tty.usbmodem1411",
    baudrate=115200,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=1
)


def readFromArduino():
    recv = comm.readline()
    if recv:
        try:
            obj = json.loads(recv.decode())
            print("A ->", obj)
            return obj
        except json.decoder.JSONDecodeError:
            print("Can't decode: ", repr(recv))
            return None

def writeToArduino(identifier, brightness, powerOn=None):

    if identifier is None:
        return

    if brightness is None or brightness < 0 or brightness > 100:
        return

    if powerOn is None:
        powerOn = 1 if brightness > 0 else -1

    jsonString = json.dumps({
        "identifier": identifier,
        "brightness": brightness,
        "power": powerOn
    })

    print("P <- ", jsonString)
    jsonString += "\n"
    comm.write(jsonString.encode())

########################################################################

writer = None
reader = None

terminated = False

def readContinously():
    while not terminated:
        readFromArduino()

def writeRandoms():
    while not terminated:
        intensity = randint(0, 10) * 10
        writeToArduino(1, intensity)
        sleep(5)

def main():
    readFromArduino()

    writer = threading.Thread(target=writeRandoms)
    reader = threading.Thread(target=readContinously)

    writer.start()
    reader.start()


########################################################################
if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        terminated = True
        writer.join()
        reader.join()
        print("Bye :)")
