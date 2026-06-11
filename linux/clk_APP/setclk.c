#include "fcntl.h"
#include "stdio.h"
#include "unistd.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  int fd, ret;
  char *filename;
  unsigned char buf[256] = {0};
  unsigned int readback;
  int buf_len = 0;

  // 检查命令行参数
  if (argc > 1 && argv[1][0] == '-') {
    strcpy((char *)buf, argv[1] + 1); // 复制'-'后面的字符串
    buf_len = strlen((char *)buf);
  } else {
    strcpy((char *)buf, "default");
    buf_len = strlen((char *)buf);
  }

  ret = system("depmod");
  ret = system("modprobe ./spi_lmk.ko");
  ret = system("modprobe ./spi_lmx.ko");

  // 写入 LMK
  filename = "/dev/lmk";
  fd = open(filename, O_RDWR);
  if (fd < 0) {
    printf("open file fail\n");
    return -1;
  }
  ret = write(fd, buf, buf_len); // 写入自定义字符串
  if (ret < 0) {
    printf("write file fail\n");
    return -1;
  }

  ret = close(fd);
  if (ret < 0) {
    printf("close file fail\n");
    return -1;
  }

  // 写入 LMX1
  filename = "/dev/lmx1";
  fd = open(filename, O_RDWR);
  if (fd < 0) {
    printf("open file fail\n");
    return -1;
  }
  ret = write(fd, buf, sizeof(buf));
  if (ret < 0) {
    printf("write file fail\n");
    return -1;
  }
  ret = close(fd);
  if (ret < 0) {
    printf("close file fail\n");
    return -1;
  }

  // 写入 LMX2
  filename = "/dev/lmx2";
  fd = open(filename, O_RDWR);
  if (fd < 0) {
    printf("open file fail\n");
    return -1;
  }
  ret = write(fd, buf, sizeof(buf));
  if (ret < 0) {
    printf("write file fail\n");
    return -1;
  }
  ret = close(fd);
  if (ret < 0) {
    printf("close file fail\n");
    return -1;
  }

  return 0;
}