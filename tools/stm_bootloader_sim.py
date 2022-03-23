import serial

STM_ACK=0x79
STM_NACK=0x1F

with serial.Serial('/dev/ttyUSB0', 115200) as s:
    cmd = s.read()
    print("Received {}", cmd)
    s.write(STM_ACK)
