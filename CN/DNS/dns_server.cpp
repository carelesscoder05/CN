// dns_server.cpp
#include <iostream>
#include <string>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

// A simple in-memory DNS database
map<string, string> create_dns_database() {
    map<string, string> dns_records;
    dns_records["www.example.com"] = "93.184.216.34";
    dns_records["www.google.com"] = "142.250.200.100";
    dns_records["www.github.com"] = "140.82.121.4";
    dns_records["localhost"] = "127.0.0.1";
    return dns_records;
}

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    map<string, string> dns_db = create_dns_database();

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Clear and set server address information
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY; // Listen on all available interfaces
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    cout << "DNS Server is listening on port " << PORT << "..." << endl;

    while (true) {
        socklen_t len = sizeof(cliaddr);
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        
        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        buffer[n] = '\0'; // Null-terminate the received data
        string hostname(buffer);
        cout << "Received query for: " << hostname << " from " << inet_ntoa(cliaddr.sin_addr) << endl;

        string response;
        if (dns_db.find(hostname) != dns_db.end()) {
            response = dns_db[hostname];
        } else {
            response = "Not Found";
        }
        
        // Send the response back to the client
        sendto(sockfd, response.c_str(), response.length(), 0, (const struct sockaddr *)&cliaddr, len);
        cout << "Sent response: " << response << endl << endl;
    }

    close(sockfd);
    return 0;
}