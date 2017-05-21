#include <stdint.h>
#include <stdio.h>

#include "display.h"
#include "util.h"

// Internal mode list
struct Display_InternalModeEntry
{
  uint16_t width;      // e.g. 320
  uint16_t height;     // e.g. 200
  uint8_t  bpp;        // e.g. 8
  uint8_t  padding;
  uint32_t noidea1;
  uint32_t mode;       // e.g. 13h
  char     name[0x18]; // e.g. MODE_320_200_8
} __attribute__((packed));

typedef struct Display_InternalModeEntry Display_InternalModeEntry;

extern Display_InternalModeEntry internal_graphic_mode_list[];

// Display buffer
extern void *display_buffer;

// Width/height stuff
extern uint32_t display_width, display_height;
extern uint32_t display_vesa_width, display_vesa_height;

// Misc stuff
extern uint16_t display_initial_mode, data_1e2eb4, display_mode;
extern uint8_t data_1e2ec5;
extern bool display_extended_mode;
extern uint32_t display_good;

// Drawing of the mouse cursor
extern int32_t mouse_pointer_x;
extern int32_t mouse_pointer_y;
extern int32_t mouse_pointer_width;
extern int32_t mouse_pointer_height;

static SDL_Window *display_window = NULL;
static SDL_Renderer *display_renderer = NULL;
static SDL_Surface *display_screen;
static SDL_Texture *display_texture = NULL;
static int display_texture_width;
static int display_texture_height;
static SDL_PixelFormat *rgba_8888_texture_pixel_format;
static bool	display_full_screen = false;
static bool display_borderless_full_screen = false;
static int display_window_width = 640;
static int display_window_height = 480;
static int display_monitor = 0;
static SDL_Color    display_palette[256];

static void
call_e0d00 (int x, int y, int w, int h)
{
  asm volatile
    ("push %%ebx;"
     "mov  %2,%%ebx;"
     "call func_e0d00;"
     "pop  %%ebx"
     : : "a" (x), "d" (y), "g" (w), "c" (h));
}

static void
call_ef4f0 (int x, int y, int w, int h)
{
  asm volatile
    ("push %%ebx;"
     "mov  %2,%%ebx;"
     "call func_ef4f0;"
     "pop  %%ebx"
     : : "a" (x), "d" (y), "g" (w), "c" (h));
}

// call e9498 aka display_set_mode_setup_mouse
static void
call_display_set_mode_setup_mouse (void)
{
  asm volatile ("call display_set_mode_setup_mouse");
}

static inline void
lock_screen (void)
{
  if (!SDL_MUSTLOCK (display_screen))
    return;

  if (SDL_LockSurface (display_screen) != 0)
    fprintf (stderr, "SDL_LockSurface: %s\n", SDL_GetError ());
}

static inline void
unlock_screen (void)
{
  if (!SDL_MUSTLOCK (display_screen))
    return;

  SDL_UnlockSurface (display_screen);
}

int
display_set_palette (const uint8_t *palette)
{
  SDL_Color colours[256];
  int n;
  int ret;

  for (n = 0; n < 256; n++)
    {
      colours[n].r = palette[3 * n + 0] * 4;
      colours[n].g = palette[3 * n + 1] * 4;
      colours[n].b = palette[3 * n + 2] * 4;
      colours[n].a = 255;
    }

  ret = SDL_SetPaletteColors(
    display_screen->format->palette,
    colours,
    0,
    256);
  if (ret != 0)
  {
    fprintf (stderr, "SDL_SetPaletteColors: %s\n", SDL_GetError ());
    return -1;
  }

  memcpy (display_palette, colours, sizeof (display_palette));

  return 1;
}

int
display_set_mode (uint16_t mode, uint32_t width, uint32_t height,
		  const uint8_t *palette)
{
  if(internal_graphic_mode_list[mode].bpp != 8)
  {
    fprintf (stderr, "Bpp wasn't 8! Actual bpp: %d\n", internal_graphic_mode_list[mode].bpp);
    goto err;
  }

  // call func_e9498
  call_display_set_mode_setup_mouse ();

  // display_initial_mode which is DWORD 1E2EB6 is used in
  // 000ED764 sub_ED764 to probably get back to text mode
  // I'm setting it to 0xFF for now
  if (!display_initial_mode)
    display_initial_mode = 0xFF;

  display_extended_mode = false;

  // Setting mode
  if (display_screen != NULL)
    {
      unlock_screen ();
      SDL_FreeSurface (display_screen);
    }

  if (display_texture != NULL)
  {
      SDL_DestroyTexture(display_texture);
  }


  // Init mode
  /*display_screen = SDL_SetVideoMode (width, height,
             internal_graphic_mode_list[mode].bpp,
             surface_flags);*/

  // Assumes 8-bit colour in all cases
  display_screen = SDL_CreateRGBSurfaceWithFormat(
    0,
    width, height,
    8,
    SDL_PIXELFORMAT_INDEX8);

  if (display_screen == NULL) {
    fprintf (stderr, "SDL_CreateRGBSurface() failed: %s", SDL_GetError());
    goto err;
  }

  display_texture = SDL_CreateTexture(
    display_renderer,
    SDL_PIXELFORMAT_RGBA8888,
    SDL_TEXTUREACCESS_STREAMING,
    width, height);

  if (display_texture == NULL) {
    fprintf (stderr, "SDL_CreateTexture() failed: %s", SDL_GetError());
    goto err;
  }


#ifdef ENABLE_DEBUG
  printf ("SDL_CreateRGBSurfaceWithFormat(0, %i, %i, %i, pixel_format) - %s\n",
          width, height, internal_graphic_mode_list[mode].bpp,
          internal_graphic_mode_list[mode].name);
#endif


  if (SDL_QueryTexture(display_texture, NULL, NULL, &display_texture_width, &display_texture_height) != 0)
  {
    fprintf (stderr, "SDL_QueryTexture failed %s\n", SDL_GetError());
    goto err;
  }

  lock_screen ();

  display_buffer = display_screen->pixels;

  // Setup some global variables
  display_vesa_width  = internal_graphic_mode_list[mode].width;
  display_vesa_height = internal_graphic_mode_list[mode].height;
  display_width  = width;
  display_height = height;
  display_mode   = mode;

  // No idea what is this
  // TODO: check if something breaks if this is removed
  data_1e2eb4 = 0;
  data_1e2ec5 = 0;

  // Call funcitons that recalculate some buffers
  // They can be switched to C++ later, but it's not needed
  call_e0d00 (0, 0, display_width, display_height);
  call_ef4f0 (0, 0, display_width, display_height);

  // Setup palette
  if (palette != NULL)
    {
      if (display_set_palette(palette) != 1)
      goto err;
    }

  display_good = true;

  return 1;

err:
  if (display_screen != NULL)
  {
    unlock_screen ();
    SDL_FreeSurface (display_screen);
    display_screen = NULL;
  }

  if (display_texture)
  {
      SDL_DestroyTexture(display_texture);
      display_texture = NULL;
  }

  display_good = false;

  return -1;
}

void
display_update_mouse_pointer (void)
{
  int x, y;
  int c, r;
  int *texture_pixels;
  int texture_pitch;
  unsigned char* display_screen_pixels = display_screen->pixels;

  x = MAX (0, mouse_pointer_x);
  y = MAX (0, mouse_pointer_y);

  /*SDL_UpdateRect (display_screen, x, y,
		  mouse_pointer_width, mouse_pointer_height);*/

  SDL_LockTexture(display_texture, NULL, (void*)&texture_pixels, &texture_pitch);
  for (r = y; r < y + mouse_pointer_height; r++)
  {
    int row_start_index = (r * display_texture_width) + x;
    for(c = row_start_index; c < row_start_index + mouse_pointer_width; c++)
    {
      texture_pixels[c] = SDL_MapRGBA(
        rgba_8888_texture_pixel_format,
        display_palette[display_screen_pixels[c]].r,
        display_palette[display_screen_pixels[c]].g,
        display_palette[display_screen_pixels[c]].b,
        display_palette[display_screen_pixels[c]].a);
    }
  }
  SDL_UnlockTexture(display_texture);

  SDL_RenderClear(display_renderer);
  SDL_RenderCopy(display_renderer, display_texture, NULL, NULL);
  SDL_RenderPresent(display_renderer);
}

void
display_update (void)
{
  int *texture_pixels;
  int texture_pitch;
  int i;
  unsigned char* display_screen_pixels = display_screen->pixels;

  SDL_LockTexture(display_texture, NULL, (void*)&texture_pixels, &texture_pitch);
  for(i = 0; i < display_texture_width * display_texture_height; i++)
  {
    texture_pixels[i] = SDL_MapRGBA(
      rgba_8888_texture_pixel_format,
      display_palette[display_screen_pixels[i]].r,
      display_palette[display_screen_pixels[i]].g,
      display_palette[display_screen_pixels[i]].b,
      display_palette[display_screen_pixels[i]].a);
  }
  SDL_UnlockTexture(display_texture);

  SDL_RenderClear(display_renderer);
  SDL_RenderCopy(display_renderer, display_texture, NULL, NULL);
  SDL_RenderPresent(display_renderer);
}

int
display_initialise (void)
{
  uint32_t window_flags = 0;
  int num_displays = 1;
  SDL_DisplayMode display_mode;
  SDL_bool ret = SDL_TRUE;

  window_flags = SDL_WINDOW_MOUSE_CAPTURE;

  if (display_full_screen)
    { window_flags |= SDL_WINDOW_FULLSCREEN; }

  if (display_borderless_full_screen)
  {
    // SDL_WINDOW_FULLSCREEN_DESKTOP didn't work for me on Windows 7 (64-bit)
    window_flags |= SDL_WINDOW_BORDERLESS;

    if (SDL_GetDesktopDisplayMode(display_monitor, &display_mode) != 0)
    {
      fprintf (stderr, "SDL_GetDesktopDisplayMode failed: %s\n", SDL_GetError());
      goto err;
    }

    display_window_width = display_mode.w;
    display_window_height = display_mode.h;
  }

  if (display_monitor > 0)
  {
    num_displays = SDL_GetNumVideoDisplays();
    if (display_monitor > (num_displays - 1))
    {
      fprintf (stderr, "Invalid monitor selected\n");
      goto err;
    }
  }

  display_window = SDL_CreateWindow(
    "Syndicate Wars",
    SDL_WINDOWPOS_CENTERED_DISPLAY(display_monitor),
    SDL_WINDOWPOS_CENTERED_DISPLAY(display_monitor),
    display_window_width, display_window_height,
    window_flags);
  if (NULL == display_window)
  {
     fprintf (stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
     goto err;
  }

  display_renderer = SDL_CreateRenderer(display_window, -1, 0);
  if (NULL == display_renderer)
  {
    fprintf (stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    goto err;
  }

  rgba_8888_texture_pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
  if (NULL == rgba_8888_texture_pixel_format)
  {
    fprintf (stderr, "SDL_AllocFormat failed %s\n", SDL_GetError());
    goto err;
  }

  ret = SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
  if (ret != SDL_TRUE)
  {
    fprintf (stderr, "WARNING: SDL_HINT_RENDER_SCALE_QUALITY failed\n");
  }

  return 1;

err:
  if (display_window)
  {
    SDL_DestroyWindow(display_window);
    display_window = NULL;
  }
  if (display_renderer)
  {
    SDL_DestroyRenderer(display_renderer);
    display_renderer = NULL;
  }
  if (rgba_8888_texture_pixel_format)
  {
    SDL_FreeFormat(rgba_8888_texture_pixel_format);
    rgba_8888_texture_pixel_format = NULL;
  }
  return -1;
}

void
display_finalise (void)
{
  unlock_screen ();
  SDL_FreeSurface (display_screen);
  display_screen = NULL;
  display_buffer = NULL;

  if (display_window)
  {
    SDL_DestroyWindow(display_window);
    display_window = NULL;
  }
  if (display_renderer)
  {
    SDL_DestroyRenderer(display_renderer);
    display_renderer = NULL;
  }
  if (rgba_8888_texture_pixel_format)
  {
    SDL_FreeFormat(rgba_8888_texture_pixel_format);
    rgba_8888_texture_pixel_format = NULL;
  }
  if (display_texture)
  {
      SDL_DestroyTexture(display_texture);
      display_texture = NULL;
  }
}

void
display_set_full_screen (bool full_screen)
{
  if (display_screen != NULL)
    return;

  display_full_screen = full_screen;
}

void display_set_borderless_full_screen(bool borderless_full_screen)
{
  if (display_screen != NULL)
    return;

  display_borderless_full_screen = borderless_full_screen;
}

void display_set_window_size(int window_width, int window_height)
{
  if (window_width < 640 || window_height < 480)
  {
    fprintf (stderr, "WARNING: Window resolution ignored. Minimum is 640x480\n");
    return;
  }

  display_window_width = window_width;
  display_window_height = window_height;
}

void display_set_monitor(int monitor)
{
  display_monitor = monitor;
}

void
display_get_size (size_t *width, size_t *height)
{
  if (display_buffer == NULL)
    {
      if (width != NULL)
        *width  = 0;

      if (height != NULL)
        *height = 0;

      return;
    }

  if (width != NULL)
    *width  = display_width;

  if (height != NULL)
    *height = display_height;
}

void
display_get_physical_size (size_t *width, size_t *height)
{
  if (display_buffer == NULL || display_screen == NULL)
    {
      if (width != NULL)
        *width  = 0;

      if (height != NULL)
        *height = 0;

      return;
    }

  if (width != NULL)
    *width  = display_window_width;

  if (height != NULL)
    *height = display_window_height;
}

void *
display_get_buffer (void)
{
  return display_buffer;
}

void
display_get_palette (SDL_Color *colours)
{
  memcpy (colours, display_palette, sizeof (display_palette));
}

void
display_lock (void)
{
  lock_screen ();
}

void
display_unlock (void)
{
  unlock_screen ();
}
