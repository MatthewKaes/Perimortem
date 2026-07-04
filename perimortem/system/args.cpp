// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/args.hpp"

#ifdef PERI_LINUX
#include <unistd.h>
#endif

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;

using Configs = Managed::Map<View::Bytes, Args::Config>;

static auto basename(View::Bytes path) -> View::Bytes {
  for (Count i = path.get_size(); i > 0; i--) {
    if (path[i - 1] == '/' || path[i - 1] == '\\') {
      return path.slice(i);
    }
  }
  return path;
}

static auto process_name() -> View::Bytes {
#ifdef PERI_LINUX
  static Bits_8 path_buffer[512];
  Signed_64 size = readlink(
      "/proc/self/exe", Data::cast<char>(path_buffer), sizeof(path_buffer));
  if (size > 0) {
    return basename(View::Bytes(path_buffer, Count(size)));
  }
#endif
  return "process"_view;
}

static auto command_name(View::Vector<View::Bytes> arguments) -> View::Bytes {
  if (!arguments.is_empty() && !arguments[0].is_empty()) {
    return basename(arguments[0]);
  }

  return process_name();
}

static auto log_argument_error(View::Bytes message, View::Bytes detail)
    -> void {
  Diagnostics::Log::Message<256> error_message(
      Diagnostics::Log::Level::Error, Diagnostics::Source());
  error_message << message << ' ' << detail << '\n';
}

static auto split_argument(
    View::Bytes argument,
    View::Bytes& name,
    View::Bytes& value) -> Bool {
  Count equals = Algorithm::search(argument, "="_view);
  if (equals == Count(-1)) {
    name = argument;
    value = View::Bytes();
    return False;
  }

  name = argument.slice(0, equals);
  value = argument.slice(equals + 1);
  return True;
}

static auto starts_argument(View::Bytes argument) -> Bool {
  return !argument.is_empty() && argument[0] == '-';
}

static auto requested_help(View::Bytes argument) -> Bool {
  return argument == "--help"_view || argument == "-help"_view ||
         argument == "-h"_view;
}

static auto format_help(
    Allocator::Arena& arena,
    View::Bytes summary,
    const Configs& variables,
    View::Bytes command) -> Managed::Bytes {
  Count label_width = "-h, --help"_view.get_size();
  for (Count i = 0; i < variables.get_size(); i++) {
    const auto* variable = variables.get_entry(i);
    label_width = Math::max(label_width, variable->key.get_size());
  }
  label_width += 2;

  Managed::Bytes output(arena);
  output.concat("usage: "_view);
  output.concat(command);
  output.concat(" [arguments]\n\n"_view);

  if (!summary.is_empty()) {
    output.concat(summary);
    output.concat("\n\n"_view);
  }

  output.concat("arguments:\n"_view);
  for (Count i = 0; i < variables.get_size(); i++) {
    const auto* variable = variables.get_entry(i);

    output.concat("  "_view);
    output.concat(variable->key);
    output.append(Bits_8(' '), label_width - variable->key.get_size());
    output.concat(variable->value.help);
    output.append('\n');
  }

  output.concat("  -h, --help"_view);
  output.append(Bits_8(' '), label_width - "-h, --help"_view.get_size());
  output.concat("Show this help.\n"_view);
  return output;
}

static auto insert_proxy(
    Allocator::Arena& arena,
    Args::Values& values,
    View::Bytes name,
    View::Bytes value) -> void {
  auto* entry = values.find(name);
  if (entry == nullptr) {
    auto& list = arena.construct<Managed::Vector<View::Bytes>>(arena);
    auto& key = arena.construct<Managed::Bytes>(arena);
    key.proxy(name);
    entry = values.insert(key.get_view(), &list);
  }

  auto& stored = arena.construct<Managed::Bytes>(arena);
  stored.proxy(value);
  entry->value->insert(stored.get_view());
}

auto Args::parse(
    Allocator::Arena& arena,
    View::Bytes summary,
    Managed::Map<View::Bytes, Config> variables,
    View::Vector<View::Bytes> arguments) -> Values {
  Values values(arena);
  View::Bytes command = command_name(arguments);

  for (Count i = 1; i < arguments.get_size(); i++) {
    View::Bytes argument = arguments[i];
    if (requested_help(argument)) {
      Diagnostics::Log::info(
          format_help(arena, summary, variables, command).get_view(),
          Diagnostics::Source());
      return Values(arena);
    }

    View::Bytes name;
    View::Bytes value;
    Bool inline_value = split_argument(argument, name, value);

    const auto* variable = variables.find(name);
    if (variable == nullptr) {
      log_argument_error("unrecognized arg"_view, name);
      Diagnostics::Log::info(
          format_help(arena, summary, variables, command).get_view(),
          Diagnostics::Source());
      return Values(arena);
    }

    if (!inline_value && i + 1 < arguments.get_size() &&
        !starts_argument(arguments[i + 1])) {
      value = arguments[++i];
    }

    insert_proxy(arena, values, variable->key, value);
  }

  for (Count i = 0; i < variables.get_size(); i++) {
    const auto* variable = variables.get_entry(i);
    if (variable->value.required && !values.contains(variable->key)) {
      log_argument_error("missing required arg"_view, variable->key);
      Diagnostics::Log::info(
          format_help(arena, summary, variables, command).get_view(),
          Diagnostics::Source());
      return Values(arena);
    }
  }

  return values;
}
