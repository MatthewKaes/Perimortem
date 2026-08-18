// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Products retains three immutable Terminal views in the caller Arena. LLVM IR
// is a review artifact while object and header bytes are independent products.
class Products {
 public:
  constexpr Products(
      Perimortem::Core::View::Bytes llvm_ir,
      Perimortem::Core::View::Bytes object,
      Perimortem::Core::View::Bytes header)
      : llvm_ir(llvm_ir), object(object), header(header) {}

  constexpr auto get_llvm_ir() const -> Perimortem::Core::View::Bytes {
    return llvm_ir;
  }

  constexpr auto get_object() const -> Perimortem::Core::View::Bytes {
    return object;
  }

  constexpr auto get_header() const -> Perimortem::Core::View::Bytes {
    return header;
  }

 private:
  Perimortem::Core::View::Bytes llvm_ir;
  Perimortem::Core::View::Bytes object;
  Perimortem::Core::View::Bytes header;
};

}  // namespace Tetrodotoxin::Library::Llvm
