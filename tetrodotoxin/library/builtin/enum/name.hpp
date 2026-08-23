// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Enum {

// Name returns the authored case name for one Enumeration value.
class Name : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_name"_view;

  TTX_CONTRACT(Name, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Types::Enumeration& enumeration,
      const Language::Model::Type& result) -> Name&;

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

  auto fold_call(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&> override;

 private:
  constexpr Name(
      Language::Parameter& self,
      const Language::Types::Enumeration& enumeration,
      const Language::Model::Type& result)
      : enumeration(enumeration),
        result_type(result),
        parameters(self, 1),
        results(result, 1) {}

  const Language::Types::Enumeration& enumeration;
  const Language::Model::Type& result_type;
  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;

  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Returns the authored name of this Enumeration value or an empty View."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Enum
