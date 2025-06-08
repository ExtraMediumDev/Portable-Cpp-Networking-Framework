// perf_client.cpp
#include "net_client.h"
#include "net_message.h"   // for olc::net::message<>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// Must match your net_message.h enum
enum class CustomMsgTypes : uint32_t {
    ServerAccept,
    ServerDeny,
    ServerPing,
    MessageAll,
    ServerMessage,
};

static std::atomic<uint64_t> sent_count{ 0 }, recv_count{ 0 };

class FloodClient : public olc::net::client_interface<CustomMsgTypes>
{
public:
    void Flood(int duration_sec)
    {
        if (!Connect("127.0.0.1", 60000)) {
            std::cerr << "[Client] Connect failed\n";
            return;
        }

        auto end_time = std::chrono::steady_clock::now()
            + std::chrono::seconds(duration_sec);

        while (std::chrono::steady_clock::now() < end_time) {
            // send a bare “ping”
            olc::net::message<CustomMsgTypes> msg;
            msg.header.id = CustomMsgTypes::ServerPing;
            Send(msg);
            sent_count.fetch_add(1, std::memory_order_relaxed);

            // immediately pull in all echoes
            auto& q = Incoming();
            while (!q.empty()) {
                q.pop_front();
                recv_count.fetch_add(1, std::memory_order_relaxed);
            }
        }

        Disconnect();
    }
};

int main(int argc, char** argv)
{
    int threads = (argc > 1 ? std::atoi(argv[1]) : 1);
    int duration_sec = (argc > 2 ? std::atoi(argv[2]) : 10);

    // Launch N flood threads
    std::vector<std::thread> workers;
    for (int i = 0; i < threads; i++) {
        workers.emplace_back([duration_sec]() {
            FloodClient c;
            c.Flood(duration_sec);
            });
    }

    // Print per-second send/recv rates
    for (int i = 0; i < duration_sec; i++) {
        std::this_thread::sleep_for(1s);
        auto s = sent_count.exchange(0);
        auto r = recv_count.exchange(0);
        std::cout << "[Client] Sent/s: " << s
            << "  Recv/s: " << r << "\n";
    }

    for (auto& t : workers) t.join();
    return 0;
}
