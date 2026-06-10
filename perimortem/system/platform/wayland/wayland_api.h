// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PeriWaylandWindow PeriWaylandWindow;

// C API marshaling type; used only as a return value to pass native handles
// to the Vulkan surface factory.
struct PeriWaylandHandles {
  void* display;
  void* surface;
};

PeriWaylandWindow* peri_wayland_create(
    unsigned int width,
    unsigned int height,
    const char* title);

void peri_wayland_destroy(PeriWaylandWindow* win);
int peri_wayland_poll(PeriWaylandWindow* win);

struct PeriWaylandHandles peri_wayland_get_handles(const PeriWaylandWindow* win);
unsigned int peri_wayland_get_width(const PeriWaylandWindow* win);
unsigned int peri_wayland_get_height(const PeriWaylandWindow* win);
unsigned int peri_wayland_get_scale(const PeriWaylandWindow* win);
int peri_wayland_needs_resize(const PeriWaylandWindow* win);
void peri_wayland_clear_resize(PeriWaylandWindow* win);

#ifdef __cplusplus
}
#endif
