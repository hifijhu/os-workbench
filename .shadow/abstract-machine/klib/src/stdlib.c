#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void itoa(int num, char *str, int base) {
  char *p = str;
  int is_negative = 0;

  // 处理负数
  if (num < 0 && base == 10) {
    is_negative = 1;
    num = -num;
  }

  // 将数字转换为字符串（从低位到高位）
  do {
    int digit = num % base;
    *p++ = (digit > 9) ? (digit - 10) + 'a' : digit + '0';
    num /= base;
  } while (num > 0);

  // 添加负号
  if (is_negative) {
    *p++ = '-';
  }

  // 添加字符串结束符
  *p = '\0';

  // 反转字符串
  for (char *start = str, *end = p - 1; start < end; start++, end--) {
    char temp = *start;
    *start = *end;
    *end = temp;
  }
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  // panic("Not implemented");
#endif
  return NULL;
}

void free(void *ptr) {
}

#endif
