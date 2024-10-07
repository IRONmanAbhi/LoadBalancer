#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>

#define PORT 80
#define TARGET_PORT 8080
using namespace std;

vector<string> server_ip = {"192.168.137.128", "192.168.137.129", "192.168.137.131", "192.168.137.132"};

void forward_request(int client_socket, int server_index)
{
    int opt = 1;
    char buffer[4096] = {0};
    char *error_response =
        "HTTP/1.1 500 OK\n"
        "Server: SimpleC++Server/1.0\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 127\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html>"
        "<head><title>500 - ERROR</title></head>"
        "<body><h1>Internal server Error</h1><p>Sorry for in convinience</p></body>"
        "</html>";

    int bytes_received = read(client_socket, buffer, sizeof(buffer));
    cout << "Recieved Request: " << buffer << endl;

    int target_socket;
    struct sockaddr_in target_address;

    if ((target_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        send(client_socket, error_response, strlen(error_response), 0);
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    if (setsockopt(target_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        send(client_socket, error_response, strlen(error_response), 0);
        close(client_socket);
        close(target_socket);
        exit(EXIT_FAILURE);
    }

    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(TARGET_PORT);

    string TARGET_IP = server_ip[server_index];

    if (inet_pton(AF_INET, TARGET_IP.c_str(), &target_address.sin_addr) <= 0)
    {
        perror("Invalid Target Address");
        send(client_socket, error_response, strlen(error_response), 0);
        close(client_socket);
        close(target_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(target_socket, (struct sockaddr *)&target_address, sizeof(target_address)) < 0)
    {
        perror("Connection to target server failed");
        send(client_socket, error_response, strlen(error_response), 0);
        close(target_socket);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    send(target_socket, buffer, bytes_received, 0);
    int bytes_received_from_target = read(target_socket, buffer, sizeof(buffer));
    cout << "response from server: " << buffer << endl;
    send(client_socket, buffer, bytes_received_from_target, 0);

    close(target_socket);
    close(client_socket);
}

int main()
{
    int server, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating load Balancer as server for cline to send request
    if ((server = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server, 4) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server is listening on port " << PORT << endl;
    int i = -1;
    while (true)
    {
        new_socket = accept(server, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        string connected_ip = inet_ntoa(address.sin_addr);
        cout << "Connection accepted from :" << connected_ip << endl;
        i = (i + 1) % server_ip.size();
        thread([new_socket, i]()
               { forward_request(new_socket, i); })
            .detach();
    }

    return 0;
}