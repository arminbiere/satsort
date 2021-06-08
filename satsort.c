// Copyright (c) 2021 Armin Biere Johannes Kepler University Linz Austria

static const char * usage = "usage: satsort [-h] [ <input> ]\n";

/*------------------------------------------------------------------------*/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------*/

#include "kissat.h"

/*------------------------------------------------------------------------*/

static int close_file;
static const char * path;
static FILE * file;

static char * buffer;
static size_t size_buffer;
static size_t capacity_buffer;

static char ** lines;
static size_t size_lines;
static size_t capacity_lines;

/*------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------*/

static void
error (const char * msg, ...)
{
  va_list ap;
  fprintf (stderr, "satsort: parse error in '%s': ", path);
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

static int
parse (void)
{
  assert (!size_buffer);
  for (int ch; (ch = getc (file)) != '\n';)
    {
      if (ch == EOF)
	{
	  if (size_buffer)
	    error ("unexpected end-of-file");
	  return 0;
	}

      if (ch == '\r')
	{
	  if (getc (file) != '\n')
	    error ("expected new-line after carriage-return");
	  else
	    break;

	}

      if (size_buffer == capacity_buffer)
	{
	  capacity_buffer = capacity_buffer ? 2*capacity_buffer : 1;
	  buffer = realloc (buffer, capacity_buffer);
	  if (!buffer)
	    die ("out-of-memory reallocating line buffer");
	}

      buffer[size_buffer++] = ch;
    }

  char * line = malloc (size_buffer + 1);
  if (!line)
    die ("out-of-memory allocating line");
  memcpy (line, buffer, size_buffer);
  line[size_buffer] = 0;
  size_buffer = 0;

  if (size_lines == capacity_lines)
    {
      capacity_lines = capacity_lines ? 2*capacity_lines : 1;
      lines = realloc (lines, capacity_lines * sizeof *lines);
      if (!lines)
	die ("out-of-memory reallocating lines array");
    }

  lines[size_lines++] = line;
  return 1;
}

/*------------------------------------------------------------------------*/

static void
print (void)
{
  for (size_t i = 0; i < size_lines; i++)
    printf ("%s\n", lines[i]);
}

/*------------------------------------------------------------------------*/

int
main (int argc, char ** argv)
{
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

  if (!path)
    path = "<stdin>", file = stdin;
  else if (!(file = fopen (path, "r")))
    die ("can not read '%s'", path);
  else
    close_file = 1;
  while (parse ())
    ;
  if (close_file)
    fclose (file);

  print ();

  free (buffer);
  for (size_t i = 0; i < size_lines; i++)
    free (lines[i]);
  free (lines);

  return 0;
}
