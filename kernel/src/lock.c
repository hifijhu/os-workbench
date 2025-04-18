#include <common.h>
#include <am.h>
#include "lock.h"
void spin_init(char* name, struct Lock* lock){
    lock->flag = UNLOCKED;
    lock->name = name;
    //lock->cpu = 0;
}

void spin_lock(struct Lock* lock){
    int me;
    //assert(lock->cpu != cpu_current());
    while (true){
        me = atomic_xchg((int *)&lock->flag, LOCKED);
        if(me == UNLOCKED){
            //assert(lock->cpu == -1);
            //lock->cpu = cpu_current();
            return;
        }
        else continue;
    }
}

void spin_unlock(struct Lock* lock){
    assert(lock->flag == LOCKED);
    //assert(lock->cpu == cpu_current());
    //lock->cpu = -1;
    lock->flag = UNLOCKED;
}
