#include <stdint.h>

#define MMIO_BASE       0x20000000

// UART0 Registre
#define UART0_DR        *(volatile uint32_t*)(MMIO_BASE + 0x201000)
#define UART0_FR        *(volatile uint32_t*)(MMIO_BASE + 0x201018)
#define UART0_IBRD      *(volatile uint32_t*)(MMIO_BASE + 0x201024)
#define UART0_FBRD      *(volatile uint32_t*)(MMIO_BASE + 0x201028)
#define UART0_LCRH      *(volatile uint32_t*)(MMIO_BASE + 0x20102C)
#define UART0_CR        *(volatile uint32_t*)(MMIO_BASE + 0x201030)
#define GPIO_GPFSEL1    *(volatile uint32_t*)(MMIO_BASE + 0x200004)

// I2C Registre pre kameru
#define BSC0_S          *(volatile uint32_t*)(MMIO_BASE + 0x205004)
#define BSC0_FIFO       *(volatile uint32_t*)(MMIO_BASE + 0x205010)

void delay(volatile uint32_t count) {
    while (count--) { asm volatile("nop"); }
}

void uart_init() {
    UART0_CR = 0; 
    uint32_t selector = GPIO_GPFSEL1;
    selector &= ~((7 << 12) | (7 << 15));
    selector |= (4 << 12) | (4 << 15);
    GPIO_GPFSEL1 = selector;

    UART0_IBRD = 19; // 9600 Baudrate
    UART0_FBRD = 34;
    UART0_LCRH = (1 << 4) | (3 << 5); 
    UART0_CR = (1 << 0) | (1 << 8) | (1 << 9); 
}

void uart_putc(char c) {
    while (UART0_FR & (1 << 5)); 
    UART0_DR = c;
}

extern "C" void kernel_main() {
    uart_init();
    delay(500000);

    while (1) {
        // Kontrola tmavšieho objektu / ruky pred kamerou
        uint32_t sensorData = BSC0_FIFO & 0xFF;
        
        // Ak kamera vidí ruku (prekážku), pošle '1', inak '0'
        if (sensorData > 0x20) {
            uart_putc('1');
        } else {
            uart_putc('0');
        }

        delay(100000); // Posiela stav každých ~100ms
    }
}
