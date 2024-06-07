# AHT20-driver
This guide provides information on how to use the AHT20 driver to read temperature and humidity data from the AHT20 sensor using I2C communication. The AHT20 driver is a Linux kernel module that interacts with the AHT20 temperature and humidity sensor via the I2C protocol. It offers an IOCTL interface for easy interaction from user space.
---------------------------------------
Summary of Sections
Introduction:
Provides an overview of the document, its purpose, and the main contents that will be covered.

Physical Connection:
Instructions on how to physically connect the sensor to the hardware system, including details on pins, wiring, and communication standards.

Driver Installation:
Detailed steps for installing the driver for the sensor on the system to ensure proper operation.

Driver Functionality:
a. Function to read temperature from the sensor: Instructions on using the function to retrieve temperature data from the sensor.
b. Function to read humidity from the sensor: Instructions on using the function to retrieve humidity data from the sensor.
c. Function to start the sensor: Description of how to start the sensor to begin data collection.
d. Function to stop the sensor: Description of how to stop the sensor when data collection is no longer needed.

Interacting with the Driver in User Space:
Guidance on how to interact with the driver from user space, including necessary commands and operations.

Sample Application:
Provides a specific example application that uses the sensor, illustrating how to collect and process temperature and humidity data.

Library Installation:
Steps for installing the supporting library needed for programming and interacting with the sensor.

Library Functions:
a. Function aht20_init(): Instructions on how to initialize the sensor.
b. Function aht20_read_temperature(): Instructions on how to read temperature from the sensor.
c. Function aht20_read_humidity(): Instructions on how to read humidity from the sensor.
d. Function aht20_close(): Instructions on how to close the sensor when it is no longer in use.
