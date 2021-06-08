// Copyright (c) 2021 Armin Biere Johannes Kepler University Linz Austria

static const char * usage = "usage: satsort [-h] [ <input> ]\n";

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
die (const char * msg, ...)
{
  va_list ap;
  fputs ("satsort: error: ", stderr);
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

int
main (int argc, char ** argv)
{
  const char * path = 0;
  for (int i = 1; i < argc; i++)
    {
      const char * arg = argv[i];
      if (!strcmp (arg, "-h"))
	fputs (usage, stdout), exit (0);
      else if (*arg == '-')
	die ("invalid option '%s' (try '-h')", arg);
      else if (path)
	die ("multiple inputs '%s' and '%s'", path, arg);
      else
	path = arg;
    }
  return 0;
}
