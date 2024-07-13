// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#define BOOST_TEST_MODULE GoincppTestRuntimeModule
#include <boost/test/included/unit_test.hpp>

#include "../src/runtime/chan.hpp"
#include <thread>

using namespace goincpp::runtime;

BOOST_AUTO_TEST_CASE(test_UnbufferedChannel_r_s) {
    auto ch = UnbufferedChannel::make();

    std::thread thd_receiver([ch]() {
        BOOST_TEST_MESSAGE("thd_receiver starting.");
        BOOST_CHECK_EQUAL(ch->receive(), true);
        BOOST_TEST_MESSAGE("thd_receiver finished.");
    });

    BOOST_CHECK_EQUAL(ch.use_count(), 2);

    std::thread thd_sender([ch]() {
        BOOST_TEST_MESSAGE("thd_sender starting.");
        ch->send();
        BOOST_TEST_MESSAGE("thd_sender finished.");
    });

    thd_sender.join();
    thd_receiver.join();
}

BOOST_AUTO_TEST_CASE(test_UnbufferedChannel_s_r) {
    auto ch = UnbufferedChannel::make();

    std::thread thd_sender([ch]() {
        BOOST_TEST_MESSAGE("thd_sender starting.");
        ch->send();
        BOOST_TEST_MESSAGE("thd_sender finished.");
    });

    BOOST_CHECK_EQUAL(ch.use_count(), 2);

    std::thread thd_receiver([ch]() {
        BOOST_TEST_MESSAGE("thd_receiver starting.");
        BOOST_CHECK_EQUAL(ch->receive(), true);
        BOOST_TEST_MESSAGE("thd_receiver finished.");
    });

    thd_sender.join();
    thd_receiver.join();
}

BOOST_AUTO_TEST_CASE(test_UnbufferedChannel_s_c) {
    auto ch = UnbufferedChannel::make();

    std::thread thd_sender([ch]() {
        BOOST_TEST_MESSAGE("thd_sender starting.");
        auto start = std::chrono::steady_clock::now();
        ch->send();
        auto end = std::chrono::steady_clock::now();
        BOOST_TEST_MESSAGE("thd_sender finished.");

        std::chrono::duration<double> elapsed_seconds = end - start;
        BOOST_CHECK(elapsed_seconds > std::chrono::seconds(5));
    });

    BOOST_CHECK_EQUAL(ch.use_count(), 2);

    std::thread thd_closer([ch]() {
        BOOST_TEST_MESSAGE("thd_closer starting.");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        ch->close();
        BOOST_TEST_MESSAGE("thd_closer finished.");
    });

    thd_sender.join();
    thd_closer.join();
}

BOOST_AUTO_TEST_CASE(test_UnbufferedChannel_r_c) {
    auto ch = UnbufferedChannel::make();

    std::thread thd_receiver([ch]() {
        BOOST_TEST_MESSAGE("thd_receiver starting.");
        auto start = std::chrono::steady_clock::now();
        BOOST_CHECK_EQUAL(ch->receive(), true);
        auto end = std::chrono::steady_clock::now();
        BOOST_TEST_MESSAGE("thd_receiver finished.");

        std::chrono::duration<double> elapsed_seconds = end - start;
        BOOST_CHECK(elapsed_seconds > std::chrono::seconds(5));
    });

    BOOST_CHECK_EQUAL(ch.use_count(), 2);

    std::thread thd_closer([ch]() {
        BOOST_TEST_MESSAGE("thd_closer starting.");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        ch->close();
        BOOST_TEST_MESSAGE("thd_closer finished.");
    });

    thd_receiver.join();
    thd_closer.join();
}

BOOST_AUTO_TEST_CASE(test_int_one_r_s) {
    auto ch = goincpp::runtime::Channel<int, 1>::make();

    std::thread thd_receiver([ch]() {
        int value;
        BOOST_TEST_MESSAGE("thd_receiver starting.");
        BOOST_CHECK_EQUAL(ch >> value, true);
        BOOST_CHECK_EQUAL(value, 5);
        std::cout << "thd_receiver finished. Received << " << value << std::endl;
    });

    BOOST_CHECK_EQUAL(ch.use_count(), 2);

    std::thread thd_sender([ch]() {
        int value = 5;
        std::cout << "thd_sender starting. sending << " << value << std::endl;
        ch << value;
        BOOST_TEST_MESSAGE("thd_sender finished.");
    });

    thd_sender.join();
    thd_receiver.join();
}

BOOST_AUTO_TEST_CASE(test_int_one_r_c) {
    auto ch = goincpp::runtime::Channel<int, 1>::make();

    std::thread thd_receiver([ch]() {
        int value;
        BOOST_TEST_MESSAGE("thd_receiver starting.");
        auto start = std::chrono::steady_clock::now();
        BOOST_CHECK_EQUAL(ch >> value, true);
        auto end = std::chrono::steady_clock::now();
        BOOST_TEST_MESSAGE("thd_receiver starting.");

        std::chrono::duration<double> elapsed_seconds = end - start;
        BOOST_CHECK(elapsed_seconds > std::chrono::seconds(5));
    });

    BOOST_CHECK_EQUAL(ch.use_count(), 2);

    std::thread thd_closer([ch]() {
        BOOST_TEST_MESSAGE("thd_closer starting.");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        ch->close();
        BOOST_TEST_MESSAGE("thd_closer finished.");
    });

    thd_receiver.join();
    thd_closer.join();
}

BOOST_AUTO_TEST_CASE(test_int_one_c_r) {
    auto ch = goincpp::runtime::Channel<int, 1>::make();

    BOOST_TEST_MESSAGE("thd_closer starting.");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    ch->close();
    BOOST_TEST_MESSAGE("thd_closer finished.");

    int value;
    BOOST_TEST_MESSAGE("thd_receiver starting.");
    BOOST_CHECK_EQUAL(ch >> value, true);
    BOOST_TEST_MESSAGE("thd_receiver starting.");
}