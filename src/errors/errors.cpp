// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "errors.hpp"
#include <concepts>

namespace goincpp {
namespace errors {

Error
newError(const std::string& message) {
    return std::make_shared<ErrorString>(message);
}

}
}