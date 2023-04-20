all: prep example

prep:
	mkdir -p build

example:
	riscv64-unknown-elf-gcc -march=rv32imazicsr -mabi=ilp32 -ffunction-sections -ffreestanding -c main.c -o build/main.o
	riscv64-unknown-elf-ld -T linker.ld build/main.o -o build/example.elf
	riscv64-unknown-elf-objcopy -O binary build/example.elf build/example