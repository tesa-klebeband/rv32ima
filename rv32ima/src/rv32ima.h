#ifndef RV32IMA_H
#define RV32IMA_H

#include <iostream>
#include <cstring>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

#ifndef RAM_SIZE
#define RAM_SIZE 0x4000000
#endif
#ifndef IMAGE_ENTRY
#define IMAGE_ENTRY 0x80000000
#endif
#ifndef RAM_START
#define RAM_START 0x80000000
#endif
#ifndef UART_BASE
#define UART_BASE 0x10000000
#endif

class UART {
    public:
        void write_char(char character);
        char read_char();

    private:
        char uart_buffer[256];
        int uart_buffer_index = 0;
};

class RAM {
    public:
        void initialize(char *filename);
        uint8_t *memory;
};

class BUS {
    public:
        uint32_t read(uint32_t addr, uint32_t *exception);
        void write(uint32_t addr, uint32_t data, uint8_t size, uint32_t *exception);
        RAM ram;

    private:
        UART uart;
};

class CPU {
    public:
        void initialize();
        void cycle(BUS *bus);
        uint32_t exception;

    private:
        int32_t x[32];
        uint32_t csr[4096];
        uint32_t reserved;
        uint32_t pc;
};

class RISC {
    public:
        CPU cpu;
        BUS bus;
        void initialize(char *filename);
        void run();
};

#endif