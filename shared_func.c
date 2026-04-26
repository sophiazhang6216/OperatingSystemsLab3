#include "shared_func.h"


//give it the buf and some where to put the new line and it will move it there
//it will also move the existing contents into the front of the array
//return the number of lines processed

int handle_error(char* msg) {
    printf("Program error in %s. Reason: %s\n", msg, strerror(errno));
    return errno;
}

//getting one line from buf
int get_one_line(char *buf, size_t* cur_size, char * dest) {
    size_t len;
    char *start, *end, *nl;
    start = buf;
    end   = buf + *cur_size;
    int num_lines_read = 0; //keeping track of how many lines were read, success should be return 1

    if ((nl = memchr(start, '\n', end - start)) == NULL) {
        return num_lines_read; //fail
    }

    len = (size_t)(nl - start + 1);
    memcpy(dest, buf, len);
    dest[len] = '\0';
    start = nl + 1;
    *cur_size = (size_t)(end - start); //move extra to the start
    memmove(buf, start, *cur_size); //memcpy is gonna blow up bc overlap
    num_lines_read++;
    return num_lines_read; // success yay
}

//creates a copy of src - src[str_len], put all the info into a tree_node and inserts it into the tree
void add_to_tree(struct rb_root * root, size_t str_len, char * src) {
    tree_node* t;
    char * end;
    long temp;

    t = malloc(sizeof(*t));
    if(t == NULL){
        handle_error("malloc tree_node");
    }
    t->cur_size = str_len;
    memcpy(t->line, src, str_len);
    temp = strtol(src, &end, BASE_10);
    if(end == src){
        return;
    }
    t->line_num = (int)temp;
    tree_insert(root, t);
}


int tree_print(struct rb_root *root, int fd)
{
    struct rb_node *n;
    tree_node *data;
    size_t len;
    size_t off;
    ssize_t w;
    char nl;

    nl = '\n'; // new line character
    for (n = rb_first(root); n; n = rb_next(n)) {
        data = rb_entry(n, tree_node, node);
        len = data->cur_size;

        printf("[tree_print] fd=%d line_num=%d cur_size=%zu line=\"%.*s\"\n",
               fd, data->line_num, len, (int)len, data->line);
        fflush(stdout);

        off = 0;
        while (off < len) {
            w = write(fd, data->line + off, len - off);
            if (w < 0) return -1;
            off += (size_t)w;
        }
        if (len == 0 || data->line[len - 1] != '\n') {
            if (write(fd, &nl, 1) < 0) return -1;
        }
    }
    return SUCCESS;
}

//goes through the entire tree to remove and free all nodes
int free_tree(struct rb_root *root){
    struct rb_node *pos = rb_first(root);
    struct rb_node *next = rb_next(pos);

    while (next != NULL){
        rb_erase(pos, root);
        next = rb_next(pos);
        free(pos);
        pos = next;
    }

    return SUCCESS;
}


//code from kernel.org: official linux kernel archive
int tree_insert(struct rb_root *root, tree_node *data) {
      struct rb_node **link = &(root->rb_node), *parent = NULL;
      int this_num, data_num;

      /* Figure out where to put link node */
      while (*link) {
            tree_node *this = rb_entry(*link, tree_node, node);
            this_num = this->line_num;
            data_num = data->line_num;
            parent = *link;
            if (data_num < this_num)
                link = &((*link)->rb_left);
            else if (this_num < data_num)
                link = &((*link)->rb_right);
            else
                return FALSE;
      }

      /* Add link node and rebalance tree. */
      rb_link_node(&data->node, parent, link);
      rb_insert_color(&data->node, root);

      return TRUE;
}

