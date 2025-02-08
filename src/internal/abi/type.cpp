#include "type.hpp"

namespace goincpp {
namespace abi {

std::string to_string(Kind k) {
    auto it = KindNames.find(k);
    return (it != KindNames.end()) ? it->second : KindNames[Kind::Invalid];
}

std::shared_ptr<Type> typeOf(const Any& a) { 
    /// Types are either static (for compiler-created types) or
	// heap-allocated but always reachable (for reflection-created
	// types, held in the central map). So there is no need to
	// escape types. noescape here help avoid unnecessary escape
	// of v.
    const std::shared_ptr<Type>& type = a.type();
    return type;
}

}
}
