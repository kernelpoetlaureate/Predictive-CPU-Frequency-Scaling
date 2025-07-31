// main32.c - Minimal 32-bit C kernel for QEMU, prints CPU info to serial
#include <stdint.h>

#define SERIAL_PORT 0x3F8
static void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static unsigned char inb(uint16_t port) {
    unsigned char ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static void serial_init() {
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x03);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);
}
static void serial_write(char c) {
    while (!(inb(SERIAL_PORT + 5) & 0x20));
    outb(SERIAL_PORT, c);
}
static void serial_print(const char *s) {
    while (*s) serial_write(*s++);
}
static void itoa(unsigned int n, char *buf, int base) {
    char tmp[32]; int i = 0, j = 0;
    if (n == 0) { buf[0] = '0'; buf[1] = 0; return; }
    while (n) { tmp[i++] = "0123456789ABCDEF"[n % base]; n /= base; }
    while (i--) buf[j++] = tmp[i];
    buf[j] = 0;
}
static void cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "0"(op)
    );
}
void main32() {
    serial_init();
    serial_print("Bare Metal CPU Detection\r\n");
    unsigned int eax, ebx, ecx, edx;
    char vendor[13];
    cpuid(0, &eax, &ebx, &ecx, &edx);
    *(unsigned int*)&vendor[0] = ebx;
    *(unsigned int*)&vendor[4] = edx;
    *(unsigned int*)&vendor[8] = ecx;
    vendor[12] = 0;
    serial_print("Vendor: "); serial_print(vendor); serial_print("\r\n");
    cpuid(1, &eax, &ebx, &ecx, &edx);
    char buf[16];
    serial_print("Family: "); itoa((eax >> 8) & 0xf, buf, 10); serial_print(buf);
    serial_print(" Model: "); itoa((eax >> 4) & 0xf, buf, 10); serial_print(buf);
    serial_print(" Stepping: "); itoa(eax & 0xf, buf, 10); serial_print(buf); serial_print("\r\n");
    while (1);
}
