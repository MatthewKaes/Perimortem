// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/reference/model/layouts/addressable.hpp"
#include "ttx/reference/model/layouts/named.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

// Clone replaces one writable Object handle with an independent buffer copy.
class Clone : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "clone"_view;


  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver) -> Clone&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

  auto accepts_receiver(
      const Ttx::Concept::Abstract& receiver,
      const Ttx::Concept::Abstract& host) const -> Bool override;

 private:
  constexpr Clone(Ttx::Model::Layouts::Addressable& self)
      : parameters(self, 1) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Named results;
  static constexpr Ttx::Documentations::Comment documentation{
    "Replaces this Object handle with an independent copy of its buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
