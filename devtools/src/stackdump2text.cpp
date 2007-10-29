#include <stdint.h>
#include <stdio.h>

int main() {
    for (;;) {
        uint32_t addr;
        uint32_t value;
        fread(&addr, sizeof(addr), 1, stdin);
        fread(&value, sizeof(addr), 1, stdin);
        if (feof(stdin))
            return 0;
        printf("%08X: %08X\n", addr, value);
    }
}
