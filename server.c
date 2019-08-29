#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 2001

void delay(int milliseconds)
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

int main()
{
    int server_conn, client_conn; // file descriptors for server and client
    int len, n;
    struct sockaddr_in server_addr, client_addr;
    char buffer[256], command[10];

    server_conn = socket(AF_INET, SOCK_STREAM, 0);

    if(server_conn < 0)
    {
        printf("ERROR on creating socket\n");
        return -1;
    }
    printf("Done creating socket\n");

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if(bind(server_conn, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        printf("ERROR on binding\n");
        return -1;
    }
    printf("Done binding\n");

    listen(server_conn, 5);

    memset(&client_addr, 0, sizeof(client_addr));

    len = sizeof(client_addr);
    client_conn = accept(server_conn, (struct sockaddr *) &client_addr, &len);

    if(client_conn < 0)
    {
        printf("ERROR on accepting request\n");
        return -1;
    }
    printf("Done accepting connection\n");

    while(1)
    {
        memset(buffer, 0, 255);
        
        n = read(client_conn, buffer, 255);
        if(n < 0)
        {
            printf("ERROR: read()\n");
            //return -1;
        }
        else 
            if(n == 0)
                break;

        printf("Received: %s\n", buffer);

        snprintf(command, 10, "%s", &buffer[11]);

        if(strcmp(command, "test") == 0)
        {
            memset(buffer, 0, 255);
            snprintf(buffer, 255, "Yes I got your message");

            if( write(client_conn, buffer, strlen(buffer)) < 0 )
            {
                printf("ERROR: write()\n");
            }
        }
    }
    
    return 0;
}