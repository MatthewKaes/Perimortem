// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/hosting.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin;

static auto compatible_scalar(
    const Library::Language::Model::Type& required,
    const Library::Language::Model::Type& supplied) -> Bool {
  auto required_value =
      required.select<Library::Language::Model::Types::Value>();
  auto supplied_value =
      supplied.select<Library::Language::Model::Types::Value>();
  return required_value && supplied_value &&
         required_value->is_equivalent(*supplied_value);
}

static auto compatible_type(
    const Ttx::Model::Type& required,
    const Ttx::Model::Type& supplied) -> Bool {
  const Abstract& required_identity = required.resolve();
  const Abstract& supplied_identity = supplied.resolve();
  if (&required_identity == &supplied_identity) {
    return True;
  }

  auto required_library =
      required_identity.select<Library::Language::Model::Type>();
  auto supplied_library =
      supplied_identity.select<Library::Language::Model::Type>();
  return required_library && supplied_library &&
         compatible_scalar(*required_library, *supplied_library);
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

    auto supplied_field = supplied
                              ->resolve_type_access(
                                  *required, required_field->get_name(),
                                  Library::Language::Model::Type::Access::Self)
                              .resolve()
                              .select<Library::Language::Field>();
    if (!supplied_field ||
        supplied_field->get_writability() !=
            Library::Language::Writability::Internal ||
        !compatible_type(
            required_field->get_type(), supplied_field->get_type())) {
      return Relation::Rejected;
    }
  }

  return Relation::Satisfied;
}
