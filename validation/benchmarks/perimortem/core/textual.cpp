// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/textual.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/writer/textual.hpp"

using namespace Perimortem::Core;
using namespace Validation;

// Buffer large enough for multiple reads/writes per iteration.
static constexpr Count io_buffer_size = 4096;
static Static::Bytes<io_buffer_size> io_buffer;

static Harness TextualBench = {
  .name = "Processing Text"_view,
};

PERIMORTEM_BENCHMARK(TextualBench, write_complex_entity) {
  Writer::Textual writer(io_buffer.get_access());
  while (writer.get_location() < io_buffer.get_size() - 256) {
    writer << "ent["_view << writer.get_location() << "] pos=("_view
           << Real_64(writer.get_location()) * 0.25 << ", "_view
           << Real_64(writer.get_location()) * 0.375 << ") hp="_view
           << Int(100 - Int(writer.get_location()) * 5) << "/"_view << Int(100)
           << " on="_view << Bool(writer.get_location() % 2 == 0);
  }
  Count loc = writer.get_location();
  Benchmark::prevent_optimization(loc);
}
