# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical multi Scene TTX application."""

load("//toolchain:tetrodotoxin.bzl", "ttx_application", "ttx_package")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "authored_sources",
    srcs = glob([
        "**/*.ttx",
        "resources/**",
    ]),
)

ttx_package(
    name = "package",
    package_name = "Tetrodotoxin.SceneLifetime",
    graphics_bindings = [
        "Graphics::Sprite|tetrodotoxin_graphics_sprite_placement_2d||tetrodotoxin_graphics_sprite_drawable_2d",
    ],
    graphics_placement = "Graphics::DrawableUI",
    manifest = "package.ttx",
    version = [
        1,
        0,
    ],
    deps = [
        "//packages/ttx:perimortem_graphics",
        "//packages/ttx:perimortem_math",
        "//packages/ttx:perimortem_system",
    ],
)

ttx_application(
    name = "scene_lifetime",
    app_member = "MainSource",
    package = ":package",
)
