#pragma once
#include<stdint.h>

#define STM32_ACK	0x79
#define STM32_NACK 0x1F
#define STM32_CMD_INIT 0x7F
#define STM32_CMD_GET	0x00

typedef struct  {
        uint32_t bl_version;
        uint32_t version;
        uint8_t get;
        uint8_t gvr;
        uint8_t gid;
        uint8_t rm;
        uint8_t go;
        uint8_t wm;
        uint8_t er;
        uint8_t wp;
        uint8_t uw;
        uint8_t rp;
        uint8_t ur;
        uint8_t ch; // ony with firmware > 3.3
}stm32_config;

void handle_command(uint8_t *data, uint32_t size);
void usb_transmit_msg(const char *format, ...);
stm32_config get_config();
