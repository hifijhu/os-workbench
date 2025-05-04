#include <stdio.h>
#include <assert.h>
#include <regex.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <string.h>
#include "myhash.h"


int main(int argc, char *argv[]) {
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);
    
    if (argc == 1){
        printf("No command.\n");
        return 0;
    }

    

    const char* regex_pattern = "^(\\w+).*<([0-9]*\\.?[0-9]+)>";
    const char* ret_str1 = "exit_group";
    const char* ret_str2 = "+++ exited with";
    
    int cnt = 0;
    double tot = 0;
    regex_t regex;
    regmatch_t matches[3];

    if(regcomp(&regex, regex_pattern, REG_EXTENDED) != 0){
        perror("regcomp");
        return 1;
    }

    char** exec_argv = malloc((argc + 2) * sizeof(char*));
    if (!exec_argv) {
        perror("malloc");
        return 1;
    }

    exec_argv[0] = "strace";
    exec_argv[1] = "-T";
      
    for (int i = 1; i < argc; i++){
        exec_argv[i+1] = argv[i];
    }

    exec_argv[argc+1] = NULL;

    char* exec_path[] = {"PATH=/bin", NULL,};


    int pipe_fd[2];
    pipe(pipe_fd);

    pid_t pid = fork();
    if (pid == 0){
        close(pipe_fd[0]);
        if (dup2(pipe_fd[1], STDERR_FILENO) == -1){
            perror("dup2");
            return -1;
        }
        close(pipe_fd[1]);
        /*int fd = open("out.log", O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (dup2(fd, STDERR_FILENO) == -1){
            perror("dup2");
            close(fd);
            return 1;
        }
        close(fd);*/

        execve("/bin/strace", exec_argv, exec_path);
        perror("execve");
        return 1;
    } else {
        close(pipe_fd[1]);
        char buffer[1024];
        char* line = (char*)malloc(sizeof(char) * 256);
        ssize_t bytes_read;
        ssize_t line_length = 0;
        while((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0){
            for (ssize_t i = 0; i < bytes_read; i++){
                if (buffer[i] == '\n'){
                    cnt++;
                    line[line_length] = '\0';
                    int code;
                    if((code = regexec(&regex, line, 3, matches, 0)) == 0){
                        char func_name[64];
                        char time_cost[64];
                        // 提取函数名（捕获组 1）
                        strncpy(func_name, line + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
                        func_name[matches[1].rm_eo - matches[1].rm_so] = '\0';
        
                        // 提取尖括号中的数字（捕获组 2）
                        strncpy(time_cost, line + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
                        time_cost[matches[2].rm_eo - matches[2].rm_so] = '\0';
                        double time_value = strtod(time_cost, NULL);
                        tot += time_value;
                        if (hsearch(func_name) == -1){
                            hinsert(func_name, time_value);
                        } else {
                            hupdate(func_name, time_value+hsearch(func_name));
                        }
                        if (cnt % 20 == 0){
                            printf("round %d\n", cnt / 20);
                            hprint(tot);
                        }
                            
                    }
                    else{
                        if (strncmp(line, ret_str1, strlen(ret_str1)) == 0 || strncmp(line, ret_str2, strlen(ret_str2)) == 0){
                        } else {
                            char errbuf[64];
                            regerror(code, &regex, errbuf, sizeof(errbuf));
                            fprintf(stderr, "Regex execution error: %s\n", errbuf);
                            perror("regexec");
                            close(pipe_fd[0]);
                            return 1;
                        }
                    }
                    line_length = 0;
                }
                else if (line_length < 255)
                {
                    line[line_length++] = buffer[i];
                }
                else{
                    perror("line_length");
                    return 1;
                }    
            }
        }
        
        
        close(pipe_fd[0]);
        int status;
        waitpid(pid, &status, 0);
        printf("final:\n");
        hprint_all(tot);
    }
    hfree();

    return 0;
}
