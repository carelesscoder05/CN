// auction_client.cpp
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <username>" << std::endl;
        return 1;
    }

    std::string username = argv[1];

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(12345);

    // Send join message
    std::string join_msg = "JOIN " + username;
    sendto(sock, join_msg.c_str(), join_msg.length(), 0, (const sockaddr*)&serv_addr, sizeof(serv_addr));

    std::cout << "Joined auction as " << username << ". Enter 'bid <item> <amount>' to bid, or 'quit' to exit." << std::endl;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_fd = std::max(sock, STDIN_FILENO) + 1;

        // No timeout, block until input or message
        int ready = select(max_fd, &readfds, NULL, NULL, NULL);

        if (ready > 0) {
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                // User input
                std::string input;
                std::getline(std::cin, input);
                if (input == "quit") {
                    break;
                } else if (input.substr(0, 4) == "bid ") {
                    // Format: bid <item> <amount>
                    std::string bid_msg = "BID " + input.substr(4);
                    sendto(sock, bid_msg.c_str(), bid_msg.length(), 0, (const sockaddr*)&serv_addr, sizeof(serv_addr));
                } else {
                    std::cout << "Invalid command. Use 'bid <item> <amount>' or 'quit'." << std::endl;
                }
            }

            if (FD_ISSET(sock, &readfds)) {
                // Message from server
                char buf[1024];
                struct sockaddr_in from_addr;
                socklen_t len = sizeof(from_addr);
                int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&from_addr, &len);
                if (n > 0) {
                    buf[n] = '\0';
                    std::cout << "Server: " << buf << std::endl;
                }
            }
        }
    }

    close(sock);
    return 0;
}
