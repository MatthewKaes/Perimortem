// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

// Clone replaces one writable Object handle with an independent buffer copy.
class Clone : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "clone"_view;

  TTX_CONTRACT(Clone, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver) -> Clone&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);
  TTX_CONSTEXPR_INVALID_CONTEXT;

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
  constexpr Clone(Language::Parameter& self) : parameters(self, 1) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Named results;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Replaces this Object handle with an independent copy of its buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
