#include <stdio.h>
#include "aht20.h"

int main() {
    int file;
    uint32_t temperature;
    uint32_t humidity;

    if (aht20_init(&file) < 0) {
        return -1;
    }

    if (aht20_read_temperature(file, &temperature) == 0) {
        printf("Temperature: %d.%d°C\n", temperature / 10, temperature % 10);
    } else {
        printf("Failed to read temperature\n");
    }

    if (aht20_read_humidity(file, &humidity) == 0) {
        printf("Humidity: %d.%d%%\n", humidity / 10, humidity % 10);
    } else {
        printf("Failed to read humidity\n");
    }

    aht20_close(file);
    return 0;
}
