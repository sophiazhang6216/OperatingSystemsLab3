#pragma once

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // unlink
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h> //htonl
#include <stdint.h>
#include <netdb.h>
#include <sys/epoll.h>
#include "rbtree.h"
#include <ctype.h> // isspace

#define BUF_SIZE 2048
#define TRUE 1
#define FALSE 0
#define BASE_10 10

typedef struct tree_node {
    struct rb_node node;
    size_t cur_size;
    char line[BUF_SIZE];
    int line_num;
} tree_node;

enum exit_values {
    SUCCESS = 0,
    WRONG_NUM_ARGS
    // other return values are the errno for the failure reason -
    // specific function that caused the error is printed to stdout
};

int handle_error(char* msg);
int get_one_line(char *buf, size_t* cur_size, char * dest);
void add_to_tree(struct rb_root * root, size_t str_len, char * src);
int tree_insert(struct rb_root *root, tree_node *data);
int free_tree(struct rb_root *root);
int tree_print(struct rb_root *root, int fd);
