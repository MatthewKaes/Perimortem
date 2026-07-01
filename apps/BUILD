"""
Demo binaries for Perimortem.

Run the icon fade demo:
  bazel run //apps:splash_screen
"""

load("//tetrodotoxin:ttx.bzl", "ttx_binary")

package(default_visibility = ["//visibility:public"])

ttx_binary(
    name = "splash_screen",
    srcs = ["splash_screen.ttx"],
    entry_module = "splash_screen",
    entry = "main",
    foreign_config_type = "TTX_splash_screen_SceneConfig",
    foreign_sprite_type = "TTX_splash_screen_Sprite",
    foreign_symbol = "run_sprite_fade_scene",
    render_deps = [],
    render_include = "ttx_generated/graphics_package.hpp",
    render_import_name = "Default2D",
    render_symbol = "default_2d_renderer",
    ttx_deps = ["//tetrodotoxin:graphics_package"],
)
