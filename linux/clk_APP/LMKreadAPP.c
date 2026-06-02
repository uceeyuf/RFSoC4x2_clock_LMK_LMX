#include "stdio.h"
#include "unistd.h"

#include "fcntl.h"

int main(int argc, char *argv[]) {
  int fd, ret;
  char *filename;
  unsigned int readback; // 改为4字节整型

  filename = "/dev/lmk";
  fd = open(filename, O_RDWR);
  if (fd < 0) {
    printf("open file fail\n");
    return -1;
  }
  ret = read(fd, &readback, sizeof(readback)); // 读取4字节
  if (ret < 0) {
    printf("read file fail\n");
    return -1;
  }

  // 也可以分别显示各个字节
  printf("Bytes: %02x %02x %02x %02x\n", (readback >> 24) & 0xFF,
         (readback >> 16) & 0xFF, (readback >> 8) & 0xFF, readback & 0xFF);
  ret = close(fd);
  if (ret < 0) {
    printf("close file fail\n");
    return -1;
  }

  return 0;
}