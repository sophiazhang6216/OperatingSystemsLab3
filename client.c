#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/un.h>
#include <stdbool.h>
#include "shared_func.c"

#define EXPECTED_ARGS 3
#define LOCAL_LOOPBACK_IP "127.0.0.1"

enum exit_values {
    SUCCESS = 0,
    WRONG_NUM_ARGS
    // other return values are the errno for the failure reason -
    // specific function that caused the error is printed to stdout
};

int main (int argc, char *argv[]) {
    char * inet_addr;
    int port_num, sfd;
    int i, ret;
    struct sockaddr_in my_addr;
    struct rb_root root;
    ssize_t buf_len;
    size_t ubuf_len, wrote;
    char * temp_str;
    char buf[BUF_SIZE];
    char dest_buf[BUF_SIZE];

    if (argc != EXPECTED_ARGS) {
        printf("Usage: %s <internet address> <port number>\n", argv[0]);
        printf("Example: %s 127.0.0.1 35000\n", argv[0]);
        return WRONG_NUM_ARGS;
    }

    inet_addr = argv[1];
    port_num = atoi(argv[2]);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        return handle_error("socket()");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_num);

    ret = inet_aton(inet_addr, &my_addr.sin_addr); 
    if (ret == 0) {
        return handle_error("inet_aton()");
    }

    ret = connect(sfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
    if (ret == -1) {
        printf("\nIf off-campus, run the following command on your pi:\n");
        printf("ssh -f -N <your-username>@shell.cec.wustl.edu -L %d:shell.cec.wustl.edu:%d\n\n", port_num, port_num);
        printf("After, you should run this program from 127.0.0.1:\n");
        printf("%s 127.0.0.1 %d\n\n", argv[0], port_num);
        return handle_error("connect()");
    }

    printf("connected!\n");

    //trying to read the whole buffer, buf_len indicates how many bytes were actually read
    buf_len = read(sfd, buf, BUF_SIZE);
    if (buf_len == -1){
        return handle_error("read()");
    }

    //cast to unsigned bc we know that its not negative --> we can safely cast positive to unsigned
    ubuf_len = (size_t) buf_len;

    //adding node to rb tree
    while (get_one_line(buf, &ubuf_len, dest_buf)) { 
        // can cast dest_buf to char* bc buf_len ensures the missing '\0' is ok
        add_to_tree(&root, buf_len, (char*)dest_buf);
    }
    
    while (dest_buf != NULL){
        //send buf to server
        tree_print(&root, sfd);
    }

    //hang up
    close(sfd);

    //returning an enum indicating success
    return SUCCESS;

}