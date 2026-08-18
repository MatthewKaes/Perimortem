// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language {

// Parameter is one named Function input Addressable. It is the real entry in
// the linked parameter Layout and borrows that owner's stable authored name.
// Its exact Type is the only semantic fact added when the Layout links. Source
// Anchors and TypeReference syntax remain in the one canonical Layout slot.
class Parameter : public Model::Addressable {
 public:
  TTX_CONTRACT(Parameter, Model::Addressable);

  Parameter(const Parameter&) = delete;
  Parameter(Parameter&&) = delete;
  auto operator=(const Parameter&) -> Parameter& = delete;
  auto operator=(Parameter&&) -> Parameter& = delete;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Model::Type& override;

  // Parameter validates and creates its own exact semantic identity. Layout
  // retains the result as its canonical linked entry but receives no private
  // construction authority over Parameter.
  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& type) -> Perimortem::Core::Option<Parameter&>;

  // A generated Callable supplies a reserved nonempty name and one completed
  // value Type directly. The semantic identity is otherwise the same exact
  // Parameter used by an authored Signature.
  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::View::Bytes name,
      const Model::Type& type) -> Parameter&;

 private:
  constexpr Parameter(
      Perimortem::Core::View::Bytes name,
      const Model::Type& type)
      : name(name), type(type) {}

  Perimortem::Core::View::Bytes name;
  const Model::Type& type;
};

}  // namespace Tetrodotoxin::Library::Language
