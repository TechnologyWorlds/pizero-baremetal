#include <stdint.h>

// BCM2835 Base Adresa pre Raspberry Pi Zero WH
#define MMIO_BASE       0x20000000

// UART0 Registre (GPIO 14 TX / GPIO 15 RX)
#define UART0_DR        *(volatile uint32_t*)(MMIO_BASE + 0x201000)
#define UART0_FR        *(volatile uint32_t*)(MMIO_BASE + 0x201018)
#define UART0_IBRD      *(volatile uint32_t*)(MMIO_BASE + 0x201024)
#define UART0_FBRD      *(volatile uint32_t*)(MMIO_BASE + 0x201028)
#define UART0_LCRH      *(volatile uint32_t*)(MMIO_BASE + 0x20102C)
#define UART0_CR        *(volatile uint32_t*)(MMIO_BASE + 0x201030)
#define GPIO_GPFSEL1    *(volatile uint32_t*)(MMIO_BASE + 0x200004)

// I2C (BSC0) Registre pre CSI Kameru (OV5647)
#define BSC0_C          *(volatile uint32_t*)(MMIO_BASE + 0x205000)
#define BSC0_S          *(volatile uint32_t*)(MMIO_BASE + 0x205004)
#define BSC0_DLEN       *(volatile uint32_t*)(MMIO_BASE + 0x205008)
#define BSC0_A          *(volatile uint32_t*)(MMIO_BASE + 0x20500C)
#define BSC0_FIFO       *(volatile uint32_t*)(MMIO_BASE + 0x205010)

// Opoždenie (Wait loop)
void delay(volatile uint32_t count) {
    while (count--) { asm volatile("nop"); }
}

// Inicializácia UART pre ESP32
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

void uart_send_string(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

// Inicializácia rozhrania CSI Kamery cez I2C
void camera_init() {
    BSC0_A = 0x36; // I2C adresa RPi kamery (OV5647)
    BSC0_C = (1 << 15) | (1 << 7) | (1 << 4); // Clear FIFO + Enable
    delay(100000);
}

// Čítanie stavu z kamery
bool read_camera_frame(uint8_t* buffer, uint32_t size) {
    // Čítanie bytov z I2C / CSI zbernice do pamäte
    if (BSC0_S & (1 << 1)) { // Ak sú dáta pripravené
        for(uint32_t i = 0; i < size; i++) {
            buffer[i] = (uint8_t)(BSC0_FIFO & 0xFF);
        }
        return true;
    }
    return false;
}

// Hlavný bod vstupu do RPi Zero Bare-Metal
extern "C" void kernel_main() {
    uart_init();
    camera_init();
    
    delay(500000);
    uart_send_string("RPI_READY\n");

    uint8_t frameBuffer[64];

    while (1) {
        // Ak kamera zachytí obrazový rámec
        if (read_camera_frame(frameBuffer, 64)) {
            
            // Analýza tmavých pixelov (prekážka)
            uint32_t darkPixels = 0;
            for(int i = 0; i < 64; i++) {
                if (frameBuffer[i] < 0x30) darkPixels++;
            }

            // Ak je viac ako polovica obrazu tmavá = Prekážka
            if (darkPixels > 32) {
                uart_send_string("WARN:OBSTACLE\n");
            } else {
                uart_send_string("FOUND:CLEAR\n");
            }
        } else {
            // Bežný patrol režim
            uart_send_string("FOUND:CLEAR\n");
        }

        delay(200000); // Taktovanie smyčky
    }
}
