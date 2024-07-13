// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_RUNTIME_CHAN_HPP
#define GOINCPP_RUNTIME_CHAN_HPP

#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <any>

namespace goincpp {
namespace runtime {

template <typename T, int Capacity>
class Channel : public std::enable_shared_from_this< Channel<T, Capacity> > {
public:
    static std::shared_ptr<Channel<T, Capacity>> make() {
        auto cha = std::make_shared<Channel<T, Capacity>>();
        return cha;
    }

    void send() {
        static_assert(Capacity == 0);
        do {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_closed) {
                return;
            }
            _queue.push(0);
            _cond_sent.notify_all();
        } while(0);
        std::unique_lock<std::mutex> lock(_mutex);
        _cond_received.wait(lock, [this]() { return _queue.empty() || _closed; });
    }

    void send(const T& message) {
        static_assert(Capacity > 0);
        do {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_closed) {
                return;
            }
            _queue.push(message);
            _cond_sent.notify_all();
        } while(0);
    }

    bool select() {
        static_assert(Capacity == 0);
        do {
            std::unique_lock<std::mutex> lock(_mutex);
            if (_closed) {
                return true;
            }
            _cond_sent.wait_for(lock, std::chrono::seconds(0),
                [this]() { return !_queue.empty() || _closed;});
            if (!_queue.empty()) {
                _queue.pop();
                _cond_received.notify_all();
            }
        } while(0);

        return true;
    }

    bool receive() {
        static_assert(Capacity == 0);
        do {
            std::unique_lock<std::mutex> lock(_mutex);
            _cond_sent.wait(lock, [this]() { return !_queue.empty() || _closed; });
            if (_closed) {
                return true;
            }
            if (!_queue.empty()) {
                _queue.pop();
                _cond_received.notify_all();
            } else {
                return false;
            }
        } while(0);

        return true;
    }

    bool receive(T& message) {
        static_assert(Capacity > 0);
        do {
            std::unique_lock<std::mutex> lock(_mutex);
            _cond_sent.wait(lock, [this]() { return !_queue.empty() || _closed; });
            if (_closed) {
                return true;
            }
            if (!_queue.empty()) {
                message = _queue.front();
                _queue.pop();
            } else {
                return false;
            }
        } while(0);

        return true;
    }

    void close() {
        std::lock_guard<std::mutex> lock(_mutex);
        _closed = true;
        _cond_sent.notify_all();
        _cond_received.notify_all();
    }

    auto operator<<(T value) {
        send(value);
        return this->shared_from_this();
    }

    auto operator>>(T& value) {
        return receive(value);
    }

private:
    std::queue<T> _queue;
    std::mutex _mutex;
    std::condition_variable _cond_sent;
    std::condition_variable _cond_received;
    bool _closed = false;
};

using UnbufferedChannel = Channel<std::any, 0>;


template <typename T, int Capacity>
auto operator<<(std::shared_ptr<Channel<T, Capacity>> channel, T value) {
    channel->send(value);
    return channel;
}

template <typename T, int Capacity>
auto operator>>(std::shared_ptr<Channel<T, Capacity>> channel, T& value) {
    return channel->receive(value);
}

}
}

#endif // GOINCPP_RUNTIME_CHAN_HPP