#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <cstring>

#define PORT 2223

using namespace std;

bool stop;

int main(int argc, char ** argv)
{
    if(argc < 2)
    {
        std::cout << "\nUsage: \n./client ADDRESS [--output]\n     --output : Show commands output\n\n";

        return -1;
    }

    int sock;
    int ret;
    int host_ind, i, len;
    char buffer[53];
    bool write_output;
    struct sockaddr_in server_addr;
    struct hostent * server_name;
    struct sigaction int_handler;
    string prompt;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    write_output = false;
    stop = false;

    int_handler.sa_handler = [](int s){stop = true;};
    sigemptyset(&int_handler.sa_mask);
    int_handler.sa_flags = 0;

    sigaction(SIGINT, &int_handler, NULL);

    if(sock < 0)
    {
        cout << "ERROR: Socket could not be created\n";

        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;

    for(i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "--output") == 0)
            write_output = true;
        else
            host_ind = i;
    }

    server_name = gethostbyname(argv[host_ind]);

    if(server_name == NULL)
    {
        if(inet_pton(AF_INET, argv[host_ind], &server_addr.sin_addr) <= 0)
        {
            cout << "ERROR: invalid hostname or host address\n";

            exit(EXIT_FAILURE);
        }
    }
    else
    {
        memcpy(server_name->h_addr_list[0],
               &server_addr.sin_addr.s_addr,
               server_name->h_length);
    }
    
    ret = connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if(ret < 0)
    {
        cout << "ERROR: could not connect to server\n";

        exit(EXIT_FAILURE);
    }

    memset(buffer, 0, sizeof(buffer));
    
    buffer[0] = write_output;
    write(sock, buffer, 1);

    read(sock, buffer, sizeof(buffer));

    prompt = string(buffer) + "@" + string(argv[host_ind]) + " > ";

    while(!stop)
    {
        memset(buffer, 0, strlen(buffer));

        cout << prompt;
        cin.getline(buffer, sizeof(buffer));

        len = strlen(buffer);

        buffer[len] = char(10);
        //buffer[len + 1] = '\r';
        buffer[len + 1] = '\0';

        write(sock, buffer, strlen(buffer));
    }

    snprintf(buffer, 256, "200 DONE");
    write(sock, buffer, strlen(buffer));

    close(sock);

    std::cout << "\n\nAll good! Exiting...\n";

    return 0;
}