// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/map.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "backend/llvm/representation/emission.hpp"
#include "llvm-c/Types.h"
#include "tetrodotoxin/language/definition.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/pack.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Backend::Llvm::Representation {

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

  // ConstructionField is one target lowering fact supplied by the aggregate
  // Type that owns the Field order and default. It retains only original graph
  // identities. LLVM neither discovers Fields nor manufactures a semantic
  // construction model.
  class ConstructionField {
   public:
    constexpr ConstructionField(
        const Ttx::Model::Addressable& field,
        const Ttx::Model::Pack& fallback,
        Bool parameter)
        : field(field), fallback(fallback), parameter(parameter) {}

    constexpr auto get_field() const -> const Ttx::Model::Addressable& {
      return field.get();
    }

    constexpr auto get_fallback() const -> const Ttx::Model::Pack& {
      return fallback.get();
    }

    constexpr auto is_parameter() const -> Bool { return parameter; }

   private:
    Ttx::Concept::Reference<const Ttx::Model::Addressable> field;
    Ttx::Concept::Reference<const Ttx::Model::Pack> fallback;
    Bool parameter;
  };

  auto reserve_function(
      Emission& program,
      const Ttx::Model::Callable& callable,
      const Tetrodotoxin::Language::Definition& definition) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_foreign(
      Emission& program,
      const Ttx::Model::Callable& callable,
      Perimortem::Core::View::Bytes abi,
      Perimortem::Core::View::Bytes symbol) const
      -> Perimortem::Core::Option<Bool>;

  auto reserve_construction(
      Emission& program,
      const Ttx::Model::Type& owner,
      Bool provider,
      Perimortem::Core::View::Vector<
          Ttx::Concept::Reference<const Ttx::Model::Addressable>> parameters)
      const -> Bool;

  auto complete_construction(Emission& program, const Ttx::Model::Type& owner)
      const -> Bool;

  auto lower_construction(
      Emission& program,
      const Ttx::Model::Type& owner,
      Perimortem::Core::View::Vector<ConstructionField> fields) const -> Bool;

  auto call_construction(
      Emission& body,
      const Ttx::Model::Pack& result,
      const Ttx::Model::Type& owner,
      const Ttx::Model::Pack& arguments) const -> Bool;

  auto complete(Emission& program, const Ttx::Model::Callable& callable) const
      -> Bool;

  auto begin_body(Emission& program, const Ttx::Model::Callable& callable) const
      -> Perimortem::Core::Option<Lowering>;

  auto bind_parameters(Emission& body, const Ttx::Model::Callable& callable)
      const -> Bool;

  auto end_body(Emission& body, const Ttx::Model::Callable& callable) const
      -> Bool;

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
  enum class Kind : U8 {
    Function,
    External,
    Foreign,
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
  };

  class ConstructionRecord {
   public:
    ConstructionRecord(Bool provider, Perimortem::Core::View::Bytes symbol)
        : provider(provider), symbol(symbol) {}

    Bool provider;
    Perimortem::Core::View::Bytes symbol;
    Perimortem::Core::Option<LLVMValueRef> function;
    Perimortem::Core::Option<LLVMTypeRef> sret_type;
    Perimortem::Memory::Dynamic::Vector<
        Ttx::Concept::Reference<const Ttx::Model::Addressable>>
        parameters;
    Perimortem::Memory::Dynamic::Vector<Bool> indirect_parameters;
  };

  auto reserve(
      Emission& program,
      const Ttx::Model::Callable& callable,
      Record record) const -> Perimortem::Core::Option<Bool>;

  mutable Perimortem::Memory::Dynamic::Map<const Ttx::Model::Callable*, Record>
      records;
  mutable Perimortem::Memory::Dynamic::
      Map<const Ttx::Model::Type*, ConstructionRecord>
          constructions;
  mutable Perimortem::Memory::Dynamic::Vector<
      Ttx::Concept::Reference<const Ttx::Model::Callable>>
      foreign_callables;
};

}  // namespace Tetrodotoxin::Backend::Llvm::Representation
