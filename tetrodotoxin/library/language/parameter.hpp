// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language {

namespace Model {
class Layout;
}

// Parameter is one named Function input Addressable. It is the real entry in
// the linked parameter Layout and borrows that owner's stable authored name.
// Its exact Type is the only semantic fact added when the Layout links; source
// Anchors and TypeReference syntax remain in the one canonical Layout slot.
class Parameter : public Ttx::Model::Addressable {
 public:
  TTX_CONTRACT(Parameter, Ttx::Model::Addressable);

  Parameter(const Parameter&) = delete;
  Parameter(Parameter&&) = delete;
  auto operator=(const Parameter&) -> Parameter& = delete;
  auto operator=(Parameter&&) -> Parameter& = delete;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;

  auto get_type() const -> const Ttx::Model::Type& override;

 private:
  friend class Model::Layout;

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Perimortem::Core::View::Bytes& name,
      const Ttx::Model::Type& type) -> Perimortem::Core::Option<Parameter&>;

  constexpr Parameter(
      const Perimortem::Core::View::Bytes& name,
      const Ttx::Model::Type& type)
      : name(name), type(type) {}

  const Perimortem::Core::View::Bytes& name;
  const Ttx::Model::Type& type;
};

}  // namespace Tetrodotoxin::Library::Language
