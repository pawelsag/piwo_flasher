#include "flasher.h"
#include "config.h"

//------------- prototypes -------------//
static void cdc_task(void);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  init_uart();
  init_gpio();

  tusb_init();

  while (1)
  {
    tud_task(); // tinyusb device task
    cdc_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
static void cdc_task(void)
{
  uint8_t itf;

  if ( tud_cdc_n_available(itf) )
  {
    uint8_t buf[64];

    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

    // echo back to both serial ports
    handle_command(buf,count);
  }
}

