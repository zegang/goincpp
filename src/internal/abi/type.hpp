// Copyright 2024 The Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef GOINCPP_INTERNAL_TYPE_HPP
#define GOINCPP_INTERNAL_TYPE_HPP

#include <size_t_fwd.hpp>
#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace goincpp {
namespace abi {

// Type is the runtime representation of a Go type.
//
// Be careful about accessing this type at build time, as the version
// of this type in the compiler/linker may not have the same layout
// as the version in the target binary, due to pointer width
// differences and any experiments. Use cmd/compile/internal/rttype
// or the functions in compiletype.go to access this type instead.
// (TODO: this admonition applies to every type in this package.
// Put it in some shared location?)
class Type {

public:
    size_t size_;           // Size of the type in bytes
    size_t ptrBytes;        // Number of (prefix) bytes in the type that can contain pointers
    uint32_t hash;          // Hash of the type for fast lookup
    TFlag tFlag;            // Extra type information flags
    uint8_t align_;         // Alignment of variable with this type
    uint8_t fieldAlign_;    // Alignment of struct field with this type
    Kind kind_;             // Enumeration for C/C++ type kind (e.g., int, float, etc.)
    // function for comparing objects of this type
	// (ptr to object A, ptr to object B) -> ==?
    bool (*equal)(void*, void*) = nullptr;
    // GCData stores the GC type data for the garbage collector.
	// If the KindGCProg bit is set in kind, GCData is a GC program.
	// Otherwise it is a ptrmask bitmap. See mbitmap.go for details.
    std::byte* gcData ;     // GC type data for the garbage collector
    std::string strName;    // String form of the type name
    Type* ptrToThis = nullptr;  // Type for pointer to this type (e.g., `Type**`)

    Kind kind() { return static_cast<Kind>(static_cast<uint8_t>(kind_) & static_cast<uint8_t>(Kind::KindMask)); }
    bool hasName() { return (static_cast<uint8_t>(tFlag) & static_cast<uint8_t>(TFlag::TFlagNamed)) != 0; }

    // Pointers reports whether contains pointers.
    bool pointers() { return ptrBytes != 0; }
    // IfaceIndir reports whether is stored indirectly in an interface value.
    bool ifaceIndir() { return (static_cast<uint8_t>(kind_) & static_cast<uint8_t>(Kind::KindDirectIface)) == 0; }
    // isDirectIface reports whether t is stored directly in an interface value.
    bool isDirectIface()  {	return (static_cast<uint8_t>(kind_) & static_cast<uint8_t>(Kind::KindDirectIface)) != 0; }
    
    virtual int len() { return 0; }
    virtual ChanDir chanDir() { return ChanDir::InvalidDir; }
    virtual Type* elem() { return nullptr; }
    virtual Type* key() { return nullptr; }

    size_t size() { return size_; }
    int align() { return align_; }
    int fieldAlign() { return fieldAlign_; }
};

// A Kind represents the specific kind of type that a Type represents.
// The zero Kind is not a valid kind.
enum class Kind : uint8_t {
    Invalid = 0,
    Bool,
    Int,
    Int8,
    Int16,
    Int32,
    Int64,
    Uint,
    Uint8,
    Uint16,
    Uint32,
    Uint64,
    Uintptr,
    Float32,
    Float64,
    Complex64,
    Complex128,
    Array,
    Chan, // Note: In C++, we might use a custom `channel` type instead
    Func,
    Interface,
    Map,
    Pointer,
    Slice,
    String,
    Struct,
    UnsafePointer,

    KindDirectIface = 1 << 5,
    KindGCProg      = 1 << 6, // Type.gc points to GC program
    KindMask        = (1 << 5) - 1
};

// TFlag is used by a Type to signal what extra type information is
// available in the memory directly following the Type value.
enum class TFlag : uint8_t {
    // TFlagUncommon means that there is a data with a type, UncommonType,
	// just beyond the shared-per-type common data.  That is, the data
	// for struct types will store their UncommonType at one offset, the
	// data for interface types will store their UncommonType at a different
	// offset.  UncommonType is always accessed via a pointer that is computed
	// using trust-us-we-are-the-implementors pointer arithmetic.
	//
	// For example, if t.Kind() == Struct and t.tflag&TFlagUncommon != 0,
	// then t has UncommonType data and it can be accessed as:
	//
	//	type structTypeUncommon struct {
	//		structType
	//		u UncommonType
	//	}
	//	u := &(*structTypeUncommon)(unsafe.Pointer(t)).u
	TFlagUncommon = 1 << 0,
    	// TFlagExtraStar means the name in the str field has an
	// extraneous '*' prefix. This is because for most types T in
	// a program, the type *T also exists and reusing the str data
	// saves binary size.
	TFlagExtraStar = 1 << 1,

	// TFlagNamed means the type has a name.
	TFlagNamed = 1 << 2,

	// TFlagRegularMemory means that equal and hash functions can treat
	// this type as a single region of t.size bytes.
	TFlagRegularMemory = 1 << 3,

	// TFlagUnrolledBitmap marks special types that are unrolled-bitmap
	// versions of types with GC programs.
	// These types need to be deallocated when the underlying object
	// is freed.
	TFlagUnrolledBitmap = 1 << 4
};

// NameOff is the offset to a name from moduledata.types.  See resolveNameOff in runtime.
using NameOff = int32_t;

// TypeOff is the offset to a type from moduledata.types.  See resolveTypeOff in runtime.
using TypeOff = int32_t;

// TextOff is an offset from the top of a text section.  See (rtype).textOff in runtime.
using TextOff = int32_t;

const std::unordered_map<Kind, std::string> KindNames = {
    { Kind::Invalid,   "invalid" },
    { Kind::Bool,      "bool" },
    { Kind::Int,       "int" },
    { Kind::Int8,      "int8" },
    { Kind::Int16,     "int16" },
    { Kind::Int32,     "int32" },
    { Kind::Int64,     "int64" },
    { Kind::Uint,      "uint" },
    { Kind::Uint8,     "uint8" },
    { Kind::Uint16,    "uint16" },
    { Kind::Uint32,    "uint32" },
    { Kind::Uint64,    "uint64" },
    { Kind::Uintptr,   "uintptr" },
    { Kind::Float32,   "float32" },
    { Kind::Float64,   "float64" },
    { Kind::Complex64, "complex64" },
    { Kind::Complex128, "complex128" },
    { Kind::Array,     "array" },
    { Kind::Chan,      "chan" },
    { Kind::Func,      "func" },
    { Kind::Interface, "interface" },
    { Kind::Map,       "map" },
    { Kind::Pointer,   "ptr" },
    { Kind::Slice,     "slice" },
    { Kind::String,    "string" },
    { Kind::Struct,    "struct" },
    { Kind::UnsafePointer, "unsafe.Pointer" }
};

/// @brief String returns the name of k.
std::string to_string(Kind k);

/// @brief TypeOf returns the abi.Type of some value.
/// @param a 
/// @return 
std::shared_ptr<Type> typeOf(const Any& a);

// ArrayType represents a fixed array type.
class ArrayType : public Type {

public:
	Type *elem;  // array element type
	Type *slice; // slice type
	size_t len;

    virtual int len() { if (kind() == Kind::Array) { return len; } return 0; }
    virtual Type* elem() { return elem; }
};

enum class ChanDir : int {
    InvalidDir = 0,
    RecvDir = 1 << 0,
    SendDir = 1 << 1,
    BothDir = RecvDir | SendDir
};

// ChanType represents a channel type
class ChanType : public Type {

public:
    Type *elem;
    ChanDir dir;

    virtual ChanDir chanDir() { if (kind() == Kind::Chan) { return dir; } return ChanDir::InvalidDir; }
    virtual Type* elem() { return elem; }
};

// ChanType represents a map type
class MapType : public Type {

public:
    Type *key;
    Type *elem;
    Type *bucket;
    std::function<std::uintptr_t(std::uintptr_t, std::uintptr_t)> hasher;
    uint8_t keySize;
    uint8_t valueSize;
    uint16_t bucketSize;
    uint32_t flags;

    virtual Type* elem() { return elem; }

    bool MapType::IndirectKey() const { return flags & 1u != 0; }
    bool MapType::IndirectElem() const { return flags & 2u != 0; }
    bool MapType::ReflexiveKey() const { return flags & 4u != 0; }
    bool MapType::NeedKeyUpdate() const { return flags & 8u != 0; }
    bool MapType::HashMightPanic() const {return flags & 16u != 0; }

    virtual Type* key() { return key; }
};

}
}

#endif // GOINCPP_INTERNAL_TYPE_HPP