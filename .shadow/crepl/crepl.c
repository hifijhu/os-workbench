#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <wait.h>
int main(int argc, char *argv[]) {
    static char line[4096];
    int count = 0;
    int flag = 0;
    const char* expr = "expr_wrapper";
    const char* cond = "int ";
    while (1) {
        count++;
        flag = 0;
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        size_t len_line = strlen(line);
        char *p_line = (char *)malloc(len_line+1);
        strncpy(p_line, line, len_line);
        p_line[len_line] = '\0';

        char temp[128] = "tmp/XXXXXX";
        int fd = mkstemp(temp);
        if (fd == -1){
            perror("mkstemp");
            close(fd);
            return 1;
        }

        char routine[256];
        char func_name[256];
        if(strncmp(cond, line, strlen(cond)) == 0){
            flag = 1;
            int i = 0;
            int j = strlen(cond);
            while(i < 255 && line[j] != '('){
                func_name[i] = line[j];
                i++;
                j++;
            }
            func_name[i] = '\0';
            strncpy(routine, line, sizeof(routine));
        }
        else{
            snprintf(func_name, sizeof(func_name), "expr_wrapper%d", count);
            
            snprintf(routine, sizeof(routine), "int expr_wrapper%d(){return %s;}", count, p_line);
        }
        
        size_t len_func_name = strlen(func_name);
        char* p_func_name = (char*) malloc(len_func_name+1);
        strncpy(p_func_name, func_name, len_func_name);
        p_func_name[len_func_name] = '\0';

        write(fd, routine, strlen(routine));


        char new_name[256];
        char lib_name[256];


        snprintf(lib_name, sizeof(lib_name), "tmp/lib%s.so", p_func_name);
        snprintf(new_name, sizeof(new_name), "%s.c", temp);

        size_t len_new_name = strlen(new_name);
        char *p_new_name = (char *)malloc(len_new_name+1);
        strncpy(p_new_name, new_name, len_new_name);
        p_new_name[len_new_name] = '\0';

        size_t len_lib_name = strlen(lib_name);
        char *p_lib_name = (char *)malloc(len_lib_name+1);
        strncpy(p_lib_name, lib_name, len_lib_name);
        p_lib_name[len_lib_name] = '\0';

        if (rename(temp, p_new_name) != 0){
            perror("rename");
            close(fd);
            return 1;
        }

        close(fd);

        int pid = fork();
        if (pid == 0){
            
            execlp("gcc","gcc", "-fPIC", "-shared", p_new_name, "-o", p_lib_name, NULL);
            perror("execlp");
            exit(-1);
        } else if (pid > 0){
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                if (exit_code != 0){
                    fprintf(stderr, "gcc failed with exited code %d\n", exit_code);
                    continue;
                }
            }
        }
        else{
            perror("fork");
            return 1;
        }
        printf("mark1\n");
        void *handle;
        int (*excu)(void);
        char *error;

        handle = dlopen(p_lib_name, RTLD_LAZY | RTLD_GLOBAL);
        if(!handle){
            perror("handle");
            return 1;
        }

        dlerror();
        
        if (flag == 1){
            printf("function %s has loaded.\n", p_func_name);
        }
        else{
            *(void **) (&excu) = dlsym(handle, p_func_name);
            if((error = dlerror()) != NULL){
                perror("dlsym");
                dlclose(handle);
                return 1;
            }

        printf("= %d\n", excu());

        }
        
        //dlclose(handle);

        
    }
}
