#include <pigpio.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <string.h>  // Include to fix memset warning
#include <math.h>    // Include for math functions

// I2C and GPIO definitions
#define I2C_ADDRESS 0x38  // FT5426 I2C address
#define I2C_DEVICE "/dev/i2c-1"  // Use I2C bus 1
#define INTERRUPT_PIN 4  // GPIO 4 (Broadcom GPIO pin 4)
#define RESET_PIN 17     // GPIO 17 (Broadcom GPIO pin 17)

// Assuming the screen resolution is 800x480 (adjust as per your setup)
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Modify these to adjust the calibration scaling factor
#define X_SCALE 1.0  // Scaling factor for x-axis (adjust as needed)
#define Y_SCALE 1.0  // Scaling factor for y-axis (adjust as needed)

// Offsets to align the coordinate system
#define X_OFFSET 0   // Offset for x-axis (adjust as needed)
#define Y_OFFSET 0   // Offset for y-axis (adjust as needed)

// I2C file descriptor
int i2c_fd;

// uinput file descriptor
int uinput_fd;

// Function to write a register address and read back the value
unsigned char read_register(unsigned char reg_addr) {
    if (write(i2c_fd, &reg_addr, 1) != 1) {
        perror("Failed to write register address");
        return 0xFF;
    }

    unsigned char value = 0;
    if (read(i2c_fd, &value, 1) != 1) {
        perror("Failed to read register value");
        return 0xFF;
    }

    return value;
}

// Function to create and configure a uinput virtual mouse device
void setup_uinput_device() {
    struct uinput_user_dev uidev;

    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        perror("Failed to open /dev/uinput");
        exit(EXIT_FAILURE);
    }

    // Enable mouse buttons and movement events
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(uinput_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(uinput_fd, UI_SET_ABSBIT, ABS_Y);

    // Prepare the uinput device for use
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Touchscreen Mouse");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;
    uidev.absmin[ABS_X] = 0;
    uidev.absmax[ABS_X] = SCREEN_WIDTH;
    uidev.absmin[ABS_Y] = 0;
    uidev.absmax[ABS_Y] = SCREEN_HEIGHT;

    if (write(uinput_fd, &uidev, sizeof(uidev)) < 0) {
        perror("Failed to write uinput device setup");
        exit(EXIT_FAILURE);
    }

    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        perror("Failed to create uinput device");
        exit(EXIT_FAILURE);
    }
}

// Function to emit an input event via uinput
void emit_event(int fd, int type, int code, int value) {
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = value;
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    if (write(fd, &ie, sizeof(ie)) < 0) {
        perror("Failed to emit input event");
    }
}

// Function to emulate mouse movement and click based on touch data
void emulate_mouse(int x_position, int y_position, int touch_event) {
    // Apply scaling and offset (adjust these calculations as needed)
    int x_mapped = (int)((x_position * X_SCALE) + X_OFFSET);
    int y_mapped = (int)((y_position * Y_SCALE) + Y_OFFSET);

    // Clamp the values to the screen boundaries
    if (x_mapped < 0) x_mapped = 0;
    if (x_mapped > SCREEN_WIDTH) x_mapped = SCREEN_WIDTH;
    if (y_mapped < 0) y_mapped = 0;
    if (y_mapped > SCREEN_HEIGHT) y_mapped = SCREEN_HEIGHT;

    // Log the received touch coordinates and the mapped mouse coordinates
    printf("Mouse Coordinates (mapped): x = %d, y = %d\n", x_mapped, y_mapped);

    // Emit absolute movement events
    emit_event(uinput_fd, EV_ABS, ABS_X, x_mapped);
    emit_event(uinput_fd, EV_ABS, ABS_Y, y_mapped);

    // If touch event indicates touch down, emit mouse click
    if (touch_event == 1) { // Assuming 1 indicates touch down
        emit_event(uinput_fd, EV_KEY, BTN_LEFT, 1); // Press left mouse button
    } else if (touch_event == 0) { // Assuming 0 indicates touch release
        emit_event(uinput_fd, EV_KEY, BTN_LEFT, 0); // Release left mouse button
    }

    // Sync the events
    emit_event(uinput_fd, EV_SYN, SYN_REPORT, 0);
}

// Read touch data from specific registers of the FT5426
void read_touch_data() {
    unsigned char xh = read_register(0x03); // Register 0x03 (high bits of X)
    unsigned char xl = read_register(0x04); // Register 0x04 (low bits of X)
    unsigned char yh = read_register(0x05); // Register 0x05 (high bits of Y)
    unsigned char yl = read_register(0x06); // Register 0x06 (low bits of Y)
    unsigned char touch_status = read_register(0x02); // Register 0x02 (touch status)

    if (xh == 0xFF || xl == 0xFF || yh == 0xFF || yl == 0xFF) {
        printf("Failed to read valid touch data from registers.\n");
        return;
    }

    // Combine the high and low bits to get the full 12-bit values
    int x_position = ((xh & 0x0F) << 8) | xl; // Use only lower 4 bits of XH
    int y_position = ((yh & 0x0F) << 8) | yl; // Use only lower 4 bits of YH

    // Determine touch event status (1 for touch down, 0 for release)
    int touch_event = (touch_status & 0x0F) > 0 ? 1 : 0;

    // Log the full coordinates
    printf("Touch Coordinates: X = %d, Y = %d, Touch Event: %d\n", x_position, y_position, touch_event);

    // Call the mouse emulation function
    emulate_mouse(x_position, y_position, touch_event);
}

// Interrupt handler for touch events
void touch_interrupt(int gpio, int level, uint32_t tick) {
    if (level == 0) {
        printf("Touch event detected! Reading register data...\n");
        read_touch_data();
    }
}

// Initialize the touchscreen
void init_touchscreen() {
    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio initialization failed\n");
        exit(EXIT_FAILURE);
    }

    gpioSetMode(RESET_PIN, PI_OUTPUT);
    gpioSetMode(INTERRUPT_PIN, PI_INPUT);
    gpioSetPullUpDown(INTERRUPT_PIN, PI_PUD_UP);

    gpioWrite(RESET_PIN, PI_LOW);
    gpioDelay(200000);
    gpioWrite(RESET_PIN, PI_HIGH);
    gpioDelay(300000);

    if ((i2c_fd = open(I2C_DEVICE, O_RDWR)) < 0) {
        perror("Failed to open the I2C bus");
        exit(EXIT_FAILURE);
    }

    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDRESS) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        exit(EXIT_FAILURE);
    }

    if (gpioSetAlertFunc(INTERRUPT_PIN, touch_interrupt) != 0) {
        fprintf(stderr, "Unable to setup ISR for interrupt pin\n");
        exit(EXIT_FAILURE);
    }

    printf("Touchscreen initialized. Waiting for touch events...\n");
}

// Cleanup function to close I2C, GPIO, and uinput resources
void cleanup() {
    close(i2c_fd);
    gpioTerminate();

    if (ioctl(uinput_fd, UI_DEV_DESTROY) < 0) {
        perror("Failed to destroy uinput device");
    }

    close(uinput_fd);
    printf("Cleanup complete\n");
}

int main() {
    setup_uinput_device();  // Setup the virtual mouse device
    init_touchscreen();     // Initialize the touchscreen

    while (1) {
        sleep(1);
    }

    cleanup();
    return 0;
}
