// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include "errors.hpp"
#include "reflect/type.hpp"

namespace goincpp {
namespace errors {

Error
newError(const std::string& message) {
    return std::make_shared<ErrorString>(message);
}

Error errUnspported = newError("unsupported operation");

bool is(Error err, Error target) {
    if (err == nullptr || target == nullptr) {
        return err == target;
    }
    
    return false;
}

}
}