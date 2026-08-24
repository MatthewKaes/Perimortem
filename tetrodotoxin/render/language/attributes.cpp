// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/render/language/attributes.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

static auto find_attribute(
    View::Vector<Language::Attribute> attributes,
    View::Bytes key) -> Option<const Language::Attribute&> {
  for (Count index = 0; index < attributes.get_size(); index++) {
    const Language::Attribute& attribute = attributes.get_data()[index];
    if (attribute.get_key() == key) {
      return attribute;
    }
  }
  return {};
}

static auto expects_unsigned(const Language::Attribute& attribute) -> Bool {
  return attribute.get_value().find<U64>() ? True : False;
}

static auto expects_name(const Language::Attribute& attribute) -> Bool {
  return attribute.get_value().find<View::Bytes>() ? True : False;
}

static auto values_equal(
    const Language::Attribute::Value& left,
    const Language::Attribute::Value& right) -> Bool {
  const View::Bytes* text = left.find<View::Bytes>();
  const U64* unsigned_value = left.find<U64>();
  const S64* signed_value = left.find<S64>();
  const R64* real_value = left.find<R64>();
  const Bool* flag = left.find<Bool>();
  if (text) {
    const View::Bytes* candidate = right.find<View::Bytes>();
    return candidate && *candidate == *text;
  }
  if (unsigned_value) {
    const U64* candidate = right.find<U64>();
    return candidate && *candidate == *unsigned_value;
  }
  if (signed_value) {
    const S64* candidate = right.find<S64>();
    return candidate && *candidate == *signed_value;
  }
  if (real_value) {
    const R64* candidate = right.find<R64>();
    return candidate && *candidate == *real_value;
  }
  if (flag) {
    const Bool* candidate = right.find<Bool>();
    return candidate && *candidate == *flag;
  }
  return right.is_null();
}

auto Render::Language::Attributes::validate(
    Cursor& cursor,
    View::Vector<Tetrodotoxin::Language::Attribute> attributes,
    Placement placement) -> Bool {
  // Attribute keys are intentionally open in the shared Definition model.
  // Render closes only the keys it understands at this boundary, where it can
  // also explain the declaration placement and value shape to the author.
  Bool valid = True;
  for (Count index = 0; index < attributes.get_size(); index++) {
    const auto& attribute = attributes.get_data()[index];
    for (Count prior = 0; prior < index; prior++) {
      if (attributes.get_data()[prior].get_key() == attribute.get_key()) {
        cursor.create_expression_error(
            attribute.get_anchor(),
            "A Render declaration cannot repeat one Attribute key."_view);
        valid = False;
      }
    }

    View::Bytes key = attribute.get_key();
    Bool accepted = False;
    Bool value_valid = False;
    if (key == "set"_view || key == "slot"_view) {
      accepted = placement == Placement::Resource;
      value_valid = expects_unsigned(attribute);
    } else if (key == "location"_view) {
      accepted = placement == Placement::StageEntry;
      value_valid = expects_unsigned(attribute);
    } else if (key == "builtin"_view) {
      accepted = placement == Placement::StageEntry;
      value_valid = expects_name(attribute);
    } else if (key == "address_space"_view) {
      accepted =
          placement == Placement::Resource || placement == Placement::Push;
      value_valid = expects_name(attribute);
    } else if (key == "capability"_view) {
      accepted =
          placement == Placement::Stage || placement == Placement::Structure;
      value_valid = expects_name(attribute);
    } else if (key == "read"_view || key == "write"_view) {
      accepted = placement == Placement::Resource;
      value_valid = !attribute.has_value();
    }

    if (!accepted) {
      auto report = cursor.create_report(attribute.get_anchor());
      report << "Render Attribute `@"_view << key
             << "` does not apply to this declaration."_view;
      report.get_hint()
          << "Move the Attribute to the Render fact that owns its meaning."_view;
      valid = False;
    } else if (!value_valid) {
      auto report = cursor.create_report(attribute.get_anchor());
      report << "Render Attribute `@"_view << key
             << "` has the wrong value shape."_view;
      report.get_hint()
          << "Use the scalar value described by the Render contract."_view;
      valid = False;
    }
  }

  auto set = find_attribute(attributes, "set"_view);
  auto slot = find_attribute(attributes, "slot"_view);
  if (Bool(set) != Bool(slot)) {
    cursor.create_expression_error(
        set ? set->get_anchor() : slot->get_anchor(),
        "A Render resource binding uses `@set` and `@slot` together."_view);
    valid = False;
  }

  auto location = find_attribute(attributes, "location"_view);
  auto builtin = find_attribute(attributes, "builtin"_view);
  if (location && builtin) {
    cursor.create_expression_error(
        builtin->get_anchor(),
        "A Render Stage entry chooses either `@location` or `@builtin`."_view);
    valid = False;
  }

  return valid;
}

auto Render::Language::Attributes::satisfies(
    View::Vector<Tetrodotoxin::Language::Attribute> supplied,
    View::Vector<Tetrodotoxin::Language::Attribute> required) -> Bool {
  // Interface negotiation needs this policy without publishing a second
  // diagnostic for every failed candidate. The selected owner reports the
  // complete failed relationship when final validation reaches its boundary.
  for (Count index = 0; index < required.get_size(); index++) {
    const auto& requirement = required.get_data()[index];
    auto selected = find_attribute(supplied, requirement.get_key());
    BAIL_IF(
        !selected ||
        !values_equal(selected->get_value(), requirement.get_value()));
  }
  return True;
}
