enum lk_status{
    LOCKED = 0,
    UNLOCKED
};

struct Lock{
    enum lk_status flag;
    char* name;
    int cpu;
};


void spin_init(char* name, struct Lock* lock);
void spin_lock(struct Lock* lock);
void spin_unlock(struct Lock* Lock);