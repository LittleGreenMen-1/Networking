/*
    What if the 2 args from send and receive are the same
    e.g.
        > send 1.txt 1.txt
*/

#include <iostream>
#include <fstream>

#include <thread>
#include <cstring>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <time.h>
#include <signal.h>
#include <errno.h>

#include <vector>

#define PORT_FTP 2100

int stop;
int thread_number;
std::thread threads[5];
int empty[5];

std::string current_directory = "./";
std::vector< std::string > directories;

extern int errno;

int GetFolders();

int init_server()
{
    int sock;
    int errnum;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        std::cerr << "ERROR: sock()\n";
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_FTP);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        errnum = errno;
        std::cerr << "ERROR: bind() >> " << strerror(errnum) << '\n';
        return -1;
    }

    listen(sock, 5);

    return sock;
}

int login(char * username, char * password)
{
    std::ifstream database("db.txt");
    std::string line;
    std::string::iterator i;
    int usr_ind, pass_ind, check;
    int usr_len, pass_len;

    usr_len = strlen(username);
    pass_len = strlen(password);

    while(getline(database, line))
    {
        check = 1;
        i = line.begin();
        usr_ind = pass_ind = 0;

        while(*i != ' ' && check && i != line.end())
        {
            if(usr_ind >= usr_len)
                check = 0;
            else if(username[usr_ind++] != *i)
                check = 0;
            i++;
        }
        i++;

        while(i != line.end() && check)
        {
            if(pass_ind >= pass_len)
                check = 0;
            else if(password[pass_ind++] != *i)
                check = 0;
            i++;
        }

        if(usr_ind < usr_len || pass_ind < pass_len)
            check = 0;

        if(check)
        {
            database.close();
            return 1;
        }
    }

    database.close();
    return 0;
}

int send_file(int sock, char * file)
{
    std::string path_to_file = current_directory + std::string(file);
    std::ifstream to_send(path_to_file, std::ios::binary);
    
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
   
    int i;

    for(i = 0; i < 10; i++)
        buffer[i] = EOF;
    
    write(sock, buffer, strlen(buffer));

    to_send.close();
    return 1;
}

int receive_file(int sock, char * file)
{
    std::string path_to_file = current_directory + std::string(file);
    std::ofstream to_recv(path_to_file, std::ios::binary);
    
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

int change_dir(char * folder)
{
    bool found = false;
    int i, last_slash;

    for(i = 0; i < directories.size() && !found; i++)
    {
        if(strcmp(directories[i].c_str(), folder) == 0)
            found = true;
    }

    if(!found)
        return -1;

    if(strcmp(folder, ".") == 0)
        return 1;

    last_slash = 0;
    std::string aux;

    if(strcmp(folder, "..") == 0)
    {
        for(i = 0; i < current_directory.length() - 1; i++)
        {
            if(current_directory[i] == '/')
                last_slash = i;
        }

        if(last_slash == 0)
            return 0;

        for(i = 0; i < last_slash; i++)
            aux.push_back(current_directory[i]);

        current_directory.clear();
        current_directory = aux;

        if(current_directory == "." || current_directory == "./")
            return 0;

        current_directory += '/';

        return 1;
    }
    else
    {
        current_directory += std::string(folder);
        current_directory += "/";

        return 1;
    }

    return 0;
}

void handle_conn(int client, int thread)
{
    printf("Thread: %d\n", thread);
    char buffer[256], command[256];
    char argv[10][256];
    int argc;
    char username[30], password[30];
    int n, i, j, usr_ind, pass_ind;
    int succ_login, exit;

    succ_login = 0;
    exit = 0;

    memset(buffer, 0, sizeof(buffer));
    n = read(client, buffer, 255);

    // Read username & password;

    memset(username, 0, 30);
    memset(password, 0, 30);

    i = usr_ind = pass_ind = 0;

    while(buffer[i] != '/' && i < n && usr_ind < 30)
    {
        username[usr_ind++] = buffer[i];
        i++;
    }

    i++;

    for(i; i < n && pass_ind < 30; i++)
        password[pass_ind++] = buffer[i];

    // Auth

    if(!login(username, password))
    {
        close(client);
        thread_number--;
        empty[thread] = 0;
        threads[thread].~thread();

        memset(buffer, 0, strlen(buffer));
        snprintf(buffer, 255, "80 Auth_err");

        write(client, buffer, strlen(buffer));

        std::cout << "Auth error\n";

        return ;
    }

    memset(buffer, 0, strlen(buffer));
    snprintf(buffer, 255, "200 OK");

    write(client, buffer, strlen(buffer));

    std::cout << "Auth successful\n";

    current_directory += std::string(username) + std::string("/");

    GetFolders();

    while(!exit)
    {
        std::cout << "=== " << current_directory << " ===\n";
        // Navigate trough files

        memset(buffer, 0, strlen(buffer));
        n = read(client, buffer, 255);

        std::cout << "Buff : " << buffer << '\n';

        j = 0;
        for(i = 0; i < n && buffer[i] != ' '; i++, j++)
            command[j] = tolower(buffer[i]);
        i++;

        j = 0;
        argc = 0;

        for(int ind = 0; ind < 10; ind++)
            memset(argv[ind], 0, strlen(argv[ind]));

        for(; i < n; i++)
        {
            if(buffer[i] == ' ')
            {
                argc++;
                j = 0;
            }
            else
            {
                argv[argc][j] = buffer[i];
                j++;
            }
        }
        argc++;

        std::cout << "command : " << command << '\n';
        
        if(strcmp(command, "exit") == 0)
        {
            exit = true;
            break;
        }

        if(strlen(command) <= 0)
            // Invalid transmission
            break;

        // Send & receive files

        if(strcmp(command, "send") == 0)
        {
            if(argc < 2)
                break;
            receive_file(client, argv[1]);
        }

        else if(strcmp(command, "fetch") == 0)
        {
            if(argc < 2)
                break;
            send_file(client, argv[0]);
        }

        else if(strcmp(command, "ls") == 0)
        {
            std::cout << "ls\n";
            memset(buffer, 0, strlen(buffer));
            buffer[0] = '\n';

            for(std::string file : directories)
            {
                std::cout << file << '\n';
                write(client, file.c_str(), strlen(file.c_str()));
                
                write(client, buffer, 1);
            }
            
            memset(buffer, 0, strlen(buffer));
            memset(buffer, EOF, 5);
            write(client, buffer, strlen(buffer));

            std::cout << "DONE\n";
        }

        else if(strcmp(command, "cd") == 0)
        {
            if(argc < 1)
                break;

            std::string aux = current_directory;
            
            std::cout << argv[0] << '\n';
            n = change_dir(argv[0]);

            memset(buffer, 0, strlen(buffer));

            if(n < 0)
            {
                // Directory inexistent

                snprintf(buffer, 255, "31 Illegal operation");
                write(client, buffer, strlen(buffer));
            }
            if(n == 0)
            {
                // Illegal operation

                snprintf(buffer, 255, "30 Illegal operation");
                write(client, buffer, strlen(buffer));
            }
            else
            {
                write(client, current_directory.c_str(), strlen(current_directory.c_str()));

                read(client, buffer, 255);

                if(strcmp(buffer, "200 OK") == 0)
                    GetFolders();
                else
                    current_directory = aux;
            }
        }
    }

    // Close everything and exit

    memset(buffer, 0, strlen(buffer));
    snprintf(buffer, 255, "500 OK");
    write(client, buffer, strlen(buffer));

    close(client);

    std::cout << "Close connection\n";

    thread_number--;
    empty[thread] = 0;
    threads[thread].~thread();

    return ;
}

int GetFolders()
{
    DIR* dir;
    struct dirent * directory_ptr;

    directories.clear();

    dir = opendir(current_directory.c_str());

    if(!dir)
    {
        std::cout << "ERROR: reading directories\n";
        stop = 1;

        return -1;
    }

    while((directory_ptr = readdir(dir)) != nullptr)
        directories.push_back(std::string(directory_ptr->d_name));

    closedir(dir);

    std::sort(directories.begin(), directories.end(), 
        [](std::string a, std::string b){
            int i;

            for(i = 0; i < a.length(); i++)
                a[i] = tolower(a[i]);

            for(i = 0; i < b.length(); i++)
                b[i] = tolower(b[i]);
            
            return a < b;
        }
    );

    return 0;
}

int main()
{
    int sock, new_client;
    struct sockaddr_in new_client_addr;
    struct sigaction int_handler;
    int len, i;

    stop = 0;
    thread_number = 0;
    new_client = -1;
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

    memset(threads, 0, sizeof(threads));
    memset(empty, 0, 5);

    GetFolders();

    while (!stop)
    {
        memset((char *)&new_client_addr, 0, sizeof(new_client_addr));
        len = sizeof(new_client_addr);
        new_client = accept(sock, (struct sockaddr *)&new_client_addr, (socklen_t *)&len);

        if (new_client < 0)
        {
            printf("ERROR: accept()\n");

            // return -1;
        }
        else if (thread_number < 5)
        {
            // threads[thread_number++] = std::thread(handle_conn, new_client, thread_number);

            for (i = 0; i < 5; i++)
            {
                if (empty[i] == 0)
                {
                    empty[i] = new_client;
                    thread_number++;
                    printf("< new_client = %d >\n", new_client);
                    threads[i] = std::thread(handle_conn, new_client, thread_number);
                    i = 5;
                }
            }
        }
    }

    std::cout << "\nAll good\n";

    close(sock);

    if (new_client >= 0)
        close(new_client);

    for (i = 0; i < 5; i++)
    {
        if (!empty[i])
            close(empty[i]);
    }

    return 0;
}