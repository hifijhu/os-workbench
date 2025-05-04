#include "uthash.h"
#include <stdlib.h>
#include <stdio.h>
// 哈希表结构
typedef struct {
    char key[50]; // 键
    double value;    // 值
    UT_hash_handle hh; // uthash 句柄
} HashItem;

HashItem *hash_table = NULL;

// 插入键值对
void hinsert(const char *key, double value) {
    HashItem *item = (HashItem *)malloc(sizeof(HashItem));
    strcpy(item->key, key);
    item->value = value;
    HASH_ADD_STR(hash_table, key, item);
}

double hsearch(const char* key){
    HashItem *item;
    HASH_FIND_STR(hash_table, key, item); // 查找键
    if (item) {
        return item->value; // 返回值
    } else {
        return -1; // 表示未找到
    }
}

// 更新键对应的值
void hupdate(const char *key, double new_value) {
    HashItem *item;
    HASH_FIND_STR(hash_table, key, item); // 查找键
    if (item) {
        item->value = new_value; // 更新值
    } else {
        // 如果键不存在，则插入新键值对
        hinsert(key, new_value);
    }
}

int value_sort_desc(HashItem *a, HashItem *b) {
    return (b->value - a->value); // 降序排序
}

void hprint(double tot) {
    HashItem *item;
    HASH_SORT(hash_table, value_sort_desc);
    int i = 0;
    for (item = hash_table; item != NULL && i < 5; item = (HashItem *)item->hh.next, i++) {
        printf("%s (%.2f%%)\n", item->key, 100*item->value/tot);
    }
}

void hprint_all(double tot) {
    HashItem *item;
    HASH_SORT(hash_table, value_sort_desc);

    for (item = hash_table; item != NULL; item = (HashItem *)item->hh.next) {
        printf("%s (%.2f%%)\n", item->key, 100*item->value/tot);
    }
}

void hfree() {
    HashItem *item, *tmp;
    HASH_ITER(hh, hash_table, item, tmp) {
        HASH_DEL(hash_table, item);
        free(item);
    }
}