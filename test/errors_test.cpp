// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#define BOOST_TEST_MODULE GoincppTestErrorsModule
#include <boost/test/included/unit_test.hpp>

#include "../src/errors/errors.hpp"

using namespace goincpp::errors;

BOOST_AUTO_TEST_CASE(test_error_new) {
    auto e = newError("test_error_new");
    BOOST_CHECK(e != nullptr);
    BOOST_CHECK_EQUAL(e->error(), "test_error_new");
}

class TestErrorString : public ErrorString {
public:
    TestErrorString(const std::string& msg) : ErrorString(msg) {}
};

BOOST_AUTO_TEST_CASE(test_error_new_subclass) {
    auto e = newError<TestErrorString>("test_error_new_subclass");
    BOOST_CHECK(e != nullptr);
    BOOST_CHECK_EQUAL(e->error(), "test_error_new_subclass");
}