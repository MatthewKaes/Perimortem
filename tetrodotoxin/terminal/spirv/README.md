# SPIR-V Terminal

Shader finishes with concrete GPU meaning, not target words. The SPIR-V
Terminal is where that meaning becomes a module with selected storage classes,
decorations, identifiers, and instructions.

The producer walks the real Library Functions and Flow retained by Shader, then
uses their exact Render interface, storage role, and Bridge relationships. It
does not ask Render to own a body, rebuild Library execution in another graph,
or feed generated words back into semantic meaning.

The word assembler beneath this folder owns the portable binary framing and
typed instruction spellings. Graph traversal and target policy will build on
that utility as the Terminal grows. Vulkan remains a later consumer that owns
device resources, commands, synchronization, and presentation.
