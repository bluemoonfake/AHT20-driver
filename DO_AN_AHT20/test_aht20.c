#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h> 
#include <errno.h> 
#include <string.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define spi0 0

#define AHT20_IOCTL_MAGIC 'A'
#define AHT20_IOCTL_MAGIC1 'B'

#define AHT20_READ_TEMPERATURE _IOR(AHT20_IOCTL_MAGIC , 1, uint32_t)
#define AHT20_READ_HUMIDITY _IOR(AHT20_IOCTL_MAGIC1, 0, uint32_t)
#define AHT20_START _IO(AHT20_IOCTL_MAGIC, 2)
#define AHT20_STOP _IO(AHT20_IOCTL_MAGIC, 3)


#define DEVICE_PATH "/dev/aht20_dev"

#ifdef _WIN32
#define CLEAR_COMMAND "cls"
#else
#define CLEAR_COMMAND "clear"
#endif

uint8_t charTo7Segment(char c) {
    switch(c) {
        case 0 : return 0b01111110;
        case 1 : return 0b00110000;
        case 2 : return 0b01101101;
        case 3 : return 0b01111001;
        case 4 : return 0b00110011;
        case 5 : return 0b01011011;
        case 6 : return 0b01011111;
        case 7 : return 0b01110000;
        case 8 : return 0b01111111;
        case 9 : return 0b01111011;
        case '-' : return 0b00000001;
        default: return 0x00; // Blank for unsupported characters
    }
}

// Function to send data to MAX7219
void sendData(uint8_t address, uint8_t value) {
    uint8_t data[2];
    data[0] = address;
    data[1] = value;
    wiringPiSPIDataRW(spi0, data, 2);
}

// Function to setup MAX7219
void setup_max7912(){
    if (wiringPiSPISetup(spi0, 1000000) < 0) {
        fprintf(stderr, "SPI Setup failed: %s\n", strerror(errno));
        exit(1);
    }
    
    // Setup MAX7219
    sendData(0x09, 0x00); // no decode mode
    sendData(0x0A, 0x08); // intensity
    sendData(0x0B, 0x07); // scan limit
    sendData(0x0C, 0x01); // normal operation mode
    sendData(0x0F, 0x00); // turn off display test
}

// Function to display temperature on MAX7219
void displayTemperature(int temperature) {
    // Convert temperature to integer for display
    int data[4];
    
    //sendData(i + 5, charTo7Segment('-'));
    if (temperature < 0)
    {
        temperature = temperature*(-1);
        data[1] = temperature/100;
        data[2] = (temperature/10)%10;
        data[3] = temperature%10;

        for(int i = 0; i < 4 ; i++)
        {  
            if(i == 3 ) sendData(i + 5, charTo7Segment('-'));
            else if (i == 1) sendData (i+5, charTo7Segment(data[3 - i]) | 0b10000000);
            else sendData(i+5, charTo7Segment(data[3 - i]));
            
        }
    }
    else
    {
        data[1] = temperature/100;
        data[2] = (temperature/10)%10;
        data[3] = temperature%10;
        for(int i = 0; i < 4 ; i++)
        {  
            if(i == 3 ) sendData(i + 5, 0);
            else if (i == 1) sendData (i+5, charTo7Segment(data[3 - i]) | 0b10000000);
            else sendData(i+5, charTo7Segment(data[3 - i]));
            
        }
    }
    
}

// Function to display humidity on MAX7219
void displayHumidity(int humidity) {
    // Convert humidity to integer for display
    int data[4];
    data[0] = 0;
    data[1] = humidity/100;
    data[2] = (humidity/10)%10;
    data[3] = humidity%10;
    

   for(int i = 0; i < 4 ; i++)
   {
        if (i == 3) sendData (i+1, 0);
        else if (i == 1) sendData (i+1, charTo7Segment(data[3 - i]) | 0b10000000);
        else sendData(i+1, charTo7Segment(data[3 - i]));
        
   }
}
void display_clear() {
    // Convert humidity to integer for display
    for (int i = 1 ; i < 9 ; i++)
    {
        sendData(i,0);
    }
}
int main() {
    int fd;
    int humidity;
    int temperature;

    // Open the device
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("Failed to open the device");
        return -1;
    }


    ioctl(fd, AHT20_START);

    // Setup MAX7219
    setup_max7912();
    display_clear();

    
    while (1){
        // Read temperature from sensor
        if (ioctl(fd, AHT20_READ_TEMPERATURE, &temperature) < 0) {
            perror("Failed to perform ioctl");
            close(fd);
            return -1;
        }
        ioctl(fd, AHT20_STOP);
        // Read humidity from sensor
        if (ioctl(fd, AHT20_READ_HUMIDITY, &humidity) < 0) {
            perror("Failed to perform ioctl");
            close(fd);
            return -1;
        }

        // Convert temperature and humidity to float
        float temp = (float) temperature / 10.0;
        float hum = (float) humidity / 10.0;
        // temperature = -200; // nhiet do am
        // Clear the screen
        system(CLEAR_COMMAND);       

        // Display temperature and humidity
        printf("Temperature: %.1f°C\n", temp);
        printf("Humidity: %.1f%%\n", hum);

        // Display temperature and humidity on MAX7219
        displayTemperature(temperature);
        displayHumidity(humidity);
        
        // Delay for one second
        usleep(500000); //0.5s
    }

    ioctl(fd, AHT20_STOP);
    // Close the device
    close(fd);

    return 0;
}
