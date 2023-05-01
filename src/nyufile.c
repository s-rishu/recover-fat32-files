/*References:
1. https://stackoverflow.com/questions/33718646/how-to-detect-end-of-directory-in-the-data-area-of-fat32
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

//define relevant data structures
#pragma pack(push,1)
typedef struct BootEntry {
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
  unsigned char  BPB_NumFATs;       // Number of FATs
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct DirEntry {
  unsigned char  DIR_Name[11];      // File name
  unsigned char  DIR_Attr;          // File attributes
  unsigned char  DIR_NTRes;         // Reserved
  unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
  unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;       // Created day
  unsigned short DIR_LstAccDate;    // Accessed day
  unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
  unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;       // Written day
  unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
  unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)

void checkEndOfOptions(int opt, char *err){
    if (opt != -1){ // no disk name or more than required arguments
        printf("in getDiskName %s %d", err, optind);
        exit(0);
    }
}

void printFSInfo(int fd){
    struct BootEntry *boot;
    boot = (struct BootEntry *)mmap(0, sizeof(struct BootEntry), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("Number of FATs = %d\nNumber of bytes per sector = %d\nNumber of sectors per cluster = %d\nNumber of reserved sectors = %d\n", boot->BPB_NumFATs, boot->BPB_BytsPerSec, boot->BPB_SecPerClus, boot->BPB_RsvdSecCnt);
}

void printRootDirInfo(char* mmap_base){
    struct DirEntry *root;
    struct BootEntry *boot;
    struct DirEntry *root_dir;
    
    int *fat;
    //printf("debug1\n");
    // boot = (struct BootEntry *)mmap(0, sizeof(struct BootEntry), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    boot = (struct BootEntry *)mmap_base;
    int fat_entry_count = (int)((boot->BPB_FATSz32*boot->BPB_BytsPerSec)/4);
    fat = (int *)(mmap_base+(boot->BPB_BytsPerSec*boot->BPB_RsvdSecCnt));
    //fat = (int *)mmap(NULL, fat_entry_count*sizeof(fat), PROT_READ, MAP_SHARED, fd,  boot->BPB_BytsPerSec*boot->BPB_RsvdSecCnt);
    //calculate offset for root directory cluster
    //printf("debug2\n");
    off_t offset = boot->BPB_BytsPerSec*(boot->BPB_RsvdSecCnt + (boot->BPB_NumFATs*boot->BPB_FATSz32) + ((boot->BPB_RootClus-2)*boot->BPB_SecPerClus));
    //printf("offset: %d\n", offset);
    //printf("debug3\n");
    int max_ent_per_cluster = (int)((boot->BPB_BytsPerSec*boot->BPB_SecPerClus)/32);
    int entry = max_ent_per_cluster;
    char *curr_char = NULL;
    int curr_cluster = boot->BPB_RootClus;
    int entry_count = 0;
    while(1){
        //root_dir = (struct DirEntry *)mmap(NULL, 1*sizeof(root_dir), PROT_READ, MAP_SHARED, fd, offset);
        root_dir = (struct DirEntry *)(mmap_base + offset);
        int idx = max_ent_per_cluster;
        //printf("debug4 %.1s\n", root_dir[idx].DIR_Name);
        while(root_dir->DIR_Name[0] != 0 && idx){
            //printf("debug5\n");
            if(root_dir->DIR_Name[0] == 0xE5){
                idx--;
                root_dir++;
                continue;
            } 
            entry_count++;
            unsigned char* name = root_dir->DIR_Name;
            //printf("debug7\n");
            int dir_flag = ((root_dir->DIR_Attr & 16) != 0);
            //printf("debug8\n");
            if(dir_flag){
                for(int i = 0; i < 11; i++){
                    if(name[i] != ' '){
                        //printf("debug9\n");
                        printf("%c", name[i]);
                    }
                    else{
                        break;
                    }
                }
                printf("/ (starting cluster = %d)\n", (root_dir->DIR_FstClusHI << 8) | root_dir->DIR_FstClusLO);
            }
            else{
                for(int i = 0; i < 8; i++){
                    if(name[i] != ' '){
                        printf("%c", name[i]);
                    }
                    else{
                        break;
                    }
                }
               
                for(int i = 8; i < 11; i++){
                    if(name[i] != ' '){
                        if(i == 8){printf(".");}
                        printf("%c", name[i]);
                    }
                    else{
                        break;
                    }
                }
                if(root_dir->DIR_FileSize){
                    printf(" (size = %d, starting cluster = %d)\n", root_dir->DIR_FileSize, (root_dir->DIR_FstClusHI << 8) | root_dir->DIR_FstClusLO);
                }
                else{
                     printf(" (size = 0)\n");
                }
            }
            idx--;
            root_dir++;
        }
        if(fat[curr_cluster] >=0x0ffffff8){
            break;
        }
        else{
            int cluster_offset = fat[curr_cluster] - curr_cluster;
            curr_cluster = fat[curr_cluster];
            offset = offset + boot->BPB_BytsPerSec*(cluster_offset*boot->BPB_SecPerClus);
        }
        //printf("debug6\n");
    }
    printf("Total number of entries = %d\n", entry_count);

    
}

void recoverContFileSha1(int fd, char *file, char *sha1){
    printf("called recoverContFileSha1");
}

void recoverContFile(int fd, char *file){
    printf("called recoverContFile");
}

void recoverNonContFileSha1(int fd, char *file, char *sha1){
    printf("called recoverNonContFileSha1");
}

int main(int argc, char **argv){
    //command parsing
    char *disk_name = argv[optind];

    char *file_name;
    char *sha1;
    char *error_msg = "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n";
    int fd = open(disk_name, O_RDWR);
    if(fd == -1){
        printf("%s", error_msg);
        exit(0);
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        exit(1);
    }
    char* mmap_base = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //check if atleast one option is necessary
    optind = 2;
    int opt = getopt(argc, argv, "ilr:R:s:");
    switch (opt){
        case 'i':
            //disk_name = getDiskName(optind, argv, error_msg);
            checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
            printFSInfo(fd);
            break;
        case 'l':
            //disk_name = getDiskName(optind, argv, error_msg);
            checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
            printRootDirInfo(mmap_base);
            break;
        case 'r':
            file_name = optarg;
            opt = getopt(argc, argv, "ilr:R:s:");
            //disk_name = getDiskName(optind, argv, error_msg);
            switch (opt){
                case 's':
                    checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
                    sha1 = optarg;
                    recoverContFileSha1(fd, file_name, sha1);
                    break;
                case -1:
                    recoverContFile(fd, file_name);
                    break;
                default:
                    printf("%s", error_msg);
                    exit(0);
            }
            break;
        case 'R':
            file_name = optarg;
            opt = getopt(argc, argv, "ilr:R:s:");
            checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
            //disk_name = getDiskName(optind, argv, error_msg);
            switch (opt){
                case 's':
                    sha1 = optarg;
                    recoverNonContFileSha1(fd, file_name, sha1);
                    break;
                default:
                    printf("%s", error_msg);
                    exit(0);
            }
            break;
        default:
            printf("%s", error_msg);
            exit(0);

    }
    exit(0);
}