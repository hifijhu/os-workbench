// DO NOT MODIFY: Will be reverted by the Online Judge.

#include <kernel.h>
#include <klib.h>

#include "ctrlblk.h"
enum ops { OP_ALLOC = 1, OP_FREE };

struct malloc_op {
    enum ops type;
    union {
        size_t sz;  // OP_ALLOC: size
        void *addr; // OP_FREE: address
    };
};

struct op_list{
    struct op_list* next;
    void* ptr;
};

typedef struct{
    struct op_list* next;
    size_t size;
}op_header;

op_header header;

static uint32_t round_up_to_power_of_two(uint32_t x) {
    if (x == 0) return 1; // 特殊情况：0 向上取整为 1
    uint32_t power = 1;
    while (power < x) {
        power *= 2;
    }
    return power;
}


static void* op_gain(){
    if(header.size == 0) return NULL;
    void* ptr = header.next->ptr;
    struct op_list* node = header.next;
    header.next = header.next->next;
    pmm->free(node);
    header.size--;
    return ptr;
}

static struct malloc_op random_op(){
    struct malloc_op op;
    int rd = rand();
    if (rd % 2 == 0){
        op.type = OP_ALLOC;
        op.sz = round_up_to_power_of_two(rd);
    }
    else{
        op.type = OP_FREE;
        op.addr = op_gain();
    }
    return op;
}

static void alloc_check(void* ptr, size_t sz){
    if (ptr){
        struct ctrlblk* sptr = (struct ctrlblk*)((char *)ptr - sizeof(struct ctrlblk));
        if (sptr->size == sz){
            struct op_list* node = pmm->alloc(sizeof(struct op_list));
            
            node->next = header.next;
            node->ptr = ptr;
            header.next = node;
            header.size++;
        }
        else{
            assert(1 == 0);
        }
    }
    else{
        assert(0 == 1);
    }
}
void stress_test() {
    int i = 0;
    while (i < 1000000) {
        // 根据 workload 生成操作
        struct malloc_op op = random_op();
        i ++;
        if (i % 1000 == 0) printf("echo: %d\n", i);
        switch (op.type) {
            case OP_ALLOC: {
                void *ptr = pmm->alloc(op.sz);
                alloc_check(ptr, op.sz);
                printf("alloc: %p, size: %d\n", ptr, op.sz);
                break;
            }
            case OP_FREE:{
                if(op.addr == NULL) break;
                pmm->free(op.addr);
                printf("free: %p\n", op.addr);
                break;
            }
        }
    }
}

void test_pmm() {
    int size = 0;
    #define CAPACITY (500)
    #define PAGESIZE (4096)
    char *array[CAPACITY] = {NULL};
    int array_size[CAPACITY] = {0};
    int total = 0, counts[2] = {80, 20};
    uintptr_t BOUNDARY1 = 128, BOUNDARY2 = 32 * 1024;
    while (1) {
      switch (rand() % 2)
      {
        case 0:
          /*
           * 为了模拟workload,我们通过rand()来模拟申请的大小
           *
           * 我们以每一轮50次一个统计
           * 则大量、频繁的小内存分配/释放；其中绝大部分不超过 BOUNDARY1 字节， 这里默认80%概率为小内存分配，也就是80轮
           * 较为频繁的，以物理页面大小为单位的分配/释放 (4 KiB)；这里默认19%概率分配，也就是19轮，大小不超过BOUNDARY2
           * 非常罕见的大内存分配，即1轮，大小不超过BOUNDARY3
           */
          if(size < CAPACITY) {
            for(int mode = rand() % 2; ; mode = (mode + 1) % 2) {
              if(counts[mode]) {
                --counts[mode];
                switch(mode) {
                  case 0:
                    array_size[size] = 1 + (rand() % BOUNDARY1);
                    break;
                  case 1:
                    array_size[size] = PAGESIZE * (1 + (rand() % (BOUNDARY2 / PAGESIZE)));
                    break;

                }
                break;
              }
            }
            if(++total == 100) {
              total = 0;
              counts[0] = 80;
              counts[1] = 20;
            }

            array[size] = (char*)pmm->alloc(array_size[size]);

            assert(array[size] != NULL);
            printf("cpu%d:pmm->alloc(%d) = %p\n", (int)cpu_current(), array_size[size], (uint64_t)(uintptr_t)array[size]);

            //填充，方便进行调试
            for(int i = 0; i < array_size[size]; ++i) { array[size][i] = (char)array_size[size];}
            ++size;
          }
          break;

        case 1:
          if(size) {
            --size;
            for(int i = 0; i < array_size[size]; ++i) {
              assert(array[size][i] == (char)array_size[size]);
            }
            pmm->free((void *)array[size]);
            printf("cpu%d:pmm->free(%p)\n", (int)cpu_current(), (uint64_t)(uintptr_t)array[size]);
          }
          break;
      }
    }
}

void test(){
    while(1){
        int i = cpu_current();
         printf("cpu: %d\n", i);
}
    }

int main() {
    os->init();
    header.next = NULL;
    header.size = 0;
    mpe_init(test_pmm);
    return 1;
}
