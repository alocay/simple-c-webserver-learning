#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>

#define PORT 8080
#define BUFF_SIZE 65536
#define GET_METHOD_UPPER "GET"
#define GET_METHOD_LOWER "get"
#define POST_METHOD_UPPER "POST"
#define POST_METHOD_LOWER "post"
#define PUT_METHOD_UPPER "PUT"
#define PUT_METHOD_LOWER "put"
#define DELETE_METHOD_UPPER "DELETE"
#define DELETE_METHOD_LOWER "delete"
#define PATCH_METHOD_UPPER "PATCH"
#define PATCH_METHOD_LOWER "patch"
#define HTTP_METHOD_GET 0
#define HTTP_METHOD_POST 1
#define HTTP_METHOD_PUT 2
#define HTTP_METHOD_DELETE 3
#define HTTP_METHOD_PATCH 4

void failure_cleanup_and_exit(char* message, int server_fd) {
  perror(message);
  printf("WSA last error: %d", WSAGetLastError());
  WSACleanup();

  if (server_fd != -1) {
    close(server_fd);
  }

  exit(EXIT_FAILURE);
}

int is_get(char* request) {
  return strncmp(request, GET_METHOD_UPPER, 3) == 0 || strncmp(request, GET_METHOD_LOWER, 3) == 0; 
}

int is_post(char* request) {
  return strncmp(request, POST_METHOD_UPPER, 4) == 0 || strncmp(request, POST_METHOD_LOWER, 4) == 0; 
}

int is_put(char* request) {
  return strncmp(request, PUT_METHOD_UPPER, 3) == 0 || strncmp(request, PUT_METHOD_LOWER, 3) == 0; 
}

int is_delete(char* request) {
  return strncmp(request, DELETE_METHOD_UPPER, 6) == 0 || strncmp(request, DELETE_METHOD_LOWER, 6) == 0; 
}

int is_patch(char* request) {
  return strncmp(request, PATCH_METHOD_UPPER, 5) == 0 || strncmp(request, PATCH_METHOD_LOWER, 5) == 0; 
}

int get_http_method(char* request) {
  if (is_get(request)) {
    return HTTP_METHOD_GET;
  } else if (is_post(request)) {
    return HTTP_METHOD_POST;
  } else if (is_put(request)) {
    return HTTP_METHOD_PUT;
  } else if (is_delete(request)) {
    return HTTP_METHOD_DELETE;
  } else if (is_patch(request)) {
    return HTTP_METHOD_PATCH;
  } else {
    return -1;
  }
}

int init_socket(struct sockaddr_in* address) {
  int server_fd;
  char opt = 1;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    failure_cleanup_and_exit("socket failure", -1);
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    failure_cleanup_and_exit("setsockopt", server_fd);
  }

  if (bind(server_fd, (struct sockaddr*)address, sizeof(*address)) < 0) {
    failure_cleanup_and_exit("bind failed", server_fd);
  }

  return server_fd;
}

void handle_next_request(int server_fd, struct sockaddr_in* address, int addrlen) {
  int new_socket;
  ssize_t valread;
  char buffer[BUFF_SIZE+1] = { 0 };
  char response_buffer[BUFF_SIZE+1];
  int http_method;

  if ((new_socket = accept(server_fd, (struct sockaddr*)address, &addrlen)) < 0) {
    failure_cleanup_and_exit("accept", server_fd);
  }

  valread = recv(new_socket, buffer, BUFF_SIZE, 0);

  if (valread == SOCKET_ERROR) {
    failure_cleanup_and_exit("recv error", server_fd);
  }

  printf("bytes read: %d\n%s\n", (int)valread, buffer);
  
  if ((http_method = get_http_method(buffer)) < 0) {
    printf("unsupported http method\n");
    close(new_socket);
    return;
  }

  printf("%d\n", http_method);

  char* response_data = "hello!";
  sprintf(response_buffer, "HTTP/1.1 200 OK\r\nServer: server\r\nContent-Length: %ld\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\nhello!", strlen(response_data));
  send(new_socket, response_buffer, (int)strlen(response_buffer), 0);
  printf("Hello message sent\n");

  close(new_socket);
}

int main(int argc, char const* argv[]) {
  int server_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  WORD winsock_ver = MAKEWORD(2, 2);
  WSADATA winsock_data;

  if (WSAStartup(winsock_ver, &winsock_data) != 0) {
    failure_cleanup_and_exit("WSAStartup failed", -1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  server_fd = init_socket(&address);

  if (listen(server_fd, 3) < 0) {
    failure_cleanup_and_exit("listen failed", server_fd);
  }

  int num_of_requests = 0;
  while (num_of_requests < 3) {
    handle_next_request(server_fd, &address, addrlen);
    num_of_requests++;
  }

  close(server_fd);
  WSACleanup();
  return 0;
}
