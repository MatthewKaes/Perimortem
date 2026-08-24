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
Render describes the GPU facing data. Vulkan then chooses the concrete resource
bindings, creates the device objects, records commands, synchronizes work, and
presents the result.

The Vulkan Description records beside `ShaderProgram` are target handoff values
derived from completed Render, Shader, and SPIR-V products. They group borrowed
module words and physical pipeline layouts while Vulkan creates its owned
objects. Graphics submissions carry only the selected Program locator, copied
draw inputs, transforms, and retained resources.

The standard Sprite path realizes that contract without changing it. A
SpriteRenderer matches one Program product, uploads each retained Image once,
records the already ordered draws, and releases a cached Texture after its real
Image identity has no remaining owner. Pixel data and frame order stay with
Graphics while descriptor layouts and push values remain Vulkan decisions.

The native Sprite Program publishes an ordinary Vulkan description over
embedded SPIR-V modules. A Package generated Shader can replace those module
words while keeping the same process lifetime locator and description boundary,
so the renderer does not need to know whether C++ or TTX produced the Program.

These Vulkan details never become Shader, Render, Library, or Package facts.
Wayland and Win32 supply different presentation surfaces through System, while
the renderer follows the same Graphics submission contract on either host.
