// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/invocation.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/concept/unknown.hpp"
#include "ttx/reference/model/layouts/addressable.hpp"
#include "ttx/reference/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Enum {

// Name returns the authored case name for one Enumeration value.
class Name : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_name"_view;


  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Types::Enumeration& enumeration,
      const Language::Model::Type& result) -> Name&;

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
  constexpr Name(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Model::Layouts::Addressable& self,
      const Language::Types::Enumeration& enumeration,
      const Language::Model::Type& result)
      : domain(domain),
        enumeration(enumeration),
        result_type(result),
        parameters(self, 1),
        results(result, 1) {}

  auto invoke(
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  Perimortem::Memory::Allocator::Arena& domain;
  const Language::Types::Enumeration& enumeration;
  const Language::Model::Type& result_type;
  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  static constexpr Ttx::Documentations::Comment documentation{
    "Returns the authored name of this Enumeration value or an empty View."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Enum
