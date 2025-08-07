#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

using namespace std;

void receive_messages(int sock) {
    char buffer[1024] = {0};
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(sock, buffer, 1024);
        if (valread <= 0) {
            cout << "Server disconnected. Exiting." << endl;
            close(sock);
            exit(0);
        }
        cout << buffer;
        cout.flush(); // Ensure output is immediately displayed
    }
}

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    int PORT = 8080;
    const char* SERVER_IP = "127.0.0.1"; // Localhost

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "\n Socket creation error \n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "\nConnection Failed \n";
        return -1;
    }

    cout << "Connected to the auction server. Enter your bids (e.g., '150.50') or 'quit' to exit.\n";

    // Start a thread to continuously receive messages from the server
    thread receiver_thread(receive_messages, sock);
    receiver_thread.detach(); // Detach the thread to run independently

    string line;
    while (getline(cin, line)) {
        if (line == "quit") {
            send(sock, "quit", 4, 0);
            break;
        }
        send(sock, line.c_str(), line.length(), 0);
    }

    close(sock);
    return 0;
}