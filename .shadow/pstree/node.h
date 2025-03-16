struct Node
{
    char name[256];
    int pid;
    int ppid;
    int cid[1024];
    int cid_size;
};
