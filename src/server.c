#include "client-server.h"
#include "server.h"
#include "log.h"
#include "fileops.h"

/*
 * TODO
 *  * writer
 *  - [ ] create buf containing file path
 *  - [ ] prepend flag to buf (from writer api)
 *  * reader
 *  - [ ] parse flag method (from reader api)
 *  - [ ] parse path method
 *  - [ ] parse content method
 *  * main
 *  - [ ] maxsd isn't used after is is assigned
 *  - [ ] send buf to all clients, skip fd of caller
 *  - [ ] struct containing calling clients fd with flag, path and content
 *  - [ ] queue data struct to hold received buffers
 *      - queue empty method: keep queue length as global and constantly check in while loop
 *  - [ ] method to congregate values from all the buffers
 */

struct sockaddr_in server_addr;
int server_sock;

int init_server(int queue_len, struct sockaddr_in *server_add) {
    int opt = 1, socket_fd;

    timestamp_log("server");

    lprintf("{server}Creating TCP stream socket\n");
    if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket\n");
        exit(EXIT_FAILURE);
    }

    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    lprintf("{server}filling sockaddr struct\n");
    server_add->sin_family = AF_INET;
    server_add->sin_port = htons(SERVER_PORT); // htons converts int to network byte order
    server_add->sin_addr.s_addr = htonl(INADDR_ANY);

    lprintf("{server}Binding socket to server address\n");
    if(bind(socket_fd, (struct sockaddr*)server_add, sizeof(*server_add)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    lprintf("{server}server is listening on port: %d\n", SERVER_PORT);
    listen(socket_fd, queue_len);
    return socket_fd;
}

int add_clients(int socket_set[], int sd, int maxsd, fd_set *newfdset) {
    for(unsigned int i = 0; i < MAX_CLIENTS; i++) {
        if((sd = socket_set[i]) > 0) {
            FD_SET(sd, newfdset); // if valid add to set
        }
        if(sd > maxsd) {
            maxsd = sd;
        }
    }
    return maxsd;
}

int add_socket(int socket_set[], int new_sock) {
    //add new socket to array of sockets
    for(unsigned int i = 0; i < MAX_CLIENTS; i++) {
        //if position is empty
        if(socket_set[i] == 0) {
            socket_set[i] = new_sock;
            lprintf("{server}Adding to list of sockets as %d\n" , i);
            return 0;
        }
    }
    return -1; // array full
}

void disconnect_sock(int socket_set[], struct sockaddr_in *server_add, int sd, int len, int arrpos) {
    /*
     * TODO
     *  - [ ] error checking for close method
     */

    // to be used in a loop
    getpeername(sd, (struct sockaddr*)server_add , (socklen_t *)&len);
    lprintf("{server}Host disconnected , ip %s , port %d \n",
           inet_ntoa(server_add->sin_addr), ntohs(server_add->sin_port));

    //Close the socket and mark as 0 in list for reuse
    close(sd);
    socket_set[arrpos] = 0;
}

void notify_clients(int socket_set[], int sd, char *line) {
    for(unsigned int j = 0; j < MAX_CLIENTS; j++) {
//        if(sd == socket_set[j])
//            continue;
        send(socket_set[j], line, strlen(line), 0);
    }
}

int listenfds(int socket_set[], int server_socket, fd_set *fds, int sd) {
    int maxsd = 0;
    FD_ZERO(fds);
    FD_SET(server_socket, fds);
    maxsd = add_clients(socket_set, sd, server_socket, fds);
    if((select( MAX_CLIENTS + 1, fds, NULL,
                NULL, NULL)) < 0) { // indefinitely (timeout = NULL)
        perror("select");
        exit(EXIT_FAILURE);
    }
    return maxsd;
}

void accept_connection(int socket_set[], int server_socket, int new_sock, int len, char *message,
        struct sockaddr_in *server_add) {
    if ((new_sock = accept(server_socket,
                           (struct sockaddr *)server_add, (socklen_t *)&len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    lprintf("{server}New connection , socket fd is %d , ip is : %s , port : %d\n", new_sock,
           inet_ntoa(server_add->sin_addr), ntohs(server_add->sin_port));

    if(send(new_sock, message, strlen(message), 0) != strlen(message)) {
        perror("send");
        exit(EXIT_FAILURE);
    }
    if(add_socket(socket_set, new_sock) < 0)
        lprintf("{server}socket fd array full!");
}

char handle_client(int socket_set[], int sd, int len, int i, char *line, struct sockaddr_in *server_add) {
    /*
     * TODO
     *  - [ ] do the switch statement with the same logic as client
     *      - prepend correct flags to each messgae: use memmove and memcpy https://stackoverflow.com/questions/2328182/prepending-to-a-string
     *      - Check if read adds terminating char
     */
    char tmp[MAX] = {0};
    if ((read(sd, line, READ_MAX)) == 0) {
        //Somebody disconnected
        disconnect_sock(socket_set, server_add, sd, len, i);
        return 1;
    }
    // Client message
    line[TERM_CHAR_MAX] = '\0';

    switch(parse_message(line)) {
        case CNT_MSG_CLI:
            lprintf("{server}received message %s from client(sd){%d}\n", line, sd);
            break;
        case NME_MSG_CLI:
            break;
        default:
            break;
    }

    sprintf(tmp, "%d%s", REQ_MSG_SER, line);
    sprintf(line, "%s", tmp);
    return 0;
}

void server_loop(int server_socket, int len, char *message) {
    int client_socks[MAX_CLIENTS] = {0}, new_sock = 0, sd = 0, maxsd = 0;
    unsigned int i = 0;
    char line[MAX] = {0};
    fd_set fdset;

    while(1) { // for accepting connections
        maxsd = listenfds(client_socks, server_socket, &fdset, sd);
        //If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(server_socket, &fdset)) {
            accept_connection(client_socks, server_socket, new_sock, len, message, &server_addr);
        }
        //else its some IO operation on some other socket
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socks[i];
            if (FD_ISSET(sd , &fdset)) {
                //Check if it was for closing , and also read the incoming message
                if(handle_client(client_socks, sd, len, i, line, &server_addr) == 0) {
                    // notify all other connected clients about message
                    notify_clients(client_socks, sd, line);
                    memset(line, 0, sizeof(line));
                }
            }
        }
    }
}

void run_server(void) {
    int len = 0;
    char message[MAX] = {0};

    server_sock = init_server(10, &server_addr);
    len = sizeof(server_addr);
    sprintf(message, "%dConnected to server address at %s and port %hu...", CONN_MSG_SER,
            inet_ntoa(server_addr.sin_addr) , ntohs(server_addr.sin_port));

    server_loop(server_sock, len, message);
}

int main(int argc, char *argv[]) {
    run_server();
    return 0;
}

