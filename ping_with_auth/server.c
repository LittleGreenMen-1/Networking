#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include <errno.h>
#include <time.h>

#define PORT 2102

extern int errno;

void func1(char * message, int len, char key)
{
    int i;
    char aux;

    for(i = 0; i < len / 2; i++)
    {
        aux = message[len - i - 1];
        message[len - i - 1] = message[i];
        message[i] = aux;
    }

    return ;
}

void func2(char * message, int len, char key)
{
    int i;

    for(i = 0; i < len; i++)
        if((message[i] & (1 << 8)) == 0)
            message[i] <<= 1;
    
    return ;
}

void func3(char * message, int len, char key)
{
    int i;

    for(i = 0; i < len; i++)
    {
        if('A' <= message[i] && message[i] <= 'Z')
            message[i] = 'Z' - message[i] + 'A';
        if('a' <= message[i] && message[i] <= 'z')
            message[i] = 'z' - message[i] + 'a';
    }

    return ;
}

void func4(char * message, int len, char key)
{
    int i, bit_pos;
    char aux, bit1, bit2;

    for(i = 0; i < len; i++)
    {
        for(bit_pos = 0; bit_pos < 4; bit_pos++)
        {
            bit1 = (message[i] >> bit_pos) & 1;
            bit2 = (message[i] >> (7 - bit_pos)) & 1;

            message[i] ^= (-bit2 ^ message[i]) & (1UL << bit_pos);
            message[i] ^= (-bit1 ^ message[i]) & (1UL << (7 - bit_pos));
        }
    }

    return ;
}

void func5(char * message, int len, char key)
{
    int i;

    for(i = 0; i < len; i++)
        message[i] ^= key;

    return ;
}

int receive_buffer(int socket, char * buffer, _Bool wait)
{
    int n;

    if(!wait)
    {
        ioctl(socket, FIONREAD, &n);

        if(n > 0)
        {
            memset(buffer, 0, strlen(buffer));
            n = read(socket, buffer, 255);

            if(n < 0)
            {
                printf("ERROR: read()\n");
            }
        }
    }
    else
    {
        memset(buffer, 0, strlen(buffer));
        n = read(socket, buffer, 255);

        if(n < 0)
        {
            printf("ERROR: read()\n");
        }
    }
    

    return n;
}

void send_buffer(int socket, char * buffer, _Bool response)
{
    int n;

    write(socket, buffer, strlen(buffer));

    if(response)
    {
        ioctl(socket, FIONREAD, &n);

        if(n > 0)
        {
            memset(buffer, 0, strlen(buffer));
            n = read(socket, buffer, 255);

            if(n < 0)
            {
                printf("ERROR: read()\n");
            }
        }
    }

    return ;
}

int check_response(char * r1, char * r2)
{
    int i;

    if(strlen(r1) < 5 || strlen(r2) < 5)
        return 0;
        
    if(r1[0] != r2[0] || r1[1] != r2[1] || r1[2] != r2[2] || r1[3] != r2[3])
        return 0;
        
    for(i = 0; i < r1[1]; i++)
        if(r1[i + 4] != r2[i + 4])
            return 0;
        
    return 1;
}

int main()
{
    int server_sock, client_sock;
    int client_len;
    struct sockaddr_in server_addr, client_addr;

    char buffer[255], check[255];

    void (*func[5])(char *, int, char) = {func1, func2, func3, func4, func5};
    int func_ind, loop_nr, key, mess_len, i;

    char success;
    int errnum;

    srand(time(NULL));
    success = 0;
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(server_sock < 0)
    {
        printf("ERROR: socket()\n");
        return -1;
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if( bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0 )
    {
        errnum = errno;
        fprintf(stderr, "%s\n", strerror(errnum));
        printf("ERROR: bind()\n");
        return -1;
    }

    listen(server_sock, 5);

    memset((char *)&client_addr, 0, sizeof(client_addr));
    client_len = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

    if(client_sock < 0)
    {
        printf("ERROR: accept()\n");
        return -1;
    }

    receive_buffer(client_sock, buffer, 0);

    if(strcmp(buffer, "CONN") == 0);
    {
        printf("Connection request...\n");
        func_ind = rand()%5;
        loop_nr = rand()%10;
        key = rand()%128;
        mess_len = 7;

        buffer[0] = func_ind;
        buffer[1] = loop_nr;
        buffer[2] = key;
        buffer[3] = mess_len;
        
        for(i = 0; i < mess_len; i++)
            buffer[4 + i] = rand()%128;

        send_buffer(client_sock, buffer, 0);
        printf("Sent auth buffer...\n");

        for(i = 0; i < loop_nr; i++)
            (func[func_ind])(&buffer[4], mess_len, key);
        
        memset(check, 0, strlen(check));
        strncpy(check, buffer, strlen(buffer));

        if(receive_buffer(client_sock, buffer, 1) > 0)
        {
            printf("Received auth buffer back...\n");
            if(check_response(check, buffer) == 1)
            {
                // Auth successful
                memset(buffer, 0, strlen(buffer));
                snprintf(buffer, 255, "AOK");

                send_buffer(client_sock, buffer, 0);

                success = 1;
            }
            else
            {
                // Auth failed
                memset(buffer, 0, strlen(buffer));
                snprintf(buffer, 255, "FAIL");

                send_buffer(client_sock, buffer, 0);

                success = 0;
            }
        }   
    }

    if(success)
        // Begin conv
        printf("Authentificatin successful\n");
    else
        printf("Authentificatin failed\n");

    // Close sockets
    printf("\n-------------------------------\n");
    close(server_sock);
    printf("Closed socket: %d\n", server_sock);
    close(client_sock);
    printf("Closed socket: %d\n", client_sock);

    return 0;
}