Theory: 
Of course. Here is a detailed explanation of the theory and data structures needed to solve these socket programming problems in C++.

-----

### 1\. Auction Marketplace

[cite\_start]This task requires creating a multi-client chat-like application where a central server manages the state of an auction, and multiple clients can connect to place bids[cite: 2, 3, 4].

#### **Theory**

The best model for this is a **centralized client-server architecture** using the **TCP protocol**.

  * **Server (The Auctioneer):** A single server program will act as the auctioneer. It will manage the items, keep track of the current highest bid, and communicate with all the bidders.
  * **Clients (The Bidders):** Multiple client programs will connect to the server. Users will interact with these clients to place their bids.
  * **TCP Protocol:** This is crucial for an auction. TCP guarantees that messages (bids) are delivered **reliably and in order**. Using UDP would be a disaster, as bids could be lost or arrive out of order, making a fair auction impossible.
  * **Handling Multiple Clients:** The server needs to handle many bidders at once. A common and straightforward approach in C++ is to use **multithreading**. The main server thread will listen for new connections. When a new client connects, the server spawns a new dedicated thread to handle all communication with that specific client.
  * **State Management & Broadcasting:** The server is the single source of truth. When a client places a valid (higher) bid, the server updates its state (the new highest bid and bidder). It must then **broadcast** this new information to *all* other connected clients so everyone knows the current price.

#### **Data Structures & Implementation Details**

**On the Server:**

  * **`struct Item`**: To hold information about the item being auctioned.
    ```cpp
    struct Item {
        std::string name;
        double basePrice;
        double currentHighestBid;
        int highestBidderSocket; // Socket ID of the highest bidder
    };
    ```
  * **`struct ClientInfo`**: To manage connected clients.
    ```cpp
    struct ClientInfo {
        int socket_fd; // The client's socket file descriptor
        std::string username;
        // Other client-specific data
    };
    ```
  * **`std::vector<ClientInfo> clients`**: A dynamic array to store information for all currently connected clients. This is essential for broadcasting messages.
  * **`std::mutex`**: A mutex is **absolutely critical** for thread safety. When multiple clients bid at nearly the same time, their dedicated threads will try to modify the `Item`'s `currentHighestBid`. A mutex ensures that only one thread can modify this shared data at a time, preventing race conditions.

**Communication Protocol:**
You need to define a simple, text-based protocol. For example:

  * Client sends: `BID 550.0`
  * Server broadcasts: `NEW_HIGH_BID 550.0 Bhaswati`
  * Server broadcasts: `AUCTION_WINNER Bhaswati 550.0`

-----

### 2\. Simple DNS Protocol

[cite\_start]This task involves implementing a client application that can perform DNS lookups: converting a hostname to an IP address and vice-versa[cite: 5, 7]. You don't build a full DNS server; you use your operating system's built-in DNS resolver libraries.

#### **Theory**

DNS (Domain Name System) is the internet's phonebook. The core task is to resolve names to numbers and numbers back to names.

  * **Forward DNS Lookup (`gethostbyname`)**: This is the most common lookup. [cite\_start]You provide a hostname like `www.google.com`, and the system returns one or more corresponding IP addresses like `142.251.42.132`[cite: 8].
  * **Reverse DNS Lookup (`gethostbyaddr`)**: This is the opposite. [cite\_start]You provide an IP address like `8.8.8.8`, and the system returns the associated hostname, such as `dns.google`[cite: 9].
  * **Resolver Functions**: Your C++ program acts as a client to the OS's DNS resolver. You make simple function calls, and the OS handles the complex process of caching and sending actual DNS packets over the network.

#### **Data Structures & Implementation Details**

The primary data structure for this task is provided by the system library `<netdb.h>`.

  * **`struct hostent`**: This structure is used to hold the results of a DNS query.
    ```c
    struct hostent {
        char  *h_name;        // Official name of the host.
        char **h_aliases;     // A list of alternative names (aliases).
        int    h_addrtype;    // Address family (e.g., AF_INET for IPv4).
        int    h_length;      // Length of the address in bytes.
        char **h_addr_list;   // A list of IP addresses for the host.
    };
    ```

**Implementation Steps:**

1.  **For `gethostbyname()`**:
      * Take a hostname string as input.
      * Call `gethostbyname()` with this string.
      * If the result is not `NULL`, loop through the `h_addr_list`.
      * Each entry in `h_addr_list` is a pointer to the address in network byte order. Use the `inet_ntoa()` function to convert this binary address into a human-readable dotted-decimal string for printing.
2.  **For `gethostbyaddr()`**:
      * Take an IP address string as input.
      * Convert the string into a binary network address using `inet_addr()` or `inet_pton()`.
      * Call `gethostbyaddr()` with the binary address.
      * If the result is not `NULL`, you can print the official hostname from the `h_name` field.

-----

### 3\. Time Server with RTT Calculation

[cite\_start]This task is to create a server that provides the current time and a client that calculates the **Round-Trip Time (RTT)** of a data packet to that server[cite: 6].

#### **Theory**

  * **RTT (Round-Trip Time)**: This is the total duration it takes for a signal to travel from a sender to a receiver and for the response to travel back to the sender. It's a key metric for measuring network latency.
  * **UDP Protocol**: For a simple request-response service like a time server, UDP is a good choice. It's lightweight and connectionless. Unlike the auction, if a single time request is lost, the client can simply send another one without much consequence.
  * **Measuring RTT**: The client is responsible for the measurement.
    1.  The client records a high-precision **start time (T1)**.
    2.  It sends a packet to the server.
    3.  The server receives the packet and immediately sends a reply containing its current time.
    4.  The client receives the reply and records a high-precision **end time (T2)**.
    5.  The RTT is calculated as **RTT = T2 - T1**.
  * **High-Resolution Clock**: Using standard `time()` is not sufficient because it only has a 1-second resolution. For measuring network latency, which is often in milliseconds or microseconds, you **must** use a high-resolution clock like `clock_gettime()` with `CLOCK_MONOTONIC`.

#### **Data Structures & Implementation Details**

The most important data structure is the one used for high-resolution time.

  * **`struct timespec`**: Defined in `<ctime>`, this structure holds time with nanosecond precision.
    ```c
    struct timespec {
        time_t   tv_sec;        // seconds
        long     tv_nsec;       // nanoseconds
    };
    ```

**Implementation Steps:**

1.  **Server:**
      * Create a UDP socket and bind it to a port.
      * Enter a loop, waiting for a packet with `recvfrom()`.
      * When a packet arrives, get the current time using `clock_gettime(CLOCK_REALTIME, &time_struct)`.
      * Send the `timespec` structure back to the client using `sendto()`.
2.  **Client:**
      * Create a UDP socket.
      * Record the start time: `clock_gettime(CLOCK_MONOTONIC, &start_time)`.
      * Send a dummy request packet to the server.
      * Wait for the server's reply with `recvfrom()`.
      * Immediately record the end time: `clock_gettime(CLOCK_MONOTONIC, &end_time)`.
      * Calculate the difference between the `end_time` and `start_time` structures. This involves subtracting the `tv_sec` and `tv_nsec` fields and handling borrowing if needed.
      * Convert the resulting seconds and nanoseconds into a single, user-friendly unit like milliseconds (`ms`) for printing.
