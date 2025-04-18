enum blk_status{
    MEM_UNAVAILABLE = 0,
    MEM_AVAILABLE 
};

struct ctrlblk{
    enum blk_status is_available;
    int size;
    struct ctrlblk* next;
    struct ctrlblk* prev;
};

