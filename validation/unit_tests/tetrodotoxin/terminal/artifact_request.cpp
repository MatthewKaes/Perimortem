// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/artifact_request.hpp"

#include "validation/unit_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <ftw.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "puffer/publisher.hpp"
#include "tetrodotoxin/terminal/query.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness ArtifactRequestTests = {
  .name = "Tetrodotoxin::Terminal::ArtifactRequest"_view,
};

static auto remove_entry(const char* path, const struct stat*, S32, struct FTW*)
    -> S32 {
  return remove(path);
}

PERIMORTEM_UNIT_TEST(ArtifactRequestTests, publishes_completed_bytes) {
  auto request = Terminal::ArtifactRequest::create(
      {'A', 'p', 'p', 'l', 'i', 'c', 'a', 't', 'i', 'o', 'n'},
      {'h', 'e', 'l', 'l', 'o'}, true);
  ASSERT(request);

  const auto observed = Terminal::observe(*request);
  ASSERT(observed.state == Terminal::ProductState::Produced);
  const auto closure = Terminal::observe_closure(observed.closure);
  ASSERT(closure.valid);
  EXPECT_EQ(closure.constants.size(), size_t(1));
  EXPECT(closure.authorities.empty());
  const auto cancelled = Terminal::cancel(*request);
  EXPECT(cancelled.state == Terminal::CancelState::AlreadySettled);

  char root[] = "/tmp/ttx-artifact-request-XXXXXX";
  ASSERT(mkdtemp(root) != nullptr);
  const Core::View::Bytes root_view = Core::NullTerminated::to_view(root);
  ASSERT(
      Puffer::Publisher(root_view, "Validation.Artifact/1.0"_view)
          .publish(observed.products));
  Memory::Allocator::Arena arena;
  Memory::Managed::Bytes path(arena, root_view);
  path.concat("/Validation.Artifact/1.0/Application"_view);
  auto output = System::File::read(path.get_view());
  ASSERT(output);
  EXPECT_TEXT(output->get_view(), "hello"_view);

  request->operations->release(request->self);
  nftw(root, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}
