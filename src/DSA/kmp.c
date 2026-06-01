#include "kmp.h"
#include "help_window.h"
#include "vec.h"
#include <string.h>

// Private utils for this file
static TokenType *get_line_results_token_types(int *line_results, HelpLine *current_line);
static void destroy_search_results(SearchResult *s);

void init_LPS(int *lps) {
    for (int i = 0; i < MAX_SEARCH_BUFFER_LEN; i++) {
        lps[i] = 0;
    }
}

void compute_lps(int *lps, const char *search_buffer, int search_len) {
    // lps is already init with 0

    int len = 0; // max match len
    int i = 1;   // explorer

    while (i < search_len) {
        if (search_buffer[i] == search_buffer[len]) {
            lps[i++] = ++len;
        } else if (search_buffer[i] != search_buffer[len]) {
            if (len == 0) {
                lps[i++] = 0;
            } else {
                len = lps[len - 1];
            }
        }
    }
}

int *search_kmp(const char *str, const char *search_buffer, int search_len, int *lps) {

    int *result_indices = NULL;
    int num_matches = 0;
    int i = 0;
    int j = 0;
    int linelen = strlen(str);

    while (i < linelen) {
        if (str[i] == search_buffer[j]) {
            j++;
            i++;
        } else {
            if (j == 0) {
                i++;
            } else {
                j = lps[j - 1]; // last longest substr match
            }
        }
        if (j == search_len) {
            num_matches++;
            vec_push(result_indices, (i - search_len));
            // reset the explorer idx
            j = lps[j - 1];
        }
    }
    return result_indices; // match not found
}

/* NOTE:
 *  The search_help() is in the help_window.h header
 *  Since the KMP algorithm ends here and the rest is
 *  implementation specefic ie: Matching tokens...etc
 */

SearchResult *search_help_kmp(HelpWindow *hwin, const char *search_buffer, int search_len) {

    /* KMP Algorithm:
     *  We redeclare LPS array each-time but since it's on the
     *  Stack it's fine for this case. After that  We init the
     *  LPS array to call the search_kmp() function Afterwards
     *  And pass the same array.
     */

    int LPS[MAX_SEARCH_BUFFER_LEN];
    SearchResult *results = NULL;

    HelpPage currPage = hwin->current_page;

    for (size_t l = 0; l < hwin->pages_db[currPage].line_count; l++) {
        // cast out the const
        HelpLine *current_line = (HelpLine *)&hwin->pages_db[currPage].lines[l];
        if (current_line->text != NULL) {
            // init LPS (zeros out the array for each use)
            init_LPS(LPS);

            int *line_results = search_kmp(current_line->text, search_buffer, search_len, LPS);

            TokenType *line_results_tts = get_line_results_token_types(line_results, current_line);

            // TODO: Handle this special case
            // if (vec_len(line_results_tts) != vec_len(line_results)) {
            //  LOG: results num mismatch (potential bug in token position offset
            //  calculations)
            //}

            if (line_results) {
                vec_push(results, (SearchResult){.line = current_line->line_start,
                                                 .idx = line_results,
                                                 .match_len = search_len,
                                                 .type = line_results_tts});
            } else {
                vec_free(line_results);
                vec_free(line_results_tts);
            }
            // TODO:
            // SearchResult array will be used for rendering later or saved in
            // a cache array in case of same search patter and maybe hash it to compare
            // or use Rabin Karp algorithm
        }
    }
    return results;
}

// For rendering later to maybe highlight each text differently or idk atp.
static TokenType *get_line_results_token_types(int *line_results, HelpLine *current_line) {
    if (!line_results) {
        return NULL;
    }
    size_t result_len = vec_len(line_results);
    TokenType *tarr = NULL;
    for (size_t i = 0; i < result_len; i++) {
        int curr_res_idx = line_results[i];
        for (size_t j = 0; j < current_line->token_count; j++) {
            int curr_token_idx = current_line->tokens[j].pos;
            if (curr_res_idx == curr_token_idx) {
                vec_push(tarr, current_line->tokens[j].type);
            }
        }
    }
    return tarr;
}

// To free memory after done with search
static void destroy_search_results(SearchResult *s) {
    if (!s) {
        // LOG: no search results available
        return;
    }

    size_t search_results_len = vec_len(s);
    for (size_t i = 0; i < search_results_len; i++) {
        vec_free(s->idx);
        vec_free(s->type);
    }
    vec_free(s);
}
