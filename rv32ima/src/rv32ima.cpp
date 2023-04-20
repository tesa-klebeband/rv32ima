#include "rv32ima.h"

void RISC::initialize(char *filename)
{
    bus.ram.initialize(filename);
    cpu.initialize();
}

void RISC::run()
{
    while(1)
    {
        cpu.cycle(&bus);
    }
}

void UART::write_char(char character)
{
    fprintf(stderr, "%c", character);
}

char UART::read_char()
{
    if (uart_buffer_index > 0) {
        char c = uart_buffer[0];
        memmove(uart_buffer, uart_buffer+1, uart_buffer_index-1);
        uart_buffer_index--;
        return c;
    }
    
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    char c = '\0';
    int n = read(STDIN_FILENO, &c, 1);
    if (n == 1) {
        if (c == '\n') {
            uart_buffer[uart_buffer_index] = '\0';
            uart_buffer_index = 0;
        } else {
            uart_buffer[uart_buffer_index] = c;
            uart_buffer_index++;
        }
    }
    
    if (uart_buffer_index > 0) {
        char c = uart_buffer[0];
        memmove(uart_buffer, uart_buffer+1, uart_buffer_index-1);
        uart_buffer_index--;
        return c;
    }
    
    return '\0';
}

void RAM::initialize(char *filename) {
    memory = (uint8_t*) malloc(RAM_SIZE);

    std::ifstream image(filename);
    if (!image.is_open())
    {
        fprintf(stderr, "Error: could not open image\n");
        exit(1);
    }
    image.seekg(0, std::ios::end);
    size_t length = image.tellg();
    image.seekg(0, std::ios::beg);
    image.read((char*) memory, length);
}

uint32_t BUS::read(uint32_t addr, uint32_t *exception)
{
    if (addr < (RAM_START + RAM_SIZE)) {
        if (addr >= RAM_START) {
            return ram.memory[addr - RAM_START] |
                    (ram.memory[addr - RAM_START + 1] << 8) |
                    (ram.memory[addr - RAM_START + 2] << 16) |
                    (ram.memory[addr - RAM_START + 3] << 24);
        } else {
            if (addr == UART_BASE) {
                return uart.read_char();
            } else {
                *exception = 5;
            }
        }
    } else {
        *exception = 5;
    }
    return 0;
}

void BUS::write(uint32_t addr, uint32_t data, uint8_t size, uint32_t *exception)
{
    if (addr < (RAM_START + RAM_SIZE)) {
        if (addr >= RAM_START) {
            for (uint8_t i = 0; i < size; i++) {
                    ram.memory[addr - RAM_START + i] = data >> (i * 8);
                }
        } else {
            if (addr == UART_BASE) {
                uart.write_char(data);
            } else {
                *exception = 6;
            }
        }
    } else {
        *exception = 6;
    }
}

void CPU::initialize()
{
    memset(x, 0, 128);
    memset(csr, 0, 16384);
    x[2] = RAM_START + RAM_SIZE - 4;
    csr[0x301] = 0x1100;
    csr[0xF11] = 0x01234567;
    pc = IMAGE_ENTRY;
}

void CPU::cycle(BUS *bus)
{
    x[0] = 0;

    uint32_t instruction = bus->read(pc, &exception);
    exception = 0;

    int32_t *rd = &x[(instruction >> 7) & 0x1F];
    int32_t *rs1 = &x[(instruction >> 15) & 0x1F];
    int32_t *rs2 = &x[(instruction >> 20) & 0x1F];

    switch (instruction & 0x7F)
    {
        case 0b0110111:
        {
            *rd = instruction & ~0xFFF;
            break;
        }
        case 0b0010111:
        {
            *rd = pc + (instruction & ~0xFFF);
            break;
        }
        case 0b0010011:
        {
            uint32_t uimm = instruction >> 20;
            int32_t imm = (uimm & 0x800) ? uimm | ~0xFFF : uimm;
            uint8_t shamt = (instruction >> 20) & 0x1F;
            switch ((instruction >> 12) & 0x7)
            {
                case 0b000: *rd = *rs1 + imm; break;
                case 0b010: *rd = (*rs1 < imm) ? 1 : 0; break;
                case 0b011: *rd = ((uint32_t) *rs1 < (uint32_t) imm) ? 1 : 0; break;
                case 0b100: *rd = *rs1 ^ imm; break;
                case 0b110: *rd = *rs1 | imm; break;
                case 0b111: *rd = *rs1 & imm; break;
                case 0b001: *rd = *rs1 << shamt; break;
                case 0b101: *rd = (instruction >> 27) ? *rs1 >> shamt : (uint32_t) *rs1 >> shamt; break;
                default: exception = 2; break;
            }
            break;
        }
        case 0b0110011:
        {
            if ((instruction >> 25) & 1) {
                switch ((instruction >> 12) & 0x7)
                {
                    case 0b000: *rd = *rs1 * *rs2; break;
                    case 0b001: *rd = ((int64_t)((int32_t) *rs1) * (int64_t)((int32_t) *rs2)) >> 32; break;
                    case 0b010: *rd = ((int64_t)((int32_t) *rs1) * (uint64_t) *rs2) >> 32; break;
                    case 0b011: *rd = ((uint64_t) *rs1 * (uint64_t) *rs2) >> 32; break;
                    case 0b100: *rd = (*rs2 == 0) ? -1 : ((int32_t) *rs1 == INT32_MIN && (int32_t) *rs2 == -1) ? *rs1 : ((int32_t) *rs1 / (int32_t) *rs2); break;
                    case 0b101: *rd = (*rs2 == 0) ? -1 : *rs1 / *rs2;
                    case 0b110: *rd = (*rs2 == 0) ? *rs1 : ((int32_t) *rs1 == INT32_MIN && (int32_t) *rs2 == -1) ? 0 : ((uint32_t)((int32_t) *rs1 % (int32_t) *rs2)); break;
                    case 0b111: *rd = (*rs2 == 0) ? *rs1 : *rs1 % *rs2; break;
                }
            } else {
                switch ((instruction >> 12) & 0x7)
                {
                    case 0b000: *rd = (instruction >> 27) ? *rs1 - *rs2 : *rs1 + *rs2; break;
                    case 0b001: *rd = *rs1 << *rs2; break;
                    case 0b010: *rd = (*rs1 < *rs2) ? 1 : 0; break;
                    case 0b011: *rd = ((uint32_t) *rs1 < (uint32_t) *rs2) ? 1 : 0; break;
                    case 0b100: *rd = *rs1 ^ *rs2; break;
                    case 0b101: *rd = (instruction >> 27) ? *rs1 >> *rs2 : (uint32_t) *rs1 >> *rs2; break;
                    case 0b110: *rd = *rs1 | *rs2; break;
                    case 0b111: *rd = *rs1 & *rs2; break;
                    default: exception = 2; break;
                }
            }
            break;
        }
        case 0b0001111:
        {
            break;
        }
        case 0b0000011:
        {
            uint32_t uoff = instruction >> 20;
            int32_t offset = (uoff & 0x800) ? uoff | ~0xFFF : uoff;
            offset += *rs1;
            uint32_t val = bus->read(offset, &exception);

            switch ((instruction >> 12) & 0x7)
            {
                case 0b000: *rd = (int8_t)(val); break;
                case 0b001: *rd = (int16_t)(val); break;
                case 0b010: *rd = (int32_t)(val); break;
                case 0b100: *rd = (uint8_t)(val); break;
                case 0b101: *rd = (uint16_t)(val); break;
                default: exception = 2; break;
            }
            break;
        }
        case 0b0100011:
        {
            uint32_t uoff = ((instruction >> 20) & ~0x1F) | ((instruction >> 7) & 0x1F);
            int32_t offset = (uoff & 0x800) ? uoff | ~0xFFF : uoff;
            offset += *rs1;

            switch ((instruction >> 12) & 0x7)
            {
                case 0b000: bus->write(offset, *rs2, 1, &exception); break;
                case 0b001: bus->write(offset, *rs2, 2, &exception); break;
                case 0b010: bus->write(offset, *rs2, 4, &exception); break;
                default: exception = 2; break;
            }
            break;
        }
        case 0b1101111:
        {
            uint32_t uoff = ((instruction & 0x80000000) >> 11) | ((instruction & 0x7FE00000) >> 20) | ((instruction & 0x100000) >> 9) | ((instruction & 0xFF000));
            int32_t offset = (uoff & 0x100000) ? uoff | 0xFFE00000 : uoff;
            *rd = pc + 4;
            pc += offset - 4;
            break;
        }
        case 0b1100111:
        {
            uint32_t uoff = instruction >> 20;
            int32_t offset = (uoff & 0x800) ? uoff | ~0xFFF : uoff;
            *rd = pc + 4;
            pc = ((*rs1 + offset) & ~1) - 4;
            break;
        }
        case 0b1100011:
        {
            uint32_t uoff = ((instruction & 0xF00) >> 7) | ((instruction & 0x7E000000) >> 20) | ((instruction & 0x80) << 4) | ((instruction >> 31) << 12);
            int32_t offset = (uoff & 0x1000) ? uoff | 0xFFFFE000 : uoff;
            offset += pc - 4;

            switch ((instruction >> 12) & 0x7)
            {
                case 0b000: pc = (*rs1 == *rs2) ? offset : pc; break;
                case 0b001: pc = (*rs1 != *rs2) ? offset : pc; break;
                case 0b100: pc = (*rs1 < *rs2) ? offset : pc; break;
                case 0b101: pc = (*rs1 >= *rs2) ? offset : pc; break;
                case 0b110: pc = ((uint32_t) *rs1 < (uint32_t) *rs2) ? offset : pc; break;
                case 0b111: pc = ((uint32_t) *rs1 >= (uint32_t) *rs2) ? offset : pc; break;
                default: exception = 2; break;
            }
            break;
        }
        case 0b1110011:
        {
            switch ((instruction >> 12) & 0x7) {
                case 0b000:
                {
                    switch ((instruction >> 20) & 0x1F)
                    {
                        case 0b000: exception = 10; break;
                        case 0b001: exception = 3; break;
                        case 0b010:
                        {
                            if ((instruction >> 27) == 0b110) {
                                pc = csr[0x341] - 4;
                            } else {
                                exception = 2;
                            }
                            break;
                        }
                        default: exception = 2; break;
                    }
                    break;
                }
                case 0b001: *rd = csr[instruction >> 20]; csr[instruction >> 20] = *rs1; break;
                case 0b010: *rd = csr[instruction >> 20]; csr[instruction >> 20] |= *rs1; break;
                case 0b011: *rd = csr[instruction >> 20]; csr[instruction >> 20] &= ~*rs1; break;
                case 0b101: *rd = csr[instruction >> 20]; csr[instruction >> 20] = (instruction >> 15) & 0x1F; break;
                case 0b110: *rd = csr[instruction >> 20]; csr[instruction >> 20] |= (instruction >> 15) & 0x1F; break;
                case 0b111: *rd = csr[instruction >> 20]; csr[instruction >> 20] &= ~((instruction >> 15) & 0x1F); break;
                default: exception = 2; break;
            }
            break;
        }
        case 0b0101111:
        {
            bool write = 1;
            uint32_t rdata = bus->read(*rs1, &exception);
            uint32_t wdata = *rs2;
            switch (instruction >> 27)
            {
                case 0b00010: write = 0; reserved = *rs1; break;
                case 0b00011: *rd = reserved != *rs1; write = !*rd; break;
                case 0b00001: break;
                case 0b00000: wdata += rdata; break;
                case 0b00100: wdata ^= rdata; break;
                case 0b01100: wdata &= rdata; break;
                case 0b01000: wdata |= rdata; break;
                case 0b10000: wdata = (*rs2 < (int32_t) rdata) ? *rs2 : rdata; break;
                case 0b10100: wdata = (*rs2 > (int32_t) rdata) ? *rs2 : rdata; break;
                case 0b11000: wdata = ((uint32_t) *rs2 < rdata) ? *rs2 : rdata; break;
                case 0b11100: wdata = ((uint32_t) *rs2 > rdata) ? *rs2 : rdata; break;
                default: exception = 2; write = 0; break;
            }
            *rd = rdata;
            if (write) bus->write(*rs1, wdata, 4, &exception);
            break;
        }
        default:
        {
            exception = 2;
            break;
        }
    }
    pc += 4;

    csr[0xB00] += (csr[0x306] & 1) ? 1 : 0;
    csr[0xB02] += (csr[0x306] & 2) ? 1 : 0;

    if (exception) {
        csr[0x341] = pc;
        if (csr[0x305] & 3) {
            pc = (csr[0x305] & ~3) + (exception * 4);
        } else {
            pc = csr[0x305];
        }
        csr[0x342] = exception;
    }
}
