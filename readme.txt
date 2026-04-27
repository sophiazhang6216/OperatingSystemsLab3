Team members:
- Sophia Zhang (s.w.zhang@wustl.edu)
- Andy Hoette (a.h.hoette@wustl.edu)
- Ayla Burba (a.burba@wustl.edu)


# Server Design:

The heart of the server design is an array of struct src_nodes. We figure out how many of these nodes we want by first traversing through the src file to see how many nodes we need. We will allocate enough space to create a number of node equal to the number of snippets we have. Each node contains both a FILE*, a char[ ] buffer, and a size_t variable. The FILE* is the first variable we assign when we walk through the list of fragments and open each file. We then use the buffer to read in the contents of the file. The size of the buffer is rather large and dwarfs any other spacial complexity concerns of our program. However, as discussed in paragraph 3 this seems to be necessary because it provides us with a lot of benefits.

	Now we set up a listen socket. Whenever the server is connected with a client it stores the connection fd in an array of such connections (note that this is separate from the src_nodes only due to poor foresight and can/should be combined into the src_nodes). The server then uses a helper method to do a full right of the first available snippet. It increments the started_clients count. This count is important because once it becomes large enough, we realize that we have no more fragments to give out and so should stop listening for connections.

When the client returns with information we again use the same buffer in the src_node to read in the contents. This is helpful as it allows multiple clients to have short reads at the same time without any conflict. This is the reason we feel it is valid to justify its large size. The reuse of the buffer is perfectly valid because the server will block on the original write to this client until it completes. This creates a happens before relationship between the end of the write and the start of the read. Each line is processed with the line number until the very end, so we can use the first “word” of the line to create a key for our red black tree.

Once all lines are returned to us by all of the clients our finished_clients will be equal to the number of fragments. This means that our red black tree has all of the lines and we can use our print tree helper method to export the contents to our dst file. 


# Client Design: 

To implement the requirements for the client, we have a C program that takes in two command line arguments, the first being the internet address and the second being the port number, both where it connects to the server. It prints the proper usage function for if the number of command line arguments is incorrect, and then stores these values into the proper variables. 

The client then creates a socket using the socket() sys call, and then connects to the server using the connect() sys call using the address and port it received from the command line. 

After the connection is successfully established, the client program attempts to read the entire buffer. The number of bytes successfully read into the buffer is 
The client uses get_one_line() to repeatedly extract the individual lines, by checking newline-terminating characters from the buffer. Each complete line is then copied into dest_buf and inserted into a red-black tree using add_to_tree(). The data structure that we chose to use is a red-black tree, which orders its entries by insertion using the integer at the beginning of each line. This way, we can easily maintain the order of our lines, and send it back to the server in order.

The client then calls the tree_print() helper function, to iterate through the tree and send the nodes in order back to the server. Lastly, cleanup is done by closing the socket and freeing the entire tree.


# Build Instructions:

We have a Makefile, and there are four options to compile our program. You can run the following: 
make: compiles all programs (server.c, client.c)
make client: compiles only client.c
make server: compiles only server.c
make clean

# Testing and Evaluation:

We ran valgrind on both the server and the client and got no memory leaks with the given jabberwocky files. 

When testing for performance we saw over a series of 5 tests on the client with the jabberwocky files, we spend an average of 187ms of clock time while only spending 5 and 5.8 seconds of user time and system time. This clearly indicates that the main bottleneck of our program is the wireless connection between the server and the client.

When we test on the larger file_shuffle_spec.cpp which is considerably longer (around 3x the number of lines) we see mostly similar performance with user time nearly doubly to 9.6 seconds. This makes sense as the client would have to spend more time sorting the file if its more lines.




# Development Effort: 

The group probably spent around 50 person-hours working on the lab. Of this, 40 of those person hours were spent working together in person. The other 10 person-hours (split approximately equally among the group) was work done asynchronously.

# Development notes:

We originally just incremented finished_clients by one whenever we got one of the events. This caused the server to completely finish whenever one of the clients finished. This was because we were not removing the event so the server kept pulling up this event and incrementing the variable. To fix this we simply closed the socket and removed the possibility of epoll bringing it up.
                       if (ev_mask & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                           finished_clients++;
                           epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                           close(fd);
                           connect_fds[j] = -1;
                       }


