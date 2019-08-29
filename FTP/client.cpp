/*
    command:
    
    exit   =  quit
    ls     =  list files and folders in directory
    cd     =  change directory
    send   =  send file
    fetch  =  get file from server
*/

#include <iostream>
#include <fstream>

#include <cstring>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <time.h>
#include <signal.h>
#include <errno.h>

#define PORT_FTP 2100

int stop;

std::string current_directory;

extern int errno;

int init_server()
{
    int sock;
    int errnum;
    struct sockaddr_in server_addr;
    struct hostent * server;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        std::cerr << "ERROR: sock()\n";
        return -1;
    }

    server = gethostbyname("localhost");

    if(server == NULL)
    {
        std::cerr << "ERROR: gethostbyname()\n";
        return -1;
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr_list[0],
           (char *) &server_addr.sin_addr.s_addr,
           server->h_length);
    
    server_addr.sin_port = htons(PORT_FTP);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        errnum = errno;
        std::cerr << "ERROR: connect() >> " << strerror(errnum) << '\n';
        return -1;
    }

    return sock;
}

int send_file(int sock, char * file)
{
    std::ifstream to_send(file, std::ios::binary);
    char buffer[257];

    if(!to_send.good())
        return -1;

    while(to_send.getline(buffer, 255, EOF))
    {
        write(sock, buffer, strlen(buffer));
        memset(buffer, 0, strlen(buffer));
    }
    
    if(strlen(buffer) != 0)
        write(sock, buffer, strlen(buffer));
    
    memset(buffer, 0, strlen(buffer));
    memset(buffer, EOF, 5);
    write(sock, buffer, strlen(buffer));

    to_send.close();
    return 1;
}

int receive_file(int sock, char * file)
{
    std::ofstream to_recv(file, std::ios::binary);
    char buffer[257];
    int n;
    
    n = 1;

    while(buffer[strlen(buffer) - 1] != EOF)
    {
        n = read(sock, buffer, 255);

        to_recv << buffer;
    }

    to_recv.close();
    return 1;
}

int main()
{
    int sock;
    struct sigaction int_handler;
    char buffer[256], aux[256];
    char command[256];
    char argv[10][256];
    int argc;

    char username[256];

    int len, i, j, n;

    stop = 0;
    sock = init_server();

    if(sock < 0)
    {
        printf("ERROR: init_server()\n");
        return -1;
    }

    int_handler.sa_handler = [](int s) { stop = 1; };
    sigemptyset(&int_handler.sa_mask);
    int_handler.sa_flags = 0;

    sigaction(SIGINT, &int_handler, NULL);

    memset(buffer, 0, sizeof(buffer));

    std::cout << "Username: ";
    std::cin.getline(username, 255);

    memcpy(buffer, username, strlen(aux));

    std::cout << "Password: ";
    std::cin.getline(aux, 255);

    buffer[strlen(buffer)] = '/';
    memcpy(&(buffer[strlen(buffer)]), aux, strlen(aux));

    write(sock, buffer, strlen(buffer));

    memset(buffer, 0, strlen(buffer));
    read(sock, buffer, 255);

    // std::cout << buffer << '\n';

    stop = strcmp(buffer, "200 OK");

    while (!stop)
    {
        std::cout << current_directory << " > ";
        memset(buffer, 0, strlen(buffer));

        std::cin.getline(buffer, 256);

        // Process commands

        n = strlen(buffer);

        j = 0;
        for(i = 0; i < n && buffer[i] != ' '; i++, j++)
            command[j] = tolower(buffer[i]);
        i++;
        
        j = 0;
        argc = 0;

        memset(argv[argc], 0, sizeof(argv[argc]));

        for(; i < n; i++)
        {
            if(buffer[i] == ' ')
            {
                argc++;
                j = 0;

                memset(argv[argc], 0, sizeof(argv[argc]));
            }
            else
            {
                argv[argc][j] = tolower(buffer[i]);
                j++;
            }
        }
        argc++;

        write(sock, buffer, strlen(buffer));

        if(strcmp(command, "exit") == 0)
            stop = 1;
        
        else if(strcmp(command, "send") == 0)
        {
            if(argc < 2)
            {
                std::cout << "Invalid command\n";
                break;
            }
            send_file(sock, argv[0]);
        }
        
        else if(strcmp(command, "fetch") == 0)
        {
            if(argc < 2)
            {
                std::cout << "Invalid command\n";
                break;
            }
            receive_file(sock, argv[1]);
        }

        else if(strcmp(command, "ls") == 0)
        {
            memset(buffer, 0, strlen(buffer));

            buffer[0] = 'a';
            
            while(buffer[strlen(buffer) - 1] != EOF)
            {
                memset(buffer, 0, strlen(buffer));
                n = read(sock, buffer, 255);

                std::cout << buffer;
            }
        }

        else if(strcmp(command, "cd") == 0)
        {
            if(argc < 1)
                break;

            read(sock, buffer, 255);

            std::cout << buffer << '\n';

            if(strcmp(buffer, "31 Illegal operation") == 0)
            {
                // Directory inexistent

                std::cout << "Directory inexistent\n";
            }
            else if(strcmp(buffer, "30 Illegal operation") == 0)
            {
                std::cout << "You already hit rock bottom! Where do you want to go?\n";
            }
            else
            {
                current_directory = std::string(buffer);

                memset(buffer, 0, strlen(buffer));
                snprintf(buffer, 255, "200 OK");

                write(sock, buffer, strlen(buffer));
            }            
        }
    }

    n = read(sock, buffer, 255);

    std::cout << "Close connection\n";

    if(strcmp(buffer, "500 OK"))
        std::cout << "\nAll good\n";

    close(sock);

    return 0;
}