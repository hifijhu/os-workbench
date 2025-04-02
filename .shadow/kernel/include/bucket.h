#include "lock.h"
#include "ctrlblk.h"
#define BUCKET_SZ 8
struct bucket{
    struct Lock locks[BUCKET_SZ];
    struct ctrlblk* headers[BUCKET_SZ];
};
