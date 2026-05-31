#include "kmp.h"

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

iVector *search_kmp(const char *str, const char *search_buffer, int search_len, int *lps) {

    // returns an integer vector of match indices
    iVector *v = vectorCreate(10); // starting with 10
    // matches

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
            vectorPush(v, (i - search_len));
            // reset the explorer idx
            j = lps[j - 1];
        }
    }
    return v; // match not found
}

/* NOTE:
 *  The search_help() is in the help_window.h header
 *  Since the KMP algorithm ends here and the rest is
 *  implementation specefic ie: Matching tokens...etc
 */
