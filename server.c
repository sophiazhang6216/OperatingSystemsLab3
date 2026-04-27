#include "shared_func.h"
#include <fcntl.h>

#define LISTEN_BACKLOG 50
#define MAX_EVENTS     10
#define EXPECTED_ARGS 3
#define SHELL_IP_ADDR "128.252.167.161" // found in studio 18
#define INF_TIMEOUT -1
#define INVALID_FD -1
#define FILE_NAME_IDX 1
#define PORT_NUM_IDX 2
#define NO_FLAGS 0
#define PROTOCOL 0

typedef struct src_node {
    FILE * ptr;
    size_t cur_size;
    char buf[BUF_SIZE];
} src_node;

//takes in the source file and sees how many snippets there are so we can properly allocate an array
int count_lines_in_file(const char * src_file_name, int* line_count) {
    FILE * src_file;
    ssize_t nread;
    char * line = NULL;
    size_t size = 0;

    *line_count = 0;

    src_file = fopen(src_file_name, "r");
    if (src_file == NULL){
        return handle_error("fopen source file");
    }

    while ((nread = getline(&line, &size, src_file)) != -1) {
        (*line_count)++;
    }

    free(line);
    fclose(src_file);

    return SUCCESS;
}

// Trims leading and trailing whitespace IN PLACE, shifting the trimmed content
// to the start of the buffer. Returns the original `line` pointer so callers
// can safely pass it back to getline/free without losing the malloc base.
// characters covered by isspace(): ' ', '\f', '\n', '\r', '\t', '\v'
//original the function was human written but changed to ai generated using the prompt "write a function that trims leading and trailing whitespace in a string in place and returns the original pointer so callers can safely pass it back to getline/free without losing the malloc base."
//this issue was simply that we were incrementing a pointer that was malloc'd so then free wasn't working properly
//this fixes that issue by simply shifting the contents of the array instead of the pointer
char* trim_whitespace(char* line) {
    size_t len, start, end, new_len;

    if (line == NULL) return NULL;

    len = strlen(line);

    start = 0;
    while (start < len && isspace((unsigned char)line[start])) {
        start++;
    }

    end = len;
    while (end > start && isspace((unsigned char)line[end - 1])) {
        end--;
    }

    new_len = end - start;
    if (start > 0 && new_len > 0) {
        memmove(line, line + start, new_len);
    }
    line[new_len] = '\0';

    return line;
}

//brings the src files into their buf
void read_from_file(int idx, src_node * nodes){
    FILE * file_to_read;
    char* my_buf;
    file_to_read = nodes[idx].ptr;
    my_buf = nodes[idx].buf;
    //i don't know if putting sizeof(char) is good style because char is gaurenteed to be 1 byte by the C standard
    size_t newLen = fread(my_buf, sizeof(char), BUF_SIZE-1, file_to_read);
    if ( ferror( file_to_read ) != 0 ) {
        fputs("Error reading src file", stderr);
    } else {
        my_buf[newLen++] = '\0';
        nodes[idx].cur_size = newLen;
    }
    fclose(file_to_read);
}

//sideffects: 
//inits connect_fds
//inits num_fragment files
//open all the files descriped in the src_file with proper permissisons
int open_fragment_files(const char * src_file_name, src_node ** nodes, int** connect_fds, int* num_fragment_files, int * dst_fd) {
    FILE * src_file;
    ssize_t nread;
    size_t size;
    char * line;
    int line_count, i, ret, idx;

    line = NULL;

    ret = count_lines_in_file(src_file_name, &line_count);
    if (ret != SUCCESS) {
        // count_lines_in_file prints out the specific error message
        return ret;
    }

    *num_fragment_files = line_count - 1; // don't need to store first line (dest file name)
    
    *connect_fds = malloc(sizeof(**connect_fds) * (*num_fragment_files)); 
    if (*connect_fds == NULL) {
        return handle_error("malloc connect_fds");
    }

    //as is good practice all unconnected fd are set to -1
    for (i = 0; i < *num_fragment_files; i++) {
        (*connect_fds)[i] = INVALID_FD;
    }

    *nodes = (src_node *)malloc(sizeof(src_node)*(*num_fragment_files));
    if (*nodes == NULL) {
        return handle_error("malloc nodes");
    }
    
    src_file = fopen(src_file_name, "r");
    if (src_file == NULL){
        return handle_error("fopen source file");
    }

    i = 0;
    while ((nread = getline(&line, &size, src_file)) != -1){
        line = trim_whitespace(line);

        if (i == 0){
            *dst_fd = open(line, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // TODO if the code broke put this back to 0644
            if (*dst_fd == INVALID_FD){
                printf("destination file name: %s\n", line);
                return handle_error("open destination file");
            }
        }
        else{
            idx = i - 1;
            (*nodes)[idx].ptr = fopen(line, "r"); //check that we can open the file with r
            if ((*nodes)[idx].ptr == NULL){
                printf("issue with %s's line %d file name: '%s'\n", src_file_name, i, line);
                return handle_error("fopen input snippet file");
            }
            read_from_file(idx, *nodes);
        }
        i++;
    }

    free(line);
    fclose(src_file);

    return SUCCESS;
}

//creates a listening sockets
int establish_socket(int* sfd, struct sockaddr_in* my_addr, int port_num) {
    *sfd = socket(AF_INET, SOCK_STREAM, PROTOCOL);
    if (*sfd == INVALID_FD) 
        return handle_error("socket()");

    memset(my_addr, 0, sizeof(*my_addr));
    (*my_addr).sin_family = AF_INET;
    (*my_addr).sin_port = htons(port_num);
    (*my_addr).sin_addr.s_addr = INADDR_ANY;

    if (bind(*sfd, (struct sockaddr*) my_addr, sizeof(*my_addr)) == -1)
        return handle_error("bind()");
        
    if (listen(*sfd, LISTEN_BACKLOG) == -1)
        return handle_error("listen()");

    return SUCCESS;
} 

//blocks until a full write is complete
void full_write(int fd, char* buf, size_t count){
    size_t written_amount, remaining;
    ssize_t write_len;
    written_amount = 0;
    remaining = count;

    while(remaining > 0){
        write_len = write(fd, buf+written_amount, remaining);
        if(write_len == -1){
            handle_error("full_write()");
            return;
        }
        written_amount += write_len;
        remaining -= write_len;
    }
}

//root, fd, temp_buf, nodes[j].buf, &nodes[j].cur_size
void read_from_socket_and_add_to_tree(struct rb_root *root, int sfd, char * temp_buf, char * src_buf, size_t * src_buf_size){
    ssize_t nread;
    size_t i, line_start, line_len, leftover;
    tree_node * node;
    char * tmp;

    nread = read(sfd, src_buf + *src_buf_size, BUF_SIZE - *src_buf_size);
    if (nread <= 0) {
        if (nread < 0) handle_error("read from client socket");
        return;
    }
    *src_buf_size += (size_t)nread;

    line_start = 0;
    for (i = 0; i < *src_buf_size; i++) {
        if (src_buf[i] != '\n') continue;

        line_len = i - line_start + 1; // include the trailing '\n'

        memcpy(temp_buf, src_buf + line_start, line_len);
        temp_buf[line_len] = '\0';

        node = (tree_node *)malloc(sizeof(tree_node));
        if (node == NULL) {
            handle_error("malloc tree_node");
            return;
        }
        memcpy(node->line, temp_buf, line_len + 1);
        node->cur_size = line_len;
        node->line_num = (int)(strtol(node->line, &tmp, BASE_10));
        if(tmp != node->line){
            if(tree_insert(root, node) == FALSE){
                free(node); 
            }
        } else{
            free(node);
        }
        line_start = i + 1;
    }

    // Preserve any partial line (no trailing newline yet) at the front of src_buf
    leftover = *src_buf_size - line_start;
    if (leftover > 0 && line_start > 0) {
        memmove(src_buf, src_buf + line_start, leftover);
    }
    *src_buf_size = leftover;
}

int main(int argc, char *argv[]){
    int dst_fd;
    char * file_name;
    int port_num, num_fragment_files;
    struct sockaddr_in my_addr;
    int epfd, sfd, finished_clients, started_clients;
    struct epoll_event events[MAX_EVENTS];
    int * connect_fds;
    src_node * nodes;
    int i, j, n, ret, fd, client_fd;
    uint32_t ev_mask;
    struct rb_root mytree;
    char* temp_buf;

    mytree = RB_ROOT;

    if (argc != EXPECTED_ARGS){
        printf("Usage: %s <input_file_name> <port number>\n", argv[0]);
        printf("    input file should be formatted as follows:\n");
        printf("    first line: output file name\n");
        printf("    subsequent lines: file names of input fragments (one per line)\n");
        printf("    note: file names should not contain spaces\n");
        return WRONG_NUM_ARGS;
    }

    file_name = argv[FILE_NAME_IDX];
    port_num = atoi(argv[PORT_NUM_IDX]);

    ret = open_fragment_files(file_name, &nodes, &connect_fds, &num_fragment_files, &dst_fd);
    if (ret != SUCCESS) return ret; // error reason printed within called function

    ret = establish_socket(&sfd, &my_addr, port_num);
    if (ret != SUCCESS) return ret; // error reason printed within called function

    printf("Clients should connect to %s port %d\n", SHELL_IP_ADDR, port_num);

    epfd = epoll_create1(NO_FLAGS); //creates the epoll object
    if (epfd == -1) return handle_error("epoll_create1()");

    struct epoll_event lis; 
    lis.events = EPOLLIN;
    lis.data.fd = sfd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &lis) == -1) //registers the listening sockets for epoll obj
        handle_error("epoll_ctl add listener");
    

    started_clients = 0; //these variables tell the server when to stop looking for more connections
    finished_clients = 0;
    client_fd = INVALID_FD;
    temp_buf = malloc(BUF_SIZE);
    while(finished_clients != num_fragment_files) {
        n = epoll_wait(epfd, events, MAX_EVENTS, INF_TIMEOUT);
        if (n == -1) {
            return handle_error("epoll_wait()");
        }
        for (i = 0; i < n; i++) { //bc epoll can see multiple events ready at the same time we have to loop
            fd = events[i].data.fd;
            fflush(stdout);
            ev_mask = events[i].events;
            if(fd == sfd){
                client_fd = accept(sfd, NULL, NULL);
                if (client_fd == INVALID_FD){
                    handle_error("accept");
                    continue;
                } 
                printf("client connected\n");
                fflush(stdout);
                struct epoll_event cev;
                cev.events  = EPOLLIN | EPOLLRDHUP;
                cev.data.fd = client_fd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev) == -1)
                    handle_error("epoll_ctl add client");
                
                //wrapper on write that makes sure it does a full write
                full_write(client_fd, nodes[started_clients].buf, nodes[started_clients].cur_size); 
                connect_fds[started_clients] = client_fd;
                started_clients++;
                if(started_clients >= num_fragment_files){ //when we hand out the last fragment stop listening
                    epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, NULL);
                    close(sfd);
                }

            } else{
                for(j = 0; j < num_fragment_files; j++){
                    if(fd == connect_fds[j]){
                        if (ev_mask & EPOLLIN) { //if they have data to read
                            read_from_socket_and_add_to_tree(&mytree, fd, temp_buf, nodes[j].buf, &nodes[j].cur_size);
                        }
                        if (ev_mask & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) { //if they stopped connection
                            finished_clients++;
                            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                            close(fd);
                            connect_fds[j] = INVALID_FD; //set it -1 so we don't ever try to use it
                        }
                        break;
                    }
                }
            }
        }
    }

    //all of the clients are done so the red black tree is finished
    //extract all of the nodes and then write it to the file
    tree_print(&mytree, dst_fd, TRUE);
    
    close(dst_fd); //we wrote to the file so good to close

    //free all the mallocs
    free_tree(&mytree);
    free(connect_fds);
    free(nodes);
    free(temp_buf);

    return  SUCCESS;
}