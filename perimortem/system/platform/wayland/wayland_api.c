// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/platform/wayland/wayland_api.h"

#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include "perimortem/system/platform/wayland/xdg-shell-client-protocol.h"

struct PeriWaylandWindow {
  struct wl_display* display;
  struct wl_registry* registry;
  struct wl_compositor* compositor;
  struct wl_surface* surface;
  struct xdg_wm_base* wm_base;
  struct xdg_surface* xdg_surf;
  struct xdg_toplevel* toplevel;

  unsigned int logical_width;
  unsigned int logical_height;
  unsigned int initial_width;
  unsigned int initial_height;
  unsigned int scale;

  int close_requested;
  int needs_resize;
};

static void on_wm_base_ping(
    void* data,
    struct xdg_wm_base* wm_base,
    uint32_t serial) {
  (void)data;
  xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
  .ping = on_wm_base_ping,
};

static void on_xdg_surface_configure(
    void* data,
    struct xdg_surface* surf,
    uint32_t serial) {
  struct PeriWaylandWindow* win = data;
  xdg_surface_ack_configure(surf, serial);
  wl_surface_commit(win->surface);
}

static const struct xdg_surface_listener xdg_surf_listener = {
  .configure = on_xdg_surface_configure,
};

static void on_toplevel_configure(
    void* data,
    struct xdg_toplevel* toplevel,
    int32_t w,
    int32_t h,
    struct wl_array* states) {
  (void)toplevel;
  (void)states;
  struct PeriWaylandWindow* win = data;
  unsigned int new_w = w > 0 ? (unsigned int)w : win->initial_width;
  unsigned int new_h = h > 0 ? (unsigned int)h : win->initial_height;
  if (new_w != win->logical_width || new_h != win->logical_height) {
    win->logical_width = new_w;
    win->logical_height = new_h;
    win->needs_resize = 1;
  }
}

static void on_toplevel_close(void* data, struct xdg_toplevel* toplevel) {
  (void)toplevel;
  ((struct PeriWaylandWindow*)data)->close_requested = 1;
}

static void on_toplevel_configure_bounds(
    void* data,
    struct xdg_toplevel* tl,
    int32_t w,
    int32_t h) {
  (void)data;
  (void)tl;
  (void)w;
  (void)h;
}

static void on_toplevel_wm_capabilities(
    void* data,
    struct xdg_toplevel* tl,
    struct wl_array* caps) {
  (void)data;
  (void)tl;
  (void)caps;
}

static const struct xdg_toplevel_listener toplevel_listener = {
  .configure = on_toplevel_configure,
  .close = on_toplevel_close,
  .configure_bounds = on_toplevel_configure_bounds,
  .wm_capabilities = on_toplevel_wm_capabilities,
};

static void on_surface_enter(
    void* data,
    struct wl_surface* surf,
    struct wl_output* output) {
  (void)data;
  (void)surf;
  (void)output;
}

static void on_surface_leave(
    void* data,
    struct wl_surface* surf,
    struct wl_output* output) {
  (void)data;
  (void)surf;
  (void)output;
}

static void on_surface_preferred_buffer_scale(
    void* data,
    struct wl_surface* surf,
    int32_t factor) {
  (void)surf;
  struct PeriWaylandWindow* win = data;
  if ((unsigned int)factor != win->scale) {
    win->scale = (unsigned int)factor;
    win->needs_resize = 1;
  }
}

static void on_surface_preferred_buffer_transform(
    void* data,
    struct wl_surface* surf,
    uint32_t transform) {
  (void)data;
  (void)surf;
  (void)transform;
}

static const struct wl_surface_listener surface_listener = {
  .enter = on_surface_enter,
  .leave = on_surface_leave,
  .preferred_buffer_scale = on_surface_preferred_buffer_scale,
  .preferred_buffer_transform = on_surface_preferred_buffer_transform,
};

static void on_registry_global(
    void* data,
    struct wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version) {
  (void)version;
  struct PeriWaylandWindow* win = data;
  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    win->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, 6);
  } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
    win->wm_base =
        wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    xdg_wm_base_add_listener(win->wm_base, &wm_base_listener, win);
  }
}

static void on_registry_global_remove(
    void* data,
    struct wl_registry* registry,
    uint32_t name) {
  (void)data;
  (void)registry;
  (void)name;
}

static const struct wl_registry_listener registry_listener = {
  .global = on_registry_global,
  .global_remove = on_registry_global_remove,
};

PeriWaylandWindow* peri_wayland_create(
    unsigned int width,
    unsigned int height,
    const char* title) {
  struct PeriWaylandWindow* win = calloc(1, sizeof(struct PeriWaylandWindow));
  win->initial_width = width;
  win->initial_height = height;
  win->logical_width = width;
  win->logical_height = height;
  win->scale = 1;

  win->display = wl_display_connect(NULL);
  win->registry = wl_display_get_registry(win->display);
  wl_registry_add_listener(win->registry, &registry_listener, win);

  wl_display_roundtrip(win->display);

  win->surface = wl_compositor_create_surface(win->compositor);
  wl_surface_add_listener(win->surface, &surface_listener, win);

  win->xdg_surf =
      xdg_wm_base_get_xdg_surface(win->wm_base, win->surface);
  xdg_surface_add_listener(win->xdg_surf, &xdg_surf_listener, win);

  win->toplevel = xdg_surface_get_toplevel(win->xdg_surf);
  xdg_toplevel_add_listener(win->toplevel, &toplevel_listener, win);
  xdg_toplevel_set_title(win->toplevel, title);
  xdg_toplevel_set_app_id(win->toplevel, "perimortem");

  wl_surface_commit(win->surface);
  wl_display_roundtrip(win->display);

  return win;
}

void peri_wayland_destroy(PeriWaylandWindow* win) {
  if (!win) {
    return;
  }
  if (win->toplevel) {
    xdg_toplevel_destroy(win->toplevel);
  }
  if (win->xdg_surf) {
    xdg_surface_destroy(win->xdg_surf);
  }
  if (win->surface) {
    wl_surface_destroy(win->surface);
  }
  if (win->wm_base) {
    xdg_wm_base_destroy(win->wm_base);
  }
  if (win->compositor) {
    wl_compositor_destroy(win->compositor);
  }
  if (win->registry) {
    wl_registry_destroy(win->registry);
  }
  if (win->display) {
    wl_display_disconnect(win->display);
  }
  free(win);
}

int peri_wayland_poll(PeriWaylandWindow* win) {
  if (!win || win->close_requested) {
    return 0;
  }
  return wl_display_dispatch_pending(win->display) >= 0 &&
         wl_display_flush(win->display) >= 0;
}

struct PeriWaylandHandles peri_wayland_get_handles(
    const PeriWaylandWindow* win) {
  struct PeriWaylandHandles h;
  h.display = win->display;
  h.surface = win->surface;
  return h;
}

unsigned int peri_wayland_get_width(const PeriWaylandWindow* win) {
  return win->logical_width;
}

unsigned int peri_wayland_get_height(const PeriWaylandWindow* win) {
  return win->logical_height;
}

unsigned int peri_wayland_get_scale(const PeriWaylandWindow* win) {
  return win->scale;
}

int peri_wayland_needs_resize(const PeriWaylandWindow* win) {
  return win->needs_resize;
}

void peri_wayland_clear_resize(PeriWaylandWindow* win) {
  win->needs_resize = 0;
}
