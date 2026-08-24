// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/graphics/hosting.hpp"

#include "tetrodotoxin/language/visibility.hpp"
#include "tetrodotoxin/library/language/field.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
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

static auto compatible_scalar(
    const Library::Language::Model::Type& required,
    const Library::Language::Model::Type& supplied) -> Bool {
  auto required_value =
      required.select<Library::Language::Model::Types::Value>();
  auto supplied_value =
      supplied.select<Library::Language::Model::Types::Value>();
  BAIL_IF(
      !required_value || !supplied_value ||
      required_value->get_width() != supplied_value->get_width());

  Bool matching_flag = required.is<Library::Language::Model::Types::Flag>() &&
                       supplied.is<Library::Language::Model::Types::Flag>();
  Bool matching_real = required.is<Library::Language::Model::Types::Real>() &&
                       supplied.is<Library::Language::Model::Types::Real>();
  Bool matching_signed =
      required.is<Library::Language::Model::Types::Signed>() &&
      supplied.is<Library::Language::Model::Types::Signed>();
  Bool matching_unsigned =
      required.is<Library::Language::Model::Types::Unsigned>() &&
      supplied.is<Library::Language::Model::Types::Unsigned>();
  return matching_flag || matching_real || matching_signed || matching_unsigned;
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

    auto supplied_field =
        find_public_field(*supplied, required_field->get_name());
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
