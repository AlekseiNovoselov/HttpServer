#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "parseRequest.h"

int cvt(char *dest, const char *src) {
	const char *p = src;
	char code[3] = {0};
	unsigned long ascii = 0;
	char *end = NULL;

  int i = 0;
	while(*p) {
		if(*p == '%') {
			memcpy(code, ++p, 2);	
			ascii = strtoul(code, &end, 16);
			*dest++ = (char)ascii;
      i++;
			p += 2;
		}
		else
			*dest++ = *p++;
	}  
  return (int)strlen(dest);
}

void setDate (void *arg) {
	request_t *request = (request_t *)arg;
	time_t current_time;
    char* c_time_string;
 
    current_time = time(NULL);
 
    if (current_time == ((time_t)-1))
        (void) fprintf(stderr, "Failure to compute the current time.");        
 
    c_time_string = ctime(&current_time);
 
    if (c_time_string == NULL)
        (void) fprintf(stderr, "Failure to convert the current time.");
    request->c_time_string = c_time_string;
}

void setStatusComment (void *arg) {
	request_t *request = (request_t *)arg;
  	switch(request->status)
   	{
   		case 200 :
      		request->statusComment = "OK";
      		break;
   		case 400 :
   			request->statusComment = "Bad Request";	
   			break;
   		case 403 :
     		 request->statusComment = "Forbidden";
     		 break;
   		case 404 :
      		request->statusComment = "Not Found";
      		break;
   		case 405 :
      		request->statusComment = "Method Not Allowed";
      		break;  
   }
}

void setContentType (void *arg) {
	request_t *request = (request_t *)arg;
	int lenghtUrl = (int)strlen(request->path);
	int a = lenghtUrl-1;	
    for (; a > 0; a--) {
        if (request->path[a] == '.')
          break;
    }
    char type[lenghtUrl-a-1];
    int g = 0;
    while ( g < lenghtUrl-a-1) {
        type[g] = request->path[g+a+1];
        g++;
    }
    type[g] = '\0';
      if ( strcmp(type, "html") == 0) {
        request->ContentType = "text/html";
      }
      else if ( strcmp(type, "css") == 0) {
        request->ContentType = "text/css";
      }
      else if ( strcmp(type, "png") == 0) {
        request->ContentType = "image/png";
      }
      else if ( strcmp(type, "swf") == 0) {
        request->ContentType = "application/x-shockwave-flash";
      }     
      else if ( strcmp(type, "js") == 0) {
        request->ContentType = "text/javascript";
      }
      else if ( strcmp(type, "gif") == 0) {
        request->ContentType = "image/gif";
      }
      else if ( strcmp(type, "jpg") == 0) {
       	request->ContentType = "image/jpeg";
      }
      else if ( strcmp(type, "jpeg") == 0) {
        request->ContentType = "image/jpeg";        
      }
      else {
        request->ContentType = "content/unknown";        
      }
}

void parse_request( unsigned char *data, void *arg) {

	  struct stat fileStat; // for file information
	  request_t *request = (request_t *)arg;
	  int i = 0;
    int j = 0;
    int k = 0;
    int z = 0;   
    char http[50];    
    while ( data[i] != ' ') {   
      i++;
    }    
    int c = 0;
    char method[i];
    while ( c < i) {
      method[c] = data[c];
      c++;
    }
    method[i] = '\0';
    i++;  
    while ( data[i] != ' ' && data[i] != '?') {   
        if ( data[i] == '%' && data[i+1] == '2' && data[i+2] == '0') {      
            j -= 2;
        }
        i++;
        j++;
    } 
    i--;
    while ( data[i] != ' ') {   	
        i++;    
    }   
    char urlRaw[j];
    while ( z < j) {
        if ( data[c+z+1] == '%' && data[c+z+2] == '2' && data[c+z+3] == '0') {      
        urlRaw[z] = ' ';
        c += 2;     
    }
    else
        urlRaw[z] = data[c+z+1];   
        z++;
    } 
    if (urlRaw[j] = '@')   
        urlRaw[j] = '\0';
  
    while ( data[i] != '\n') { 
        http[k] = data[i];
        i++;
        k++;
    }
    if (http[k] = '@')    
        http[k] = '\0'; 

  	char url[sizeof urlRaw];
    int pogr = cvt(url, urlRaw);  
    url[(int)strlen(url)-pogr] = '\0';

	  if ((int)strlen(url) > (int)strlen(urlRaw))
		     url[(int)strlen(urlRaw)] = '\0';

	  request->method = method;
	  request->url = url;
	  request->http = http;

    if ( strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0) {
	      request->status = 200;
	   
        char* HOMEDIR;
        HOMEDIR = request->document_root;
        int pathSize = 0;
        if (url[strlen(url)-1] == '/')
            pathSize = strlen(HOMEDIR) + strlen(url) + strlen("index.html");
        else 
            pathSize = strlen(HOMEDIR) + strlen(url);
        char path[pathSize];
        strncat(path, HOMEDIR, strlen(HOMEDIR));
        strncat(path, url, strlen(url));
        if (url[strlen(url)-1] == '/')
            strncat(path, "index.html", strlen("index.html"));

        while (path[0] != '/' || (int)strlen(path) != pathSize) {   
            memmove(path, path+1, strlen(path));     
        }

        request->path = path;    

        i = 0;
        j = 0;
        int raz = 0;
        int urlLen;
        char newUrl[1000];
        for (i; i < 1000; i++) {
            newUrl[i] = '\0';
        } 
        i = 0;
        urlLen = strlen(path); 
        for (i; i < urlLen; i++) {
            newUrl[i-raz] = path[i];
            if ( path[i] == '.' && path[i+1] == '.' && path[i+2] == '/') {      
                for (j = i-2-raz; j >= 0; j--) {
                    if (newUrl[j] == '/') {          
                        break;
                    }             
                } 
                int y;
                for (y = i-2; y >= 0; y--) {
                    if (path[y] == '/') {          
                        break;
                    }             
                }
                raz += i - y;       
                int len = (int)strlen(newUrl);           
                int k = 0;
                if (j > 0)
                    k = j;                
                if ( k >= 0) {  
                    if ( k == 0)
                        raz = i;         
                    for ( k ; k < len; k++) {         
                        newUrl[k+1] = '\0'; 
                    }
                }
                i+=2;     
                raz+=2;
            }    
        }

        int v = 0;
        for (v; v < i; v++ ) {
            path[v] = newUrl[v];
        }
        path[i] = '\0';

        v = 0;
        for (v; v < (int)strlen(request->document_root) - 1; v++) {
            if (path[v] != request->document_root[v]) {
            //printf("OUT OF DOCUMENT DIRECTORY\n");
            request->status = 403;        
            break;
        }     
    } 


    int readFile = 0;
    int fileDescriptor;

    fileDescriptor = open(path, O_RDONLY | O_NONBLOCK);
    if (fileDescriptor != -1) {
        fstat(fileDescriptor, &fileStat);      
        request->fileDescriptor = fileDescriptor;
   	    request->fileSize = (int)fileStat.st_size;
        readFile = 1;
      }
       
    if ( readFile ) {    
      request->readFile = 1;
      int i = (int)fileStat.st_size;      
      request->ContentLength = i;
      setContentType(request);
   }
   else {
      if (url[strlen(url)-1] == '/') {
          if (url[strlen(url)-6] == '.')
            	request->status = 404;
          else
            	request->status = 403;
          }
          else 
      	     request->status = 404;
      }
  }
  else if ( strcmp(method, "POST") == 0) {
  		request->status = 405;
  }
  else { 
     //	 printf("Merhod not found\n");
  }
  setDate(request);
  setStatusComment(request);
}