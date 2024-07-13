// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#define BOOST_TEST_MODULE GoincppTestContextModule
#include <boost/test/included/unit_test.hpp>

#include "../src/context/context.hpp"

using namespace goincpp::context;

BOOST_AUTO_TEST_CASE(test_background) {
    auto e = background();
    BOOST_CHECK(e != nullptr);
    BOOST_CHECK_EQUAL(e->done(), nullptr);
    auto b = std::dynamic_pointer_cast<BackgroundCtx>(e);
    BOOST_CHECK(b != nullptr);
    BOOST_CHECK_EQUAL(b->string(), "context.Background");
}

BOOST_AUTO_TEST_CASE(test_withCancel) {
    auto e = background();
    BOOST_CHECK(e != nullptr);
    auto [c, cancel] = withCancel(e);
    cancel();
    BOOST_CHECK(c->done() == closedChan);
}