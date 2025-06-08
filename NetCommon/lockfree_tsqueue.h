#pragma once
#pragma once

#include <atomic>
#include <memory>
#include <utility>

// A lock-free MPMC queue based on Michael & Scott (1996)
// Supports multiple producers and multiple consumers.
// Requires C++17 for std::atomic<>

namespace olc {
    namespace net {

        template<typename T>
        class lockfree_queue {
        private:
            struct Node {
                std::shared_ptr<T> data;
                std::atomic<Node*> next;
                Node() : next(nullptr) {}
                Node(const T& value) : data(std::make_shared<T>(value)), next(nullptr) {}
                Node(T&& value) : data(std::make_shared<T>(std::move(value))), next(nullptr) {}
            };

            // Head: points to dummy node; consumers advance head
            std::atomic<Node*> head;
            // Tail: points to last node; producers advance tail
            std::atomic<Node*> tail;

        public:
            lockfree_queue() {
                Node* dummy = new Node;
                head.store(dummy, std::memory_order_relaxed);
                tail.store(dummy, std::memory_order_relaxed);
            }

            ~lockfree_queue() {
                // Drain & delete remaining nodes
                while (pop());
                delete head.load();
            }

            // Enqueue a new item (copy)
            void push(const T& value) {
                Node* new_node = new Node(value);
                Node* prev_tail = tail.exchange(new_node, std::memory_order_acq_rel);
                prev_tail->next.store(new_node, std::memory_order_release);
            }

            // Enqueue a new item (move)
            void push(T&& value) {
                Node* new_node = new Node(std::move(value));
                Node* prev_tail = tail.exchange(new_node, std::memory_order_acq_rel);
                prev_tail->next.store(new_node, std::memory_order_release);
            }

            // Try to dequeue an item. Returns nullptr if empty.
            std::shared_ptr<T> pop() {
                Node* old_head = head.load(std::memory_order_acquire);
                Node* next = old_head->next.load(std::memory_order_acquire);
                if (!next) {
                    // queue is empty
                    return nullptr;
                }
                // Advance head
                if (head.compare_exchange_strong(old_head, next, std::memory_order_acq_rel)) {
                    std::shared_ptr<T> res = next->data;
                    delete old_head;
                    return res;
                }
                return nullptr;
            }

            std::shared_ptr<T> peek() const {
                // Head is a dummy; the real first item is head->next
                Node* h = head.load(std::memory_order_acquire);
                Node* nx = h->next.load(std::memory_order_acquire);
                return nx ? nx->data : nullptr;
            }

            // Non-blocking test for emptiness
            bool empty() const {
                Node* hd = head.load(std::memory_order_acquire);
                Node* nx = hd->next.load(std::memory_order_acquire);
                return (nx == nullptr);
            }
        };

    } // namespace net
} // namespace olc
