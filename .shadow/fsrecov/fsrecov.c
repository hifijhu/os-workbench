#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "fat32.h"

size_t clus_sz;
struct fat32hdr *hdr;
int clus_class[MAX_CLUS];
struct dnode head;

void *mmap_disk(const char *fname);
void disk_scan(u32 clusId, struct dnode* head);

void dir_traversal(struct dnode* head);

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

    clus_sz = hdr->BPB_BytsPerSec * hdr->BPB_SecPerClus;
    // File system traversal.
    disk_scan(hdr->BPB_RootClus, &head);
    dir_traversal(&head);
    
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
        while(! p->LDIR_Ord & LAST_LONG_MASK){
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

void disk_scan(u32 clusId, struct dnode* head){
    struct dnode* p = head;
    while (clusId < CLUS_INVALID && clusId < 100000) {
        struct fat32dent* dent = (struct fat32dent *) cluster_to_sec(clusId);
        struct bmphdr* bmp = (struct bmphdr *)cluster_to_sec(clusId);
        struct lfn* lfn = (struct lfn*)cluster_to_sec(clusId);
        if (strncmp(dent->DIR_Name[8], "BMP", 3) == 0 || strncmp(dent->DIR_Name[0], ".", 1) == 0 || (lfn->LDIR_Attr == 0x0F && lfn->LDIR_FstClusLO == 0 && lfn->LDIR_Type == 0)){
            clus_class[clusId] = CLUS_DIR;
            struct dnode node = {.clusId = clusId, .next = NULL};
            p->next = &node;
            p = p->next;
            clusId++;
        } else if (memcmp(&(bmp->magic_num), "\x42\x4D", 2) == 0){
            clus_class[clusId] = CLUS_BMPHDR;
            clusId++;
            size_t bmp_size = bmp->file_size - clus_sz;
            while (bmp_size > 0){
                clus_class[clusId] = CLUS_BMP;
                bmp_size -= clus_sz;
                clusId++;
            }
        } else {
            clus_class[clusId] = CLUS_FREE;
            clusId++;
        }
    }
    free(p);
}

int recoverpic(u32 clusId, char* path){
    if(clus_class[clusId] != CLUS_BMPHDR) return -1;
    struct bmphdr* bmp = (struct bmphdr*)cluster_to_sec(clusId);
    size_t bmp_size = bmp->file_size;
    size_t height = bmp->height;
    size_t width = bmp->width;
    size_t offset = bmp->offset;
    FILE * fd = fopen(path, "w");
    void* p = cluster_to_sec(clusId);
    while(bmp_size > clus_sz){
        if(clus_class[clus_sz] != CLUS_BMPHDR || clus_class[clus_sz] != CLUS_BMP) return -1;
        write(fd, p, clus_sz);
        clusId++;
        bmp_size -= clus_sz;
        p = cluster_to_sec(clusId);
    }
    write(fd, p, bmp_size);
    fclose(fd);
    return 0;
}

void dir_traversal(struct dnode* head){
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
            
            char order[256];
            snprintf(order, sizeof(order), "sha1sum /tmp/%s", lname);
            u32 dataClus = dent->DIR_FstClusLO | (dent->DIR_FstClusHI << 16);
           
            if(recoverpic(dataClus, order) < 0) {
                continue;
            }
            char checksum[128];
            FILE * fp = popen(order, "r");
            panic_on(fp < 0, "popen");
            fscanf(fp, "%s", checksum); // Get it!
            pclose(fp);
            
            char result[256];
            snprintf(result, "%s  %s\n", checksum, lname);
            FILE * fd = fopen("record.txt", "w");
            write(fd, result, sizeof(result));
        }
        p = p->next;
    }

    free(p);
}

