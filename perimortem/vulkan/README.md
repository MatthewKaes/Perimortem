# Perimortem Vulkan

Vulkan is Perimortem's concrete Vulkan renderer. It depends on Graphics for
backend-independent image and render data, then owns the Vulkan instance,
device, surface, swapchain, pipelines, textures, commands, and synchronization
needed to execute that data.

Vulkan does not own an operating-system window or event loop. The application
runtime owns those System concerns and supplies the native presentation handles
and physical extent when it constructs a renderer. This keeps the dependency
direction explicit: the runtime selects Vulkan, and Vulkan consumes Graphics.
