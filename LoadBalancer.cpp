#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <thread>
#include <mutex>

#define PORT 80
#define TARGET_PORT 8080
using namespace std;

struct sv
{
    string ip_address;
    bool isalive;
};

vector<sv> servers = {{"192.168.139.137", true}, {"192.168.139.132", true}, {"192.168.139.135", true}, {"192.168.139.200", true}};

mutex servers_mutex;

bool check_server(string &ip)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return false;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TARGET_PORT);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        if (errno == EINPROGRESS || errno == EWOULDBLOCK)
            return false;
        else
            return false;
        close(sockfd);
        return false;
    }
    close(sockfd);

    return true;
}

void health_check()
{
    while (true)
    {
        this_thread::sleep_for(chrono::seconds(120));
        for (auto server : servers)
        {
            bool is_alive = check_server(server.ip_address);
            if (is_alive != server.isalive)
            {
                server.isalive = is_alive;
                if (is_alive)
                    cout << "Server " << server.ip_address << " is back online." << endl;
                else
                    cout << "Server " << server.ip_address << " is down." << endl;
            }
        }
    }
}

void forward_request(int client_socket, sv server)
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

    int target_socket;
    struct sockaddr_in target_address;

    if ((target_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        send(client_socket, error_response, strlen(error_response), 0);
        close(client_socket);
    }
    if (setsockopt(target_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        send(client_socket, error_response, strlen(error_response), 0);
        close(client_socket);
        close(target_socket);
    }

    target_address.sin_family = AF_INET;
    target_address.sin_port = htons(TARGET_PORT);

    string TARGET_IP = server.ip_address;

    if (inet_pton(AF_INET, TARGET_IP.c_str(), &target_address.sin_addr) <= 0)
    {
        perror("Invalid Target Address");
        send(client_socket, error_response, strlen(error_response), 0);
        close(client_socket);
        close(target_socket);
    }

    if (connect(target_socket, (struct sockaddr *)&target_address, sizeof(target_address)) < 0)
    {
        perror("Connection to target server failed");
        send(client_socket, error_response, strlen(error_response), 0);
        close(target_socket);
        close(client_socket);
    }

    send(target_socket, buffer, bytes_received, 0);
    int bytes_received_from_target = read(target_socket, buffer, sizeof(buffer));
    send(client_socket, buffer, bytes_received_from_target, 0);

    close(target_socket);
    close(client_socket);
}

void initial_health_check()
{
    lock_guard<mutex> guard(servers_mutex); // Lock the servers list during update
    cout << "Performing initial health check on backend servers..." << endl;

    for (auto &server : servers)
    {
        server.isalive = check_server(server.ip_address);
        if (server.isalive)
        {
            cout << "Server " << server.ip_address << " is alive." << endl;
        }
        else
        {
            cout << "Server " << server.ip_address << " is down." << endl;
        }
    }

    cout << "Initial health check completed." << endl;
}

int main()
{
    int server, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

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
    initial_health_check();
    cout << "Server is listening on port " << PORT << endl;
    thread health_thread(health_check);
    health_thread.detach();

    int i = -1;
    while (true)
    {
        new_socket = accept(server, (struct sockaddr *)&address, (socklen_t *)&addrlen);

        sv selected_server;

        while (true)
        {
            i = (i + 1) % servers.size();
            if (servers[i].isalive)
            {
                selected_server = servers[i];
                break;
            }
        }

        thread([new_socket, selected_server]()
               { forward_request(new_socket, selected_server); })
            .detach();
    }

    return 0;
}