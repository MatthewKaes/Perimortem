// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/static/vector.hpp"

#include "ttx/concept/alias.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/structured.hpp"
#include "ttx/model/packs/positional.hpp"
#include "ttx/model/types/signed.hpp"
#include "ttx/model/types/unsigned_8.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Model;

/// A field proves that constexpr Layouts retain the real Addressable identity.
class ConstexprField final : public Addressable {
 public:
  constexpr ConstexprField(View::Bytes name, const Abstract& type)
      : name(name), type(type) {}

  constexpr auto get_name() const -> View::Bytes override { return name; }
  constexpr auto get_documentation() const -> const Documentation& override {
    return documentation;
  }
  constexpr auto resolve() const -> const Abstract& override {
    return type.resolve();
  }
  auto resolve_context(View::Bytes route) const -> const Abstract& override {
    return type.resolve().resolve_context(route);
  }

 private:
  View::Bytes name;
  const Abstract& type;
  static constexpr Comment documentation{"A compile-time field."_view};
};

// These objects intentionally live at translation-unit scope so every query
// below is required to succeed during constant evaluation.
constexpr Ttx::Model::Types::Unsigned_8 unsigned_8;
constexpr Comment alias_comment{"A byte-sized count."_view};
constexpr Alias octet("Octet"_view, unsigned_8, alias_comment);
constexpr ConstexprField value("value"_view, octet);
constexpr Static::Vector<Reference<Addressable>, 1> fields = {{value}};
constexpr Ttx::Model::Layouts::Structured structure(fields);
constexpr Static::Vector<Reference<Abstract>, 1> values = {{octet}};
constexpr Ttx::Model::Packs::Positional pack(values);

static_assert(unsigned_8.is<Abstract>());
static_assert(unsigned_8.is<Type>());
static_assert(unsigned_8.is<Ttx::Model::Types::Terminal>());
static_assert(unsigned_8.is<Ttx::Model::Types::Unsigned>());
static_assert(!unsigned_8.is<Ttx::Model::Types::Signed>());
static_assert(unsigned_8.get_width() == 8);
static_assert(unsigned_8.get_size() == sizeof(::Unsigned_8));
static_assert(unsigned_8.get_alignment() == alignof(::Unsigned_8));
static_assert(unsigned_8.get_layout().is_empty());

static_assert(octet.get_name() == "Octet"_view);
static_assert(&octet.resolve() == &unsigned_8);
static_assert(octet.get_documentation().line_count() == 2);

static_assert(structure.get_size() == 1);
static_assert(&structure.get_abstract(0) == &value);
static_assert(structure.fits(structure));
static_assert(&structure.get_fitted(structure, 0) == &value);

static_assert(pack.get_layout().get_size() == 1);
static_assert(&pack.get_layout().get_abstract(0) == &octet);
