#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// I2C and GPIO definitions
#define I2C_ADDRESS 0x38  // FT5426 I2C address
#define I2C_DEVICE "/dev/i2c-1"  // Adjust this if using a different I2C bus
#define INTERRUPT_PIN 4  // GPIO 17 (Broadcom GPIO pin 17)
#define RESET_PIN 17     // GPIO 18 (Broadcom GPIO pin 18)

// I2C file descriptor
int i2c_fd;

// Function to write a register address and read back the value
unsigned char read_register(unsigned char reg_addr) {
    // Write the register address to the FT5426
    if (write(i2c_fd, &reg_addr, 1) != 1) {
        perror("Failed to write register address");
        return 0xFF;  // Returning 0xFF as an error code
    }

    // Read one byte from the specified register
    unsigned char value = 0;
    if (read(i2c_fd, &value, 1) != 1) {
        perror("Failed to read register value");
        return 0xFF;  // Returning 0xFF as an error code
    }

    return value;
}

// Read touch data from specific registers of the FT5426
void read_touch_data() {
    unsigned char data_04 = read_register(0x04); // Register 0x04
    unsigned char data_06 = read_register(0x06); // Register 0x06

    if (data_04 != 0xFF && data_06 != 0xFF) {
        printf("Register 0x04: 0x%02x\n", data_04);
        printf("Register 0x06: 0x%02x\n", data_06);
    } else {
        printf("Failed to read valid touch data from registers.\n");
    }
}

// Interrupt handler for touch events
void touch_interrupt(int gpio, int level, uint32_t tick) {
    if (level == 0) {  // GPIO went to LOW level, indicating a falling edge
        printf("Touch event detected! Reading register data...\n");
        read_touch_data();
    }
}

// Initialize the touchscreen
void init_touchscreen() {
    // Initialize pigpio library
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        exit(1);
    }

    // Set GPIO pins
    gpioSetMode(RESET_PIN, PI_OUTPUT);
    gpioSetMode(INTERRUPT_PIN, PI_INPUT);
    gpioSetPullUpDown(INTERRUPT_PIN, PI_PUD_UP);

    // Reset the touchscreen
    gpioWrite(RESET_PIN, PI_LOW);
    gpioDelay(200000);  // 200ms reset delay
    gpioWrite(RESET_PIN, PI_HIGH);
    gpioDelay(300000);  // 300ms initialization delay

    // Open the I2C device
    if ((i2c_fd = open(I2C_DEVICE, O_RDWR)) < 0) {
        perror("Failed to open the I2C bus");
        exit(1);
    }

    // Set the I2C address for the device
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDRESS) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(1);
    }

    // Attach interrupt handler for the touch event
    if (gpioSetAlertFunc(INTERRUPT_PIN, touch_interrupt) != 0) {
        fprintf(stderr, "Unable to setup ISR for interrupt pin\n");
        exit(1);
    }

    printf("Touchscreen initialized. Waiting for touch events...\n");
}

// Cleanup function to close I2C and GPIO
void cleanup() {
    close(i2c_fd);
    gpioTerminate();
    printf("Cleanup complete\n");
}

int main() {
    init_touchscreen();

    // Main loop to keep the program running and waiting for interrupts
    while (1) {
        sleep(1);  // Sleep for 1 second
    }

    cleanup();
    return 0;
}
