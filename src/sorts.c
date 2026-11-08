#include "sorts.h"
#include <stdlib.h>
#include <string.h>

#define EL(i) ((char *)base + (size_t)(i) * sz)
#define MAX_SZ 16

static void echanger(char *a, char *b, size_t sz) {
    for (size_t k = 0; k < sz; k++) { char t = a[k]; a[k] = b[k]; b[k] = t; }
}

/* 1. Tri par selection : O(n^2) dans tous les cas */
void tri_selection(void *base, size_t n, size_t sz, cmp_fn cmp) {
    if (n < 2) return;
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++)
            if (cmp(EL(j), EL(min)) < 0) min = j;
        if (min != i) echanger(EL(i), EL(min), sz);
    }
}

/* 2. Tri par insertion : O(n^2), O(n) si deja trie */
void tri_insertion(void *base, size_t n, size_t sz, cmp_fn cmp) {
    char tmp[MAX_SZ];
    for (size_t i = 1; i < n; i++) {
        memcpy(tmp, EL(i), sz);
        size_t j = i;
        while (j > 0 && cmp(EL(j - 1), tmp) > 0) {
            memcpy(EL(j), EL(j - 1), sz);
            j--;
        }
        memcpy(EL(j), tmp, sz);
    }
}

/* 3. Tri a bulles (avec arret si aucun echange) : O(n^2), O(n) si deja trie */
void tri_bulles(void *base, size_t n, size_t sz, cmp_fn cmp) {
    for (size_t pass = 0; pass + 1 < n; pass++) {
        int echange = 0;
        for (size_t j = 0; j + 1 + pass < n; j++)
            if (cmp(EL(j), EL(j + 1)) > 0) { echanger(EL(j), EL(j + 1), sz); echange = 1; }
        if (!echange) break;
    }
}

/* 4. Tri fusion : O(n log n), tableau auxiliaire alloue une seule fois */
static void fusion_rec(char *b, char *aux, size_t n, size_t sz, cmp_fn cmp) {
    if (n < 2) return;
    size_t mid = n / 2;
    fusion_rec(b, aux, mid, sz, cmp);
    fusion_rec(b + mid * sz, aux, n - mid, sz, cmp);
    size_t i = 0, j = mid, k = 0;
    while (i < mid && j < n) {
        if (cmp(b + j * sz, b + i * sz) < 0) { memcpy(aux + k * sz, b + j * sz, sz); j++; }
        else                                  { memcpy(aux + k * sz, b + i * sz, sz); i++; }
        k++;
    }
    if (i < mid) memcpy(aux + k * sz, b + i * sz, (mid - i) * sz);
    if (j < n)   memcpy(aux + k * sz, b + j * sz, (n - j) * sz);
    memcpy(b, aux, n * sz);
}
void tri_fusion(void *base, size_t n, size_t sz, cmp_fn cmp) {
    if (n < 2) return;
    char *aux = malloc(n * sz);
    if (!aux) return;
    fusion_rec((char *)base, aux, n, sz, cmp);
    free(aux);
}

/* 5. Tri Shell : ecarts de Knuth (1, 4, 13, 40, ...) */
void tri_shell(void *base, size_t n, size_t sz, cmp_fn cmp) {
    char tmp[MAX_SZ];
    size_t h = 1;
    while (h < n / 3) h = 3 * h + 1;
    for (; h >= 1; h = (h - 1) / 3) {
        for (size_t i = h; i < n; i++) {
            memcpy(tmp, EL(i), sz);
            size_t j = i;
            while (j >= h && cmp(EL(j - h), tmp) > 0) {
                memcpy(EL(j), EL(j - h), sz);
                j -= h;
            }
            memcpy(EL(j), tmp, sz);
        }
    }
}

/* 6. Tri rapide : pivot = mediane de trois, on recurse sur la plus petite
      partie et on boucle sur l'autre (pile en O(log n)) */
static void rapide_rec(char *b, long lo, long hi, size_t sz, cmp_fn cmp) {
    char *base = b;
    char piv[MAX_SZ];
    while (lo < hi) {
        long mid = lo + (hi - lo) / 2;
        if (cmp(EL(mid), EL(lo)) < 0) echanger(EL(mid), EL(lo), sz);
        if (cmp(EL(hi),  EL(lo)) < 0) echanger(EL(hi),  EL(lo), sz);
        if (cmp(EL(hi), EL(mid)) < 0) echanger(EL(hi), EL(mid), sz);
        memcpy(piv, EL(mid), sz);
        long i = lo, j = hi;
        while (i <= j) {
            while (cmp(EL(i), piv) < 0) i++;
            while (cmp(EL(j), piv) > 0) j--;
            if (i <= j) { echanger(EL(i), EL(j), sz); i++; j--; }
        }
        if (j - lo < hi - i) { rapide_rec(b, lo, j, sz, cmp); lo = i; }
        else                 { rapide_rec(b, i, hi, sz, cmp); hi = j; }
    }
}
void tri_rapide(void *base, size_t n, size_t sz, cmp_fn cmp) {
    if (n < 2) return;
    rapide_rec((char *)base, 0, (long)n - 1, sz, cmp);
}

const char *ALGO_NOMS[NB_ALGOS] = {
    "Tri par selection", "Tri par insertion", "Tri a bulles",
    "Tri fusion", "Tri Shell", "Tri rapide"
};
const sort_fn ALGO_FONCTIONS[NB_ALGOS] = {
    tri_selection, tri_insertion, tri_bulles, tri_fusion, tri_shell, tri_rapide
};
const int ALGO_QUADRATIQUE[NB_ALGOS] = { 1, 1, 1, 0, 0, 0 };
const char *ALGO_COMPLEXITE_MOY[NB_ALGOS]  = { "O(n²)", "O(n²)", "O(n²)", "O(n log n)", "O(n log² n)", "O(n log n)" };
const char *ALGO_COMPLEXITE_PIRE[NB_ALGOS] = { "O(n²)", "O(n²)", "O(n²)", "O(n log n)", "O(n²)",       "O(n²)" };
