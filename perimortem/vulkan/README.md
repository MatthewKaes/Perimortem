# Perimortem Vulkan

Tetrodotoxin windowed applications can use Perimortem Vulkan as their native
renderer. It takes backend independent draw data from Graphics and turns it
into Vulkan images, buffers, pipelines, commands, and presentation work. The
same renderer remains available to C++ applications that submit Graphics data.

The application owns the window and event loop. It gives the renderer a native
presentation surface and its current size. Window management stays in System,
while rendering stays in Vulkan.

For a Tetrodotoxin application, Vulkan receives the compiled SPIR-V module and
the selected host surface. Shader describes how CPU and GPU data coordinate.
Render describes the GPU-facing data. Vulkan then chooses the concrete resource
bindings, creates the device objects, records commands, synchronizes work, and
presents the result.

These Vulkan details never become Shader, Render, Library, or Package facts.
Wayland and Win32 supply different presentation surfaces through System, while
the renderer follows the same Graphics submission contract on either host.
