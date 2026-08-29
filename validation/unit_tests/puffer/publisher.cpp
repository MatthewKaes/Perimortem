// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/publisher.hpp"

#include "validation/unit_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <ftw.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/language/product.hpp"
#include "ttx/bootstrap/model/layouts/fluid.hpp"
#include "ttx/model/context.h"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Validation;

static Harness PufferPublisher = {
  .name = "Puffer::Publisher"_view,
};

static auto remove_entry(const char* path, const struct stat*, S32, struct FTW*)
    -> S32 {
  return remove(path);
}

static auto publish(
    Memory::Allocator::Arena& arena,
    Core::View::Bytes root,
    Core::View::Vector<const Abstract*> products) -> Bool {
  Ttx::Model::Layouts::Fluid layout(products);
  ttx_model_pack packs[1];
  ttx_fluid_layout layouts[1];
  ttx_named_layout named_layouts[1];
  Memory::Managed::Vector<const ttx_abstract*> entries(arena);
  for (Count index = 0; index < products.get_size(); index++) {
    entries.insert(nullptr);
  }
  ttx_model_context context;
  ttx_model_context_initialize(
      &context, ttx_model_context_storage{
                  .packs = packs,
                  .layouts = layouts,
                  .named_layouts = named_layouts,
                  .entries = entries.is_empty() ? nullptr : &entries[0],
                  .names = nullptr,
                  .name_bytes = nullptr,
                  .pack_capacity = 1,
                  .entry_capacity = products.get_size(),
                  .name_capacity = 0,
                  .name_byte_capacity = 0,
                });
  const ttx_pack* packed = ttx_context_pack(&context.context, layout.get_abi());
  return packed != nullptr && Puffer::Publisher(root).publish(packed);
}

PERIMORTEM_UNIT_TEST(PufferPublisher, complete_pack_commit) {
  char root[] = "/tmp/puffer-publisher-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  Memory::Allocator::Arena arena;
  auto& first = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/first.bin"_view, "first"_view);
  const Abstract* first_product[] = {&first};
  ASSERT(publish(arena, root_view, first_product));
  Memory::Managed::Bytes first_path(arena, root_view);
  first_path.concat("/Example.Package/1.0/first.bin"_view);
  auto first_bytes = System::File::read(first_path.get_view());
  ASSERT(first_bytes);
  EXPECT_TEXT(*first_bytes, "first"_view);

  auto& second = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/second.bin"_view, "second"_view);
  const Abstract* second_product[] = {&second};
  ASSERT(publish(arena, root_view, second_product));
  EXPECT_NOT(System::File::read(first_path.get_view()));
  Memory::Managed::Bytes second_path(arena, root_view);
  second_path.concat("/Example.Package/1.0/second.bin"_view);
  auto second_bytes = System::File::read(second_path.get_view());
  ASSERT(second_bytes);
  EXPECT_TEXT(*second_bytes, "second"_view);

  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}

PERIMORTEM_UNIT_TEST(PufferPublisher, rejects_escape_and_collision) {
  char root[] = "/tmp/puffer-publisher-invalid-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  Memory::Allocator::Arena arena;
  auto& escaping = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/../escape"_view, {});
  const Abstract* escaped[] = {&escaping};
  EXPECT_NOT(publish(arena, root_view, escaped));

  auto& first = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/api.hpp"_view, "first"_view);
  auto& second = Tetrodotoxin::Language::Product::create(
      arena, "Example.Package/1.0/api.hpp"_view, "second"_view);
  const Abstract* collided[] = {&first, &second};
  EXPECT_NOT(publish(arena, root_view, collided));
  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}
