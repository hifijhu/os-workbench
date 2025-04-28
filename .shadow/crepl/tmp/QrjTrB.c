#include <unistd.h>
int main(){ int result = 2+2
;
write(STDOUT_FILENO, &result, sizeof(result));}