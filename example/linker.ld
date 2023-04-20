OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")

MEMORY
{
  instruction_ram : ORIGIN = 0x80000000, LENGTH = 0xFFF
  data_ram : ORIGIN = 0x80000FFF, LENGTH = 0xFFF
}

SECTIONS
{
  .text :
  {
    *(.text.main)
    *(.text*)
  } > instruction_ram

  .rodata :
  {
    *(.rodata)
  } > instruction_ram

  .data :
  {
    *(.data)
  } > data_ram

  .bss :
  {
    *(.bss)
  } > data_ram

  /DISCARD/ :
  {
    *(.comment)
    *(.debug*)
  }
}
