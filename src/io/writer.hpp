// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_IO_WRITER_HPP
#define GOINCPP_IO_WRITER_HPP

#include <iostream>
#include <fstream>
#include <vector>

namespace goincpp {

namespace io {

class Writer {
private:
    std::ostream& _os;

public:
    explicit Writer(std::ostream& stream) : _os(stream) {}

    template<typename T>
    void Write(const T& data) {
        _os.write(reinterpret_cast<const char*>(&data), sizeof(data));
    }

    void Write(const std::string& str) {
        _os << str;
    }

    void Write(const char* data) {
        _os << data;
    }

    void Write(const std::vector<char>& buffer) {
        _os.write(buffer.data(), buffer.size());
    }

    // Add more overloads as needed for different types of data

    // Example of a method that writes bytes
    void WriteBytes(const char* buffer, size_t size) {
        _os.write(buffer, size);
    }

    // Example of a method that returns the underlying std::ostream
    std::ostream& GetStream() {
        return _os;
    }
};

}
}

#endif // GOINCPP_IO_WRITER_HPP