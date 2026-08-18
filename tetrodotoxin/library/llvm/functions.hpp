// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "llvm-c/Types.h"
#include "tetrodotoxin/language/definition.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Library::Llvm {

class Functions {
 public:
  class Lowering {
   public:
    constexpr Lowering(
        LLVMValueRef function,
        const Ttx::Model::Callable& callable,
        Perimortem::Core::Option<LLVMValueRef> sret,
        Perimortem::Core::Option<LLVMTypeRef> sret_type)
        : function(function),
          callable(callable),
          sret(sret),
          sret_type(sret_type) {}

    constexpr auto get_function() const -> LLVMValueRef { return function; }

    constexpr auto get_callable() const -> const Ttx::Model::Callable& {
      return callable.get();
    }

    constexpr auto get_sret() const -> Perimortem::Core::Option<LLVMValueRef> {
      return sret;
    }

    constexpr auto get_sret_type() const
        -> Perimortem::Core::Option<LLVMTypeRef> {
      return sret_type;
    }

   private:
    LLVMValueRef function;
    Ttx::Concept::Reference<const Ttx::Model::Callable> callable;
    Perimortem::Core::Option<LLVMValueRef> sret;
    Perimortem::Core::Option<LLVMTypeRef> sret_type;
  };

  auto reserve_function(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_foreign(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_get_size(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_get_access(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<Bool>;

  auto complete(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable) const -> Bool;

  auto begin_body(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<Lowering>;

  auto bind_parameters(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Callable& callable) const -> Bool;

  auto end_body(
      Ttx::Concept::Abstract& body,
      const Ttx::Model::Callable& callable) const -> Bool;

  auto find_function(const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<LLVMValueRef>;

  auto find_symbol(const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<Perimortem::Core::View::Bytes>;

  auto find_sret_type(const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<LLVMTypeRef>;

  auto get_indirect_parameters(const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::View::Vector<Bool>;

  auto get_foreign_callables() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>;

 private:
  enum class Kind : Unsigned_8 {
    Function,
    Foreign,
    GetSize,
    GetAccess,
  };

  class Record {
   public:
    Record(
        Kind kind,
        Perimortem::Core::View::Bytes abi = {},
        Perimortem::Core::View::Bytes symbol = {},
        Perimortem::Core::Option<const Tetrodotoxin::Language::Definition&>
            definition = {})
        : kind(kind), abi(abi), symbol(symbol), definition(definition) {}

    Kind kind;
    Perimortem::Core::View::Bytes abi;
    Perimortem::Core::View::Bytes symbol;
    Perimortem::Core::Option<const Tetrodotoxin::Language::Definition&>
        definition;
    Perimortem::Core::Option<LLVMValueRef> function;
    Perimortem::Core::Option<LLVMTypeRef> sret_type;
    Perimortem::Memory::Dynamic::Vector<Bool> indirect_parameters;
    Bool completed = False;
  };

  auto reserve(
      Ttx::Concept::Abstract& program,
      const Ttx::Model::Callable& callable,
      Record record) const -> Perimortem::Core::Option<Bool>;

  mutable Perimortem::Memory::Dynamic::Map<const Ttx::Model::Callable*, Record>
      records;
  mutable Perimortem::Memory::Dynamic::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>
      foreign_callables;
};

}  // namespace Tetrodotoxin::Library::Llvm
