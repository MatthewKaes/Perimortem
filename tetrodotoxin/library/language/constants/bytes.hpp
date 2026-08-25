// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "tetrodotoxin/language/resource.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Bytes is the Constant contract for immutable byte array data. Quoted source,
// hexadecimal byte literals, and embedded files may all produce this value.
// Library defines no native String constant. A concrete owner may materialize
// its own String Type from these bytes through an ordinary Callable. The graph
// owner keeps the immutable backing storage alive for the Constant.
class Bytes : public Constant {
 public:
  TTX_CONTRACT(Bytes, Constant);
  using Value = Perimortem::Core::View::Bytes;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type,
      Value value,
      Ttx::Lexical::Anchor anchor,
      Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&>
          resource = {}) -> Bytes& {
    return Expression::create_authored<Bytes>(
        domain, anchor, [&](auto source) -> Bytes {
          return Bytes(type, value, source, resource);
        });
  }

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Model::Type& type,
      Value value,
      Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&>
          resource = {}) -> Bytes& {
    return Expression::create_synthetic<Bytes>(
        domain, [&](auto source) -> Bytes {
          return Bytes(type, value, source, resource);
        });
  }

  constexpr auto get_type() const -> const Model::Type& override {
    return type;
  }

  virtual constexpr auto get_value() const -> Value { return value; }

  constexpr auto get_resource() const
      -> Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&> {
    return resource.visit(
        []() -> Perimortem::Core::Option<
                 const Tetrodotoxin::Language::Resource&> { return {}; },
        [](const Ttx::Concept::Reference<
            const Tetrodotoxin::Language::Resource>& selected)
            -> Perimortem::Core::Option<
                const Tetrodotoxin::Language::Resource&> {
          return selected.get();
        });
  }

  constexpr auto equals(const Constant& rhs) const -> Bool override {
    return rhs.visit<Bytes>(
        [this, &rhs](const Bytes& selected) {
          return has_same_type(rhs) && get_value() == selected.get_value()
                     ? ::True
                     : ::False;
        },
        [](const Ttx::Concept::Abstract&) { return ::False; });
  }

 private:
  constexpr Bytes(
      const Model::Type& type,
      Value value,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor,
      Perimortem::Core::Option<const Tetrodotoxin::Language::Resource&>
          resource)
      : Constant(anchor),
        type(type),
        value(value),
        resource(resource.visit(
            []() -> Perimortem::Core::Option<Ttx::Concept::Reference<
                     const Tetrodotoxin::Language::Resource>> { return {}; },
            [](const Tetrodotoxin::Language::Resource& selected)
                -> Perimortem::Core::Option<Ttx::Concept::Reference<
                    const Tetrodotoxin::Language::Resource>> {
              return Ttx::Concept::Reference<
                  const Tetrodotoxin::Language::Resource>(selected);
            })) {}

  const Model::Type& type;
  Value value;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Tetrodotoxin::Language::Resource>>
      resource;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
