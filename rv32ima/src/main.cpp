#include "rv32ima.h"

int main(int argc, char **argv)
{
    RISC risc;

    risc.initialize(argv[1]);
    risc.run();
    return 0;
}