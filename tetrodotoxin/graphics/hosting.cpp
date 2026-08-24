// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/hosting.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

static auto find_public_field(
    const Library::Language::Types::Composite& candidate,
    View::Bytes name) -> Option<const Library::Language::Field&> {
  for (const Reference<Abstract>& retained :
       candidate.get_addressables(Language::Visibility::Public)) {
    auto field = retained.get().select<Library::Language::Field>();
    if (field && field->get_name() == name) {
      return *field;
    }
  }
  return {};
}

auto Graphics::Hosting::negotiate(
    const Abstract& requirement,
    const Abstract& candidate) const -> Relation {
  const Abstract& required_identity = requirement.resolve();
  const Abstract& candidate_identity = candidate.resolve();
  auto required =
      required_identity.select<Library::Language::Types::Composite>();
  auto supplied = candidate_identity.select<Library::Language::Types::Object>();
  if (!required || !supplied || !required->is_finalized() ||
      !supplied->is_finalized()) {
    return Relation::Rejected;
  }
  if (&required_identity == &candidate_identity) {
    return Relation::Equivalent;
  }

  for (const Reference<Abstract>& retained :
       required->get_addressables(Language::Visibility::Public)) {
    auto required_field = retained.get().select<Library::Language::Field>();
    if (!required_field || required_field->get_writability() !=
                               Library::Language::Writability::Internal) {
      return Relation::Rejected;
    }

    auto supplied_field =
        find_public_field(*supplied, required_field->get_name());
    if (!supplied_field ||
        supplied_field->get_writability() !=
            Library::Language::Writability::Internal ||
        &supplied_field->get_type().resolve() !=
            &required_field->get_type().resolve()) {
      return Relation::Rejected;
    }
  }

  return Relation::Satisfied;
}
