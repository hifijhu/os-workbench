#include <unistd.h>
int main(){ int result = 10+100
;
write(STDOUT_FILENO, (char *)&result, sizeof((char)result));}