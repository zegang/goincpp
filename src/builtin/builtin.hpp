#ifndef GOINCPP_BUILTIN_HPP
#define GOINCPP_BUILTIN_HPP

#include <exception>
#include <string>

namespace goincpp {

namespace builtin {

/// @brief Class ErrorInterface is an interface type.
/// An error variable represents any value that can describe itself as a string.
class ErrorInterface : public std::exception {

public:
    virtual ~ErrorInterface() {}

    /// @brief error description in string
    /// @return string to describe error
    virtual std::string error() const noexcept = 0;

    /// @brief error description in char array
    /// @return char array to describe error
    virtual const char* what() const noexcept = 0;
};

}

using Error = std::shared_ptr<builtin::ErrorInterface>;

}

#endif // GOINCPP_BUILTIN_HPP