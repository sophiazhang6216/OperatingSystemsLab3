#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/un.h>
#include <stdbool.h>
#include "shared_func.h"

#define EXPECTED_ARGS 3
#define LOCAL_LOOPBACK_IP "127.0.0.1"
#define SERVER_NAME_INDEX 0
#define INET_ADDR_INDEX 1
#define PORT_NUM_INDEX 2



int main (int argc, char *argv[]) {
    char * inet_addr;
    int port_num, sfd;
    int ret;
    struct sockaddr_in my_addr;
    struct rb_root root;
    ssize_t buf_len;
    size_t ubuf_len;
    char buf[BUF_SIZE];
    // dest_buf needs one extra byte so get_one_line can null-terminate even
    // when an entire BUF_SIZE-length payload ends in '\n'. Without this,
    // dest_buf[BUF_SIZE] = '\0' clobbers adjacent stack memory (e.g. `root`)
    // and we crash later in free_tree.
    char dest_buf[BUF_SIZE + 1];
    int line_len;

    root = RB_ROOT;

    if (argc != EXPECTED_ARGS) {
        printf("Usage: %s <internet address> <port number>\n", argv[SERVER_NAME_INDEX]);
        printf("Example: %s 127.0.0.1 35000\n", argv[SERVER_NAME_INDEX]);
        return WRONG_NUM_ARGS;
    }

    inet_addr = argv[INET_ADDR_INDEX];
    port_num = atoi(argv[PORT_NUM_INDEX]);

    //establish a socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        return handle_error("socket()");
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port_num);

    ret = inet_aton(inet_addr, &my_addr.sin_addr); 
    if (ret == SUCCESS) {
        return handle_error("inet_aton()");
    }

    ret = connect(sfd, (struct sockaddr *) &my_addr, sizeof(my_addr));
    if (ret == -1) {
        printf("\nIf off-campus, run the following command on your pi:\n");
        printf("ssh -f -N <your-username>@shell.cec.wustl.edu -L %d:shell.cec.wustl.edu:%d\n\n", port_num, port_num);
        printf("After, you should run this program from 127.0.0.1:\n");
        printf("%s 127.0.0.1 %d\n\n", argv[SERVER_NAME_INDEX], port_num);
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
    while ((line_len = get_one_line(buf, &ubuf_len, dest_buf)) > 0) {
        add_to_tree(&root, (size_t)line_len, dest_buf);
    }
    
    printf("starting to print\n");
    fflush(stdout);

    tree_print(&root, sfd, FALSE);

    printf("finished to print\n");
    fflush(stdout);

    //hang up
    close(sfd);
    free_tree(&root);

    //returning an enum indicating success
    return SUCCESS;

}