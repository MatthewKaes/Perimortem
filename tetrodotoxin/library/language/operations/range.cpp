// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/operations/range.hpp"

#include "tetrodotoxin/library/language/generics/range.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

TTX_BINARY_OP(Range);

auto Language::Operations::Range::select_type(
    const Ttx::Concept::Abstract& context) const
    -> Core::Option<const Language::Model::Type&> {
  auto inputs = get_inputs();
  const Model::Pack& left = *inputs.get_data()[0];
  const Model::Pack& right = *inputs.get_data()[1];
  const Abstract& left_type = left.get_type().resolve();
  const Abstract& right_type = right.get_type().resolve();
  auto element = left_type.select<Language::Model::Type>();
  if (!element || &left_type != &right_type ||
      (!left_type.is<Tetrodotoxin::Library::Language::Model::Types::Signed>() &&
       !left_type
            .is<Tetrodotoxin::Library::Language::Model::Types::Unsigned>())) {
    return {};
  }

  Core::Static::Vector<Generic::Argument, 1> arguments = {{
    Generic::Argument(*element),
  }};
  const Abstract& selected = context.resolve_concept(Generics::Range::name);
  auto generic = selected.select<Generic>();
  BAIL_IF(!generic);
  return generic->materialize(arguments.get_view())
      .visit(
          [](const Language::Model::Type& type)
              -> Core::Option<const Language::Model::Type&> { return type; },
          [](const Generic::Failure&)
              -> Core::Option<const Language::Model::Type&> { return {}; });
}

auto Language::Operations::Range::evaluate_constants(Memory::Allocator::Arena&)
    -> Utility::Result<
        Core::Option<Tetrodotoxin::Library::Language::Constant&>,
        Expression::Error> {
  return Core::Option<Tetrodotoxin::Library::Language::Constant&>{};
}
