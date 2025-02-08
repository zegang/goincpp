// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_ERRORS_ERRORS_HPP
#define GOINCPP_ERRORS_ERRORS_HPP

#include "builtin/builtin.hpp"
#include <memory>
#include <concepts>

namespace goincpp {
namespace errors {

class ErrorString : public builtin::ErrorInterface {

public:
    ErrorString(const std::string& msg) : s(msg) {}
    std::string error() const noexcept override { return s; }
    const char* what() const noexcept override { return s.c_str(); }

private:
    std::string s;
};


extern Error newError(const std::string& message);

template <typename T> requires std::derived_from<T, ErrorString>
Error newError(const std::string& message) {
    return std::make_shared<T>(message);
}

/// @brief ErrUnsupported indicates that a requested operation cannot be performed,
/// because it is unsupported.
/// @details For example, a call to [os.Link] when using a file system that does
/// not support hard links.
///
/// Functions and methods should not return this error but should instead
/// return an error including appropriate context that satisfies
///
///	errors.Is(err, errors.ErrUnsupported)
///
/// either by directly wrapping ErrUnsupported or by implementing an [Is] method.
///
/// Functions and methods should document the cases in which an error
/// wrapping this will be returned.
extern Error errUnspported;

/// @brief Is reports whether any error in err's tree matches target.
/// @details The tree consists of err itself, followed by the errors obtained by repeatedly
/// calling its Unwrap() error or Unwrap() []error method. When err wraps multiple
/// errors, Is examines err followed by a depth-first traversal of its children.
///
/// An error is considered to match a target if it is equal to that target or if
/// it implements a method Is(error) bool such that Is(target) returns true.
///
/// An error type might provide an Is method so it can be treated as equivalent
/// to an existing error. For example, if MyError defines
///
///	func (m MyError) Is(target error) bool { return target == fs.ErrExist }
///
/// then Is(MyError{}, fs.ErrExist) returns true. See [syscall.Errno.Is] for
/// an example in the standard library. An Is method should only shallowly
/// compare err and the target and not call [Unwrap] on either.
bool is(Error err, Error target);

}
}

#endif // GOINCPP_ERRORS_ERRORS_HPP