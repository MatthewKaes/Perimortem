// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/reference/model/layouts/addressable.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Object {

class Capacity : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_capacity"_view;


  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> Capacity&;

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
  constexpr Capacity(
      Ttx::Model::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  static constexpr Ttx::Documentations::Comment documentation{
    "Returns the number of elements available in this Object buffer."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Object
