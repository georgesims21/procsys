#ifndef PROCSYS_SERVER_NEW_H
#define PROCSYS_SERVER_NEW_H

#include <glob.h>
#include <netinet/in.h>

#include "ds_new.h"
/*
 * Used to deal with any requests from other machines. Including file content
 * requests. For host machine requests see client.c
 */

int init_server(Address *address, int queue_length, int port_number, const char *interface);
/*
 * init server: - by main thread
 *  @param  queue length - given as argv to filesystem
 *          Address *server_addr
 *  @ret    error codes - print errors to log file too
 *
 *  make server socket into actual socket
 *  fill server address struct with IP information, socket and length
 *  bind the socket and Address->sockaddr_in->... struct
 *  listen to socket and return
 */


struct connect_to_file_IPs_args {
    // understand passing arrays with/out as pointers is the same, server_addr is without * to
    // ensure it is not modified and that a local copy should be made
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address *host_client; pthread_mutex_t *host_client_lock;
    Address *client_host; pthread_mutex_t *client_host_lock;
    Address host_addr;
    int arrlen;
    const char *filename;
};
void *connect_to_file_IPs(void *arg);
/*
 * connect to IPs: - by main thread 0
 *  @param  void* - pass connIPs_args struct
 *  @ret    NULL - we aren't joining main to this
 *
 *  file pointer to IP file
 *  loop to read line by line each IP:
 *      ignore own
 *      fill in client_add struct with extracted IP and port (mindmap has details)
 *      create new address struct with server_add and add an fd number (socket) and len
 *      if not contained in the hostclient[] add it (don't append, find first empty spot)
 *      if also contained in the clienthost[] then add to connectedclients[] (ditto)
 *  exit thread
 * TODO
 *      * how to find port number from IP?
 *      * test the if free < 0 part
 *      * connect this socket to the IP - comms need testing
 *      * more secure string checking on the file (remove whitespace etc)
 */
static int fetch_IP(Address *addr, const char *interface);
static int next_space(Address *addr, int addrlen);
static int pconnect(Address *addrarr, int arrlen, in_addr_t conn);
static int contained_within(Address *addr, Address lookup, int arrlen);

static int add_address(Address *arr, Address addr, pthread_mutex_t *arr_lock, int arrlen);

struct server_loop_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address *host_client; pthread_mutex_t *host_client_lock;
    Address *client_host; pthread_mutex_t *client_host_lock;
    Inprog *inprog; pthread_mutex_t *inprog_lock;
    Address host_addr;
    int arrlen;
};
void *server_loop(void *arg);
/*
 * server loop: - by server thread
 * @param   void* - pass server_loop_args struct
 * @ret     NULL - not joining main, it's a inf loop
 *
 * create fdset
 * for(;;):
 *      add sockets from connected clients[] - can extract the int sock
 *      listen to fdset
 *      noise on master:
 * TODO
 *          accept_connection()
 *          BUG
 *          need to make sure it is being added to host_client correctly or if search methods are
 *          correctly identifying whether an address is within an array. Maybe can be changed to
 *          search for an IP rather than all elements of an address (maybe socket changed?)
 *
 *      handle_client()-remove
 *      noise on fdset:
 *          client disconnect:
 *              disconnect_client()
 *          *need to think about send/recv not reading/sending all bytes in one call!:*
 *          http://brunov.info/blog/2013/02/09/tcpip-client-server-application-exchange-with-string-messages/
 *          parse flag()
 *          file request:
 *              malloc Request fr;
 *              extract_file_request(fr, host_addr)
 *              fetch file contents:
 *                  remember to realloc fr with size of file and save into buf
 *              malloc buffer to send over socket with length of fr
 *              create_request(fr, frlen, buf, buflen)
 *              free(fr)
 *              send request to caller
 *              free(buf)
 *          file content:
 *              *message request struct already exists for this*
 *              malloc Request fc;
 *              extract_content_request(fc, host_addr)
 *              mark_as_received(fc, inpr_ll_ptr, inprog_tracker_ll_lock)
 *
 */

struct new_connection_args {
    Address *conn_clients; pthread_mutex_t *conn_clients_lock;
    Address *host_client; pthread_mutex_t *host_client_lock;
    Address *client_host; pthread_mutex_t *client_host_lock;
    Address host_addr;
    int arrlen; int *fdcount;
    struct pollfd *pfds; pthread_mutex_t *pfds_lock;
};


/*
 * STATIC accept_connection - by server thread n
 * @param   void* - pass accept_connection_args struct
 * @ret     NULL
 *
 * create address new_client
 * accept connection via the server_sock - use new_client->addr struct to save IP and port info
 * send connected message to socket
 * mutex_lock client_host
 * add address to client_host[]
 * mutex_unlock
 * if contained in host_client[]:
 *      mutex_lock connected_clients
 *      add to connected_clients[]
 *      mutex_unlock
 */

/*
 * struct disconnect_client_args {
 *      mutex_lock_t connected_clients_lock;
 *      address *connected_clients[];
 *      mutex_lock_t client_host_lock;
 *      address *client_host[];
 *      mutex_lock_t host_client_lock;
 *      address *host_client[];
 *      int connected_clients_arrpos;
 *      sd client_sock;
 * };
 */

/*
 * STATIC disconnect_client - by server thread n
 * @param   void* - pass disconnect_client_args struct
 * @ret     NULL
 *
 * mutex_lock connected_clients
 * remove from connected_clients[connected_clients_arrpos] = 0;
 * mutex_unlock
 *
 * mutex_lock host_client
 * remove from host_client[] by matching the sd
 * mutex_unlock
 *
 * mutex_lock client_host
 * remove from client_host[] by matching the sd
 * mutex_unlock
 *
 * free(sock) - could do earlier, not sure if sock would have value after though
 */

/*
 * struct extract_file_request_args {
 *      struct Request *fr; (malloc'ed)
 *      struct Address host_addr;
 * };
 */

/*
 * STATIC extract_file_request - by server thread n
 * @param   void* - pass extract_file_request_args struct
 * @ret     NULL
 *
 * save IP into fr->address->...->sin_addr
 * save port into fr->...->sin_port
 * save host IP and port
 * make sure they match our credentials
 * save path into fr->...
 * save counter into fr->...
 */

/*
 * struct extract_file_content_args {
 *      struct Request *fr; (malloc'ed)
 *      struct Address host_addr;
 * };
 */

/*
 * STATIC extract_file_content - by server thread n
 * @param   void* - pass extract_file_content_args struct
 * @ret     NULL
 *
 * save IP into fr->address->...->sin_addr
 * save port into fr->...->sin_port
 * save host IP and port
 * make sure they match our credentials
 * save path
 * save counter
 */

/*
 * struct create_request_args {
 *      struct Request *req;
 *      int req_len; // length of request struct
 *      char *content; // malloc'ed
 *      int con_len; // length of content buffer
 * };
 */

/*
 * STATIC create_request - by server thread n
 * @param   void* - pass create_request_args struct
 * @ret     NULL
 *
 * convert all ints to chars like I did with flags
 * convert given Request struct into a buffer
 * https://stackoverflow.com/questions/8000851/passing-a-struct-over-tcp-sock-stream-socket-in-c
 */


/*
 * struct mark_as_received_args {
 *      struct Request *req;
 *      int req_len; // length of request struct
 *      mutex_lock_t inprog_tracker_ll_lock;
 *      int *inpr_ll_ptr; // head of Inprog_tracker_ll
 * };
 */

/*
 * STATIC mark_as_received - by server thread n
 * @param   void* - pass mark_as_received_args struct
 * @ret     NULL
 *
 *      mutex_lock(inprog_tracker_ll_lock)
 *      create iterator pointers **inprgpt, **reqpt, **reqpt2;
 *      find correct request struct using counter (req->atomiccounter == *inprgpt->inpt->atomiccounter)
 *      point *reqpt to *inprgpt->inpt->*req_ll_ptr
 *      find correct sender IP (req->...->sin_addr == *reqpt->inpt->req->sender->addr->sin_addr->sin_addr)
 *      check correct sender port (req->...->sin_port == *reqpt->inpt->req->sender->addr->sin_port)
 *      add file content to buf (req->buf = *reqpt->inpt->req->buf)
 *      mark as received (*reqpt->inpt->req->received = 1)
 *      point *reqpt2 to *inprgpt->inpt->*req_ll_ptr
 *          while(*reqpt2->next =! NULL)
 *          if(*reqpt2->req->received == 1)
 *              count++
 *          if(count == *inprgpt->sent_msgs)
 *              *inprgpt->inpt->complete = 1
 *      mutex_unlock(inprog_tracker_ll_lock)
 */

#endif //PROCSYS_SERVER_NEW_H