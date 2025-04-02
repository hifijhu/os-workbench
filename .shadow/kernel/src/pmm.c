#include <common.h>
#include <bucket.h>

#define CTRLBLK_SZ sizeof(struct ctrlblk)
static uint32_t round_up_to_power_of_two(uint32_t x) {
    if (x == 0) return 1; // 特殊情况：0 向上取整为 1
    uint32_t power = 1;
    while (power < x) {
        power *= 2;
    }
    return power;
}

struct bucket* bucket;
static void lkinit(struct bucket* bucket, uintptr_t onepiece){
    for (int i=0; i<BUCKET_SZ; i++){
        spin_init((char *)&i, &bucket->locks[i]);
        bucket->headers[i] = heap.start + i * onepiece;
        bucket->headers[i]->next = NULL;
        bucket->headers[i]->prev = NULL;
        bucket->headers[i]->size = onepiece - CTRLBLK_SZ;
    }
}


static void *kalloc(size_t size) {
    size = round_up_to_power_of_two(size);
    int cpu = cpu_current();
    spin_lock(&bucket->locks[cpu]);
    assert(bucket->locks[cpu].cpu == cpu && bucket->locks->flag == LOCKED);
    struct ctrlblk* header = bucket->headers[cpu];
    struct ctrlblk* p = header;
    while (p){
        if (p->is_available == MEM_UNAVAILABLE){
            p = p->next;
            continue;
        }
        if(size+CTRLBLK_SZ <= p->size){
            struct ctrlblk* next = (struct ctrlblk *)p +size + CTRLBLK_SZ;
            next->prev = p;
            next->next = NULL;
            p->next = next;
            next->is_available = MEM_AVAILABLE;
            next->size = p->size - size - CTRLBLK_SZ;
            p->size = size;
            p->is_available = MEM_UNAVAILABLE;
        }        
        else{
            p = p->next;
            continue;
        }
    }

    spin_unlock(&bucket->locks[cpu]);
    return p;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
    struct ctrlblk* sptr = (struct ctrlblk *)ptr - CTRLBLK_SZ;
    sptr->is_available = MEM_AVAILABLE;
    if (sptr->next && sptr->next->is_available==MEM_AVAILABLE){
        sptr->next = sptr->next->next;
        sptr->size = sptr->size + sptr->next->size + CTRLBLK_SZ;

    }
    if (sptr->prev && sptr->prev->is_available==MEM_AVAILABLE){
        sptr->prev->next = sptr->next;
        sptr->prev->size = sptr->prev->size + sptr->size + CTRLBLK_SZ;
    }

}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
    uintptr_t onepiece = pmsize / BUCKET_SZ;

    lkinit(bucket, onepiece);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
