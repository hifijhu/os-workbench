#include <unistd.h>
int main(){ int result = 10+10
;
write(STDOUT_FILENO, &result, sizeof(result));}