#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <csignal>

#define PORT 2223

bool stop; // Stop flag

int main(int argc, char ** argv)
{
    if(argc < 2)
    {
        std::cout << "\nUsage: \n./client ADDRESS [--output]\n     --output : Show commands output\n\n";

        return -1;
    }

    int sock; // File descriptor for the server connection
    int i;  // "Standard" index
    int ret;
    char buffer[256];   // R/W buffer

    bool write_output;  // Output flag

    struct sockaddr_in server_addr; // Server address
    struct hostent * server_name;
    struct sigaction handler; // Interrupt handler

    int host_ind;   // argv index for host
    std::string server_username;

    stop = false;
    write_output = false;

    handler.sa_handler = [](int s){ stop = true; } ;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;

    sigaction(SIGINT, &handler, NULL);

    // Init socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        std::cout << "ERROR: cannot create socket\n";

        return -1;
    }

    // Set server address
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Command arguments evaluation
    for(i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--output") == 0)
            write_output = true;
        else
            host_ind = i;
    }

    // Finding host address
    server_name = gethostbyname(argv[host_ind]);

    if(server_name == NULL)
    {
        if(inet_pton(AF_INET, argv[host_ind], &server_addr.sin_addr) <= 0)
        {
            std::cout << "Invalid host\n";

            return -1;
        }
    }
    else
    {
        memcpy(server_name->h_addr_list[0],
               &server_addr.sin_addr.s_addr,
               server_name->h_length);
    }

    // Connect to server
    if(connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        std::cout << "ERROR: cannot connect to server\n";

        return -1;
    }
    memset(buffer, 0, sizeof(buffer));

    buffer[0] = write_output;
    write(sock, buffer, 1);

    read(sock, buffer, sizeof(buffer));
    server_username = std::string(buffer);

    while(!stop)
    {
        memset(buffer, 0, strlen(buffer));

        std::cout << server_username << '@' << argv[host_ind] << " > ";

        std::cin.getline(buffer, 256);

        std::cout << "Client: writing command\n";

        write(sock, buffer, strlen(buffer));

        std::cout << "Client: sent command\n";

        if(write_output)
        {
            std::cout << "Client: writing output\n";

            memset(buffer, 0, strlen(buffer));
            read(sock, buffer, sizeof(buffer));
            int ret2;
            if(buffer[0])
            {
                buffer[0] = 'O';
                buffer[1] = 'K';

                write(sock, buffer, 2);

                memset(buffer, 0, strlen(buffer));

                ret2 = ioctl(sock, FIONREAD, &ret);

                while(ret >= 0)
                {
                    std::cout << "======== " << ret << ' ' << ret2 << '\n';

                    read(sock, buffer, sizeof(buffer));
                    
                    std::cout << buffer;

                    ret2 = ioctl(sock, FIONREAD, &ret);
                }
            }

            std::cout << "Client: Done writing output\n";
        }
    }

    snprintf(buffer, 256, "200 DONE");
    write(sock, buffer, strlen(buffer));

    close(sock);

    std::cout << "\n\nAll good! Exiting...\n";

    return 0;
}