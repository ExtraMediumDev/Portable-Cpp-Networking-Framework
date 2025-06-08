// perf_server.cpp
#include "net_server.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::chrono_literals;

// Must match your net_message.h enum
enum class CustomMsgTypes : uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

// Global counter of how many messages we’ve echoed
static std::atomic<uint64_t> g_msg_count{ 0 };

class PerfServer : public olc::net::server_interface<CustomMsgTypes>
{
public:
    PerfServer(uint16_t port)
        : server_interface<CustomMsgTypes>(port)
    {
    }

    // Expose number of active connections
    size_t GetConnectionCount() const {
        return m_deqConnections.size();
    }

protected:
    // Accept every client
    bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) override {
        return true;
    }

    void OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) override {
        std::cout << "[Server] Client " << client->GetID() << " disconnected\n";
    }

    void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client,
        olc::net::message<CustomMsgTypes>& msg) override
    {
        // Simply echo it back immediately
        MessageClient(client, msg);
        g_msg_count.fetch_add(1, std::memory_order_relaxed);
    }
};

int main()
{
    PerfServer server(60000);
    if (!server.Start()) {
        std::cerr << "[Server] Failed to start\n";
        return 1;
    }

    // Stats thread: print every second
    std::thread stats([&]() {
        while (true) {
            auto before = g_msg_count.load();
            std::this_thread::sleep_for(1s);
            auto after = g_msg_count.load();
            std::cout
                << "[Stats] Msg/s: " << (after - before)
                << "  Connections: " << server.GetConnectionCount()
                << "\n";
        }
        });

    // Main loop: process all incoming messages
    while (true) {
        server.Update();                // handle all queued messages
        std::this_thread::sleep_for(1ms);
    }

    stats.join();
    return 0;
}
