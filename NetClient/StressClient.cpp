#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include "olc_net.h"

// Message IDs, including an initial Accept for handshake
enum class StressMsg : uint32_t {
    Accept,
    Ping
};

// Client that can send timestamped Ping messages
class StressClient : public olc::net::client_interface<StressMsg> {
public:
    void PingMsg(uint64_t timestamp_us) {
        olc::net::message<StressMsg> msg;
        msg.header.id = StressMsg::Ping;
        msg << timestamp_us;
        Send(msg);
    }
};

// Per-thread task: connect, handshake, send pings, measure RTT
void ClientTask(const std::string& host,
    uint16_t port,
    int client_id,
    int messages_per_client)
{
    StressClient client;
    if (!client.Connect(host, port)) {
        std::cerr << "[CLIENT " << client_id << "] Connection failed\n";
        return;
    }
    // Wait until TCP/ASIO layers are up
    while (!client.IsConnected())
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // --- Handshake: wait for server Accept ---
    bool accepted = false;
    while (client.IsConnected() && !accepted) {
        if (!client.Incoming().empty()) {
            auto owned = client.Incoming().pop_front();
            auto& msg = owned.msg;
            if (msg.header.id == StressMsg::Accept) {
                accepted = true;
                std::cout << "[CLIENT " << client_id << "] Accepted by server\n";
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    if (!accepted) {
        std::cerr << "[CLIENT " << client_id
            << "] No Accept from server, aborting\n";
        return;
    }

    // --- Send all Ping messages as fast as possible ---
    for (int i = 0; i < messages_per_client; ++i) {
        uint64_t t0 = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
        client.PingMsg(t0);
    }

    // Give the server a moment to echo back
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // --- Collect and compute latencies ---
    std::vector<double> latencies;
    latencies.reserve(messages_per_client);
    while (!client.Incoming().empty()) {
        auto owned = client.Incoming().pop_front();
        auto& msg = owned.msg;
        if (msg.header.id == StressMsg::Ping && msg.body.size() >= sizeof(uint64_t)) {
            uint64_t t0;
            std::memcpy(&t0, msg.body.data(), sizeof(uint64_t));
            uint64_t t1 = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
            latencies.push_back((t1 - t0) / 1000.0); // ms
        }
    }

    int received = static_cast<int>(latencies.size());
    double sum = 0.0, max_lat = 0.0;
    for (double l : latencies) {
        sum += l;
        if (l > max_lat) max_lat = l;
    }
    double avg = received ? sum / received : 0.0;

    std::cout << "[CLIENT " << client_id << "] Sent: "
        << messages_per_client
        << "  Received: " << received
        << "  Avg RTT: " << avg << " ms"
        << "  Max RTT: " << max_lat << " ms\n";
}

int main(int argc, char* argv[])
{
    // Default parameters
    std::string host = "127.0.0.1";
    uint16_t    port = 60000;
    int         num_clients = 4;
    int         messages_per_client = 50000;

    // Parse overrides if provided
    if (argc == 5) {
        host = argv[1];
        port = static_cast<uint16_t>(std::stoi(argv[2]));
        num_clients = std::stoi(argv[3]);
        messages_per_client = std::stoi(argv[4]);
    }
    else {
        std::cout << "[StressClient] Using defaults: host=" << host
            << " port=" << port
            << " clients=" << num_clients
            << " messages=" << messages_per_client << "\n";
    }

    // Launch client threads
    std::vector<std::thread> threads;
    threads.reserve(num_clients);
    for (int i = 0; i < num_clients; ++i) {
        threads.emplace_back(ClientTask,
            host, port,
            i,
            messages_per_client);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    for (auto& t : threads) t.join();

    std::cout << "\nDone. Press Enter to exit...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return 0;
}