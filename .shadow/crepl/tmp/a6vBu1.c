#include <unistd.h>
int main(){ int result = 100*100
;
write(STDOUT_FILENO, &result, sizeof(result));}