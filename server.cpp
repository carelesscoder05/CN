#include <iostream>
#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cstring>
#include <sstream>
#include <iomanip>

struct Item {
    std::string name;
    double base_price;
};

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(12345);

    if (bind(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        close(sock);
        return 1;
    }

    std::vector<std::pair<struct sockaddr_in, std::string>> clients;
    std::vector<Item> items = {{"Painting", 100.0}, {"Vase", 50.0}};

    for (const auto& item : items) {
        std::cout << "Starting auction for " << item.name << " with base price " << item.base_price << std::endl;

        double current_price = item.base_price;
        std::string highest_bidder = "";
        struct timeval last_bid_time;
        gettimeofday(&last_bid_time, NULL); // Initialize to current time

        bool auction_ended = false;

        while (!auction_ended) {
            // Broadcast current auction status
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << current_price;
            std::string msg = "AUCTION " + item.name + " " + ss.str() + " " + highest_bidder;
            for (const auto& cl : clients) {
                sendto(sock, msg.c_str(), msg.length(), 0, (const sockaddr*)&cl.first, sizeof(cl.first));
            }

            // Wait for incoming messages with 1-second timeout
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);

            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int ready = select(sock + 1, &readfds, NULL, NULL, &timeout);

            if (ready > 0) {
                char buf[1024];
                struct sockaddr_in sender_addr;
                socklen_t len = sizeof(sender_addr);
                int n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (sockaddr*)&sender_addr, &len);
                if (n > 0) {
                    buf[n] = '\0';
                    std::string recv_msg(buf);

                    if (recv_msg.substr(0, 5) == "JOIN ") {
                        std::string username = recv_msg.substr(5);
                        bool found = false;
                        for (const auto& cl : clients) {
                            if (memcmp(&cl.first, &sender_addr, sizeof(sender_addr)) == 0) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            clients.push_back({sender_addr, username});
                            std::cout << username << " joined the auction" << std::endl;
                        }
                    } else if (recv_msg.substr(0, 4) == "BID ") {
                        size_t space1 = recv_msg.find(' ', 4);
                        if (space1 != std::string::npos) {
                            std::string bid_item = recv_msg.substr(4, space1 - 4);
                            if (bid_item == item.name) {
                                double bid_amount;
                                std::istringstream iss(recv_msg.substr(space1 + 1));
                                iss >> bid_amount;
                                std::string bidder = "";
                                for (const auto& cl : clients) {
                                    if (memcmp(&cl.first, &sender_addr, sizeof(sender_addr)) == 0) {
                                        bidder = cl.second;
                                        break;
                                    }
                                }
                                if (!bidder.empty() && bid_amount > current_price) {
                                    current_price = bid_amount;
                                    highest_bidder = bidder;
                                    gettimeofday(&last_bid_time, NULL);
                                    std::cout << bidder << " placed a bid of " << bid_amount << " on " << item.name << std::endl;
                                }
                            }
                        }
                    }
                }
            } else {
                // Timeout: check if auction should end
                struct timeval now;
                gettimeofday(&now, NULL);
                double elapsed = (now.tv_sec - last_bid_time.tv_sec) + (now.tv_usec - last_bid_time.tv_usec) / 1000000.0;
                if (elapsed > 30.0) {
                    if (!highest_bidder.empty()) {
                        std::stringstream ss_sell;
                        ss_sell << std::fixed << std::setprecision(2) << current_price;
                        std::string sell_msg = "SOLD " + item.name + " to " + highest_bidder + " for " + ss_sell.str();
                        for (const auto& cl : clients) {
                            sendto(sock, sell_msg.c_str(), sell_msg.length(), 0, (const sockaddr*)&cl.first, sizeof(cl.first));
                        }
                        std::cout << sell_msg << std::endl;
                    } else {
                        std::string nosale_msg = "NOSALE " + item.name;
                        for (const auto& cl : clients) {
                            sendto(sock, nosale_msg.c_str(), nosale_msg.length(), 0, (const sockaddr*)&cl.first, sizeof(cl.first));
                        }
                        std::cout << "No sale for " << item.name << std::endl;
                    }
                    auction_ended = true;
                }
            }
        }
    }

    close(sock);
    return 0;
}
