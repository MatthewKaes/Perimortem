# Perimortem Vulkan

Tetrodotoxin windowed applications can use Perimortem Vulkan as their native
renderer. It takes backend independent draw data from Graphics and turns it
into Vulkan images, buffers, pipelines, commands, and presentation work. The
same renderer remains available to C++ applications that submit Graphics data.

The application owns the window and event loop. It gives the renderer a native
presentation surface and its current size. Window management stays in System,
while rendering stays in Vulkan.

For a Tetrodotoxin application, Vulkan receives every reachable compiled SPIR V
module, one generated Program description per module locator, and the selected
host surface. Shader describes executable flow and custom parameters. Pipeline
describes resources, base host inputs, and ordered Stage interfaces. Each frame
draw supplies topology, blending, geometry, and vertex count. Vulkan creates the
device objects, records commands, synchronizes work, and presents the result.

The Vulkan Description records beside `ShaderProgram` are target handoff values
derived from completed Pipeline, Shader, and SPIR-V products. They group borrowed
module words and physical pipeline layouts while Vulkan creates its owned
objects. Graphics submissions carry only the selected Program locator, copied
draw inputs, transforms, retained resources, and backend-neutral fixed state.

`Pipelines` realizes that generated table without changing it. It caches one
ShaderProgram per exact locator and fixed-state selection, negotiates required device features such as
Float64, uploads each retained Texture2D once, and records the already ordered
draws through the selected pipeline. Host roles fill the reflected transform
fields, while each Batch supplies the exact copied Parameters bytes expected by
its Program. Pixel data and frame order stay with Graphics.

Program descriptions and embedded words are generated products. Vulkan carries
no checked in Shader source, SPIR V array, Shader specific push structure, or
handwritten Program table.

These Vulkan details never become Shader, Pipeline, Library, or Package facts.
Wayland and Win32 supply different presentation surfaces through System, while
the renderer follows the same Graphics submission contract on either host.
