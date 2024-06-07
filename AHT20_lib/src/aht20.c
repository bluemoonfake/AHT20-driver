#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include "aht20.h"

static uint8_t crc8(const uint8_t *data, int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

int aht20_init(int *file) {
    *file = open(I2C_DEVICE, O_RDWR);
    if (*file < 0) {
        perror("Failed to open I2C device");
        return -1;
    }

    if (ioctl(*file, I2C_SLAVE, AHT20_ADDR) < 0) {
        perror("Failed to set I2C address");
        close(*file);
        return -1;
    }

    return 0;
}

int aht20_read_temperature(int file, uint32_t *temperature) {
    uint8_t cmd[3];
    uint8_t buf[7];
    uint8_t crc;
    int tem;

    // Step 1: Check sensor status
    cmd[0] = AHT20_CMD_STATUS;
    if (write(file, cmd, 1) != 1) {
        perror("Failed to send status command");
        return -1;
    }

    usleep(20000);

    if (read(file, buf, 1) != 1) {
        perror("Failed to read status");
        return -1;
    }

    if ((buf[0] & 0x18) != 0x18) {
        // Sensor needs initialization
        cmd[0] = AHT20_CMD_INIT;
        cmd[1] = 0x08;
        cmd[2] = 0x00;
        if (write(file, cmd, 3) != 3) {
            perror("Failed to initialize sensor");
            return -1;
        }
        usleep(10000);
    }

    // Step 2: Trigger measurement
    cmd[0] = AHT20_CMD_MEASURE;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    if (write(file, cmd, 3) != 3) {
        perror("Failed to send measurement command");
        return -1;
    }

    usleep(80000);

    // Step 3: Read data
    if (read(file, buf, 7) != 7) {
        perror("Failed to read data");
        return -1;
    }

    if (buf[0] & 0x80) {
        perror("Sensor is busy");
        return -1;
    }

    crc = crc8(buf, 6);
    if (crc != buf[6]) {
        perror("CRC check failed");
        return -1;
    }

    tem = ((buf[3] & 0xF) << 16) | (buf[4] << 8) | buf[5];
    *temperature = ((tem * 2000) / 0x100000) - 500;
    
    return 0;
}

int aht20_read_humidity(int file, uint32_t *humidity) {
    uint8_t cmd[3];
    uint8_t buf[7];
    uint8_t crc;
    int hum;

    // Step 1: Check sensor status
    cmd[0] = AHT20_CMD_STATUS;
    if (write(file, cmd, 1) != 1) {
        perror("Failed to send status command");
        return -1;
    }

    usleep(20000);

    if (read(file, buf, 1) != 1) {
        perror("Failed to read status");
        return -1;
    }

    if ((buf[0] & 0x18) != 0x18) {
        // Sensor needs initialization
        cmd[0] = AHT20_CMD_INIT;
        cmd[1] = 0x08;
        cmd[2] = 0x00;
        if (write(file, cmd, 3) != 3) {
            perror("Failed to initialize sensor");
            return -1;
        }
        usleep(10000);
    }

    // Step 2: Trigger measurement
    cmd[0] = AHT20_CMD_MEASURE;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    if (write(file, cmd, 3) != 3) {
        perror("Failed to send measurement command");
        return -1;
    }

    usleep(80000);

    // Step 3: Read data
    if (read(file, buf, 7) != 7) {
        perror("Failed to read data");
        return -1;
    }

    if (buf[0] & 0x80) {
        perror("Sensor is busy");
        return -1;
    }

    crc = crc8(buf, 6);
    if (crc != buf[6]) {
        perror("CRC check failed");
        return -1;
    }

    hum = (buf[1] << 12) | (buf[2] << 4) | (buf[3] >> 4);
    *humidity = ((hum * 1000) / 0x100000);
    
    return 0;
}

void aht20_close(int file) {
    close(file);
}
