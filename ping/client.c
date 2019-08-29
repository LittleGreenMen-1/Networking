#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

#define PORT 2100

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
    int sock, n;
    int i;
    struct sockaddr_in server_addr;
    struct hostent * server;
    char buffer[256], input[50];
    
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        printf("ERROR on creating socket\n");
        return -1;
    }
    printf("Done creating socket\n");

    server = gethostbyname("localhost");

    if(server == NULL)
    {
        printf("ERROR no such host");
        return -1;
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr_list[0], 
           (char *)&server_addr.sin_addr.s_addr,
           server->h_length);
    
    server_addr.sin_port = htons(PORT);

    printf("DONE setting up\n");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("ERROR on connecting to server\n");
        return -1;
    }
    printf("Done connecting to server\n");

    for(i = 1; i <= 10; i++)
    {
        memset(buffer, 0, 255);

        scanf("%s", &input);
        //snprintf(buffer, 255, "Ping %d/10: %s", i, input);
        snprintf(buffer, 255, "%s", input);

        if(write(sock, buffer, strlen(buffer)) < 0)
        {
            printf("ERROR: write()\n");
            //return -1;
        }
        printf("Sent message: %s\n", buffer);

        if(strcmp(input, "test") == 0)
        {
            memset(buffer, 0, 255);

            n = read(sock, buffer, 255);

            if(n < 0)
            {
                printf("ERROR: read()\n");
            }
            else
            {
                printf("Mess form server: %s\n", buffer);
            }
        }
    }

    return 0;
}