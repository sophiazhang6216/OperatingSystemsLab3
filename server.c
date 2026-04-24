#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/epoll.h>
#include "rbtree.h"

#define LISTEN_BACKLOG 50
#define MAX_EVENTS     10
#define BUF_SIZE 2048
#define TRUE 1
#define FALSE 0

enum exit_values {
    SUCCESS = 0,
    WRONG_NUM_ARGS,
    OPEN_SRC_FILE_FAILED,
    OPEN_DESTINATION_FILE_FAILED,
    OPEN_INPUT_SNIPPET_FILE_FAILED
};

typedef struct src_node {
    FILE * ptr;
    char buf[BUF_SIZE];
} src_node;

typedef struct tree_node {
    struct rb_node node;
    char line[BUF_SIZE];
} tree_node;

int handle_error(char* msg) {
    printf("Program error in %s, reason: %s\n", msg, strerror(errno));
    return errno;
}

int count_lines_in_file(const char * src_file_name, int* line_count) {
    FILE * src_file;
    ssize_t nread;
    char * line;
    size_t size = 0;

    src_file = fopen(src_file_name, "r");
    if (src_file == NULL){
        return handle_error("fopen source file");
    }

    while (nread = getline(&line, &size, src_file) != -1) {
        (*line_count)++;
    }

    fclose(src_file);

    return SUCCESS;
}

int open_fragment_files(const char * src_file_name, src_node ** nodes, int** connect_fd) {
    FILE * src_file;
    FILE * dst_file;
    ssize_t nread;
    char * line;
    size_t size = 0;
    int i, ret;
    int line_count = 0;

    ret = count_lines_in_file(src_file_name, &line_count);
    if (ret != SUCCESS) {
        return ret;
    }
    
    connect_fd = malloc(sizeof(int) * (line_count-1)); // don't need to store first line (dest file name)
    if (connect_fd == NULL) {
        return handle_error("malloc connect_fd");
    }

    printf("got through malloc 1\n");

    *nodes = (src_node *)malloc(sizeof(src_node)*(line_count-1));
    if (*nodes == NULL) {
        return handle_error("malloc nodes");
    }

    printf("got through malloc 2\n");
    
    src_file = fopen(src_file_name, "r");
    if (src_file == NULL){
        return handle_error("fopen source file");
    }

    while (nread = getline(&line, &size, src_file) != -1){
        if (i == 0){
            dst_file = fopen(line, "w");
            if (src_file == NULL){
                printf("fopen dest file failed \n");
                return OPEN_DESTINATION_FILE_FAILED;
            }
        }
        else{
            (*nodes)[i].ptr = fopen(line, "r");
            if ((*nodes)[i].ptr == NULL){
                printf("fopen input snippet file failed \n");
                return OPEN_INPUT_SNIPPET_FILE_FAILED;
            }
        }
        i++;
    }

    return SUCCESS;
}

int tree_insert(struct rb_root *root, tree_node *data) //code from kernel.org: official linux kernel archive
{
      struct rb_node **link = &(root->rb_node), *parent = NULL;

      /* Figure out where to put link node */
      while (*link) {
            tree_node *this = rb_entry(*link, tree_node, node);
            int result = strcmp(data->line, this->line);

            parent = *link;
            if (result < 0)
                link = &((*link)->rb_left);
            else if (result > 0)
                link = &((*link)->rb_right);
            else
                return FALSE;
      }

      /* Add link node and rebalance tree. */
      rb_link_node(&data->node, parent, link);
      rb_insert_color(&data->node, root);

      return TRUE;
}

void read_from_file(struct rb_root *root, int idx, src_node * nodes){
    FILE * file_to_read = nodes[idx].ptr;
    char* my_buf = nodes[idx].buf;
    //read
    //if complete
        //make a tree_node
    //else like do the shift
}

struct tree_node *tree_get_and_remove(struct rb_root *root, char *string) //code from kernel.org: official linux kernel archive
{
    struct rb_node *node = root->rb_node;

    while (node) {
        tree_node *data = rb_entry(node, tree_node, node);
        int result;

        result = strcmp(string, data->line);

        if (result < 0)
            node = node->rb_left;
        else if (result > 0)
            node = node->rb_right;
        else
            rb_erase(&data->node, root);
            return data;
    }
    return NULL;
}



int main(int argc, char *argv[]){
    char * file_name;
    int port_num;
    struct epoll_event events[MAX_EVENTS];
    int* connect_fd;
    FILE * src_file;
    FILE * dst_file;
    char * line;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;
    size_t size = 0;
    ssize_t nread;
    int epfd, sfd;
    int finished_clients;
    int line_count = 0;
    src_node * nodes;
    int i = 0;
    int n;
    int ret;
    finished_clients = 0;
    struct rb_root mytree;
    mytree = RB_ROOT;

    if (argc != 3){
        printf("wrong number of cmd line args\n");
        printf("Usage: %s <input_file_name> <port number>\n", argv[0]);
        return WRONG_NUM_ARGS;
    }

    file_name = argv[1];
    port_num = atoi(argv[2]);

    ret = open_fragment_files(file_name, &nodes, &connect_fd);
    if (ret != SUCCESS) {
        // message for specific error reason is printed out within function
        return ret;
    }

    printf("about to socket\n");

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) return handle_error("socket");

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_num);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
        return handle_error("bind");
    if (listen(sfd, LISTEN_BACKLOG) == -1)
        return handle_error("listen");

    epfd = epoll_create1(0);
    if (epfd == -1) return handle_error("epoll_create1");

    for(;;){
        n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n == -1) {
            return handle_error("epoll_wait");
        }
        for (int i = 0; i < n; i++) { //bc epoll can see multiple events ready at the same time gotta loop
            int fd = events[i].data.fd;
            uint32_t ev_mask = events[i].events;
            if(fd == sfd){
                //accept the connection
                //send them a chunk
                //do the bookkeeping
            } else{
                for(int j = 0; j < MAX_EVENTS; j++){
                    if(fd == connect_fd[j]){
                        if (ev_mask & EPOLLIN) {
                            //read in their desc
                            //if full read then add it to the rb tree
                            //repeat til short read
                        }
                        if(ev_mask & EPOLLRDHUP){
                            finished_clients++;
                        }
                    }
                }
            }
        }
        if(finished_clients == line_count-1){
            //all of our chunks have been processed
            break;
        }
    }

    //all of the clients are done so the red black tree is finished
    
    //extract all of the nodes and then write it to the file

    
    // TODO free all the mallocs
    return 0;

}