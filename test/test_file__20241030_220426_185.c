#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to parse command line arguments
int parse_args(int argc, char **argv) {
    int opt;
    static char *file_name = NULL;

    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                file_name = optarg;
                break;
            default:
                printf("Invalid argument");
                return 1;
        }
    }

    if (!file_name)
        printf("Please provide the LLVM IR file name as an argument");

    return 0;
}

int main(int argc, char **argv) {
    int ret;

    // Parse command line arguments
    ret = parse_args(argc, argv);

    if (ret != 0)
        return ret;

    // Compile the given LLVM IR file with -O3 optimization flags
    char *cmd = malloc(256);
    sprintf(cmd, "clang -O3 -emit-llvm %s -o output.ll", file_name);
    system(cmd);

    // Parse and print the compiled LLVM IR file
    cmd = malloc(256);
    sprintf(cmd, "clang -O3 -emit-llvm %s | llc -filetype=llvm -o output.ll", file_name);
    system(cmd);

    return 0;
}