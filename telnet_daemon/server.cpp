#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#define DAEMON_NAME "custom_telnet"
#define PORT 2223

using namespace std;

int init_server()
{
    int sock;
    int ret;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
        return -1;

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    ret = bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if(ret < 0)
        return -1;

    listen(sock, 5);

    return sock;
}

int main()
{
    int sock, client;
    int len;
    char buffer[50];
    char * username;
    bool write_output, stop;
    struct sockaddr_in client_addr;
    pid_t pid, sid;

    pid = fork();

    if(pid < 0)
        exit(EXIT_FAILURE);

    if(pid > 0)
        exit(EXIT_SUCCESS);

    umask(0);

    sid = setsid();

    if(sid < 0)
        exit(EXIT_FAILURE);
    
    if((chdir("/")) < 0)
        exit(EXIT_FAILURE);

    //close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    sock = init_server();

    if(sock < 0)
        exit(EXIT_FAILURE);

    memset(&client_addr, 0, sizeof(client_addr));

    len = sizeof(client_addr);
    client = accept(sock, (struct sockaddr *) &client_addr, (socklen_t *) &len);

    if(client < 0)
        exit(EXIT_FAILURE);

    memset(buffer, 0, sizeof(buffer));
    read(client, buffer, sizeof(buffer));

    write_output = buffer[0];

    username = getenv("USER");
    write(client, username, strlen(username));

    stop = false;

    while(!stop)
    {
        memset(buffer, 0, strlen(buffer));
        read(client, buffer, sizeof(buffer));

        if(strstr(buffer, "200 OK") != NULL)
        {
            write(STDIN_FILENO, "*******", 7);
            stop = true;
            break;
        }
        else
        {
            write(STDIN_FILENO, buffer, sizeof(buffer));
        }
    }

    close(client);
    close(sock);

    exit(EXIT_SUCCESS);
}