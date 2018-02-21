import serial
import socket
import threading
from time import sleep

arduino = serial.Serial(port="/dev/tty.usbmodem1411", baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)


HOST = ''                 # Symbolic name meaning all available interfaces
PORT = 12345              # Arbitrary non-privileged port
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen(1)

nodejs = None

def startListening():
    global nodejs
    nodejs, addr = server.accept()
    print("Client connected from", addr)

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
    jsonString = normalizeJSON(jsonString)
    # lets encode the string in bytes and write it via serial connection
    arduino.write(jsonString.encode())

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

terminated = False

def fromArduinoToNode():
    while not terminated:
        jsonString = readFromArduino()
        if jsonString is not None:
            print("A ->", jsonString)
            writeToNode(jsonString)

def fromNodeToArduino():
    while not terminated:
        jsonString = readFromNode()
        if jsonString is not None:
            writeToArduino(jsonString)
        else:
            startListening()

def main():
    readFromArduino()
    startListening()

    sleep(1)

    arduinoToNode = threading.Thread(target=fromArduinoToNode)
    nodeToArduino = threading.Thread(target=fromNodeToArduino)

    arduinoToNode.start()
    nodeToArduino.start()


########################################################################
if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        terminated = True
        arduinoToNode.join()
        nodeToArduino.join()
        print("Bye :)")
