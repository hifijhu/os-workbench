#include <unistd.h>
int main(){ int result = 1+1
;
write(STDOUT_FILENO, (char *)&result, sizeof((char)result));}