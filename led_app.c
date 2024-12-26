#include <stdio.h>
#include <stdlib.h>  // For atoi()
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define DEVICE "/dev/etx_device"

// Function to create a delay in milliseconds
void delay_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[]) {
    int fd;
    char led_on = '1';
    char led_off = '0';
    int blink_count;
    int delay_time = 500; // Default delay is 500ms if not specified

    // Check if the user provided the number of blinks (and optionally delay time)
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <blink_count> [delay_time_in_ms]\n", argv[0]);
        return -1;
    }

    // Parse the number of blinks from the command line
    blink_count = atoi(argv[1]);  // Convert the first argument to integer
    if (blink_count <= 0) {
        printf("Invalid blink count. Please provide a positive integer value.\n");
        return -1;
    }

    // Parse the delay time if provided
    if (argc == 3) {
        delay_time = atoi(argv[2]);  // Convert the second argument to integer
        if (delay_time <= 0) {
            printf("Invalid delay time. Please provide a positive integer value.\n");
            return -1;
        }
    }

    // Open the device
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device...");
        return -1;
    }

    printf("Device opened successfully. Blinking LED %d times with a delay of %d ms.\n", blink_count, delay_time);

    // Blink the LED
    for (int i = 0; i < blink_count; i++) {  // Blink the specified number of times
        printf("Turning LED ON (%d/%d)\n", i + 1, blink_count);
        write(fd, &led_on, 1);  // Turn LED on
        delay_ms(delay_time);   // Wait for the specified delay

        printf("Turning LED OFF (%d/%d)\n", i + 1, blink_count);
        write(fd, &led_off, 1); // Turn LED off
        delay_ms(delay_time);   // Wait for the specified delay
    }

    printf("Blinking complete.\n");

    // Close the device
    close(fd);

    return 0;
}
