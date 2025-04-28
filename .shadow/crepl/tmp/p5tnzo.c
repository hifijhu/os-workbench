#include <unistd.h>
int main(){ int result = 1+1
;
char res[64] = (char)result;
write(STDOUT_FILENO, res, sizeof(res));}