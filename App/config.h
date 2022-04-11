#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/regs/intctrl.h"

#include "flasher.h"
#include "iface.h"

#define USB_IFACE 0

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_EVEN

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define RESET_PIN 2
#define BOOT0_PIN 3

#define UART_READ_OK 0
#define UART_READ_FAIL 1

