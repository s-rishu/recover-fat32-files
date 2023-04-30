/*References:

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

void checkEndOfOptions(int opt, char *err){
    if (opt != -1){ // no disk name or more than required arguments
        printf("in getDiskName %s %d", err, optind);
        exit(0);
    }
}

void printFSInfo(char *disk){
    printf("called printFSInfo");
}

void printRootDirInfo(char *disk){
    printf("called printRootDirInfo");
}

void recoverContFileSha1(char *disk, char *file, char *sha1){
    printf("called recoverContFileSha1");
}

void recoverContFile(char *disk, char *file){
    printf("called recoverContFile");
}

void recoverNonContFileSha1(char *disk, char *file, char *sha1){
    printf("called recoverNonContFileSha1");
}

int main(int argc, char **argv){
    //command parsing
    char *disk_name = argv[optind];
    char *file_name;
    char *sha1;
    char *error_msg = "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.";
    //check if atleast one option is necessary
    optind = 2;
    int opt = getopt(argc, argv, "ilr:R:s:");
    switch (opt){
        case 'i':
            //disk_name = getDiskName(optind, argv, error_msg);
            checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
            printFSInfo(disk_name);
            break;
        case 'l':
            //disk_name = getDiskName(optind, argv, error_msg);
            checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
            printRootDirInfo(disk_name);
            break;
        case 'r':
            file_name = optarg;
            opt = getopt(argc, argv, "ilr:R:s:");
            //disk_name = getDiskName(optind, argv, error_msg);
            switch (opt){
                case 's':
                    checkEndOfOptions(getopt(argc, argv, "ilr:R:s:"), error_msg);
                    sha1 = optarg;
                    recoverContFileSha1(disk_name, file_name, sha1);
                    break;
                case -1:
                    recoverContFile(disk_name, file_name);
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
                    recoverNonContFileSha1(disk_name, file_name, sha1);
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