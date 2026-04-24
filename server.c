#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

typedef struct src_node{
    FILE * ptr;

} src_node;

int main(int argc, char *argv[]){

    const char * file_name = argv[1];
    const int port_num = atoi(argv[2]);
    FILE * src_file;
    FILE * dst_file;
    char * line;
    size_t size = 0;
    ssize_t nread;
    int line_count = 0;
    src_node * nodes;
    int i = 0;

    if (argc > 3){
        printf("wrong number of cmd line args\n");
        printf("Usage: ./%s file_name\n", argv[0]);
        return -1;
    }

    src_file = fopen(file_name, "r");
    if (src_file == NULL){
        printf("fopen failed \n");
        return -1;
    }

    while (nread = getline(&line, &size, src_file) != -1){
        line_count++;
    }

    fclose(src_file);

    nodes = (src_node *)malloc(sizeof(src_node)*(line_count-1));

    src_file = fopen(file_name, "r");
    if (src_file == NULL){
        printf("fopen failed \n");
        return -1;
    }

    while (nread = getline(&line, &size, src_file) != -1){
        if (i == 0){
            dst_file = fopen(line, "w");
            if (src_file == NULL){
                printf("fopen dest file failed \n");
                return -2;
            }
        }
        else{
            nodes[i].ptr = fopen(line, "r");
            if (nodes[i].ptr == NULL){
                printf("fopen input snippet file failed \n");
                return -3;
            }
        }
        i++;
    }


}