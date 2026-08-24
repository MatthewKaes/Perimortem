// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/terminal/llvm/module/emission.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/pack.hpp"

namespace Tetrodotoxin::Terminal::Llvm::Module {

class Globals {
 public:
  auto reserve_static(
      Emission& program,
      const Ttx::Model::Addressable& addressable) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_foreign(
      Emission& program,
      const Ttx::Model::Addressable& addressable,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol,
      Bool writable) const -> Perimortem::Core::Option<Bool>;

  auto complete(Emission& program, const Ttx::Model::Addressable& addressable)
      const -> Bool;

  auto begin_initializer(
      Emission& program,
      const Ttx::Model::Addressable& addressable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto end_initializer(
      Emission& body,
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
    enum class Property : U8 {
      Foreign = 1,
      Writable = 2,
      External = 4,
      Published = 8,
    };

    Record(
        Bool foreign,
        Perimortem::Core::View::Bytes abi = {},
        Perimortem::Core::View::Bytes symbol = {},
        Bool writable = True,
        Bool external = False,
        Bool published = False)
        : abi(abi),
          symbol(symbol),
          properties(
              U8(foreign ? Property::Foreign : Property{}) |
              U8(writable ? Property::Writable : Property{}) |
              U8(external ? Property::External : Property{}) |
              U8(published ? Property::Published : Property{})) {}

    constexpr auto has(Property property) const -> Bool {
      return Bool(properties & U8(property));
    }

    Perimortem::Core::View::Bytes abi;
    Perimortem::Core::View::Bytes symbol;
    U8 properties;
    Perimortem::Core::Option<LLVMValueRef> global;
    Perimortem::Core::Option<LLVMValueRef> initializer_function;
  };

  auto reserve(
      Emission& program,
      const Ttx::Model::Addressable& addressable,
      Record record) const -> Perimortem::Core::Option<Bool>;

  mutable Perimortem::Memory::Dynamic::
      Map<const Ttx::Model::Addressable*, Record>
          records;
  mutable Perimortem::Memory::Dynamic::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      foreign_addressables;
};

}  // namespace Tetrodotoxin::Terminal::Llvm::Module
