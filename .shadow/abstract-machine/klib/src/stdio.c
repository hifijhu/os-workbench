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
