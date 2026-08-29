// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/addressable.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

class Access : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_access"_view;

  TTX_CONTRACT(Access, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> Access&;

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
  constexpr Access(
      Ttx::Model::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Borrows writable access to this Object buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
