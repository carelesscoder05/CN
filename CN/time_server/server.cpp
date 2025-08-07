#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <cstring>

// Use a user-defined namespace to avoid polluting the global scope
namespace TimeServerConfig {
    const int PORT = 43454;
    const char* IP_STR = "127.0.0.1";
}

using namespace std;

int main() {
    int sfd;
    struct timespec server_time; // Use timespec for high resolution
    struct sockaddr_in servaddr, clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    char buffer[16]; // A small buffer for the request

    // 1. Create a UDP socket
    sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sfd == -1) {
        cerr << "Error: Could not open a socket" << endl;
        return 1;
    }

    // 2. Set up server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TimeServerConfig::PORT);

    // 3. Bind the socket to the server address
    if (bind(sfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        cerr << "Error: Could not bind socket" << endl;
        close(sfd);
        return 2;
    }

    cout << "Server is running on port " << TimeServerConfig::PORT << endl;

    // 4. Main server loop
    while (true) {
        // Wait for a request from a client
        recvfrom(sfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientaddr, &clientlen);

        // Get current wall-clock time with high precision
        clock_gettime(CLOCK_REALTIME, &server_time);

        cout << "Request from " << inet_ntoa(clientaddr.sin_addr) << ":" << ntohs(clientaddr.sin_port)
             << ". Sending back current time." << endl;

        // Send the high-resolution time back to the client
        sendto(sfd, &server_time, sizeof(server_time), 0, (struct sockaddr *)&clientaddr, clientlen);
    }

    close(sfd);
    return 0;
}