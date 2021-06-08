// Copyright (c) 2021 Armin Biere Johannes Kepler University Linz Austria

static const char *usage = "usage: satsort [-h] [-v] [-d] [ <input> ]\n";

/*------------------------------------------------------------------------*/

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------*/

#include "kissat.h"

/*------------------------------------------------------------------------*/

static int verbosity;

static void
die (const char *msg, ...)
{
  va_list ap;
  fputs ("satsort: error: ", stderr);
  va_start (ap, msg);
  vfprintf (stderr, msg, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

static void
verbose (const char * msg, ...)
{
  if (!verbosity)
    return;
  va_list ap;
  fputs ("c [satsort] ", stdout);
  va_start (ap, msg);
  vprintf (msg, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

/*------------------------------------------------------------------------*/

static int close_file;
static const char *path;
static FILE *file;

static char *buffer;
static int size_buffer;
static int capacity_buffer;

static char **lines;
static int size_lines;
static int capacity_lines;

/*------------------------------------------------------------------------*/

static void
error (const char *msg, ...)
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
	  if (capacity_buffer >= 1<<29)
	    die ("too many characters in line");
	  capacity_buffer = capacity_buffer ? 2 * capacity_buffer : 1;
	  buffer = realloc (buffer, capacity_buffer);
	  if (!buffer)
	    die ("out-of-memory reallocating line buffer");
	}

      buffer[size_buffer++] = ch;
    }

  char *line = malloc (size_buffer + 1);
  if (!line)
    die ("out-of-memory allocating line");
  memcpy (line, buffer, size_buffer);
  line[size_buffer] = 0;
  size_buffer = 0;

  if (size_lines == capacity_lines)
    {
      if (capacity_lines >= 1<<29)
	die ("too many lines");
      capacity_lines = capacity_lines ? 2 * capacity_lines : 1;
      lines = realloc (lines, capacity_lines * sizeof *lines);
      if (!lines)
	die ("out-of-memory reallocating lines array");
    }

  lines[size_lines++] = line;
  return 1;
}

/*------------------------------------------------------------------------*/

void
print_original (void)
{
  for (int i = 0; i < size_lines; i++)
    verbose ("original[%d] %s", i, lines[i]);
}

/*------------------------------------------------------------------------*/

static int dimacs;
static kissat *solver;

static void
unit (int lit)
{
  if (dimacs)
    printf ("%d 0\n", lit);
  else
    {
      kissat_add (solver, lit);
      kissat_add (solver, 0);
    }
}

/*------------------------------------------------------------------------*/

static int max_line_length;
static int bits_per_line;
static int variables;

static int
bit (int i, int j)
{
  assert (0 <= i), assert (i < size_lines);
  assert (0 <= j), assert (j < bits_per_line);

  for (const char * p = lines[i]; *p; p++)
    for (int bit = 7; bit >= 0; bit--)
      if (!j--)
	return !!(*p & (1<<bit));

  return 0;
}

static int ** input;
static int ** output;
static int ** map;

static void
encode (void)
{
  for (int i = 0; i < size_lines; i++)
    {
      const int length = strlen (lines[i]);
      if (length > max_line_length)
	max_line_length = length;
    }
  bits_per_line = max_line_length * 8;

  verbose ("maximum line length %d", max_line_length);
  verbose ("number of input-bits per line %d", bits_per_line);

  input = malloc (size_lines * sizeof *input);
  if (!input)
    die ("out-of-memory allocating input table");
  for (int i = 0; i < size_lines; i++)
    {
      input[i] = malloc (bits_per_line * sizeof *input[i]);
      if (!input[i])
	die ("out-of-memory allocating input[%d]", i);
      for (int j = 0; j < bits_per_line; j++)
	input[i][j] = ++variables;
    }

  map = malloc (size_lines * sizeof *map);
  if (!map)
    die ("out-of-memory allocating map table");
  for (int i = 0; i < size_lines; i++)
    {
      map[i] = malloc (size_lines * sizeof *map[i]);
      if (!map[i])
	die ("out-of-memory allocating map[%d]", i);
      for (int j = 0; j < size_lines; j++)
	map[i][j] = ++variables;
    }

  output = malloc (size_lines * sizeof *output);
  if (!output)
    die ("out-of-memory allocating output table");
  for (int i = 0; i < size_lines; i++)
    {
      output[i] = malloc (bits_per_line * sizeof *output[i]);
      if (!output[i])
	die ("out-of-memory allocating output[%d]", i);
      for (int j = 0; j < bits_per_line; j++)
	output[i][j] = ++variables;
    }

  verbose ("using %d variables", variables);

  if (!dimacs)
    {
      solver = kissat_init ();
      if (verbosity)
	kissat_set_option (solver, "verbose", verbosity - 1);
      else
	kissat_set_option (solver, "quiet", 1);
    }

  for (int i = 0; i < size_lines; i++)
    for (int j = 0; j < bits_per_line; j++)
      {
	if (bit (i, j))
	  unit (input[i][j]);
	else
	  unit (-input[i][j]);
      }
}

/*------------------------------------------------------------------------*/

static void
solve (void)
{
  verbose ("starting SAT solving");
  int res = kissat_solve (solver);
  if (res != 10)
    die ("unexpected solver result %d", res);
  verbose ("finished SAT solving with result %d", res);
}

/*------------------------------------------------------------------------*/

static void
print_input (void)
{
}

/*------------------------------------------------------------------------*/

static void
reset (void)
{

  free (buffer);

  for (int i = 0; i < size_lines; i++)
    free (lines[i]);
  free (lines);

  for (int i = 0; i < size_lines; i++)
    free (input[i]);
  free (input);

  for (int i = 0; i < size_lines; i++)
    free (map[i]);
  free (map);

  for (int i = 0; i < size_lines; i++)
    free (output[i]);
  free (output);

  if (solver)
    kissat_release (solver);
}

/*------------------------------------------------------------------------*/

int
main (int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
    {
      const char *arg = argv[i];
      if (!strcmp (arg, "-h"))
	fputs (usage, stdout), exit (0);
      else if (!strcmp (arg, "-d"))
	dimacs = 1;
      else if (!strcmp (arg, "-v"))
	verbosity++;
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

  verbose ("parsed %d lines", size_lines);

  if (verbosity)
    print_original ();

  encode ();

  if (!dimacs)
    {
      solve ();
      if (verbosity)
        print_input ();
    }

  reset ();
  return 0;
}
