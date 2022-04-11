#include"iface.h"
#include "config.h"
#include "ring_buffer.hpp"

ring_buffer<100, uint8_t> rx_queue;

void init_uart()
{
   // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 9600);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);
}

void init_gpio()
{
  gpio_init(RESET_PIN);
  gpio_set_dir(RESET_PIN, GPIO_OUT);
  gpio_put(RESET_PIN, 1);

  gpio_init(BOOT0_PIN);
  gpio_set_dir(BOOT0_PIN, GPIO_OUT);
  gpio_put(BOOT0_PIN, 0);
}

void on_uart_rx() {
    if (uart_is_readable(UART_ID)) {
        // add bytes to ring buffer
        rx_queue.push_no_wait(uart_getc(UART_ID));
    }
}

void gpio_set(int pin, bool state)
{
  gpio_put(pin, state);
}

void sleep(unsigned int ms)
{
  sleep_ms(ms);
}

bool usb_write(const uint8_t *data, uint32_t size)
{
  const int attempts = 10;
  int attempt = 0;

  tud_cdc_n_write(USB_IFACE, data, size);
  do {
    tud_cdc_n_write_flush(USB_IFACE);
    tud_task();
    attempt++;
  } while(tud_cdc_n_write_available(USB_IFACE) > 0 && attempt < attempts);
  return attempt != attempts;
}

bool uart_write(const uint8_t *data, uint32_t size)
{
  uart_write_blocking(UART_ID, data, size);
  return true;
}

