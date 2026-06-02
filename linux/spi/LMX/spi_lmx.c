#include "../LMK_LMX.h"
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
};

static struct class *soc_spi_class;

static int char_dev_open(struct inode *inode_p, struct file *file_p) {
  int ret = 0;
  return ret;
}

static int char_dev_release(struct inode *inode_p, struct file *file_p) {
  return 0;
}

static ssize_t char_dev_read(struct file *file_p, char __user *buf, size_t len,
                             loff_t *loff_t_p) {
  int ret = 0;
  return ret;
}

static ssize_t char_dev_write(struct file *file_p, const char __user *buf,
                              size_t len, loff_t *loff_t_p) {
  struct soc_spi_dev *soc_spi;
  u8 TempBuffer[3];
  int i, ret;
  u32 reg;
  struct spi_transfer t;
  struct spi_message m;

  // 获取 spi_device
  soc_spi = container_of(file_p->f_inode->i_cdev, struct soc_spi_dev, cdev);
  if (!soc_spi || !soc_spi->client)
    return -ENODEV;

  // 先复位 LMX
  reg = 0x020000; // 复位寄存器
  TempBuffer[0] = (reg >> 16) & 0xFF;
  TempBuffer[1] = (reg >> 8) & 0xFF;
  TempBuffer[2] = reg & 0xFF;
  memset(&t, 0, sizeof(t));
  t.tx_buf = TempBuffer;
  t.len = 3;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);

  ret = spi_sync(soc_spi->client, &m);
  if (ret) {
    pr_err("SPI reset failed, ret = %d\n", ret);
    return ret;
  }
  udelay(1000); // 延时20us
  // 清除复位寄存器
  reg = 0x000000; // 清除复位寄存器
  TempBuffer[0] = (reg >> 16) & 0xFF;
  TempBuffer[1] = (reg >> 8) & 0xFF;
  TempBuffer[2] = reg & 0xFF;
  memset(&t, 0, sizeof(t));
  t.tx_buf = TempBuffer;
  t.len = 3;

  spi_message_init(&m);
  spi_message_add_tail(&t, &m);

  ret = spi_sync(soc_spi->client, &m);
  if (ret) {
    pr_err("SPI clear reset failed, ret = %d\n", ret);
    return ret;
  }
  udelay(1000); // 延时20us

  // 写 LMX，循环发送 ClockingLmx_reg[] 中的数据
  for (i = 0; i < LMX2594_count; i++) {
    reg = ClockingLmx_reg[i];
    TempBuffer[0] = (reg >> 16) & 0xFF;
    TempBuffer[1] = (reg >> 8) & 0xFF;
    TempBuffer[2] = reg & 0xFF;

    memset(&t, 0, sizeof(t));
    t.tx_buf = TempBuffer;
    t.len = 3;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);

    ret = spi_sync(soc_spi->client, &m);
    if (ret) {
      pr_err("SPI transfer failed at index %d, ret = %d\n", i, ret);
      return ret;
    }
    udelay(1000); // 每次写入后延时20us
  }

  pr_info("LMX write done\n");
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
  //  soc_spi->class = class_create("soc_spi_lmx1");
  //  if(IS_ERR(soc_spi->class)){
  //      ret = PTR_ERR(soc_spi->class);
  //      dev_err(&spi->dev, "class_create failed\n");
  //      cdev_del(&soc_spi->cdev);
  //      unregister_chrdev_region(soc_spi->devid, 1);
  //      goto err;
  //  }

  // 创建设备文件
  soc_spi->class = soc_spi_class;
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
}
static const struct of_device_id soc_spi_of_match[] = {
    {.compatible = "soc_spi_lmx"},
    {/*end of table*/},
};

// 定义结构体变量，表示设备驱动
static struct spi_driver soc_spi_drv = {
    .driver =
        {
            .owner = THIS_MODULE,
            .name = "lmx2594",
            .of_match_table = soc_spi_of_match,
        },
    .probe = soc_spi_probe,
    .remove = soc_spi_remove,
};

// 驱动入口函数，注册驱动
static int soc_spi_init(void) {
  int ret;
  soc_spi_class = class_create("soc_spi_lmx");
  if (IS_ERR(soc_spi_class)) {
    ret = PTR_ERR(soc_spi_class);
    pr_err("init class_create failed\n");
    return PTR_ERR(soc_spi_class);
  }

  return spi_register_driver(&soc_spi_drv);
}
// 驱动出口函数
static void soc_spi_exit(void) {
  printk(KERN_INFO "soc_spi_lmx driver exit\n");
  spi_unregister_driver(&soc_spi_drv);
}

module_init(soc_spi_init);
module_exit(soc_spi_exit);
MODULE_LICENSE("GPL");