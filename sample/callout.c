/*
 * callout.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "oniguruma.h"

static int
callout_func(OnigCalloutArgs* args, void* user_data)
{
  int r;
  int i;
  int n;
  int begin, end;
  int len;
  UChar* content;

  len = args->content_end - args->content;
  content = (UChar* )strndup((const char* )args->content, len);

  fprintf(stdout, "CALLOUT: content: \"%s\", start: \"%s\", current: \"%s\"\n",
          content, args->start, args->current);
  free(content);

  n = onig_number_of_captures(args->regex);
  for (i = 1; i <= n; i++) {
    r = onig_get_capture_range_in_callout(args, i, &begin, &end);
    if (r != ONIG_NORMAL) return r;

    fprintf(stdout, "capture %d: (%d-%d)\n", i, begin, end);
  }

  fflush(stdout);
  return ONIG_CALLOUT_SUCCESS;
}

static int
test(char* in_pattern, char* in_str)
{
  int r;
  unsigned char *start, *range, *end;
  regex_t* reg;
  OnigErrorInfo einfo;
  OnigRegion *region;
  OnigMatchParams mparams;
  UChar* pattern;
  UChar* str;

  pattern = (UChar* )in_pattern;
  str = (UChar* )in_str;

  r = onig_new(&reg, pattern, pattern + strlen((char* )pattern),
	ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII, ONIG_SYNTAX_DEFAULT, &einfo);
  if (r != ONIG_NORMAL) {
    char s[ONIG_MAX_ERROR_MESSAGE_LEN];
    onig_error_code_to_str((UChar* )s, r, &einfo);
    fprintf(stderr, "COMPILE ERROR: %d: %s\n", r, s);
    return -1;
  }

  region = onig_region_new();

  onig_initialize_match_params(&mparams);
  mparams.callout = callout_func;

  end   = str + strlen((char* )str);
  start = str;
  range = end;
  r = onig_search_with_params(reg, str, end, start, range, region,
                              ONIG_OPTION_NONE, &mparams);
  if (r >= 0) {
    int i;

    fprintf(stderr, "match at %d\n", r);
    for (i = 0; i < region->num_regs; i++) {
      fprintf(stderr, "%d: (%d-%d)\n", i, region->beg[i], region->end[i]);
    }
  }
  else if (r == ONIG_MISMATCH) {
    fprintf(stderr, "search fail\n");
  }
  else { /* error */
    char s[ONIG_MAX_ERROR_MESSAGE_LEN];
    onig_error_code_to_str((UChar* )s, r);
    fprintf(stderr, "SEARCH ERROR: %d: %s\n", r, s);
  }

  onig_region_free(region, 1 /* 1:free self, 0:free contents only */);
  onig_free(reg);
  return r;
}

extern int main(int argc, char* argv[])
{
  OnigEncoding use_encs[] = { ONIG_ENCODING_UTF8 };
  onig_initialize(use_encs, sizeof(use_encs)/sizeof(use_encs[0]));

  test("a+(?{foo bar baz...})$", "aaab");
  test("(?{{!{}#$%&'()=-~^|[_]`@*:+;<>?/.\\,}})c", "abc");
  test("\\A(...)(?{{{booooooooooooo{{ooo}}ooooooooooz}}})", "aaab");

  onig_end();
  return 0;
}
