#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>


#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  // panic("printf not implemented");
  
  va_list args;
  va_start(args, fmt);
  int count = 0;
  while (*fmt) {
    if (*fmt == '%') {
      fmt++;
      switch (*fmt) {
        case 'd': {
          int num = va_arg(args, int);
          char buffer[32];
          itoa(num, buffer, 10);
          memcpy((void *)&buffer, (void *)&num, sizeof(buffer));
          char *p = buffer;
          while (*p) {
            putch(*p);
            p++;
          }
          count += strlen(buffer);
          break;
        }
        case 's': {
          char *str = va_arg(args, char*);
          char *p = str;
          while (*p) {
            putch(*p);
            p++;
          }
          count += strlen(str);
          break;
        }
        case 'p': { // 处理指针
          void *ptr = va_arg(args, void*); // 获取指针值
          char buffer[32];
          uintptr_t addr = (uintptr_t)ptr; // 将指针转换为无符号整数
          char *p = buffer;
        
          // 添加 "0x" 前缀
          *p++ = '0';
          *p++ = 'x';
        
          // 将地址转换为十六进制字符串
          itoa(addr, p, 16);
        
          // 从 buffer 的起始位置输出
          char *start = buffer;
          while (*start) {
            putch(*start);
            start++;
          }
          count += strlen(buffer); // 更新输出字符计数
          break;
        }
        default:
          putch(*fmt);
        count++;
        break;
      }
    } else {
      putch(*fmt);
      count++;
    }
    fmt++;
  }

  va_end(args);
  return count;

}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
