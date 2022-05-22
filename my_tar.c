#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <err.h>
#include <setjmp.h>

#define TRY do{ jmp_buf ex_buf__; if( !setjmp(ex_buf__) ){
#define CATCH } else {
#define ETRY } }while(0)
#define THROW longjmp(ex_buf__, 1)


#define LR_SIZE 512
#define SIZE_OCTET 12

struct Header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[SIZE_OCTET];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
};


long int oct_to_int(char* string, int length) {
    long int number;
    char * pEnd;
    number = strtol(string, &pEnd, 8);
    return number;
}


int is_empty(struct Header* header) {
    for (int i = 0; i < LR_SIZE; i++)
        if (((char*)header)[i] != 0)
            return 0;
    return 1;
}

int get_size(FILE *f) {
    fseek(f, 0L, SEEK_END);
    long int sz = ftell(f);
    if (sz == -1)
            errx(2, "error in ftell");
    fseek(f, 0L, SEEK_SET);
    return sz;
}

enum actions {
    none,
    list,
};


int main(int argc, char *argv[]) {
    if (argc == 1)
        errx(2, "not enought arguments");

    int args = 0;
    char** pargs = malloc(sizeof(char*) * argc);
    if (!pargs) {
        errx(2, "malloc error");
    }
    char* file;
    enum actions action = none;
    

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 't':
                action = list;
                break;
                    
                case 'f':
                if (i + 1 == argc)
                    errx(2, "please provide arfument -- 'f'");

                file = argv[++i];
                break;

                default:
                errx(2, "argument %s is not recognized.", argv[i]);
            }
        }
        else
            pargs[args++] = argv[i];
    }

    if (action == none)
        errx(2, "bad action");

    FILE *f;

    if ((f = fopen(file, "r")) == NULL)
        errx(2, "%s: Cannot open: No such file or directory", file);


    int fs = get_size(f);

    bool first_record = true;
    bool prev_empty  = false;
    int lr_count = 0;


    int* used_pargs = calloc(args, sizeof(int));
    if (!used_pargs)
            errx(2, "failed to allocate memory, exiting.");
    
    struct Header* header = malloc(sizeof(char) * LR_SIZE);
    if (!header) {
        errx(2, "malloc error");
    while (true) {
        long int read = fread(header, sizeof(char), LR_SIZE, f);

        if (read != LR_SIZE)
            errx(2, "Block %d incomplete, exiting", lr_count);

        lr_count++;

        if (first_record) {
            if (strcmp("ustar  ", header->magic) != 0) {
                errx(2, "fail status");
            }

            first_record = false;
        }

        if (is_empty(header)) {
            if (prev_empty)
                break;

            prev_empty = true;
            continue;
        }

        prev_empty = false;


        if (header->typeflag[0] != '0')
            errx(2, "Bad header type: %d", header->typeflag[0]);

        long int header_size = oct_to_int(header->size, SIZE_OCTET);
        long int header_diff = (header_size + (LR_SIZE - 1)) / LR_SIZE;
        lr_count += header_diff;

        if (action == list) {
            TRY
            {
                fseek(f, header_diff * LR_SIZE, SEEK_CUR);
            }
            CATCH
            {
                errx(2, "failed fseek");
            }
            ETRY;
            

            if (ftell(f) > fs) {
                errx(2, "EOF");
            }
        }

    
    bool all_found = true;
    for (int i = 0; i < args; i++) {
        if (used_pargs[i] == 0) {
            warnx("%s: Not found", pargs[i]);
            all_found = false;
        }
    }

    if (!all_found)
        errx(2, "failure due to previous errors");
    }
}
}
