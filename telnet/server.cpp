#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <csignal>

#define PORT 2223

bool stop; // Interrupt handler

std::string RunCommand(std::string command) // Run a command and display it's output
{
    FILE * stream;
    std::string output;

    char buffer[256];

    command.append(" 2>&1");
    stream = popen(command.c_str(), "r");

    if(stream)
    {
        while(!feof(stream))
            if(fgets(buffer, 256, stream) != NULL)
                output.append(buffer);
        pclose(stream);
    }

    return output;
}

int main()
{
    int sock; // Socket file descriptor
    int conn; // Connection file descriptor
    int conn_len; // Connection address length
    char buffer[100]; // R/W buffer
    std::string output = "";
    bool write_output;

    int resp; // Stores response from different functions

    struct sockaddr_in server_addr, client_addr; // Server and client address
    struct sigaction handler; // Interrupt handler

    char * user; // Username

    stop = false;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        std::cout << "ERROR: cannot create socket\n";

        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    resp = bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if(resp < 0)
    {
        std::cout << "ERROR: binding\n";

        return -1;
    }

    listen(sock, 5);

    memset(&client_addr, 0, sizeof(client_addr));

    conn_len = sizeof(client_addr);
    conn = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &conn_len);

    if(conn < 0)
    {
        std::cout << "ERROR: cannot accept connection\n";
        return -1;
    }

    handler.sa_handler = [](int s){ stop = true; } ;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;

    sigaction(SIGINT, &handler, NULL);

    read(conn, buffer, sizeof(buffer));
    write_output = buffer[0];

    user = getenv("USER");

    write(conn, user, strlen(user));

    while(!stop)
    {
        std::cout << "Server: receiving command\n";

        memset(buffer, 0, strlen(buffer));
        read(conn, buffer, sizeof(buffer));

        std::cout << buffer << '*' << strlen(buffer) << '*' << '\n';

        if(strcmp(buffer, "200 DONE") == 0)
        {
            stop = true;
            break;
        }

        std::cout << "Server: running command\n";

        output = RunCommand(std::string(buffer));

        if(write_output)
        {
            std::cout << "Server: writing output\n";

            memset(buffer, 0, strlen(buffer));
            buffer[0] = (output != "");
            write(conn, buffer, 1);

            read(conn, buffer, sizeof(buffer));

            write(conn, output.c_str(), output.size());

            std::cout << "Server: Done writing output\n";
        }
    }

    close(conn);
    close(sock);

    std::cout << "\n\nAll good! Exiting...\n";

    return 0;
}