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


//give it the buf and some where to put the new line and it will move it there
//it will also move the existing contents into the front of the array
//return the number of lines processed

int handle_error(char* msg) {
    printf("Program error in %s. Reason: %s\n", msg, strerror(errno));
    return errno;
}

int get_one_line(char *buf, size_t* cur_size, char * dest) {
    size_t len;
    char *start, *end, *nl;
    start = buf;
    end   = buf + *cur_size;

    if ((nl = memchr(start, '\n', end - start)) == NULL) {
        return 0; // TODO remove magic number
    }

    len = (size_t)(nl - start + 1);
    memcpy(dest, buf, len);
    start = nl + 1;
    *cur_size = (size_t)(end - start); //move extra to the start
    memmove(buf, start, *cur_size); //memcpy is gonna blow up bc overlap
    return 1; // TODO remove magic number
}

//creates a copy of src - src[str_len], put all the info into a tree_node and inserts it into the tree
void add_to_tree(struct rb_root * root, size_t str_len, char * src) {
    tree_node* t;
    t = malloc(sizeof(*t));
    if(t == NULL){
        handle_error("malloc tree_node");
    }
    t->cur_size = str_len;
    memcpy(t->line, src, str_len);
    tree_insert(root, t);
}
