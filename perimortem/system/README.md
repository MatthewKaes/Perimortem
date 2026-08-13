# Perimortem System

Perimortem System provides the operating-system services used by applications.
This includes process startup, arguments, files, paths, terminals, clocks,
input, windows, and event loops.

The public System API does not expose Linux, Windows, Wayland, or Win32 handles.
Those details stay inside the selected platform backend.

## CPU targets and platform hosts

The CPU target and the platform host answer different questions:

* The CPU target defines instructions, data layout, calling conventions, and
  native value representation.
* The platform host defines process startup, loading, terminal behavior,
  windows, and events.

For example, an x86-64 System V target normally pairs with the Linux host. An
x86-64 Win64 target pairs with the Windows host. A build names both, and an
incompatible pair is rejected.

Library compilation depends on the CPU target, not on a window system. Linker
then combines the compiled objects with the selected host support to produce an
ELF program for Linux or a PE program for Windows. LLVM and the direct native
compiler use the same target and host boundary.

## Platform backends

The Linux backend provides Linux process and terminal support together with a
Wayland window and event loop. The Windows backend provides the matching
services through Win32. Both implement the same System window, input snapshot,
and presentation-surface contracts.

Application startup selects one backend. It may pass that backend's native
surface to Vulkan, but Vulkan does not take ownership of the window or event
loop. Graphics, Render, and Shader remain independent of the host platform.

## Runtime values

System finishes collecting process arguments before the Program entry function
runs. During a windowed frame, it finishes one input snapshot and one monotonic
time delta before Scene update begins.

Platform events, file descriptors, window handles, and process addresses remain
runtime details. Standard `Perimortem.System` Package functions expose ordinary
language values instead of turning those native handles into TTX identities.
