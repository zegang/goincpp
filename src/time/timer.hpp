// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_TIME_TIMER_HPP
#define GOINCPP_TIME_TIMER_HPP

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <functional>

namespace goincpp {
namespace time {

class Timer {
public:
    Timer() : _isRunning(false) {}

    // Start the timer with a specified duration
    void start(std::chrono::milliseconds duration, std::function<void()> callback) {
        _isRunning.store(true);
        _timerThread = std::thread([this, duration, callback]() {
            std::this_thread::sleep_for(duration);
            if (_isRunning.load()) {
                callback();
            }
        });
    }

    // Stop the timer
    void stop() {
        _isRunning.store(false);
        if (_timerThread.joinable()) {
            _timerThread.join();
        }
    }

    bool isRunning() const {
        return _isRunning.load();
    }

    ~Timer() {
        stop();
    }

private:
    std::atomic<bool> _isRunning; // Atomic flag to check if the timer is running
    std::thread _timerThread;     // Thread object for the timer
};

std::chrono::milliseconds
util(std::chrono::system_clock::time_point d)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        d - std::chrono::system_clock::now());
}

}
}

#endif // GOINCPP_TIME_TIMER_HPP