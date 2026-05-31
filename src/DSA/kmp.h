#pragma once
#include "help_window.h"
#include "vec.h"

void init_LPS(int *lps);
void compute_lps(int *lps, const char *search_buffer, int search_len);
iVector *search_kmp(const char *str, const char *search_buffer, int search_len, int *lps);
