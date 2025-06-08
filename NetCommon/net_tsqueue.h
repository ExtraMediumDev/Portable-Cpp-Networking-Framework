#pragma once

#include "lockfree_tsqueue.h"
#include <condition_variable>
#include <mutex>

namespace olc {
    namespace net {

        template<typename T>
        class tsqueue {
        public:
            tsqueue() = default;
            tsqueue(const tsqueue&) = delete;
            ~tsqueue() = default;

            // --- Enqueue ------------------------------------

            void push_back(const T& item) {
                m_core.push(item);
                cv_.notify_one();
            }
            void push_back(T&& item) {
                m_core.push(std::move(item));
                cv_.notify_one();
            }

            // --- Inspect / pop ------------------------------

            // Block until there's at least one element, then return a reference.
            // We keep a thread_local shared_ptr alive so the data in the node
            // isn't freed until after the async_write buffer is copied.
            const T& front() {
                std::shared_ptr<T> sp;
                {
                    std::unique_lock<std::mutex> lk(mux_);
                    cv_.wait(lk, [&] { return (bool)(sp = m_core.peek()); });
                }
                // stash it so it lives until the next pop()
                thread_local std::shared_ptr<T> tl_sp;
                tl_sp = sp;
                return *tl_sp;
            }

            // Remove and return the front element
            T pop_front() {
                std::shared_ptr<T> sp;
                {
                    std::unique_lock<std::mutex> lk(mux_);
                    cv_.wait(lk, [&] { return (bool)(sp = m_core.peek()); });
                }
                // now pop it
                sp = m_core.pop();
                return std::move(*sp);
            }

            // Non‐blocking empty test
            bool empty() const {
                return !m_core.peek();
            }

            // Block until non‐empty
            void wait() {
                std::unique_lock<std::mutex> lk(mux_);
                cv_.wait(lk, [&] { return (bool)m_core.peek(); });
            }

        private:
            lockfree_queue<T>       m_core;  // the lock-free MPMC queue
            mutable std::mutex      mux_;
            std::condition_variable cv_;
        };

    }
} // namespace olc::net
