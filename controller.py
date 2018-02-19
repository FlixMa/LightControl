import serial
from time import sleep
from random import randint

comm = serial.Serial(port="/dev/tty.usbmodem1411", baudrate=115200, bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE, timeout=1)

i = 0

while True:
    # wait until arduino is ready
    #print("Waiting for Arduino")
    recv = comm.readline()
    if recv:
        print("A ->", str(recv))

    #print("Writing...")
    number = str(i)
    print("P <- ", number)
    comm.write(number.encode())

    if i >= 100:
        i = 0
    else:
        i += 1
