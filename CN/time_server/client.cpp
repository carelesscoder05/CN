#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <cstring>

// Shared configuration with the server
namespace TimeServerConfig {
    const int PORT = 43454;
    const char* IP_STR = "127.0.0.1";
}

using namespace std;

int main() {
    int sfd;
    struct sockaddr_in servaddr;
    socklen_t serverlen = sizeof(servaddr);

    // timespec structs for high-resolution timestamps
    struct timespec start_time, end_time, server_time;

    // 1. Create a UDP socket
    sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sfd == -1) {
        cerr << "Error: Could not open a socket" << endl;
        return 1;
    }

    // 2. Set up server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(TimeServerConfig::IP_STR);
    servaddr.sin_port = htons(TimeServerConfig::PORT);

    // 3. Record start time using a monotonic clock
    // CLOCK_MONOTONIC is essential for measuring time intervals
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // 4. Send a dummy request packet to the server
    const char* request = "GET TIME";
    sendto(sfd, request, strlen(request), 0, (struct sockaddr *)&servaddr, serverlen);
    cout << "Request sent to server. Waiting for reply..." << endl;


    // 5. Wait for the server's response
    recvfrom(sfd, &server_time, sizeof(server_time), 0, (struct sockaddr *)&servaddr, &serverlen);

    // 6. Record end time as soon as the reply is received
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // 7. Calculate RTT
    double rtt_s = (end_time.tv_sec - start_time.tv_sec);
    double rtt_ns = (end_time.tv_nsec - start_time.tv_nsec);
    
    // Convert the entire RTT into milliseconds
    double rtt_ms = (rtt_s * 1000.0) + (rtt_ns / 1000000.0);

    // 8. Print the result
    cout << "----------------------------------------" << endl;
    cout << "Reply received from server." << endl;
    cout.precision(4); // Set precision for floating point output
    cout << fixed << "Round-Trip Time (RTT): " << rtt_ms << " ms" << endl;
    cout << "----------------------------------------" << endl;


    close(sfd);
    return 0;
}