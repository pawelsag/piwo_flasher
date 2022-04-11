#pragma once
#include<stdint.h>
#include <stdbool.h>

// init uart interface
void init_uart();
// init gpio interface
void init_gpio();

// function called on rx irq
void on_uart_rx();

void gpio_set(int pin, bool state);
void sleep(unsigned int ms);

bool usb_write(const uint8_t *write, uint32_t size);
bool uart_write(const uint8_t *write, uint32_t size);
