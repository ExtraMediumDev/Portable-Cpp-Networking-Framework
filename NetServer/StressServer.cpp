// StressServer.cpp

#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include "olc_net.h"

enum class StressMsg : uint32_t
{
    Accept,
    Ping
};

class StressServer : public olc::net::server_interface<StressMsg>
{
public:
    StressServer(uint16_t port)
        : server_interface<StressMsg>(port)
    {
    }

protected:
    // Allow every incoming connection
    virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<StressMsg>> client) override
    {
        olc::net::message<StressMsg> msg;
        msg.header.id = StressMsg::Accept;   // ← you’ll need to add this enum value
        client->Send(msg);

        std::cout << "[SERVER] Client connected: ID="
            << client->GetID() << std::endl;
        return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<olc::net::connection<StressMsg>> client) override
    {
        std::cout << "[SERVER] Client disconnected: ID=" << client->GetID() << std::endl;
    }

    // Whenever we get a Ping, count it and echo it back immediately
    virtual void OnMessage(
        std::shared_ptr<olc::net::connection<StressMsg>> client,
        const olc::net::message<StressMsg>& msg
    ) override
    {
        if (msg.header.id == StressMsg::Ping)
        {
            messages_received_.fetch_add(1, std::memory_order_relaxed);
            client->Send(msg);
        }
    }

private:
    std::atomic<uint64_t> messages_received_{ 0 };

public:
    // Called periodically to print and reset stats
    void PrintStats()
    {
        uint64_t count = messages_received_.exchange(0, std::memory_order_relaxed);
        size_t clients = m_deqConnections.size();  // protected member of base
        std::cout << "[SERVER] Clients: " << clients
            << " | Msgs/sec: " << count
            << std::endl;
    }
};

int main(int argc, char* argv[])
{
    uint16_t port = 60000;
    if (argc == 2)
    {
        port = static_cast<uint16_t>(std::stoi(argv[1]));
    }
    else
    {
        std::cout << "Usage: StressServer [port]\n"
            << "Using default port " << port << std::endl;
    }

    StressServer server(port);
    if (!server.Start())
        return 1;

    auto last_print = std::chrono::high_resolution_clock::now();
    const auto print_interval = std::chrono::seconds(1);

    while (true)
    {
        server.Update(-1, true);

        auto now = std::chrono::high_resolution_clock::now();
        if (now - last_print >= print_interval)
        {
            server.PrintStats();
            last_print = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
