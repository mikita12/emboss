#include <stdio.h>
#include <stdint.h>

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    __asm__ volatile (
        "cpuid\n\t"
        "rdtsc\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        : "=r" (hi), "=r" (lo)
        :
        : "%rax", "%rbx", "%rcx", "%rdx"
    );
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t start_cycles = 0;

void timer_start() {
    start_cycles = rdtsc();
}

void timer_stop() {
    uint64_t end_cycles = rdtsc();
    uint64_t elapsed = end_cycles - start_cycles;

    FILE *f = fopen("wyniki.txt", "w");
    if (!f) {
        perror("Nie można otworzyć pliku");
        return;
    }
    fprintf(f, "%lu\n", elapsed);
    fclose(f);

    start_cycles = 0; // reset
}

