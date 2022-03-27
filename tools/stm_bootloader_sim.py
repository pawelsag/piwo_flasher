import serial

STM_ACK=0x79
STM_NACK=0x1F

bootloader_ver=0x20
bootloader_flash_size = 62000 #62KB
flash_memory = [0]*bootloader_flash_size
flash_mem_base = 0x80000000

with serial.Serial('/dev/ttyUSB0', 115200) as s:
    while True:
        # received init
        init = s.read()
        if init[0] != 0x7F:
            print("Incorrect init byte, {:02X} != 0x7F".format(init[0]))
            exit(1)
        s.write(bytearray([STM_ACK]))

        while True:
            # get command to decode
            cmd = s.read(2)
            print("Received command {:x} {:x}".format(cmd[0], cmd[1]))

            if cmd[0] == 0x0 and cmd[1] == 0xff:
                # get cmd handle
                COMMANDS_CNT = 12
                s.write(bytearray([STM_ACK]))
                s.write(bytearray([COMMANDS_CNT]))
                GET_RESPONSE = bytearray([bootloader_ver, 0x0, 0x1, 0x2, 0x11, 0x21, 0x31, 0x43, 0x63, 0x73, 0x82, 0x92])
                s.write(GET_RESPONSE)
                s.write(bytearray([STM_ACK]))

            elif cmd[0] == 0x31 and cmd[1] == 0xce:
                s.write(bytearray([STM_ACK]))
                addr = s.read(4)
                addr_chksum = s.read(1)
                addr_rev = addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3]
                addr_chksum_gen = addr[0] ^ addr[1] ^ addr[2] ^ addr[3]
                print("Received address 0x{:x}, addr cksum {:x}, addr chksum calculated {:x}".format(addr_rev, addr_chksum[0], addr_chksum_gen))
                if addr_chksum[0] != addr_chksum_gen:
                    print("Invalid address checksum")
                    s.write(bytearray([STM_NACK]))
                    continue

                s.write(bytearray([STM_ACK]))
                number_of_bytes = s.read(1)
                program_piece = s.read(number_of_bytes[0]+1);
                program_chksum_rec = s.read(1)
                mem_off = addr_rev - flash_mem_base

                i=0
                program_chksum_gen = 0x0
                print()
                for b in program_piece:
                    flash_memory[i+mem_off] = b
                    program_chksum_gen = program_chksum_gen ^ b

                if program_chksum_rec[0] != program_chksum_gen:
                    print("Invalid payload checksum {:x} != {:x}".format(program_chksum_gen, program_chksum_rec[0]))
                    s.write(bytearray([STM_NACK]))
                    s.write(bytearray([STM_NACK]))
                    continue

                s.write(bytearray([STM_ACK]))
                print("{} bytes flashed on address = {:x}".format(number_of_bytes[0]+1, addr_rev))

            elif cmd[0] == 0x43 and cmd[1] == 0xbc:
                s.write(bytearray([STM_ACK]))
                num_of_pages = s.read(2)
                if num_of_pages[0] == 0xff and num_of_pages[1] == 0x0:
                    flash_memory = [0]*bootloader_flash_size
                    s.write(bytearray([STM_ACK]))
                else: # only all pages erase command supported
                    s.write(bytearray([STM_NACK]))
            elif cmd[0] == 0x3 and cmd[1] == 0xfc: # custom reset bootloader command
                print("Reset done, exitng")
                break

            else:
                s.write(bytearray([STM_NACK]))


