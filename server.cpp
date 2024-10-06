#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

using namespace std;

int main()
{
    int server, new_socket;
    struct sockaddr_in svr;
    int opt = 1;
    int addrlen = sizeof(svr);
    char buffer[1024] = {0};
    char *hello =
        "HTTP/1.1 200 OK\n"
        "Server: SimpleC++Server/1.0\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 127\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>"
        "<head><title>Welcome to localhost</title></head>"
        "<body><h1>Hello, World!</h1><p>This is a response from the server.</p></body>"
        "</html>";

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    svr.sin_family = AF_INET;
    svr.sin_addr.s_addr = INADDR_ANY;
    svr.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&svr, sizeof(svr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if ((new_socket = accept(server, (struct sockaddr *)&svr, (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    string connected_ip = inet_ntoa(svr.sin_addr);
    cout << connected_ip << endl;
    read(new_socket, buffer, 1024);
    cout << buffer << endl;
    send(new_socket, hello, strlen(hello), 0);
    cout << hello << endl;

    close(new_socket);
    shutdown(server, SHUT_RDWR);

    return 0;
}