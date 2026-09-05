// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/union.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/utility/result.hpp"

#include "tetrodotoxin/library/language/model/completion.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/reference/concept/layout.hpp"
#include "ttx/ffi/cpp/domain.hpp"

namespace Tetrodotoxin::Library::Language {

// Generic is the Types model's Abstract contract for a named compile time
// formula. It generates Types but is not itself a Type. Each formula owns its
// canonical generated Types in the same Arena as its Library root. Its retained
// construction context proves scalar arguments but is not a lexical parent.
class Generic : public Ttx::Concept::Abstract {
 public:
  enum class Parameters : U8 {
    Type,
    SemanticType,
    U64,
    S64,
    Bool,
  };

  // SemanticType marks the higher order Type accepted by formulas such as
  // Implementation. Ordinary Library Type arguments keep their narrower
  // alternative, so existing formulas cannot accept another Dialect by
  // accident.
  class SemanticType {
   public:
    static constexpr auto create(const Ttx::Model::Domain& type)
        -> SemanticType {
      return SemanticType(type);
    }

    constexpr auto get() const -> const Ttx::Model::Domain& { return *type; }

    constexpr auto operator==(const SemanticType& rhs) const -> Bool {
      return type == rhs.type;
    }

   private:
    explicit constexpr SemanticType(const Ttx::Model::Domain& type)
        : type(&type) {}

    const Ttx::Model::Domain* type;
  };

  // Semantic graph queries expose const references. Scalar arguments are
  // copied directly, while Type arguments retain their exact selected
  // identity even when its owner has not completed the Type's Layout yet.
  using Argument = Perimortem::Core::Static::
      Union<const Model::Type&, SemanticType, ::U64, ::S64, ::Bool>;

  class Failure {
   public:
    enum class Type : U8 {
      Unavailable,
      Arity,
      Parameter,
      Recursive,
      Formula,
    };

    constexpr Failure(Type type, Count argument = 0)
        : type(type), argument(argument) {}

    constexpr auto get_type() const -> Type { return type; }

    constexpr auto get_argument() const -> Count { return argument; }

   private:
    Type type;
    Count argument;
  };

  using Materialization =
      Perimortem::Utility::Result<const Model::Type&, Failure>;


  Generic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Abstract& context)
      : domain(domain), context(context), entries(domain), active(nullptr) {}

  Generic(const Generic&) = delete;
  Generic(Generic&&) = delete;
  auto operator=(const Generic&) -> Generic& = delete;
  auto operator=(Generic&&) -> Generic& = delete;

  virtual constexpr auto get_parameterization() const
      -> Perimortem::Core::View::Vector<Parameters> = 0;

  auto materialize(Perimortem::Core::View::Vector<Argument> arguments) const
      -> Materialization;

  auto materialize(const Ttx::Concept::Layout& arguments) const
      -> Materialization;

  // Forward Type arguments may settle their Layouts after materialization.
  // Validate every retained result only after the complete authored graph
  // links.
  auto validate_materializations(Ttx::Lexical::Cursor& cursor) const -> Bool;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

 protected:
  struct Created {
    const Model::Type& value;
    const Model::Completion* completion;
  };

  using Creation = Perimortem::Core::Option<Created>;

  template <typename Value>
  static constexpr auto created(const Value& value) -> Creation {
    const Model::Completion* completion = nullptr;
    if constexpr (__is_base_of(Model::Completion, Value)) {
      completion = &value;
    }
    return Created{.value = value, .completion = completion};
  }

  // None means the supplied values do not satisfy this formula. Returning an
  // incomplete or redirected Type is rejection.
  virtual auto create(Perimortem::Core::View::Vector<Argument> arguments) const
      -> Creation = 0;

  constexpr auto get_domain() const -> Perimortem::Memory::Allocator::Arena& {
    return domain;
  }

  constexpr auto get_context() const -> const Ttx::Concept::Abstract& {
    return context;
  }

 private:
  struct Entry {
    Entry(
        Perimortem::Memory::Allocator::Arena& domain,
        Perimortem::Core::View::Vector<Argument> source_arguments,
        const Model::Type& value,
        const Model::Completion* completion);

    Perimortem::Memory::Managed::Vector<Argument> arguments;
    const Model::Type& value;
    const Model::Completion* completion;
  };

  struct Active {
    constexpr Active(
        Perimortem::Core::View::Vector<Argument> arguments,
        Active* previous)
        : arguments(arguments), previous(previous), reentered(False) {}

    Perimortem::Core::View::Vector<Argument> arguments;
    Active* previous;
    Bool reentered;
  };

  auto normalize_argument(
      Parameters parameter,
      const Ttx::Concept::Abstract& argument) const
      -> Perimortem::Core::Option<Argument>;

  Perimortem::Memory::Allocator::Arena& domain;
  const Ttx::Concept::Abstract& context;
  mutable Perimortem::Memory::Managed::Vector<Entry*> entries;
  mutable Active* active;
};

}  // namespace Tetrodotoxin::Library::Language
