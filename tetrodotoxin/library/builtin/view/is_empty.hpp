// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/invocation.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/reference/model/layouts/addressable.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// IsEmpty reports whether one View or Access contains no elements.
class IsEmpty : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "is_empty"_view;


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

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;
  void invoke(
      ttx_abstract operation,
      ttx_pack input,
      ttx_context context,
      ttx_pack_result result) const override;

 private:
  constexpr IsEmpty(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Model::Layouts::Addressable& self,
      const Language::Model::Type& result)
      : domain(domain),
        parameters(self, 1),
        results(result, 1),
        result_type(result) {}

  auto invoke(
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  Perimortem::Memory::Allocator::Arena& domain;
  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  const Language::Model::Type& result_type;
  static constexpr Ttx::Documentations::Comment documentation{
    "Returns whether this contiguous value contains no elements."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
