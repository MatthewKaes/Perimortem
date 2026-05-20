// Perimortem Engine
// Copyright © Matt Kaes

// Should only be included in cpp files — including this in a header would
// pull source location machinery into every translation unit that includes it.

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/diagnostics/source.hpp"

namespace Perimortem::Diagnostics {

class Log {
 public:
  enum class Level : Byte {
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
  };

  // Receives the fully formatted log message as a byte view.
  // The sink function is local to each thread.
  using Sink = void (*)(Level level, Core::View::Bytes message);

  // The default sink function the logger uses for each thread.
  // Creates a canonical file for the thread the first time sink is called
  // for the thread.
  static auto file_sink(Level level, Core::View::Bytes message) -> void;

  // Causes the thread to only log messages to the console (stdout / stderr).
  // Writes to the console are thread safe to prevent multiple threads from
  // creating garbage output.
  // Debug, Info and Warning are written to stdout.
  // Error and Fatal log to stderr.
  static auto console_sink(Level level, Core::View::Bytes message) -> void;

  // Uses the console sink but writes colored text.
  // Grey for debug, white for info, yellow for warning, red for error and
  // fatal.
  static auto color_sink(Level level, Core::View::Bytes message) -> void;

  // Logs to both the console_sink and the file_sink for real time and
  // persistant logs.
  static auto debug_sink(Level level, Core::View::Bytes message) -> void;

  // Sets the function to forward log messages to on this thread.
  // By default the sink is set to a file on disk named after the thread.
  // If the sink is set to null then messages for the thread are dropped.
  static auto set_sink(Sink sink) -> void;

  // Sets the minimum level that will be forwarded to the sink on this thread.
  // Messages below this level are dropped before formatting.
  static auto set_level(Level level) -> void;

  // Logging calls are thread-safe
  // Each thread writes to its own buffer and file.
  // The Source default argument is consteval, so it captures the caller's
  // location at compile time with zero runtime cost.
  // Source is passed by reference to make it easier for the TTX ABI.
  static auto log(Level level, Core::View::Bytes msg, const Source& location)
      -> void;

  static auto debug(
      Core::View::Bytes msg,
      const Source& location = Source::current()) -> void;
  static auto info(
      Core::View::Bytes msg,
      const Source& location = Source::current()) -> void;
  static auto warning(
      Core::View::Bytes msg,
      const Source& location = Source::current()) -> void;
  static auto error(
      Core::View::Bytes msg,
      const Source& location = Source::current()) -> void;

  // Fatal logs flush, print a stack trace to stderr, then abort the process.
  // Mark it as [[noreturn]] to help the compiler handle fatal call paths.
  [[noreturn]] static auto fatal(
      Core::View::Bytes msg,
      const Source& location = Source::current()) -> void;

  // Flushes the calling thread's message buffer.
  // Stdout writes are guaranteed to be thread safe to avoid output mangling.
  // Flush is called automatically whenever the message buffer is close to being
  // filled or when the thread exits.
  static auto flush() -> void;
};

}  // namespace Perimortem::Diagnostics
