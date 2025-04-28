#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <wait.h>
int main(int argc, char *argv[]) {
    static char line[4096];
    int count = 0;
    const char* expr = "expr_wrapper";
    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        

        char temp[128] = "tmp/XXXXXX";
        int fd = mkstemp(temp);
        if (fd == -1){
            perror("mkstemp");
            close(fd);
            return 1;
        }

        char* func_name = malloc(sizeof(char) * (strlen(expr) + 2));
        snprintf(func_name + strlen(expr), 1, "%d", count);
        func_name[strlen(expr)+1] = '\0';

        size_t len_line = strlen(line);
        char *p_line = (char *)malloc(len_line+1);
        strncpy(p_line, line, len_line);
        p_line[len_line] = '\0';

        char routine[256];
        snprintf(routine, sizeof(routine), "int %s(){return %s;}", func_name, p_line);
        write(fd, routine, strlen(routine));


        char new_name[256];
        char lib_name[256];


        snprintf(lib_name, sizeof(lib_name), "tmp/lib%s.so", func_name);
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
            execlp("gcc","gcc", "-FPIC", "-shared", p_new_name, "-o", p_lib_name, NULL);
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
        
        void *handle;
        int (*excu)(void);
        char *error;

        handle = dlopen(p_lib_name, RTLD_LAZY);
        if(!handle){
            perror("handle");
            return 1;
        }

        dlerror();

        *(void **) (&excu) = dlsym(handle, func_name);
        if((error = dlerror()) != NULL){
            perror("dlsym");
            dlclose(handle);
            return 1;
        }

        printf("= %d\n", excu());

        dlclose(handle);

        count++;
    }
}
