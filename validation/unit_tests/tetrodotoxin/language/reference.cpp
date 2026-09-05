// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/language/reference.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/concept/unknown.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Ttx::Concept;
using namespace Validation;

static Harness LanguageReference = {
  .name = "Tetrodotoxin::Language::Reference"_view,
};

class ReferenceHost : public Abstract {
 public:
  TTX_NAME("Host"_view);
  TTX_DOCUMENTATION(Documentation::get_empty());

  auto resolve_concept(Core::View::Bytes name) const
      -> const Abstract& override {
    return name == "answer"_view ? *answer : Unknown::get_unknown();
  }

  const Abstract* answer = &Unknown::get_unknown();
};

PERIMORTEM_UNIT_TEST(LanguageReference, resolves_current_host_answer) {
  Memory::Allocator::Arena arena;
  ReferenceHost host;
  auto& reference = Language::Reference::create(arena, host, "answer"_view);

  EXPECT(&reference.get_host() == &host);
  EXPECT(&reference.resolve() == &Unknown::get_unknown());

  host.answer = &None::get_none();
  EXPECT(&reference.resolve() == &None::get_none());

  host.answer = &Unknown::get_unknown();
  EXPECT(&reference.resolve() == &Unknown::get_unknown());
}
