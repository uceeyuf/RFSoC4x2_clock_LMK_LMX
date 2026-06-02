#include "../LMK_LMX.h"
#include "linux/gpio/consumer.h"
#include "linux/printk.h"
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/uacce.h>

// 设备数据结构体
struct soc_spi_dev {
  dev_t devid;
  struct cdev cdev;
  struct class *class;
  struct device *device;
  struct spi_device *client;
  int RST;
  int SEL0;
  int SEL1;
};

static int char_dev_open(struct inode *inode_p, struct file *file_p) {
  int ret = 0;
  return ret;
}

static int char_dev_release(struct inode *inode_p, struct file *file_p) {
  return 0;
}

static u32 read_reg(u32 reg, struct soc_spi_dev *soc_spi) {
  u32 readback = 0;
  u8 TxBuffer[3];
  u8 RxBuffer[3];
  struct spi_transfer t;
  struct spi_message m;
  int ret;

  TxBuffer[0] = (reg >> 16) & 0xFF;
  TxBuffer[1] = (reg >> 8) & 0xFF;
  TxBuffer[2] = reg & 0xFF;
  memset(&t, 0, sizeof(t));
  t.tx_buf = TxBuffer;
  t.rx_buf = RxBuffer;
  t.len = 3;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);
  ret = spi_sync(soc_spi->client, &m);
  if (ret) {
    pr_err("SPI transfer failed\n");
    return ret;
  }
  // 打印读取的数据
  // pr_info("Received data: %02x %02x %02x\n", RxBuffer[0], RxBuffer[1],
  // RxBuffer[2]);
  readback = (RxBuffer[0] << 16) | (RxBuffer[1] << 8) | RxBuffer[2];
  return readback;
}

static int write_reg(u32 reg, struct soc_spi_dev *soc_spi) {
  int ret;
  u8 TxBuffer[3];
  struct spi_transfer t;
  struct spi_message m;
  TxBuffer[0] = (reg >> 16) & 0xFF;
  TxBuffer[1] = (reg >> 8) & 0xFF;
  TxBuffer[2] = reg & 0xFF;
  memset(&t, 0, sizeof(t));
  t.tx_buf = TxBuffer;
  t.len = 3;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);

  ret = spi_sync(soc_spi->client, &m);
  if (ret) {
    pr_err("SPI transfer failed\n");
    return ret;
  }
  return 0;
}

static int clock_reg_set_sequence(struct soc_spi_dev *soc_spi, u32 *reg_array,
                                  int count) {
  int i;
  int ret;
  for (i = 0; i < count; i++) {
    ret = write_reg(reg_array[i], soc_spi);
    if (ret) {
      pr_err("Failed to write reg at index %d\n", i);
      return ret;
    }
    udelay(1000); // 每次写入后延时20us
  }
  return 0;
}

static ssize_t char_dev_read(struct file *file_p, char __user *buf, size_t len,
                             loff_t *loff_t_p) {
  // int ret = 0;
  struct soc_spi_dev *soc_spi;
  u32 reg;
  u32 readback;

  soc_spi = container_of(file_p->f_inode->i_cdev, struct soc_spi_dev, cdev);
  if (!soc_spi || !soc_spi->client)
    return -ENODEV;

  // 设置 LMK 进入 SPI 读回模式
  reg = 0x016E3B;
  write_reg(reg, soc_spi);

  // 读取PLL1状态
  reg = 0x018200;
  write_reg(reg, soc_spi);
  reg = 0x018201;
  write_reg(reg, soc_spi);
  reg = 0x818200;
  readback = read_reg(reg, soc_spi);

  // 设置 LMK 返回 LED 显示模式
  reg = 0x016E13;
  write_reg(reg, soc_spi);

  // 将读取的值复制到用户空间
  if (copy_to_user(buf, &readback, sizeof(readback))) {
    pr_err("Failed to copy data to user\n");
    return -EFAULT;
  }
  return sizeof(readback);
}

static ssize_t char_dev_write(struct file *file_p, const char __user *buf,
                              size_t len, loff_t *loff_t_p) {
  struct soc_spi_dev *soc_spi;
  int ret;
  char kernel_buf[256] = {0};

  // 获取 spi_device
  soc_spi = container_of(file_p->f_inode->i_cdev, struct soc_spi_dev, cdev);
  if (!soc_spi || !soc_spi->client)
    return -ENODEV;

  // 从用户空间复制数据到内核空间
  if (len >= sizeof(kernel_buf))
    len = sizeof(kernel_buf) - 1;

  if (copy_from_user(kernel_buf, buf, len)) {
    pr_err("Failed to copy data from user\n");
    return -EFAULT;
  }
  kernel_buf[len] = '\0'; // 确保字符串结束

  // 先复位 LMK
  soc_spi->RST = of_get_named_gpio(soc_spi->client->dev.of_node, "lmk-rst", 0);
  soc_spi->SEL0 =
      of_get_named_gpio(soc_spi->client->dev.of_node, "lmk-sel0", 0);
  soc_spi->SEL1 =
      of_get_named_gpio(soc_spi->client->dev.of_node, "lmk-sel1", 0);
  ret = gpio_request(soc_spi->RST, "lmk_rst");
  gpiod_direction_output(gpio_to_desc(soc_spi->RST), 0);
  gpiod_set_value_cansleep(gpio_to_desc(soc_spi->RST), 1);
  gpiod_set_value_cansleep(gpio_to_desc(soc_spi->RST), 0);
  msleep(10); // 等待复位完成

  ret = gpio_request(soc_spi->SEL0, "lmk_sel0");
  gpiod_direction_output(gpio_to_desc(soc_spi->SEL0), 0);
  gpiod_set_value_cansleep(gpio_to_desc(soc_spi->SEL0), 0);
  msleep(10); // 等待 SEL0 设置完成

  ret = gpio_request(soc_spi->SEL1, "lmk_sel1");
  gpiod_direction_output(gpio_to_desc(soc_spi->SEL1), 0);
  gpiod_set_value_cansleep(gpio_to_desc(soc_spi->SEL1), 0);
  msleep(10); // 等待 SEL1 设置完成

  // 根据从用户空间传入的字符串选择不同的寄存器配置
  if (strcmp(kernel_buf, "default") == 0) {
    ret = clock_reg_set_sequence(soc_spi, ClockingLmk_reg, LMK04828_count);
    if (ret) {
      pr_err("Failed to set LMK registers (default)\n");
      return ret;
    }
    pr_info("LMK configured with default settings\n");
  } else if (strcmp(kernel_buf, "ext") == 0) {
    ret =
        clock_reg_set_sequence(soc_spi, ClockingLmk_reg_ext_in, LMK04828_count);
    if (ret) {
      pr_err("Failed to set LMK registers (ext)\n");
      return ret;
    }
    pr_info("LMK configured with external clock settings\n");
  } else {
    pr_warn("Unknown configuration: %s, using default\n", kernel_buf);
    ret = clock_reg_set_sequence(soc_spi, ClockingLmk_reg, LMK04828_count);
    if (ret) {
      pr_err("Failed to set LMK registers (default fallback)\n");
      return ret;
    }
  }
  return len;
}

static struct file_operations char_dev_opt = {
    .owner = THIS_MODULE,
    .open = char_dev_open,
    .read = char_dev_read,
    .write = char_dev_write,
    .release = char_dev_release,
};

static int soc_spi_probe(struct spi_device *spi) {
  int ret;
  struct soc_spi_dev *soc_spi;
  const char *label = "soc_spi_dev";

  // 获取设备树中 label
  if (!of_property_read_string(spi->dev.of_node, "label", &label))
    dev_info(&spi->dev, "Label from DT: %s\n", label);
  else
    dev_warn(&spi->dev, "No label found in device tree, using default: %s\n",
             label);

  // 给设备结构体指针分配内存
  soc_spi = devm_kzalloc(&spi->dev, sizeof(struct soc_spi_dev), GFP_KERNEL);
  if (!soc_spi) {
    dev_err(&spi->dev, "Failed to allocate soc_spi_dev struct\n");
    return -ENOMEM;
  }
  soc_spi->client = spi;

  // 类似platform_set_drvdata，将设备数据和设备绑定
  spi_set_drvdata(spi, soc_spi);

  soc_spi->RST = of_get_named_gpio(soc_spi->client->dev.of_node, "lmk-rst", 0);
  soc_spi->SEL0 =
      of_get_named_gpio(soc_spi->client->dev.of_node, "lmk-sel0", 0);
  soc_spi->SEL1 =
      of_get_named_gpio(soc_spi->client->dev.of_node, "lmk-sel1", 0);
  ret = gpio_request(soc_spi->RST, "lmk_rst");
  if (ret < 0) {
    pr_err("gpio failed\n");
  }
  ret = gpio_request(soc_spi->SEL0, "lmk_sel0");
  if (ret < 0) {
    pr_err("gpio failed\n");
  }
  ret = gpio_request(soc_spi->SEL1, "lmk_sel1");
  if (ret < 0) {
    pr_err("gpio failed\n");
  }

  soc_spi->client->mode = SPI_MODE_0;
  soc_spi->client->bits_per_word = 8;
  soc_spi->client->max_speed_hz = 500000; // 设置 SPI 最大速度为 500kHz
  spi_setup(soc_spi->client);

  // 先复位 LMK
  gpiod_direction_output(gpio_to_desc(soc_spi->RST), 1);
  msleep(1);
  gpiod_set_value_cansleep(gpio_to_desc(soc_spi->RST), 0);
  gpiod_direction_output(gpio_to_desc(soc_spi->SEL0), 0);
  gpiod_direction_output(gpio_to_desc(soc_spi->SEL1), 0);

  // 动态分配设备号
  ret = alloc_chrdev_region(&soc_spi->devid, 0, 1, label);
  if (ret) {
    dev_err(&spi->dev, "alloc_chrdev_region failed\n");
    return ret;
  }
  // 初始化字符设备
  cdev_init(&soc_spi->cdev, &char_dev_opt);
  soc_spi->cdev.owner = THIS_MODULE;
  // 添加字符设备
  ret = cdev_add(&soc_spi->cdev, soc_spi->devid, 1);
  if (ret) {
    dev_err(&spi->dev, "cdev_add failed\n");
    unregister_chrdev_region(soc_spi->devid, 1);
    return ret;
  }
  // 创建类
  soc_spi->class = class_create("soc_spi_lmk");
  if (IS_ERR(soc_spi->class)) {
    ret = PTR_ERR(soc_spi->class);
    dev_err(&spi->dev, "class_create failed\n");
    cdev_del(&soc_spi->cdev);
    unregister_chrdev_region(soc_spi->devid, 1);
    goto err;
  }
  // 创建设备文件

  soc_spi->device =
      device_create(soc_spi->class, NULL, soc_spi->devid, NULL, label);
  if (IS_ERR(soc_spi->device)) {
    ret = PTR_ERR(soc_spi->device);
    dev_err(&spi->dev, "device_create failed\n");
    class_destroy(soc_spi->class);
    cdev_del(&soc_spi->cdev);
    unregister_chrdev_region(soc_spi->devid, 1);
    goto err;
  }
  dev_info(&spi->dev,
           "Character device created: /dev/%s (major=%d, minor=%d)\n", label,
           MAJOR(soc_spi->devid), MINOR(soc_spi->devid));
  return 0;

err:
  return ret;
}

static void soc_spi_remove(struct spi_device *client) {
  struct soc_spi_dev *soc_spi = spi_get_drvdata(client);
  device_destroy(soc_spi->class, soc_spi->devid);
  class_destroy(soc_spi->class);
  cdev_del(&soc_spi->cdev);
  unregister_chrdev_region(soc_spi->devid, 1);
  gpio_free(soc_spi->RST);
  gpio_free(soc_spi->SEL0);
  gpio_free(soc_spi->SEL1);
}
static const struct of_device_id soc_spi_of_match[] = {
    {.compatible = "soc_spi_lmk"},
    {/*end of table*/},
};

// static const struct spi_device_id soc_spi_id[] = {
//     {"soc_spi_lmk"},
//     { },
// };

// 定义结构体变量，表示设备驱动
static struct spi_driver soc_spi_drv = {
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "lmk04828",
            .of_match_table = soc_spi_of_match,
        },
    //.id_table = soc_spi_id,
    .probe = soc_spi_probe,
    .remove = soc_spi_remove,
};

// 驱动入口函数，注册驱动
static int soc_spi_init(void) {
  printk(KERN_INFO "soc_spi_lmk driver init\n");
  return spi_register_driver(&soc_spi_drv);
}
// 驱动出口函数
static void soc_spi_exit(void) {
  printk(KERN_INFO "soc_spi_lmk driver exit\n");
  spi_unregister_driver(&soc_spi_drv);
}

module_init(soc_spi_init);
module_exit(soc_spi_exit);
MODULE_LICENSE("GPL");