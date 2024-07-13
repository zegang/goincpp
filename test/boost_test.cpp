// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#define BOOST_TEST_MODULE BoostTestModule
#include <boost/test/included/unit_test.hpp>

// BOOST_AUTO_TEST_CASE(test_case_1)
// {
//     BOOST_CHECK(false);  // This will fail
// }

BOOST_AUTO_TEST_CASE(test_case_2)
{
    double x = 0.1;
    double y = 0.1;
    BOOST_CHECK_CLOSE(x, y, 0.01);  // Check with a tolerance
}