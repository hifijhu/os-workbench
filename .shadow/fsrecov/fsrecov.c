#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "fat32.h"

size_t clus_sz;
int clus_max;
struct fat32hdr *hdr;
struct dnode head;

void *mmap_disk(const char *fname);
void disk_scan(u32 clusId, struct dnode* head, int* clus_class);

void dir_traversal(struct dnode* head, int* clus_class);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s fs-image\n", argv[0]);
        exit(1);
    }

    setbuf(stdout, NULL);

    assert(sizeof(struct fat32hdr) == 512);
    assert(sizeof(struct fat32dent) == 32);

    head.clusId = -1;
    head.next = NULL;
    // Map disk image to memory.
    // The file system is a in-memory data structure now.
    hdr = mmap_disk(argv[1]);
    int rootDirSectors = ((hdr->BPB_RootEntCnt * 32) + (hdr->BPB_BytsPerSec-1)) / hdr->BPB_BytsPerSec;
    clus_max = (hdr->BPB_TotSec32 - hdr->BPB_RsvdSecCnt - hdr->BPB_NumFATs * hdr->BPB_FATSz32 - rootDirSectors) / hdr->BPB_SecPerClus;
    clus_sz = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    // File system traversal.
    int clus_class[clus_max+2];
    disk_scan(hdr->BPB_RootClus, &head, clus_class);
    dir_traversal(&head, clus_class);

    munmap(hdr, hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec);
}


void *mmap_disk(const char *fname) {
    int fd = open(fname, O_RDWR);

    if (fd < 0) {
        goto release;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        goto release;
    }

    struct fat32hdr *hdr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == MAP_FAILED) {
        goto release;
    }

    close(fd);

    assert(hdr->Signature_word == 0xaa55); // this is an MBR
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size);

    printf("%s: DOS/MBR boot sector, ", fname);
    printf("OEM-ID \"%s\", ", hdr->BS_OEMName);
    printf("sectors/cluster %d, ", hdr->BPB_SecPerClus);
    printf("sectors %d, ", hdr->BPB_TotSec32);
    printf("sectors %d, ", hdr->BPB_TotSec32);
    printf("sectors/FAT %d, ", hdr->BPB_FATSz32);
    printf("serial number 0x%x\n", hdr->BS_VolID);
    return hdr;

release:
    perror("map disk");
    if (fd > 0) {
        close(fd);
    }
    exit(1);
}


void *cluster_to_sec(int n) {
    // RTFM: Sec 3.5 and 4 (TRICKY)
    // Don't copy code. Write your own.

    u32 DataSec = hdr->BPB_RsvdSecCnt + hdr->BPB_NumFATs * hdr->BPB_FATSz32;
    DataSec += (n - 2) * hdr->BPB_SecPerClus;
    return ((char *)hdr) + DataSec * hdr->BPB_BytsPerSec;
}

void get_filename(struct fat32dent *dent, char *buf, char* lbuf) {
    // RTFM: Sec 6.1

    int len = 0;
    for (int i = 0; i < sizeof(dent->DIR_Name); i++) {
        if (dent->DIR_Name[i] != ' ') {
            if (i == 8)
                buf[len++] = '.';
            buf[len++] = dent->DIR_Name[i];
        }
    }
    buf[len] = '\0';
    
    int llen = 0;
    struct lfn *p = (struct lfn *)(dent - 1);
    if (p->LDIR_Attr == 0x0F){
        while(~ p->LDIR_Ord & LAST_LONG_MASK){
            for (int i = 0; i < sizeof(p->LDIR_Name1); i+=2){
                lbuf[llen++] = p->LDIR_Name1[i]; 
            }
            for (int i = 0; i < sizeof(p->LDIR_Name2); i+=2){
                lbuf[llen++] = p->LDIR_Name2[i]; 
            }
            for (int i = 0; i < sizeof(p->LDIR_Name3); i+=2){
                lbuf[llen++] = p->LDIR_Name3[i]; 
            }
            p--;
            assert(p);
        }
        for (int i = 0; i < sizeof(p->LDIR_Name1); i+=2){
            if (p->LDIR_Name1[i] == 0x00 || p->LDIR_Name1[i] == 0xff) break; 
            lbuf[llen++] = p->LDIR_Name1[i]; 
        }
        for (int i = 0; i < sizeof(p->LDIR_Name2); i+=2){
            if (p->LDIR_Name2[i] == 0x00 || p->LDIR_Name2[i] == 0xff) break; 
            lbuf[llen++] = p->LDIR_Name2[i]; 
        }
        for (int i = 0; i < sizeof(p->LDIR_Name3); i+=2){
            if (p->LDIR_Name3[i] == 0x00 || p->LDIR_Name3[i] == 0xff) break; 
            lbuf[llen++] = p->LDIR_Name3[i]; 
        }
    lbuf[llen] = '\0';
    }
}

void disk_scan(u32 clusId, struct dnode* head, int* clus_class){
    struct dnode* p = head;
    while (clusId < CLUS_INVALID && clusId < clus_max) {
        struct fat32dent* dent = (struct fat32dent *) cluster_to_sec(clusId);
        struct bmphdr* bmp = (struct bmphdr *)cluster_to_sec(clusId);
        struct lfn* lfn = (struct lfn*)cluster_to_sec(clusId);
        if (strncmp((char *)&dent->DIR_Name[8], "BMP", 3) == 0 || strncmp((char *)&dent->DIR_Name, ".   ", 4) == 0 || strncmp((char *)&dent->DIR_Name, "..  ", 4) == 0 || (lfn->LDIR_Attr == 0x0F && lfn->LDIR_FstClusLO == 0x00 && lfn->LDIR_Type == 0x00)){
            clus_class[clusId] = CLUS_DIR;
            struct dnode *node = (struct dnode*)malloc(sizeof(struct dnode));
            if(node == NULL){
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            node->next = NULL;
            node->clusId = clusId;
            p->next = node;
            p = p->next;
            clusId++;
        } else if (memcmp(&(bmp->magic_num), "\x42\x4D", 2) == 0 && bmp->reversed == 0x00000000){
            clus_class[clusId] = CLUS_BMPHDR;
            clusId++;
            int bmp_size = bmp->file_size - clus_sz;
            while (bmp_size > clus_sz){
                clus_class[clusId] = CLUS_BMP;
                bmp_size -= clus_sz;
                clusId++;
            }
        } else {
            clus_class[clusId] = CLUS_FREE;
            clusId++;
        }
    }
}

int recoverpic(u32 clusId, char* path, int *clus_class){
    if(clus_class[clusId] != CLUS_BMPHDR) return -1;
    struct bmphdr* bmp = (struct bmphdr*)cluster_to_sec(clusId);
    size_t bmp_size = bmp->file_size;
    size_t height = bmp->height;
    size_t width = bmp->width;
    size_t offset = bmp->offset;
    FILE * fd = fopen(path, "w");
    if (fd == NULL) {
        perror("Failed to open file");
        fclose(fd);
        return -1;
    }
    void* p = cluster_to_sec(clusId);
    while(bmp_size > clus_sz){
        if((clus_class[clusId] != CLUS_BMPHDR) && (clus_class[clusId] != CLUS_BMP)) {
            fclose(fd);
            perror("wrong class");
            return -1;
        }
        if (fwrite(p, clus_sz, 1, fd) != 1) {
            perror("Failed to write to file");
            fclose(fd);
            return -1;
        }
        clusId++;
        bmp_size -= clus_sz;
        p = cluster_to_sec(clusId);
    }
    if (fwrite(p, bmp_size, 1, fd) != 1) {
        perror("Failed to write to file");
        fclose(fd);
        return -1;
    }
    fclose(fd);
    return 0;
}

void dir_traversal(struct dnode* head, int * clus_class){
    int ndents = clus_sz / sizeof(struct fat32dent);
    struct dnode* p = head->next;
    while (p != NULL){
        u32 clusId = p->clusId;
        assert(clus_class[clusId] == CLUS_DIR);
        for (int d = 0; d < ndents; d++) {
            struct fat32dent* dent = (struct fat32dent *)cluster_to_sec(clusId) + d;
            if (dent->DIR_Name[0] == 0x00 ||
                dent->DIR_Name[0] == 0xe5 ||
                dent->DIR_Attr & ATTR_HIDDEN ||
                dent->DIR_Attr & ATTR_DIRECTORY)
                continue;

            char fname[32];
            char lname[256];
            get_filename(dent, fname, lname);
            
            char path[266];
            snprintf(path, sizeof(path), "tmp/%s", lname);
            char order[288];
            snprintf(order, sizeof(order), "/usr/bin/sha1sum %s", path);
            u32 dataClus = dent->DIR_FstClusLO | (dent->DIR_FstClusHI << 16);
           
            if(recoverpic(dataClus, path, clus_class) < 0) {
                continue;
            }
            char checksum[128];
            FILE *fp = popen(order, "r");
            if (fp == NULL) {
                perror("popen failed");
                exit(EXIT_FAILURE);
            }
            if (fgets(checksum, sizeof(checksum), fp) == NULL) {
                perror("fgets failed");
                pclose(fp);
                exit(EXIT_FAILURE);
            }
            pclose(fp);
            
            char result[512];
            snprintf(result, sizeof(result), "%s  %s\n", checksum, lname);
            FILE * fd = fopen("record.txt", "w");
            fwrite(result, sizeof(result), 1, fd);
        }
        p = p->next;
    }
}

