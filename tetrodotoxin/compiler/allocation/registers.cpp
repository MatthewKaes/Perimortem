// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/compiler/allocation/registers.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/utility/range.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Compiler;

Allocation::Registers::Registers(
    const Execution::Body& body,
    View::Vector<Count> source_widths,
    Count color_count) {
  View::Vector<Execution::Binding> bindings = body.get_bindings();
  if (source_widths.get_size() != bindings.get_size()) {
    Diagnostics::Log::fatal(
        "Register allocation requires one width for every binding."_view);
  }

  // Definitions begin a one-position interval. Uses extend that interval
  // below, which is conservative at operation granularity and keeps the pass
  // independent from target instruction scheduling.
  Dynamic::Vector<Range> ranges;
  for (Count i = 0; i < bindings.get_size(); i++) {
    if (source_widths[i] == 0) {
      Diagnostics::Log::fatal(
          "Register allocation does not accept zero-width bindings."_view);
    }

    ranges.insert({bindings[i].get_start(), 1});
    widths.insert(source_widths[i]);
  }

  auto use = [&](const Execution::Operand& operand, Count operation) {
    const Execution::Addressable* addressable =
        operand.find<Execution::Addressable>();
    if (addressable != nullptr && addressable->get_id() < ranges.get_size()) {
      ranges[addressable->get_id()].extend(operation);
    }
  };

  View::Vector<Execution::Operation> operations = body.get_operations();
  View::Vector<Execution::Operand> operands = body.get_operands();
  for (Count i = 0; i < operations.get_size(); i++) {
    const Count position = i + 1;
    operations[i].visit(
        []() {},
        [&](const Execution::Binary& binary) {
          use(binary.get_left(), position);
          use(binary.get_right(), position);
        },
        [&](const Execution::Call& call) {
          Range arguments = call.get_arguments();
          for (Count k = 0; k < arguments.size; k++) {
            use(operands[arguments.start + k], position);
          }
        },
        [&](const Execution::Return& result) {
          Range values = result.get_values();
          for (Count k = 0; k < values.size; k++) {
            use(operands[values.start + k], position);
          }
        });
  }

  // First-fit interval coloring favors a stable, inspectable result over a
  // pressure-optimal one. A candidate is rejected only when both its lifetime
  // and its consecutive color range overlap an earlier binding.
  colors.resize(bindings.get_size());
  spills.resize(bindings.get_size());
  for (Count binding = 0; binding < bindings.get_size(); binding++) {
    colors[binding] = Count(-1);
    spills[binding] = Count(-1);
    Count width = widths[binding];
    for (Count color = 0; color + width <= color_count; color++) {
      Bool available = True;
      for (Count prior = 0; prior < binding && available; prior++) {
        if (colors[prior] == Count(-1) ||
            !ranges[binding].has_overlap(ranges[prior])) {
          continue;
        }

        Count prior_start = colors[prior];
        Count prior_end = prior_start + widths[prior];
        Count candidate_end = color + width;
        if (color < prior_end && prior_start < candidate_end) {
          available = False;
        }
      }

      if (available) {
        colors[binding] = color;
        break;
      }
    }

    if (colors[binding] == Count(-1)) {
      spills[binding] = spill_count;
      spill_count += width;
    }
  }
}

auto Allocation::Registers::get_color(Count binding, Count component) const
    -> Count {
  if (binding >= colors.get_size() || component >= widths[binding]) {
    return Count(-1);
  }

  return colors[binding] == Count(-1) ? Count(-1) : colors[binding] + component;
}

auto Allocation::Registers::get_spill(Count binding, Count component) const
    -> Count {
  if (binding >= spills.get_size() || component >= widths[binding] ||
      spills[binding] == Count(-1)) {
    return Count(-1);
  }

  return spills[binding] + component;
}
