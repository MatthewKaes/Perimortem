// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/pack.hpp"

namespace Tetrodotoxin::Library::Llvm {

class Globals {
 public:
  auto reserve_static(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Addressable& addressable) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_foreign(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Addressable& addressable,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol,
      Bool writable) const -> Perimortem::Core::Option<Bool>;

  auto complete(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Addressable& addressable) const -> Bool;

  auto begin_initializer(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Addressable& addressable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto end_initializer(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Addressable& addressable,
      const Ttx::Model::Pack& value) const -> Bool;

  auto find_address(const Ttx::Model::Addressable& addressable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto find_symbol(const Ttx::Model::Addressable& addressable) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  auto permits_foreign_write(const Ttx::Model::Addressable& addressable) const
      -> Bool;

  auto get_foreign_addressables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>;

 private:
  class Record {
   public:
    Record(
        Bool foreign,
        Perimortem::Core::View::Bytes abi = {},
        Perimortem::Core::View::Bytes symbol = {},
        Bool writable = True,
        Bool external = False,
        Bool published = False)
        : foreign(foreign),
          abi(abi),
          symbol(symbol),
          writable(writable),
          external(external),
          published(published) {}

    Bool foreign;
    Perimortem::Core::View::Bytes abi;
    Perimortem::Core::View::Bytes symbol;
    Bool writable;
    Bool external;
    Bool published;
    Perimortem::Core::Option<LLVMValueRef> global;
    Perimortem::Core::Option<LLVMValueRef> initializer_function;
    Bool completed = False;
    Bool initialized = False;
  };

  auto reserve(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Addressable& addressable,
      Record record) const -> Perimortem::Core::Option<Bool>;

  mutable Perimortem::Memory::Dynamic::
      Map<const Ttx::Model::Addressable*, Record>
          records;
  mutable Perimortem::Memory::Dynamic::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      foreign_addressables;
};

}  // namespace Tetrodotoxin::Library::Llvm
