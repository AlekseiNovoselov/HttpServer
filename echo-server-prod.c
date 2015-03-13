#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event.h> // for EVBUFFER_LENGTH
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "parseRequest.h"
char document_root[256];
int NCPU;  

typedef struct client {  
  int fd; 
  struct bufferevent *buf_ev;
  struct evconnlistener *listener;
  evutil_socket_t sfd;
} client_t;

/* Функция обратного вызова для события: данные готовы для чтения в buf_ev */
void echo_read_cb( struct bufferevent *buf_ev, void *arg )
{
  client_t *client = (client_t *)arg;
  struct stat fileStat; // for file information
  struct evbuffer *input = bufferevent_get_input( buf_ev );
  struct evbuffer *output = bufferevent_get_output( buf_ev );
  int inputSize = evbuffer_get_length(input);
  unsigned char *data;
  data = evbuffer_pullup(input, inputSize);

  request_t *request;
  if ((request = malloc(sizeof(*request))) == NULL) {
    warn("failed to allocate memory for request state");
    //close(client_fd);
    return;
  }
  memset(request, 0, sizeof(*request));
  request->document_root = document_root;
  parse_request(data, request);
  int sendFile = 0;

  if ( strcmp(request->method, "GET") == 0 ) {
       //printf("Send file \n");
       sendFile = 1;
  }
  //printf("HTTP/1.1 %d OK\r\nDate: %sServer: LexaLorisServer\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n", request->status, request->c_time_string, request->ContentLength, request->ContentType );

  evbuffer_add_printf(output, "HTTP/1.1 %d %s\r\nDate: %sServer: LexaLorisServer\r\nConnection: close\r\n", 
      request->status, request->statusComment, request->c_time_string);
  if (request->status == 200) 
      evbuffer_add_printf(output, "Content-Length: %d\r\nContent-Type: %s\r\n\r\n", 
      request->ContentLength, request->ContentType);
  else evbuffer_add_printf(output, "\r\n");

  if ( sendFile && request->readFile == 1) {           
      evbuffer_add_file(output, request->fileDescriptor, 0, request->fileSize);    
  }
  client->fd = request->fileDescriptor;
  free(request);
}


void buffered_on_write(struct bufferevent *bev, void *arg) {
    client_t *client = (client_t *)arg;
    close(client->fd);
    bufferevent_free(bev);
    free(client);
}

void echo_event_cb( struct bufferevent *buf_ev, short events, void *arg )
{   

  client_t *client = (client_t *)arg;
  if( events & BEV_EVENT_ERROR ) {
    //printf("Error\n");
    perror( "Ошибка объекта bufferevent" );
    close(client->fd);
    free(client);
  }
  if( events & (BEV_EVENT_EOF | BEV_EVENT_ERROR) ) {
    //printf("Error\n");
    bufferevent_free( buf_ev );
    close(client->fd);
    free(client);
  }
}

void do_client_request(void* arg) {
    client_t *client = (client_t *)arg;
    struct event_base *base = evconnlistener_get_base( client->listener );
    struct bufferevent *buf_ev = bufferevent_socket_new( base, client->sfd, BEV_OPT_CLOSE_ON_FREE );
    client->buf_ev = buf_ev;
    bufferevent_setcb( buf_ev, echo_read_cb, buffered_on_write, echo_event_cb, client );
    bufferevent_enable( buf_ev, (EV_READ) );
}


void on_accept( struct evconnlistener *listener,
                   evutil_socket_t sfd, struct sockaddr *addr, int sock_len,
                   void *arg )
{
  client_t *client;
  if ((client = malloc(sizeof(*client))) == NULL) {
    warn("failed to allocate memory for client state");
    close(client->fd);
    return;
  }
  memset(client, 0, sizeof(*client));
  client->listener = listener;
  client->sfd = sfd; 
  do_client_request(client);
}

void accept_error_cb( struct evconnlistener *listener, void *arg )
{
  //printf("Error connection\n");
  printf("ACCEPT ERROR!!!!");
  client_t *client = (client_t *)arg;  
  struct event_base *base = evconnlistener_get_base( listener );
  int error = EVUTIL_SOCKET_ERROR();
  fprintf( stderr, "Ошибка %d (%s) в мониторе соединений. Завершение работы.\n",
           error, evutil_socket_error_to_string( error ) );
  if (client->fd > 0) {
    close(client->fd);
    free(client);
  }
  event_base_loopexit( base, NULL );
}

int main( int argc, char **argv )
{
  struct event_base *base;
  struct evconnlistener *listener;
  struct sockaddr_in sin;
  int port = 8080;


  if(argc <= 1) {
      printf("syntax : \n\t./httpd -r DOCUMENT_ROOT -c NCPU\n");
      return 0;
  }
  else {
      if ( strcmp(argv[1], "-r") == 0 && sscanf(argv[2], "%s", document_root)) {
            printf("correct directory\n");
      } else {
          printf("Error scan -r\n");  
          return 0;
      }
      if ( strcmp(argv[3], "-c") == 0 && sscanf(argv[4], "%d", &NCPU)) {
          printf("correct NCPU\n");
      } else {
          printf("Error scan -c\n");  
          return 0;
      }
  }
  printf("%s\n", document_root);
  printf("%d\n", NCPU);

  base = event_base_new();
  if( !base )
  {
    fprintf( stderr, "Ошибка при создании объекта event_base.\n" );
    return -1;
  }

  memset( &sin, 0, sizeof(sin) );
  sin.sin_family = AF_INET;    /* работа с доменом IP-адресов */
  sin.sin_addr.s_addr = htonl( INADDR_ANY );  /* принимать запросы с любых адресов */
  sin.sin_port = htons( port );


  printf("Start server on port %d\n", port);

  listener = evconnlistener_new_bind( base, on_accept, NULL,
                                     (LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE),
                                     -1, (struct sockaddr *)&sin, sizeof(sin) );

  if( !listener )
  {
    perror( "Ошибка при создании объекта evconnlistener" );
    return -1;
  }

  evconnlistener_set_error_cb( listener, accept_error_cb );

  int numberOfWorkers = NCPU;
  //numberOfWorkers = sysconf( _SC_NPROCESSORS_ONLN );
  int i = 0;
  for (i = 0; i < numberOfWorkers-1; i++) {
      int pid = fork();

      if (!pid) {
          event_reinit(base);
          break;
      }
  }

  event_base_dispatch( base );
  return 0;
}