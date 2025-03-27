#include <stdio.h>

int main (int argc, char ** argv) {
    *(char *)0 = 0x1234;
    
    int a = 3 / 0;
    return 0;
}