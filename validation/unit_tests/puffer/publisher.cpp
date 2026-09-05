// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/publisher.hpp"

#include "validation/unit_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <ftw.h>
#include <utility>
#include <vector>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/language/product.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/query.hpp"

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
    Core::View::Bytes root,
    Core::View::Bytes publication_root,
    Core::View::Vector<const Abstract*> products) -> Bool {
  std::vector<Ttx::Layouts::Fluid::Entry> entries;
  entries.reserve(products.get_size());
  for (uint64_t index = 0; index < products.get_size(); ++index) {
    std::vector<uint8_t> path(sizeof(index));
    for (uint64_t byte = 0; byte < path.size(); ++byte) {
      path[byte] = uint8_t(index >> (byte * 8));
    }
    entries.push_back({
      .path = std::move(path),
      .producer = products[index]->get_handle(),
    });
  }
  Ttx::Layouts::Fluid layout(std::move(entries));
  const ttx_context context = ttx_context_create();
  const Ttx::PackObservation packed = Ttx::pack(context, layout.get_abi());
  const Bool published =
      packed.state == Ttx::PackObservationState::Packed &&
              Puffer::Publisher(root, publication_root).publish(packed.pack)
          ? True
          : False;
  context.operations->release(context);
  return published;
}

PERIMORTEM_UNIT_TEST(PufferPublisher, complete_pack_commit) {
  char root[] = "/tmp/puffer-publisher-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  Memory::Allocator::Arena arena;
  auto& first = Tetrodotoxin::Language::Product::create(
      arena, "first.bin"_view, "first"_view, True);
  const Abstract* first_product[] = {&first};
  ASSERT(publish(root_view, "Example.Package/1.0"_view, first_product));
  Memory::Managed::Bytes first_path(arena, root_view);
  first_path.concat("/Example.Package/1.0/first.bin"_view);
  auto first_bytes = System::File::read(first_path.get_view());
  ASSERT(first_bytes);
  EXPECT_TEXT(*first_bytes, "first"_view);
  Memory::Dynamic::Bytes terminated_first(first_path.get_view());
  terminated_first.append(0);
  struct stat first_status = {};
  ASSERT(
      stat(
          reinterpret_cast<const char*>(terminated_first.get_view().get_data()),
          &first_status) == 0);
  EXPECT((first_status.st_mode & S_IXUSR) != 0);

  auto& second = Tetrodotoxin::Language::Product::create(
      arena, "second.bin"_view, "second"_view);
  const Abstract* second_product[] = {&second};
  ASSERT(publish(root_view, "Example.Package/1.0"_view, second_product));
  EXPECT_NOT(System::File::read(first_path.get_view()));
  Memory::Managed::Bytes second_path(arena, root_view);
  second_path.concat("/Example.Package/1.0/second.bin"_view);
  auto second_bytes = System::File::read(second_path.get_view());
  ASSERT(second_bytes);
  EXPECT_TEXT(*second_bytes, "second"_view);
  Memory::Dynamic::Bytes terminated_second(second_path.get_view());
  terminated_second.append(0);
  struct stat second_status = {};
  ASSERT(
      stat(
          reinterpret_cast<const char*>(
              terminated_second.get_view().get_data()),
          &second_status) == 0);
  EXPECT((second_status.st_mode & S_IXUSR) == 0);

  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}

PERIMORTEM_UNIT_TEST(PufferPublisher, rejects_escape_and_collision) {
  char root[] = "/tmp/puffer-publisher-invalid-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  Memory::Allocator::Arena arena;
  auto& escaping =
      Tetrodotoxin::Language::Product::create(arena, "../escape"_view, {});
  const Abstract* escaped[] = {&escaping};
  EXPECT_NOT(publish(root_view, "Example.Package/1.0"_view, escaped));

  auto& first = Tetrodotoxin::Language::Product::create(
      arena, "api.hpp"_view, "first"_view);
  auto& second = Tetrodotoxin::Language::Product::create(
      arena, "api.hpp"_view, "second"_view);
  const Abstract* collided[] = {&first, &second};
  EXPECT_NOT(publish(root_view, "Example.Package/1.0"_view, collided));
  nftw(root, remove_entry, 32, FTW_DEPTH | FTW_PHYS);
}
