#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/delay.h>


#define DEVICE_NAME "aht20_dev"
#define CLASS_NAME  "aht20_class"

#define AHT20_IOCTL_MAGIC 'A'
#define AHT20_IOCTL_MAGIC1 'B'
#define AHT20_READ_TEMPERATURE _IOR(AHT20_IOCTL_MAGIC, 1, uint32_t)
#define AHT20_READ_HUMIDITY _IOR(AHT20_IOCTL_MAGIC1, 0, uint32_t)
#define AHT20_START _IO(AHT20_IOCTL_MAGIC, 2)
#define AHT20_STOP _IO(AHT20_IOCTL_MAGIC, 3)

// Read
#define AHT20_ADDR 0x38
#define AHT20_CMD_MEASURE 0xAC
#define AHT20_CMD_STATUS 0x71
#define AHT20_CMD_INIT 0xBE
//#define AHT20_CMD_SOFT_RESET 0xBA  // Soft reset command
#define AHT20_CMD_MEASURE_STOP 0x00 //stop

// Khai bao bien
static int major_number;
static struct class* aht20_class = NULL;
static struct device* aht20_device = NULL;
static struct i2c_client* aht20_client;


uint32_t humidity;
uint32_t temperature;



// Khai bao ham
static int aht20_open(struct inode *inodep, struct file *filep);
static int aht20_release(struct inode *inodep, struct file *filep);
static int aht20_read_temperature(struct i2c_client *client, uint32_t *temperature);
static int aht20_read_humidity(struct i2c_client *client, uint32_t *humidity);
static long aht20_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);


// ham CRC
static u8 crc8(const u8 *data, int len)
{
    u8 crc = 0xFF;
    int i, j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;  // Polynomial: x^8 + x^5 + x^4 + 1
            else
                crc <<= 1;
        }
    }
    return crc;
}

// Ham read temperature
static int aht20_read_temperature(struct i2c_client *client, uint32_t *temperature)
{
    u8 cmd[3];
    u8 buf[7];
    int ret;
    u8 crc;
    int tem;

    msleep(100); // cho 100ms

    // Bước 1: Kiểm tra trạng thái cảm biến
    cmd[0] = AHT20_CMD_STATUS; //0x71, byte status
    ret = i2c_master_send(client, cmd, 1); // gui lenh kiem tra trang thai
    if (ret < 0) {
        printk(KERN_ERR "Failed to send status command\n");
        return ret;
    }
    
    msleep(100); // cho 100ms

    ret = i2c_master_recv(client, buf, 1); // Doc du lieu tu byte status
    if (ret < 0) {
        printk(KERN_ERR "Failed to read status\n");
        return ret;
    }
    

    if ((buf[0] & 0x18) != 0x18) {
        // Cảm biến cần khởi tạo
        cmd[0] = AHT20_CMD_INIT; //0xBE khoi tao gom thanh ghi 0x1B, 0x1C, 0x1E
        cmd[1] = 0x08; // byte cai dat cac che do or cau hinh them
        cmd[2] = 0x00; // byte bo sung
        ret = i2c_master_send(client, cmd, 3); // Gui du lieu khoi tao lai cam bien
        if (ret < 0) {
            printk(KERN_ERR "Failed to initialize sensor\n");
            return ret;
        }
        msleep(10); // cho 10ms
    }

    // Bước 2: Kích hoạt đo lường
    cmd[0] = AHT20_CMD_MEASURE; //0xAC
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    ret = i2c_master_send(client, cmd, 3); // gui lenh kich hoat do luong
    if (ret < 0) {
        printk(KERN_ERR "Failed to send measurement command\n");
        return ret;
    }

    msleep(80);// cho 80ms 

    // Kiểm tra nếu cảm biến đang bận
    if (buf[0] & 0x80) {
        printk(KERN_ERR "Sensor is busy\n"); // kiem tra bit[7] cua byte status
        return -EAGAIN;                      // 1 busy, 0 idle: ngu dong
    }

    // Bước 3: Đọc dữ liệu
    ret = i2c_master_recv(client, buf, 7);  // Đọc 7 byte, bao gồm cả CRC
    if (ret < 0) {
        printk(KERN_ERR "Failed to read data\n");
        return ret;
    }

    

    // Tính toán và kiểm tra CRC
    crc = crc8(buf, 6);  // CRC được tính toán trên 6 byte đầu
    if (crc != buf[6]) {
        printk(KERN_ERR "CRC check failed\n");
        return -EIO;
    }

    // Tính toán nhiệt độ
    tem = ((buf[3] & 0xF) << 16) | (buf[4] << 8) | buf[5]; // tong co 20bit
    // buf[3] & 00001111 de lay 4 bit cuoi, dich trai 16 lan de 4 bit vao 4 bit dau cua tem
    // buf [4] lay 8 bit, dich trai 8 lan de vao 8 bit ke  tiep
    // buf [5] lay 8 bit, vao 8 bit cuoi cua tem
    *temperature = ((tem * 2000) / 1048576) - 500;// 1048576 = 2^20 0x100000
    // tinh theo cong thuc T = (()S_T/2^20)* 200 -50)*10
    tem = *temperature;
    printk(KERN_INFO "AHT20 Read - Temperature: %d.%d\n", tem/10, tem % 10);
    return 0;
}

// Ham read humidity
static int aht20_read_humidity(struct i2c_client *client, uint32_t *humidity)
{
    u8 cmd[3];
    u8 buf[7];
    int ret;
    u8 crc;
    int hum;

    msleep(100);
    // Bước 1: Kiểm tra trạng thái cảm biến
    cmd[0] = AHT20_CMD_STATUS; // 0x71
    ret = i2c_master_send(client, cmd, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to send status command\n");
        return ret;
    }
    
    msleep(100);

    ret = i2c_master_recv(client, buf, 1);
    if (ret < 0) {
        printk(KERN_ERR "Failed to read status\n");
        return ret;
    }
    
    if ((buf[0] & 0x18) != 0x18) {
        // Cảm biến cần khởi tạo
        cmd[0] = AHT20_CMD_INIT;
        cmd[1] = 0x08;
        cmd[2] = 0x00;
        ret = i2c_master_send(client, cmd, 3);
        if (ret < 0) {
            printk(KERN_ERR "Failed to initialize sensor\n");
            return ret;
        }
        msleep(10);
    }

    // Bước 2: Kích hoạt đo lường
    cmd[0] = AHT20_CMD_MEASURE;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    ret = i2c_master_send(client, cmd, 3);
    if (ret < 0) {
        printk(KERN_ERR "Failed to send measurement command\n");
        return ret;
    }

    msleep(80);

    // Kiểm tra nếu cảm biến đang bận
    if (buf[0] & 0x80) {
        printk(KERN_ERR "Sensor is busy\n");// kiem tra bit[7] cua status 
        return -EAGAIN;
    }

    // Bước 3: Đọc dữ liệu
    ret = i2c_master_recv(client, buf, 7);  // Đọc 7 byte, bao gồm cả CRC
    if (ret < 0) {
        printk(KERN_ERR "Failed to read data\n");
        return ret;
    }


    // Tính toán và kiểm tra CRC
    crc = crc8(buf, 6);  // CRC được tính toán trên 6 byte đầu
    if (crc != buf[6]) {
        printk(KERN_ERR "CRC check failed\n");
        return -EIO;
    }

    // Tính toán nhiệt độ và độ ẩm
    hum = (buf[1] << 12) | (buf[2] << 4) | (buf[3] >> 4);// tong 20bit
    // buf[1] lay 8 bit, dich trai 12 lan de vao 8 bit dau cua hum
    // buf[2] lay 8 bit, dich trai 4 lan de vao 8 bit tiep theo
    // buf[3] lay 4 bit dau cua byte[3], dich phai la de lay 4 bit dau tao ra ma 0000xxxx

    *humidity = ((hum * 1000) / 1048576 ); // 1048576 0x100000
    //Cong thuc: RH = ((S_RH/2^20)*100%)*10
    hum = *humidity;
    printk(KERN_INFO "AHT20 Read - Humidity: %d.%d%%\n", hum / 10, hum % 10);
    return 0;
}

//Ham start
static int aht20_start(struct i2c_client *client)
{
    u8 cmd[3];
    int ret;

    // Gửi lệnh khởi tạo
    cmd[0] = AHT20_CMD_INIT;
    cmd[1] = 0x08;
    cmd[2] = 0x00;
    ret = i2c_master_send(client, cmd, 3);
    if (ret < 0) {
        printk(KERN_ERR "Failed to start sensor\n");
        return ret;
    }

    msleep(10);

    printk(KERN_INFO "AHT20 sensor started\n");
    return 0;
}

//Ham Stop
static int aht20_stop(struct i2c_client *client)
{
    u8 cmd [3];
    int ret;

    //Gửi lệnh ngừng đo lường hoặc vào trạng thái ngủ
    cmd[0] = AHT20_CMD_MEASURE_STOP;  // lệnh giả định, nếu không có lệnh dừng cụ thể
    cmd[1] = 0x00;
    cmd[2] = 0x00;
    ret = i2c_master_send(client, cmd, 3);
    if (ret < 0) {
        printk(KERN_ERR "Failed to stop sensor\n");
        return ret;
    }

    msleep(10);

    printk(KERN_INFO "AHT20 sensor stopped\n");
    return 0;
}

//Ham ioctl
static long aht20_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) 
{
    int ret;
    int temperature;
    int humidity;

    switch (cmd) {
        case AHT20_READ_TEMPERATURE:
            // Gọi hàm aht20_read_data để đọc dữ liệu nhiệt độ từ cảm biến AHT20
            ret = aht20_read_temperature(aht20_client, &temperature);
            if (ret < 0) {
                printk(KERN_ERR "Failed to read temperature data from AHT20\n");
                return ret;
            }
            // Sao chép dữ liệu nhiệt độ đến không gian người dùng
            if (copy_to_user((int __user *)arg, &temperature, sizeof(temperature))) {
                return -EFAULT;
            }
            break;
        case AHT20_READ_HUMIDITY:
            // Gọi hàm aht20_read_data để đọc dữ liệu độ ẩm từ cảm biến AHT20
            ret = aht20_read_humidity(aht20_client, &humidity);
            if (ret < 0) {
                printk(KERN_ERR "Failed to read humidity data from AHT20\n");
                return ret;
            }
            // Sao chép dữ liệu độ ẩm đến không gian người dùng
            if (copy_to_user((int __user *)arg, &humidity, sizeof(humidity))) {
                return -EFAULT;
            }
            break;
        case AHT20_START:
            ret = aht20_start(aht20_client);
            if (ret < 0) {
                printk(KERN_ERR "Failed to start AHT20\n");
                return ret;
            }
            break;
        case AHT20_STOP:
            ret = aht20_stop(aht20_client);
            if (ret < 0) {
                printk(KERN_ERR "Failed to stop AHT20\n");
                return ret;
            }
            break;
        default:
            return -EINVAL;
    }

    return 0;
}



//Ham open
static int aht20_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "AHT20 device opened\n");
    return 0;
}
// Ham file_operations
static struct file_operations fops = {
    .open = aht20_open,
    .unlocked_ioctl = aht20_ioctl,
    .release = aht20_release,
    
};

// Ham release
static int aht20_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "AHT20 device closed\n");
    return 0;
}


// Ham probe
static int aht20_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    aht20_client = client;

    // Tạo một char device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major number\n");
        return major_number;
    }

    aht20_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(aht20_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to register device class\n");
        return PTR_ERR(aht20_class);
    }

    aht20_device = device_create(aht20_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(aht20_device)) {
        class_destroy(aht20_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create the device\n");
        return PTR_ERR(aht20_device);
    }

    printk(KERN_INFO "AHT20 driver installed\n");
    return 0;
}


// Ham remove
static void aht20_remove(struct i2c_client *client)
{
    device_destroy(aht20_class, MKDEV(major_number, 0));
    class_unregister(aht20_class);
    class_destroy(aht20_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "AHT20 driver removed\n");
}


// Dang ki dia chi cho aht20 in driver I2C
static const struct of_device_id aht20_of_match[] = {
    { .compatible = "adafruit,aht20", },
    { },
};

MODULE_DEVICE_TABLE(of, aht20_of_match);

static struct i2c_driver aht20_driver = {
    .driver = {
        .name           = DEVICE_NAME,
        .owner          = THIS_MODULE,
        .of_match_table = of_match_ptr(aht20_of_match),
    },
    .probe      = aht20_probe,
    .remove     = aht20_remove,
};



// Ham init
static int __init aht20_init(void)
{
    printk(KERN_INFO "Initializing AHT20 driver\n");
    return i2c_add_driver(&aht20_driver);
}

// Ham exit
static void __exit aht20_exit(void)
{
    printk(KERN_INFO "Exit AHT20 driver\n");
    i2c_del_driver(&aht20_driver);
}


module_init(aht20_init);
module_exit(aht20_exit);

MODULE_AUTHOR("NamVanDuyCuong");
MODULE_DESCRIPTION("AHT20 I2C Client Driver with IOCTL");
MODULE_LICENSE("GPL");