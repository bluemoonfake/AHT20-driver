#ifndef AHT20_IOCTL_H
#define AHT20_IOCTL_H

#include <linux/ioctl.h>

#define AHT20_MAGIC 'a'
#define AHT20_IOCTL_MEASURE _IOR(AHT20_MAGIC, 0, int[2])

#endif