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

static void* op_gain(){
    void* ptr = header.next->ptr;
    header.next = header.next->next;
    return ptr;
}
static struct malloc_op random_op(){
    struct malloc_op op;
    int rd = rand();
    if (rd % 2 == 0){
        op.type = OP_ALLOC;
        op.sz = rd;
    }
    else{
        op.type = OP_FREE;
        op.addr = op_gain();
    }
    return op;
}

static void alloc_check(void* ptr, size_t sz){
    if (ptr){
        struct ctrlblk* sptr = ptr - sizeof(struct ctrlblk);
        if (sptr->size == sz){
            struct op_list node;
            node.next = header.next;
            node.ptr = ptr;
            header.next = &node;
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
    while (1) {
        // 根据 workload 生成操作
        struct malloc_op op = random_op();

        switch (op.type) {
            case OP_ALLOC: {
                void *ptr = pmm->alloc(op.sz);
                alloc_check(ptr, op.sz);
                break;
            }
            case OP_FREE:
                pmm->free(op.addr);
                break;
        }
    }
}

int main() {
    os->init();
    header.next = NULL;
    header.size = 0;
    mpe_init(stress_test);
    return 1;
}
