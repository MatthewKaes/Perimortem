// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/library/llvm/publication.hpp"

namespace Tetrodotoxin::Library::Llvm {

// Products retains three immutable Terminal views in the caller Arena. LLVM IR
// is a review artifact while object and header bytes are independent products.
class Products {
 public:
  constexpr Products(
      Perimortem::Core::View::Bytes llvm_ir,
      Perimortem::Core::View::Bytes object,
      Perimortem::Core::View::Bytes header,
      Perimortem::Core::View::Vector<Publication> publications = {})
      : llvm_ir(llvm_ir),
        object(object),
        header(header),
        publications(publications) {}

  constexpr auto get_llvm_ir() const -> Perimortem::Core::View::Bytes {
    return llvm_ir;
  }

  constexpr auto get_object() const -> Perimortem::Core::View::Bytes {
    return object;
  }

  constexpr auto get_header() const -> Perimortem::Core::View::Bytes {
    return header;
  }

  constexpr auto get_publications() const
      -> Perimortem::Core::View::Vector<Publication> {
    return publications;
  }

 private:
  Perimortem::Core::View::Bytes llvm_ir;
  Perimortem::Core::View::Bytes object;
  Perimortem::Core::View::Bytes header;
  Perimortem::Core::View::Vector<Publication> publications;
};

}  // namespace Tetrodotoxin::Library::Llvm
