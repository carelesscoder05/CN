#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <mutex>
#include <condition_variable>

using namespace std;

// Shared data structures
mutex mtx;
condition_variable cv;
vector<int> client_sockets;
map<string, double> items = {{"Painting", 100.0}, {"Antique Vase", 500.0}, {"Rare Book", 250.0}};

string current_item_name;
double current_price = 0.0;
int current_winner_socket = -1;
bool auction_active = false;

// Time to wait for a new bid before closing the auction
const auto BID_GRACE_PERIOD = chrono::seconds(5);

void broadcast_message(const string& message) {
    lock_guard<mutex> guard(mtx);
    for (int sock : client_sockets) {
        send(sock, message.c_str(), message.length(), 0);
    }
}

void handle_client(int client_socket) {
    char buffer[1024] = {0};
    string client_id = to_string(client_socket);

    broadcast_message("User " + client_id + " has joined the auction.\n");

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_socket, buffer, 1024);
        if (valread <= 0) {
            // Client disconnected
            lock_guard<mutex> guard(mtx);
            for (auto it = client_sockets.begin(); it != client_sockets.end(); ++it) {
                if (*it == client_socket) {
                    client_sockets.erase(it);
                    break;
                }
            }
            cout << "Client " << client_id << " disconnected." << endl;
            broadcast_message("User " + client_id + " has left.\n");
            close(client_socket);
            return;
        }

        string bid_str(buffer);
        try {
            double new_bid = stod(bid_str);
            unique_lock<mutex> lock(mtx);
            if (auction_active && new_bid > current_price) {
                current_price = new_bid;
                current_winner_socket = client_socket;
                lock.unlock();
                string broadcast_msg = "New highest bid on " + current_item_name + ": $" + to_string(current_price) + " by user " + client_id + "\n";
                broadcast_message(broadcast_msg);
                cv.notify_one(); // Notify the main auction thread of a new bid
            } else {
                string error_msg = "Invalid bid. Your bid must be higher than $" + to_string(current_price) + "\n";
                send(client_socket, error_msg.c_str(), error_msg.length(), 0);
            }
        } catch (const invalid_argument& ia) {
            // Not a valid bid (e.g., "quit")
            if (bid_str == "quit") {
                // Handle client disconnect gracefully
                close(client_socket);
                return;
            }
        }
    }
}

void run_auction_for_item(const string& item_name, double starting_price) {
    unique_lock<mutex> lock(mtx);
    current_item_name = item_name;
    current_price = starting_price;
    current_winner_socket = -1;
    auction_active = true;
    lock.unlock();

    string start_msg = "--- Auction for " + item_name + " has started! Starting price: $" + to_string(starting_price) + " ---\n";
    broadcast_message(start_msg);

    bool new_bid_received = true;
    while (new_bid_received) {
        lock.lock();
        // Wait for a new bid to be placed, with a timeout
        if (cv.wait_for(lock, BID_GRACE_PERIOD) == cv_status::timeout) {
            new_bid_received = false;
        }
        lock.unlock();
    }
    
    lock.lock();
    auction_active = false;
    string end_msg;
    if (current_winner_socket != -1) {
        end_msg = "--- Auction for " + item_name + " has ended! Winner is user " + to_string(current_winner_socket) + " with a final bid of $" + to_string(current_price) + " ---\n";
    } else {
        end_msg = "--- Auction for " + item_name + " has ended! No bids received. ---\n";
    }
    lock.unlock();
    broadcast_message(end_msg);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int PORT = 8080;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Auction server listening on port " << PORT << endl;

    thread client_accepter([&]() {
        while (true) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }

            lock_guard<mutex> guard(mtx);
            client_sockets.push_back(new_socket);
            cout << "New client connected. Socket FD: " << new_socket << endl;

            thread(handle_client, new_socket).detach();
        }
    });

    for (auto const& [item_name, price] : items) {
        run_auction_for_item(item_name, price);
        this_thread::sleep_for(chrono::seconds(5)); // Pause between auctions
    }

    client_accepter.join();
    close(server_fd);

    return 0;
}