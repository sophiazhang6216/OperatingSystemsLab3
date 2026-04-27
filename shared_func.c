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
        return;
    }
    t->cur_size = str_len;
    memcpy(t->line, src, str_len);
    temp = strtol(src, &end, BASE_10);
    if(end == src){
        free(t);
        return;
    }
    t->line_num = (int)temp;
    tree_insert(root, t);
}

//adding the strip_first_word functionality was generated with ai using a prompt of "add a parameter to this function that says whether or not we want to remove the first "word" from each line."
//the way it works is by using the fact that line_num encode the data that tells us how long the line is going to be.
//prints out the tree in order to the fd
int tree_print(struct rb_root *root, int fd, int strip_first_word)
{
    struct rb_node *n;
    tree_node *data;
    size_t len, off, start;
    ssize_t w;
    char nl;
    int num, digits;

    nl = '\n';
    for (n = rb_first(root); n; n = rb_next(n)) {
        data = rb_entry(n, tree_node, node);
        len   = data->cur_size;
        start = 0;

        if (strip_first_word) {
            // count the digits in line_num, then add 1 for the trailing space
            digits = 1;
            num = data->line_num;
            if (num < 0) { digits++; num = -num; }
            while (num >= 10) { num /= 10; digits++; }
            start = (size_t)digits + 1;
            if (start > len) start = len; // defensive: malformed line shorter than expected
        }

        off = start;
        while (off < len) {
            w = write(fd, data->line + off, len - off);
            if (w < 0) return handle_error("write");
            off += (size_t)w;
        }
        if (len == 0 || data->line[len - 1] != '\n') {
            if (write(fd, &nl, 1) < 0) return handle_error("write");
        }
    }
    return SUCCESS;
}

void free_subtree(struct rb_node *n) {
    if (!n) return;
    free_subtree(n->rb_left);
    free_subtree(n->rb_right);
    free(rb_entry(n, tree_node, node));
}

int free_tree(struct rb_root *root) {
    free_subtree(root->rb_node);
    root->rb_node = NULL;
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

