#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
int main(int argc, char *argv[]) {
    static char line[4096];
    int count = 0;
    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        // To be implemented.
        int pipe_fd[2];
        char temp[128] = "tmp/XXXXXX";
        int fd = mkstemp(temp);
        if (fd == -1){
            perror("mkstemp");
            close(fd);
            return 1;
        }
        char pre[20] = "int main(){ return ";
        char aft[3] = ";}";
        write(fd, pre, strlen(pre));
        write(fd, line , strlen(line));
        write(fd, aft, strlen(aft));

        char new_name[256];
        char exec_name[256];
        snprintf(exec_name, sizeof(exec_name), "%s", temp);
        snprintf(new_name, sizeof(new_name), "%s.c", temp);
        if (rename(temp, new_name) != 0){
            perror("rename");
            close(fd);
            return 1;
        }

        close(fd);
        
        if (pipe(pipe_fd) < 0){
            perror("pipe");
            return 1;
        }

        int pid = fork();
        if (pid == 0){
            execlp("gcc", temp, "-o", exec_name, NULL);
        } else if (pid > 0){
            int status;
            waitpid(pid, &status, 0);
        }
        else{
            perror("fork");
            return 1;
        }
        
        int ppid = fork();
        if (ppid == 0){
            char* const pargv[] = {NULL};
            execv(exec_name, pargv);

        } else if (ppid > 0){
            int sstatus;
            waitpid(ppid, &sstatus, 0);
        } else {
            perror("ppid");
            return 1;
        }

        count++;
    }
}
