#include <stdio.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <tree.h>

void print_func(struct tree* t, struct Node* node, int depth) {
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("name: %s", node->name);
  for(int i = 0; i < node->cid_size; i++) {
    print_func(t, &t->nodes[node->cid[i]], depth + 1);
  }
}
int main(int argc, char *argv[]) {

  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  
  struct tree* t = malloc(sizeof(struct tree));
  t->tree_size = 0;
  t->tree_capacity = 1024;
  t->nodes = malloc(sizeof(struct Node) * t->tree_capacity);
  assert(t->nodes);
  // 用malloc等函数分配内存

  char name[256];
  DIR* dp;
  char* path = "/proc";
  if ((dp = opendir(path)) == NULL){
    fprintf(stderr, "error happend: %s\n", path);
    return 0;
  }

  struct dirent* stp;

  while ((stp = readdir(dp)) != NULL){
    if (strcmp(stp->d_name, ".") == 0 || strcmp(stp->d_name, "..") == 0)
      continue;
    if (strlen(path) + strlen(stp->d_name) + 2 > sizeof(name)) {
      fprintf(stderr, "fetchdir name %s %s too long\n", path, stp->d_name);
    } 
    else {
        int tindex;
        if (stp->d_name[0] >= '0' && stp->d_name[0] <= '9') {
          tindex = atoi(stp->d_name);
        }
        else {
          continue;
        }
        char filename[512];
        snprintf(filename, sizeof(filename), "%s/%s/status", path, stp->d_name);
        FILE *fp = fopen(filename, "r");
        if (!fp) goto release;

        char line[256];
        while (fgets(line, sizeof(line), fp)) {
          if (tindex >= t->tree_capacity) {
            t->tree_capacity *= 2;
            t->nodes = realloc(t->nodes, sizeof(struct Node) * t->tree_capacity);
            assert(t->nodes);
          }
          struct Node* node = &t->nodes[tindex];
          if (strncmp(line, "Name:", 5) == 0) {
            strcpy(node->name, line + 5);
          }
          if (strncmp(line, "Pid:", 4) == 0) {
            node->pid = atoi(line + 4);
          }
          if (strncmp(line, "PPid:", 5) == 0) {
            node->ppid = atoi(line + 5);
            struct Node* parent = &t->nodes[node->ppid];
            parent->cid[parent->cid_size++] = tindex;
            t->tree_size++;
            break;
          }
        }
        // 用fscanf, fgets等函数读取

      release:
        if (fp) fclose(fp);
    }
  }
  closedir(dp);



  return 0;
}
