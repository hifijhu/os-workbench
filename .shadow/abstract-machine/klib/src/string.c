#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  // panic("strlen");
  size_t len = 0;
  while(*s) {
    s++;
    len++;
  }
  return len;

}

char *strcpy(char *dst, const char *src) {
  // panic("strcpy not implemented");
  size_t len = 0;
  while(*src != '\0') {
    dst[len] = src[len];
    len++;
  }
  dst[len] = '\0';
  return dst;
}  

char *strncpy(char *dst, const char *src, size_t n) {
  // panic("strncpy");
  size_t len = 0;
  while(len < n && src[len] != '\0') {
    dst[len] = src[len];
    len++;
  }
  while(len < n) {
    dst[len] = '\0';
    len++;
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  // panic("strcat not implemented");
  char *p = dst;
  while(*p != '\0'){
    p++;
  }
  while(*src != '\0'){
    *p = *src;
    p++;
    src++;
  }
  *p = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {

  while(*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
    s1++;
    s2++;
  }
  return *s1 - *s2;

}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("strncmp not implemented");
  size_t i = 0;
  while(*s1++ == *s2++ && *s1 != '\0' && *s2 != '\0' && i++ < n) {}
  return *s1 - *s2;
}

void *memset(void *s, int c, size_t n) {
  assert(c<=255 && c>=0);
  uintptr_t u = (uintptr_t)s;
  while(n-- > 0) {
    *(char *)u++ = (char)c;
  }
  return s;

}

void *memmove(void *dst, const void *src, size_t n) {
  panic("memmove(dst, src, n)");
  /*
  assert(dst && src);
  assert(sizeof(dst) >= n && sizeof(src) >= n && n >= 0);

  uintptr_t u = (uintptr_t)dst;
  uintptr_t l = (uintptr_t)src;
  if(u < l) {
    for(; n > 0; n--) {
    *(char *)u = *(char *)l;
    (char *)u++;
    (char *)l++;
    }
  }
  else{
    for(; n > 0; n--) {
      *((char *)u + n)= *((char *)l + n);
      (char *)u++;
      (char *)l++;
    }
  }


  return dst;
  */
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("memcpy not implemented");
  /*
  assert(out && in);
  assert(sizeof(out) >= n && sizeof(in) >= n);
  uintptr_t u = (uintptr_t)out;
  uintptr_t l = (uintptr_t)in;
  for(; n > 0; n--) {
    *(char *)u = *(char *)l;
    (char *)u++;
    (char *)l++;
  }
  */
}

int memcmp(const void *s1, const void *s2, size_t n) {
  // panic("Not implemented");
  assert(s1 && s2);
  assert(n > 0);
  uintptr_t u = (uintptr_t)s1;
  uintptr_t l = (uintptr_t)s2;
  while(*(char *)u++ == *(char *)l++ && n-- > 0) {}
  return *(char *)u - *(char *)l;
}

#endif
