#include <stdint.h>

volatile uint32_t * const UART0DR = (uint32_t *)(0x101f1000);
volatile uint32_t * const UART0FR = (uint32_t *)(0x101f1000 + 0x18);



enum {
    RXFE = 0x10,
    TXFF = 0x20,
    TXFE = 0x80
};

void send_char(char c)
{
    while(*UART0FR & TXFF);
    *UART0DR = c;
}

void print_uart0(const char *s) {
    while(*s != '\0') { /* Loop until end of string */
        send_char(*s);
        s++; /* Next char */
    }
}

void c_entry() {
    if (*UART0FR & TXFE) {
      print_uart0("TXFE 1\r\n");
    }
    else {
      print_uart0("TXFE 0\r\n");
    }

    if (*UART0FR & RXFE) {
      print_uart0("RXFE 1\r\n");
    }
    else {
      print_uart0("RXFE 0\r\n");
    }
    print_uart0("Hello world!\r\n");

    while(1) {
        if ((*UART0FR & RXFE) == 0) {
            char c = (char)(*UART0DR);
            send_char(c);
            if (c == '\r') {
      	        send_char('\n');
            }
        }
    }
}
