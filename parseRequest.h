typedef struct request {
  unsigned char* method;
  unsigned char* url;
  unsigned char* http;
  int status;
  unsigned char* statusComment;
  unsigned char* path;
  int ContentLength;
  unsigned char* ContentType;
  int readFile;
  int fileDescriptor;
  int fileSize;
  char* c_time_string;
  char* document_root;
} request_t;

void parse_request( unsigned char *data, void *arg);
void setContentType (void *arg);
void setContentLenght (void *arg, int i);