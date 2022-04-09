#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    setup_default_uart();
    printf("dd = Hello, world!\n");
    return 0;
}
