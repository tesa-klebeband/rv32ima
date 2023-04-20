typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

#define UART_BASE 0x10000000

void print_hex(uint32_t val)
{
    uint8_t *p = (uint8_t*) UART_BASE;
    
    for (int i = 0; i < 8; i++) {
        uint8_t digit = (val >> ((7 - i) * 4)) & 0xF;
        *p = digit + (digit < 0xA ? '0' : 'A' - 0xA);
    }

}

void print_char(char c)
{
    uint8_t *p = (uint8_t*) UART_BASE;
    *p = c;
}

int print_string(char* buffer) {
    uint8_t *p = (uint8_t*) UART_BASE;
    int i = 0;
    while (buffer[i] != 0) {
        *p = buffer[i];
        i++;
    }
    return i;
}

void exception_handler()
{
    uint32_t cause;
    uint32_t epc;
    asm("csrr %0, mcause"
        : "=r" (cause)
        :
        :);
    asm("csrr %0, mepc"
        : "=r" (epc)
        :
        :);

    print_string("Exception: cause=0x");
    print_hex(cause);
    print_string(" addr=0x");
    print_hex(epc);
    print_char('\n');
    asm("mret");
}

int main(void)
{
    asm("la a0, %0"
        :
        : "X" (&exception_handler)
        : "a0");
    asm("csrrw x0, 0x305, a0");
    print_string("Hello World! Testing exceptions...\n");
    asm ("ecall");
    print_string("Exception handler returned!\n");
    int i = 45;
    int u = 32;
    print_string("Testing 'm' extension: 45*32=0x");
    print_hex(i * u);
    print_string(" 45/32=0x");
    print_hex(i / u);
    print_string(" 45%32=0x");
    print_hex(i % u);
    print_string("\nTesting UART input...");

    uint8_t *p = (uint8_t*) UART_BASE;

    uint8_t c = 0;
    while(1) {
        print_string("\nEnter input over UART: ");
        while (c == 0) c = *p;
        print_string("-> ");
        while (c != 0) {
            print_char(c);
            c = *p;
        }
    }
}