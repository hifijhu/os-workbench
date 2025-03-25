#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <assert.h>


#define STACK_SIZE 64*1024
#define MAX_CO 512

static inline void
stack_switch_call(void *sp, void *entry, uintptr_t arg) {
    asm volatile (
#if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; call *%1"
          :
          : "b"((uintptr_t)sp),
            "d"(entry),
            "a"(arg)
          : "memory"
#else

            "movl %%esp, -8(%0);"
            "movl %0, %%esp;"
            "subl $8, %%esp;"
            "push %%edx;"
            "push %%ecx;"
            "push %%eax;"
            "push %2;"
            "call *%1;"
          :
          : "b"((uintptr_t)sp - 8),
            "d"(entry),
            "a"(arg)
          : "memory"
#endif
    );
}

enum co_status {
    CO_NEW = 1,
    CO_RUNNING = 2,
    CO_WAITING = 3,
    CO_DEAD = 4,
};

__attribute__((aligned(16))) struct Wrapper {
    void (*func)(void *);
    void *arg;
};
struct co {
    const char *name;
    enum co_status status;

    struct co * waiter;
    jmp_buf context;
    uint8_t stack[STACK_SIZE];
    struct co* next;
    struct Wrapper* warg;
    void (*wrapper)(struct Wrapper *);
};


struct Head {
    int list_size;
    struct co* next;
    struct co* end;
};

struct Head head;

struct co* current;

void init()__attribute__((constructor));

void init(){
    current = (struct co*)malloc(sizeof(struct co));
    current->name = "main";
    current->status = CO_RUNNING;
    current->next = NULL;
    head.list_size = 0;
    head.next = NULL;
    head.end = NULL;
}


void coroutine_wrapper(struct Wrapper* warg) {
    void (*func)(void *) = warg->func;
    void *arg = warg->arg;
    func(arg);
    current->status = CO_DEAD;
    if (current->waiter){

        struct co* p = head.next;
        struct co* pre = NULL;
        while (p!=current->waiter){
            pre = p;
            p = p->next;
        }
        if (p == head.end){
            head.end = pre;
        }
        if (p==head.next){
            head.next = p->next;
        }
        if (pre) pre->next = p->next;
        head.list_size--;

        current = current->waiter;
        current->status = CO_RUNNING;
        longjmp(current->context, 1);
    }
    
    co_yield();
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    
    struct co* thread = (struct co*)malloc(sizeof(struct co));
    thread->warg = (struct Wrapper*)malloc(sizeof(struct Wrapper));
    
    if (!thread) return NULL;
    thread->name = name;
    thread->warg->func = func;
    thread->warg->arg = arg;
    thread->status = CO_NEW;
    thread->waiter = NULL;
    thread->next = NULL;
    thread->wrapper = coroutine_wrapper;

    if (head.end==NULL){
        head.next = thread;
        head.end = thread;
    } else {
        head.end->next = thread;
        head.end = thread;
    }
    head.list_size++;

    return thread;
}

void co_wait(struct co *co) {

    assert(co);

    co->waiter = current;
    
    while (co->status != CO_DEAD){
        co_yield();
    }

    free(co);
}

void co_yield() {

    if(current->status != CO_DEAD){
        current->status = CO_WAITING;

        if(head.end!=NULL){
        head.end->next = current;
            head.end = current; 
        } else {
            head.next = current;
            head.end = current;
        }
        
        head.list_size++;
    }
    
    
    int val = setjmp(current->context);
    if (val == 0) {

        assert(head.list_size!=0);

        int r = rand() % head.list_size;

        struct co* p = head.next;
        struct co* pre = NULL;
        
        while(r!=0){
            assert(p!=NULL);
            pre = p;
            p = p->next;
            r--;
        }

        assert(p!=NULL);
        current = p;
        if (p == head.end){
            head.end = pre;
        }
        if (p == head.next){
            head.next = p->next;
        }
        
        if (pre) pre->next = p->next;
        
        p->next = NULL;
        
        head.list_size--;
    
        assert(current->status!=CO_DEAD);
        assert(current->status!=CO_RUNNING);
        if (current->status == CO_NEW){
            current->status = CO_RUNNING;
            stack_switch_call((void *)current->stack + STACK_SIZE, (void*)current->wrapper, (uintptr_t)current->warg);
        } else {
            current->status = CO_RUNNING;
            longjmp(current->context, 1);
        }
    } else {
        return ;
    }
}
