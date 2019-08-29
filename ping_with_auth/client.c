#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 2102

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

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    struct hostent * server;

    char buffer[256];

    void (*func[5])(char *, int, char) = {func1, func2, func3, func4, func5};
    int func_ind, loop_nr, key, mess_len, i;

    char success;

    success = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        printf("ERROR: sock()\n");
        return -1;
    }

    server = gethostbyname("localhost");

    if(server == NULL)
    {
        printf("ERROR: getting host\n");
        return -1;
    }

    memset((char *) &server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr_list[0],
           (char *)&server_addr.sin_addr.s_addr,
           server->h_length);

    server_addr.sin_port = htons(PORT);

    if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("ERROR: connect()\n");
        return -1;
    }

    memset(buffer, 0, strlen(buffer));
    snprintf(buffer, 255, "CONN");

    // "Authentificating"
    send_buffer(sock, buffer, 0);
    receive_buffer(sock, buffer, 1);

    if(strlen(buffer) < 5)
    {
        printf("ERROR: server response too short. One more time...\n");

        receive_buffer(sock, buffer, 1);

        if(strlen(buffer) < 5)
        {
            printf("[CLIENT] I got:\n");
            for(i = 0; i < strlen(buffer); i++)
                printf("%d ", buffer[i]);
            printf("\n");
            printf("ERROR: server respone too short, again :((\n");
            return -1;
        }
    }
    else
    {
        printf("Got auth buffer...\n");
        func_ind = buffer[0];
        loop_nr = buffer[1];
        key = buffer[2];
        mess_len = buffer[3];

        for(i = 0; i < loop_nr; i++)
            (func[func_ind])(&buffer[4], mess_len, key);
        memset(&buffer[mess_len + 4], 0, (255 - mess_len - 4));

        printf("Sending response...\n");
        send_buffer(sock, buffer, 0);
        receive_buffer(sock, buffer, 1);

        if(strcmp(buffer, "AOK") == 0)
        {
            // Auth successful

            success = 1;
        }
        else
        {
            // Auth failed

            success = 0;
        }
    }

    if(success)
        // Begin conv
        printf("Authentificatin successful\n");
    else
        printf("Authentificatin failed\n");
    
    // Close socket
    printf("\n-------------------------------\n");
    close(sock);
    printf("Closed socket: %d\n", sock);

    return 0;
}