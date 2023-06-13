#define MESSAGE_FOUND "Required series of byte found: 0x"
#define OPEN_ERR "Error: impossible to open file %s: %s\n"
#define DIGIT_ERR "Error: string %s couldn't be converted. Symbol %c does not belong to hex digits\n"
#define LENGTH_ERROR "Error: byte series should consist of even number of digits\n"
#define HELP_STR "Use this program this way: <utility_name> [options] <catalogue_name> 0xBB[BB]\nAvailable options: -h, -v, --help, --version\nByte string must start with '0x' and may content as many bytes, as you wish\n"
#define VERSION_STR "This is first and the last version of this utility, so enjoy it as much, as you want\n"
#define UNACCESSIBLE_DIR "Directory %s skipped: access restricted/n"
#define SYMBOLIC_LINK "File %s skipped: it is symbolic link"
#define UNSUPPORTED_FILE "Error: impossible to process file %s. Unsupported format of struct sb."
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ftw.h>
#include <sys/param.h>      // for MIN()
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>

#define MAX_INDENT_LEVEL 128

void walk_dir(char *dir);
char* target;
char* target_dir;
int checkTheFile(const char* path, const struct stat* sb, int typeflag);
unsigned target_length;
unsigned success_counter = 0;

unsigned checkByteString(char* str) {
    unsigned length = strlen(str);
    for (unsigned i=0; i < length; i++) {
        if ((str[i] < '0' || str[i] > '9') && (str[i] < 'A' || str[i] > 'F') && (str[i] != 'x')) {
            fprintf(stderr, DIGIT_ERR, str, str[i]);
            return 1;
        }
    }
    return 0;
}    // check, if only hex symbols are used in "byte string" (except 0x)

unsigned checkByteStringLength(char *str) {
    if (strlen(str)%2 != 0)
    {
        fprintf(stderr, LENGTH_ERROR);
        return 0;
    }
    return (strlen(str) - 2)/2;
}   // return quantity of bytes in string (-1, if length is even)

char readCharAsDigit(char sym) {
    if (sym > '0' && sym <= '9')
    {
        return sym - '0';
    }
    if (sym >= 'A' && sym <= 'F')
    {
        return sym - 'A'+ 10;
    }
    else {
        return -1;
    }
}       // convert hex char as symbol to char as 1 byte value

char* toByteSeries(char* str) {
    char *bytes = calloc(target_length, sizeof(char));
    for (unsigned i=2; i <= target_length*2; i+=2) {
        bytes[i / 2 - 1] += readCharAsDigit(str[i]) * 16 + readCharAsDigit(str[i + 1]);
    }
    return bytes;
}        // convert char string to series of bytes (array)

int processOptions(int argc, char* argv[]) {
    int opt_long = 0;
    int opt_short = 0;
    const struct option long_options[] = {
            { "version", no_argument, &opt_long, 'v' },
            { "help", no_argument, &opt_long, 'h'},
            { NULL, 0, NULL, 0}
    };

    while ((opt_short = getopt_long(argc, argv, "hv", long_options, NULL)) != -1) {
        if (opt_long == 'v' || opt_short == 'v')  printf(VERSION_STR);
        else if (opt_long == 'h' || opt_short == 'h')  printf(HELP_STR);
        opt_long = 0;
        opt_short = 0;
    }
    return 0;
}       // read options and write proper response

int checkDirName(char* dir) {
    DIR* dirP;
    if ((dirP = opendir(dir)) == NULL) {
        fprintf(stderr, "Error: failure while opening directory %s", dir);
        return 1;
    }
    closedir(dirP);
    target_dir = dir;
    return 0;
}

int matches(char* bytes_series) {
    for (unsigned j = 0; j < target_length; j++) {
        if (target[j] != bytes_series[j])
         {
             return 0;
         }
    }
    success_counter++;
    return 1;
}

int searchForSeries(char* bytes, unsigned length)
{
    int res = 0;
    for (unsigned i=0; i <= length - target_length; i++)
        if (matches(bytes+i)) {
            if (getenv("LAB11DEBUG")) {
                printf(MESSAGE_FOUND);
                for (unsigned j=0; j<target_length; j++)
                {
                    printf("%x", bytes[i+j]);
                }
                printf("\nIt starts from byte № %d\n", i+1);
            }
            res++;
        }
    return res;
}
// search for "target" in "bytes"

int checkTheFile(const char* path, const struct stat* sb, int typeflag)
{
    switch (typeflag)
    {
        case FTW_DNR:
            printf(UNACCESSIBLE_DIR, path);
            return 0;
        case FTW_D:
            return 0;
        case FTW_SL:
            printf(SYMBOLIC_LINK, path);
            return 0;
        case FTW_NS:
            printf(UNSUPPORTED_FILE, path);
            return 1;
    }


    // Open file
    FILE* file = fopen(path, "r");
    if (getenv("LAB11DEBUG")) {
        printf("Checking: %s\n", path);
    }
    if (!file) {
        fprintf(stderr, OPEN_ERR, path, strerror(errno));
        return 1;
    }
    
    // Get length of file
    struct stat filestat;
    fstat(fileno(file), &filestat);
    int file_length = sb->st_size;
    
    // Create file buff
    char* file_bytes = calloc(file_length, sizeof(char));
    if (!fread(file_bytes, 1, file_length, file)){
        printf("Error while reading file %s", path);
        free(file_bytes);
        fclose(file);
        return 1;
    }

    // find target series of bytes
    int res=0;
    char* rp;
    if ((res = searchForSeries(file_bytes, file_length))) {
    	rp = realpath(path, NULL);
        printf("Found in file %s (number of exemplars: %d)\n", rp, res);
        free(rp);
    }
    free(file_bytes);
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    //setenv("LAB11DEBUG", "TRUE", 1);

    // Read and check arguments
    if (checkByteString(argv[argc-1])) return EXIT_FAILURE;

    if (!(target_length = checkByteStringLength(argv[argc-1]))) return EXIT_FAILURE;
    target = toByteSeries(argv[argc-1]);
    if (checkDirName(argv[argc-2])) return EXIT_FAILURE;

    // Command line options
    processOptions(argc, argv);

    int result = ftw(target_dir, checkTheFile, 100);
    if (result) {
        fprintf(stderr, "ERROR while file tree walk");
        return 1;
    }

    free(target);
    printf("Total matches: %i\nThanks for using lab11avkN32471\n", success_counter);

    return EXIT_SUCCESS;
}
