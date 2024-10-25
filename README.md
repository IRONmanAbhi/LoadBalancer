# Load Balancer in C++

This C++ program is a basic load balancer designed to distribute incoming network traffic across multiple backend servers. It ensures optimal performance by conducting regular health checks and only forwarding requests to servers that are online.

## Features

- **Round-Robin Load Balancing**: Distributes client requests across available servers in a round-robin fashion.
- **Health Checks**: Performs initial and periodic health checks to monitor the availability of backend servers.
- **Threaded Request Handling**: Utilizes multithreading to handle multiple client requests simultaneously, ensuring efficient resource use.

## Prerequisites

- **C++ Compiler** (e.g., `g++`)
- **POSIX-compliant OS**: Required for socket programming and threading libraries.
- **Network Configuration**: Ensure all backend servers are reachable on the specified IP addresses and ports.

## Compilation

To compile the program, use the following command:

```bash
g++ load_balancer.cpp -o load_balancer
```

## Usage

Run the compiled load balancer program:

```bash
./load_balancer
```

The server listens for client connections on port **80** by default and forwards requests to available backend servers on **port 8080**.

## Code Structure

- **Main Server Setup**: Creates a socket to listen on the specified port for incoming client requests.
- **Health Check Thread**: Spawns a separate thread to periodically check the status of each backend server.
- **Request Forwarding**: Each client request is forwarded to a healthy backend server.

## Configuration

### Backend Servers

Replace these existing IP addresses with the IP address of your backend servers and specify the port number they are working on, defined in the `servers` vector in the following format:

```cpp
vector<sv> servers = {{"192.168.139.137", true}, {"192.168.139.132", true}, {"192.168.139.135", true}, {"192.168.139.200", true}};
```

- **IP Address**: Specify the IP address of each backend server.
- **Initial Health Status**: Set each server's initial health status to `true` if you assume it to be online at program start.

### Ports

- **Load Balancer Port**: Set in the `PORT` constant (default is **80**).
- **Target Server Port**: Set in the `TARGET_PORT` constant (default is **8080**).

## Implementation Details

- **`check_server`**: Checks if a specific server is reachable and updates its status.
- **`health_check`**: Periodically checks the health of each server every 120 seconds and prints any status changes.
- **`forward_request`**: Forwards client requests to an available server and returns the server's response.
- **`initial_health_check`**: Performs an initial check on all backend servers at program start.

## Contributing

If you have suggestions for improving this load balancer, please feel free to contribute by opening a pull request.
