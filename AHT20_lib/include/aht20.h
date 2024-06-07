#ifndef AHT20_H
#define AHT20_H

#include <stdint.h>

#define I2C_DEVICE "/dev/i2c-1"
#define AHT20_ADDR 0x38

// Function prototypes
int aht20_init(int *file);
int aht20_read_temperature(int file, uint32_t *temperature);
int aht20_read_humidity(int file, uint32_t *humidity);
void aht20_close(int file);

// Bien
#define AHT20_ADDR 0x38
#define AHT20_CMD_MEASURE 0xAC
#define AHT20_CMD_STATUS 0x71
#define AHT20_CMD_INIT 0xBE


#endif // AHT20_H
