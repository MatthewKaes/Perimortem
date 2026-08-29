// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/documentations/comment.hpp"
#include "ttx/bootstrap/model/layouts/addressable.hpp"
#include "ttx/bootstrap/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

// IsShared reports whether another owned handle retains this Object buffer.
class IsShared : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "is_shared"_view;

  TTX_CONTRACT(IsShared, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> IsShared&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

 private:
  constexpr IsShared(
      Ttx::Model::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Returns whether another owned handle retains this Object buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
