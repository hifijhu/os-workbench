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
        char* pre = "#include <stdio.h>\nint main(){ int result = ";
        char* aft = ";\nprintf(\"%%d\",result);}";
        write(fd, pre, strlen(pre));
        write(fd, line , strlen(line));
        write(fd, aft, strlen(aft));

        char new_name[256];
        char exec_name[256];
        //char txt_name[256];
        snprintf(exec_name, sizeof(exec_name), "%s", temp);
        snprintf(new_name, sizeof(new_name), "%s.c", temp);
        //snprintf(txt_name, sizeof(txt_name), "%s.txt", temp);

        size_t len_new_name = strlen(new_name);
        char *p_new_name = (char *)malloc(len_new_name+1);
        strncpy(p_new_name, new_name, len_new_name);
        p_new_name[len_new_name] = '\0';

        size_t len_exec_name = strlen(exec_name);
        char *p_exec_name = (char *)malloc(len_exec_name+1);
        strncpy(p_exec_name, exec_name, len_exec_name);
        p_exec_name[len_exec_name] = '\0';

        if (rename(temp, p_new_name) != 0){
            perror("rename");
            close(fd);
            return 1;
        }

        close(fd);

        int pid = fork();
        if (pid == 0){
            execlp("gcc","gcc",  p_new_name, "-o", p_exec_name, NULL);
            perror("execlp");
            return 1;
        } else if (pid > 0){
            int status;
            waitpid(pid, &status, 0);
        }
        else{
            perror("fork");
            return 1;
        }

        if (pipe(pipe_fd) < 0){
            perror("pipe");
            return 1;
        }
        
        int ppid = fork();
        if (ppid == 0){
            close(pipe_fd[0]);
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[1]);
            char* const pargv[] = {NULL};
            execv(p_exec_name, pargv);

        } else if (ppid > 0){
            close(pipe_fd[1]);
            char buffer[128];
            int sstatus;
            waitpid(ppid, &sstatus, 0); 

            ssize_t n = read(pipe_fd[0], buffer, sizeof(buffer)-1);
            if(n > 0){
                buffer[n] = '\0';
                printf("= %s\n", buffer);
            } else {
                printf("no receive\n");
            }

            close(pipe_fd[0]);
        } else {
            perror("ppid");
            return 1;
        }

        count++;
    }
}
