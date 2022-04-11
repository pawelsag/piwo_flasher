#include"iface.h"

#define WEAK __attribute__((weak))

WEAK void init_uart()
{
}

WEAK void init_gpio()
{
}

WEAK void on_uart_rx()
{
}

WEAK void gpio_set(int pin, bool state)
{
}

WEAK void sleep(unsigned int ms)
{
}

WEAK bool usb_write(const uint8_t *write, uint32_t size)
{
  return 0;
}

WEAK bool uart_write(const uint8_t *write, uint32_t size)
{
  return 0;
}
