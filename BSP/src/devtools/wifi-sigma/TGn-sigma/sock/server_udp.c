#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int port=6789;

int main(int argc, char** argv) {

    int sin_len;
    char message[256];

    int socket_descriptor;
    struct sockaddr_in sin;
    printf("Waiting for data form sender \n");

    bzero(&sin,sizeof(sin));
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=htonl(INADDR_ANY);
    if (argc == 1) {
        /* user not specific port, use default 6789 */
	printf("use default port 6789\n");
    } else {
        printf("User port argv[1]: %s\n", argv[1]);
        port = atoi(argv[1]);
        printf("User port: %d\n", port);
    }
    sin.sin_port=htons(port);
    sin_len=sizeof(sin);

    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);
    bind(socket_descriptor,(struct sockaddr *)&sin,sizeof(sin));

    while(1)
    {
        recvfrom(socket_descriptor,message,sizeof(message),0,(struct sockaddr *)&sin,&sin_len);
        printf("Response from server:%s\n",message);
        if(strncmp(message,"stop",4) == 0)
        {

            printf("Sender has told me to end the connection\n");
            break;
        }
    }

    close(socket_descriptor);
    exit(0);

    return (EXIT_SUCCESS);
}
