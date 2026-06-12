#pragma once
#include "../help_window.h"
#include <stdio.h>

typedef struct SearchResult {
        int *idx;
        int line;
        size_t match_len;
        TokenType *type;
} SearchResult;

void init_LPS(int *lps);
void compute_lps(int *lps, const char *search_buffer, int search_len);

// returns an integer vector of match indices of the same line
int *search_kmp(const char *str, const char *search_buffer, int search_len, int *lps);

// returns the actual result depending on the page the user is searching at
SearchResult *search_help_kmp(HelpWindow *hwin, const char *search_buffer, int search_len);

void destroy_search_results(SearchResult *s);
