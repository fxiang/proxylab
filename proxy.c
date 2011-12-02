/*
 * Name     : Dalong Cheng, Fan Xiang
 * Andrew ID: dalongc, fanx
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"

#define MAX_OBJECT_SIZE ((1 << 10) * 100) 
#define state_offset 9
#define move_offset 9
int min(int a, int b) {
    return a > b ? b : a;
}


typedef struct {
    int client_fd;
} Thread_Input;

void proxy_error(char *msg) {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

static inline void copy_single_line_str(rio_t *client_rio, char *buffer) {
    if (rio_readlineb(client_rio, buffer, MAXLINE) < 0) {
        proxy_error("copy_single_lienstr error");
    }
    buffer[strlen(buffer)] = '\0';
}

static inline void extract_hostname(char *src, char *desc) {
    int copy_length;
    copy_length = strlen(src + 6) - 2;  
    strncpy(desc, src + 6, copy_length); 
    desc[copy_length] = '\0';
}

void modify_request_header(Request *request) {
    char *str;
    if ((str = strstr(request->request_str, "HTTP/1.1")) != NULL) {
        strncpy(str, "HTTP/1.0", 8); 
    } else {
        fprintf(stderr, "Warning: HTTP/1.1 not found in request\n");
    }
    #ifdef DEBUG
    printf("modify request str:%s", request->request_str);
    #endif
}

void send_client(int client_fd, Response *response) {
    #ifdef DEBUG
    printf("send_client !!!!!!!!!!\n");
    #endif
    if (response != NULL) {
        if (response->content != NULL) {
            if (rio_writen(client_fd, response->content, 
                        response->content_size) < 0) {
                proxy_error("rio_writen in send_client error");  
            }
        } else {
            fprintf(stderr, "error in send_client, content is NULL\n"); 
            abort();
        }
    } else {
        fprintf(stderr, "error in send_client, response is NULL\n"); 
        abort();
    }
}

static inline int extract_port_number(char *resquest_str) {
    return 80;
}



void forward_response(int client_fd, int server_fd, Response *response) {
    #ifdef DEBUG
    printf("enter forward_response\n");
    #endif
    size_t n;
    int length = -1;
    char header_buffer[MAXLINE];
    char content_buffer[MAX_OBJECT_SIZE];
    int read_size;
    rio_t   server_rio;
    int header_line = 0;
    int move_flag = 0;
    char move_new_address[MAXLINE];
    rio_readinitb(&server_rio, server_fd);
    while ((n = rio_readlineb(&server_rio, header_buffer, MAXLINE)) != 0) { 
        strcat(response->header, header_buffer); 
//parse response
//if return status code is 3XX
//address moved. 
//need to send another request to the indicated place
//until get 2XX or 4XX or 5XX.
header_line ++;
//status code 3XX  
if((header_line == 1) && (header_buffer[state_offset] == '3')) {
move_flag =1;
}
if((move_flag == 1) && strstr(header_buffer,"Location:")) {
//moved to new address.
//I suggest change the address in request and return. then send request again.
strcat(move_new_address, (header_buffer+move_offset));
}
//add exit here
        /*
        if (rio_writen(client_fd, header_buffer, n) < 0) {
            proxy_error("rio_writen in forward_response header error");  
        }*/
        
        if (strstr(header_buffer, "Content-Length: ")) {
            sscanf(header_buffer + 16, "%d", &length);
        }
        if (!strcmp(header_buffer, "\r\n")) {
            break;
        }
    }

    if (length == -1)
        read_size = MAX_OBJECT_SIZE;
    else 
        read_size = min(length, MAX_OBJECT_SIZE);
    
    #ifdef DEBUG
    printf("finish response header\n");
    #endif

    int sum = 0;
    while ((n = rio_readnb(&server_rio, content_buffer, read_size)) != 0) { 
        if (rio_writen(client_fd, content_buffer, n) < 0) {
            proxy_error("rio_writen in forward_response content error");  
            Close(client_fd);
        }
        sum += n;
    }

    #ifdef DEBUG 
    printf("read byte size:%u\n", sum);
    #endif
    
    if (sum <= MAX_OBJECT_SIZE) {
        response->content = Malloc(sizeof(char) * sum);
        memcpy(response->content, content_buffer, sum * sizeof(char)); 
        response->content_size = sum;
    } else {
        response->content_size = sum;
    }
    #ifdef DEBUG
    printf("leave forward_response\n");
    #endif
}

int forward_request(int client_fd, Request *request, Response *response) {
    rio_t   server_rio;
    int server_fd;
    char hostname[MAXLINE];
    int  port = 80;

    port = extract_port_number(request->request_str);
    extract_hostname(request->host_str, hostname);

    #ifdef DEBUG
    printf("hostname:%s\n", hostname);
    printf("port:%d\n", port);
    #endif

    if ((server_fd = open_clientfd(hostname, port)) < 0) {
       fprintf(stderr, "Warning connection refused !\n"); 
       return -1;
    }

    rio_readinitb(&server_rio, server_fd);
    #ifdef DEBUG
    printf("request_str:%s", request->request_str); 
    #endif
    
    rio_writen(server_fd, request->request_str, strlen(request->request_str));
    rio_writen(server_fd, request->host_str, strlen(request->host_str));
    rio_writen(server_fd, "\r\n", strlen("\r\n"));
    
    forward_response(client_fd, server_fd, response);
    Close(server_fd);
    return 1;
}

void parse_request_header(int client_fd, Request *request) {
    size_t  n;
    rio_t   client_rio;
    char buffer[MAXLINE];

    rio_readinitb(&client_rio, client_fd);
    copy_single_line_str(&client_rio, request->request_str);
    copy_single_line_str(&client_rio, request->host_str);

    #ifdef DEBUG
    printf("request str:%s", request->request_str);
    printf("host str: %s", request->host_str);
    #endif
    
    while ((n = rio_readlineb(&client_rio, buffer, MAXLINE)) != 0) { 
        #ifdef DEBUG
        printf("%s", buffer); 
        #endif
        if (!strcmp(buffer, "\r\n")) {
            break;
        }
    }
}

void *request_handler(void *ptr) {
    #ifdef DEBUG
    printf("enter request_handler\n");
    #endif

    int client_fd = ((Thread_Input*)ptr)->client_fd; 
    Request request;
    Response response;
    parse_request_header(client_fd, &request);
    modify_request_header(&request);
    
    if (check_cache(&request, &response)) {
        send_client(client_fd, &response);
    } else {
        if (forward_request(client_fd, &request, &response) < 0) {
            Close(client_fd);
            return NULL;
        } else {
            if (response.content_size <= MAX_OBJECT_SIZE)
                save_to_cache(&request, &response);
        }
    }
    free(ptr);
    Close(client_fd);
    #ifdef DEBUG
    printf("connection close\n\n");
    printf("leave request_handler\n");
    #endif
    return NULL; 
}

/*
 * scheduler -
 */
void scheduler(int client_fd) {
    pthread_t tid;

    Thread_Input *thread_input = (Thread_Input*)malloc(sizeof(Thread_Input));
    thread_input->client_fd = client_fd;

    Pthread_create(&tid, NULL, request_handler, thread_input);
    Pthread_join(tid, NULL);
    //Pthread_detach(tid);
}


int main (int argc, char *argv []) {
    int listen_fd, port;
    int client_fd;
    socklen_t socket_length;
    struct sockaddr_in client_socket;

    pthread_mutex_init(&mutex_lock, NULL);
    init_cache();
    Signal (SIGPIPE, SIG_IGN);
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }

    port = atoi(argv[1]);
    listen_fd = Open_listenfd(port);
    socket_length = sizeof(client_socket);

    while (1) {
        client_fd = Accept(listen_fd, (SA*)&client_socket, &socket_length);   
        scheduler(client_fd);
    }
    pthread_mutex_destroy(&mutex_lock);
    clean_cache();
}
