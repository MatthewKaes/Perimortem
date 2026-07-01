// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/reader/binary.hpp"

#include "validation/benchmark.hpp"

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/system/random.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Validation;

static Static::Bytes<1 << 12> io_buffer;

// Make sure keys are randomized
auto fill_buffer() -> void {
  Count* data = Data::cast<Count>(io_buffer.get_data());
  for (Count i = 0; i < io_buffer.get_size() / 8; i++) {
    data[i] = Random::generate();
  }
}

static Harness BinaryBench = {
  .name = "Binary I/O"_view,
  .setup = fill_buffer,
};

PERIMORTEM_BENCHMARK(BinaryBench, binary_read_32) {
  Reader::Binary<Data::ByteOrder::Little> reader(io_buffer.get_view());
  Bits_32 accumulator = 0;

  while (reader.get_location() < io_buffer.get_size() - 64) {
    accumulator ^= reader.read_bits_32();
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(BinaryBench, binary_read_64) {
  Reader::Binary<Data::ByteOrder::Little> reader(io_buffer.get_view());
  Bits_64 accumulator = 0;

  while (reader.get_location() < io_buffer.get_size() - 64) {
    accumulator ^= reader.read_bits_64();
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(BinaryBench, binary_read_view) {
  Reader::Binary<Data::ByteOrder::Little> reader(io_buffer.get_view());
  Count accumulator = 0;

  while (reader.get_location() < io_buffer.get_size() - 64) {
    auto chunk = reader.read_bytes(16);
    accumulator ^= chunk.get_size();
  }

  Benchmark::prevent_optimization(accumulator);
}

PERIMORTEM_BENCHMARK(BinaryBench, binary_write_32) {
  Writer::Binary<Data::ByteOrder::Little> writer(io_buffer.get_access());

  while (writer.get_location() < io_buffer.get_size() - 64) {
    writer << Bits_32(writer.get_location());
  }

  Count writer_location = writer.get_location();
  Benchmark::prevent_optimization(writer_location);
}

PERIMORTEM_BENCHMARK(BinaryBench, binary_write_64) {
  Writer::Binary<Data::ByteOrder::Little> writer(io_buffer.get_access());

  while (writer.get_location() < io_buffer.get_size() - 64) {
    writer << Bits_64(writer.get_location());
  }

  Count writer_location = writer.get_location();
  Benchmark::prevent_optimization(writer_location);
}

PERIMORTEM_BENCHMARK(BinaryBench, binary_write_view) {
  Writer::Binary<Data::ByteOrder::Little> writer(io_buffer.get_access());

  while (writer.get_location() < io_buffer.get_size() - 64) {
    writer << "16 characters!!!"_view;
  }

  Count writer_location = writer.get_location();
  Benchmark::prevent_optimization(writer_location);
}
