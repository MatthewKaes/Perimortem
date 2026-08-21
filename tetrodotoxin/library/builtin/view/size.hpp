// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/documentations/comment.hpp"
#include "ttx/model/layouts/ranged.hpp"

namespace Tetrodotoxin::Library::Builtin::View {

// Size exposes the runtime element count retained by one View or Access.
class Size : public Language::Model::Callable {
 public:
  static constexpr Perimortem::Core::View::Bytes name = "get_size"_view;

  TTX_CONTRACT(Size, Language::Model::Callable);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Language::Model::Type& receiver,
      const Language::Model::Type& result) -> Size&;

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

 private:
  constexpr Size(Language::Parameter& self, const Language::Model::Type& result)
      : parameters(self, 1), results(result, 1) {}

  Ttx::Model::Layouts::Ranged parameters;
  Ttx::Model::Layouts::Ranged results;

  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Returns the number of elements available through this contiguous value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Builtin::View
