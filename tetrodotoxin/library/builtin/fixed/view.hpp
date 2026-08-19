// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::Fixed {

// View borrows the complete storage of one Fixed value.
class View : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_view"_view;

  TTX_CONTRACT(View, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> View&;

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

  auto lower_call(
      Llvm::Builder& body,
      const Ttx::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<const Ttx::Model::Pack&> receiver_source) const
      -> Bool override;

  auto fold_call(
      Perimortem::Memory::Allocator::Arena& domain,
      Perimortem::Core::Option<const Language::Model::Pack&> receiver,
      const Language::Model::Pack& arguments) const
      -> Perimortem::Core::Option<Language::Model::Pack&> override;

 private:
  constexpr View(Language::Parameter& self, const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1), result_type(result) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;
  const Language::Model::Type& result_type;

  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Borrows a read only View over every element of this Fixed value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::Fixed
