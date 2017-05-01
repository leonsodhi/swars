#include <SDL.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "game.h"
#include "util.h"

#if defined _WIN32 && defined main
// Anti SDL
# undef main
#endif

static void
print_help (const char *argv0)
{
  printf (
"Usage: %s [OPTIONS]\n\n"
"Available options:\n"
"  --windowed=WxH Run in windowed mode at specified resolution. Default is 640x480\n"
"  --fullwindow   Run in fullscreen borderless window mode\n"
"  --monitor=X    Display on monitor X\n"
"  --help         Display the help message\n",
  argv0);
}

static int
get_windowed_resolution(char* str, int *out_width, int *out_height)
{
  char* found = NULL;
  char *endptr = NULL;
  int width, height;

  strtolower(str);
  found = strstr(str, "x");
  if (!found) { return -1; }
  *found++ = '\0';

  width = (int)strtoul(str, &endptr, 10);
  if (endptr == str) { return -1; }
  height = (int)strtoul(found, &endptr, 10);
  if (endptr == str) { return -1; }

  if (width <= 0 || height <= 0)
    { return -1; }

  *out_width = width;
  *out_height = height;

  return 1;
}

static int
get_monitor(char* str, int *out_monitor)
{
  char *endptr = NULL;
  int monitor = 0;

  monitor = (int)strtoul(str, &endptr, 10);
  if (endptr == str || monitor < 0)
    { return -1; }

  *out_monitor = monitor;

  return 1;
}

static void
process_options (int *argc, char ***argv)
{
  int index;
  int val;
  char *argv0;

  static struct option options[] =
  {
    {"windowed",   optional_argument, NULL, 'w'},
    {"fullwindow", no_argument,       NULL, 'f'},
    {"monitor",    required_argument, NULL, 'm'},
    {"help",       no_argument,       NULL, 'h'},
    {NULL,         0, NULL,  0 },
  };

  argv0 = (*argv)[0];
  index = 0;

  while ((val = getopt_long (*argc, *argv, "w::fm:h", options, &index)) >= 0)
  {
    switch (val)
    {
      case 'w':
      {
        display_set_full_screen (false);
        if (optarg)
        {
          int width, height;
          if (get_windowed_resolution(optarg, &width, &height) == 1)
          {
            display_set_window_size(width, height);
          }
          else
          {
            fprintf (stderr, "WARNING: Window resolution ignored. Invalid values specified\n");
          }
        }
      }
      break;

      case 'f':
      {
        display_set_full_screen (false);
        display_set_borderless_full_screen (true);
      }
      break;

      case 'm':
      {
        int monitor;
        if (get_monitor(optarg, &monitor) == 1)
          { display_set_monitor(monitor); }
      }
      break;

      case 'h':
      {
        print_help (argv0);
        exit (0);
      }
      break;

      default:
        exit (1);
    }
  }

  *argc -= optind - 1;
  *argv += optind - 1;

  (*argv)[0] = argv0;
}

int
main (int argc, char **argv)
{
  int retval;

  display_set_full_screen (true);

  process_options (&argc, &argv);

  printf ("Syndicate Wars Port "VERSION"\n"
	  "The original by Bullfrog\n"
	  "Ported by Unavowed <unavowed@vexillium.org> "
	  "and Gynvael Coldwind <gynvael@vexillium.org>\n"
	  "Web site: http://swars.vexillium.org/\n");

  if (!game_initialise ())
    return 1;

  // Call game main
  asm volatile ("call asm_main\n"
		: "=a" (retval) : "a" (argc), "d" (argv));

  game_quit ();

  return retval;
}
