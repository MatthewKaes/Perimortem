// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/fold_call.h"
#include "ttx/bootstrap/concept/unknown.hpp"
#include "ttx/bootstrap/model/documentations/comment.hpp"
#include "ttx/bootstrap/model/layouts/addressable.hpp"
#include "ttx/bootstrap/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// IsEmpty reports whether one View or Access contains no elements.
class IsEmpty : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "is_empty"_view;

  TTX_CONTRACT(IsEmpty, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> IsEmpty&;

  TTX_NAME(name);
  TTX_DOCUMENTATION(documentation);

  constexpr auto get_parameters() const
      -> const Ttx::Concept::Layout& override {
    return parameters;
  }

  constexpr auto get_results() const -> const Ttx::Concept::Layout& override {
    return results;
  }

  auto negotiate_interface(const ttx_abstract* requirement) const
      -> ttx_interface override;

 private:
  constexpr IsEmpty(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Model::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : domain(domain),
        parameters(self, 1),
        results(result, 1),
        result_type(result) {}

  static auto fold_abi(
      const ttx_abstract* callable,
      const ttx_pack* receiver,
      const ttx_pack* arguments) -> const ttx_abstract*;
  auto fold(
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  Perimortem::Memory::Allocator::Arena& domain;
  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  const Language::Model::Type& result_type;
  static const ttx_library_fold_call_operations fold_operations;
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Returns whether this contiguous value contains no elements."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
