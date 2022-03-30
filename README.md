# Project P.I.W.O flasher

## Introduction
This project aims to deliver firmware and software that allows to
flash the binary to stm32xxx device that supports bootloader uart prtocol.
The document with stm32 flash over uart protocol was specified [here](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=&cad=rja&uact=8&ved=2ahUKEwiB4p6m6-72AhUGzYsKHYMeAecQFnoECAYQAQ&url=https%3A%2F%2Fwww.st.com%2Fresource%2Fen%2Fapplication_note%2Fcd00264342-usart-protocol-used-in-the-stm32-bootloader-stmicroelectronics.pdf&usg=AOvVaw1ZYTQrKmPBWUqBUq_7k3O2)
Basically, the project delivers the usb --- uart converter, the app that allows to flash the binary usning the converter and the simulator that simplifies the debuging process.
## Requirements
### HW
To flash device you will need some HW. The firmware currently supports:
 - stm32f303
 - stm32f103
But you can easly add another boards, the process will be descrived later.
However there are some limitations, cause the MCU has to support 3 peripherals:
 - USB
 - UART
 - 2 x GPIO
### Software
To build the software you will need the [spdlog](https://github.com/gabime/spdlog) installed.
The pkg-config is also required. On ubuntu it will be
```
sudo apt-get install -y pkg-config
```

## Project structure

### Firmware
The firmware is placed in the main directory. The platform specific files are located in the stm32xxxxx directories.
To build the project we have to select the board that will be using as our reference when we are selecting the device on which
we will operate. The App directory contains the files that are hw agnostic and provide the interface for the given platform.
The cmake directory contains toolchains for all supported boards.
Proto directory contains the protocol that is shared between the firmware and software and connects both parts.
### Software
The software that can be used for flashing is placed in the `flash_stm` directory. It allows to chose the binary and send it
through the usb'uart channel directly to the targeted board.
### Simulator
Simulator is placed in the tools directory
