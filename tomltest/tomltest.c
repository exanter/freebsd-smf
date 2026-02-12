#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "tomlc17.h"

int main(int argc, char **argv)
{
  toml_result_t result;

  result = toml_parse_file_ex("testfiles/test1.toml");
  if (!result.ok)
  {
    fprintf(stderr, "Error processing file: %s\n", result.errmsg);
    exit(1);
  }

  toml_datum_t title = toml_seek(result.toptab, "title");
  if (title.type != TOML_STRING)
  {
    fprintf(stderr, "missing or invalid title property in config");
    exit(2);
  }

  printf("read title: [%s]\n", title.u.s);
  exit(0);
}
