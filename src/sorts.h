#ifndef SORTS_H
#define SORTS_H
#include <stddef.h>

/* Les tris sont generiques : ils travaillent sur des octets (base, n elements
   de taille sz) et une fonction de comparaison, comme qsort().
   Taille maximale d'un element : 16 octets (int, float, char, double OK). */
typedef int (*cmp_fn)(const void *, const void *);
typedef void (*sort_fn)(void *base, size_t n, size_t sz, cmp_fn cmp);

#define NB_ALGOS 6

void tri_selection(void *base, size_t n, size_t sz, cmp_fn cmp);
void tri_insertion(void *base, size_t n, size_t sz, cmp_fn cmp);
void tri_bulles(void *base, size_t n, size_t sz, cmp_fn cmp);
void tri_fusion(void *base, size_t n, size_t sz, cmp_fn cmp);
void tri_shell(void *base, size_t n, size_t sz, cmp_fn cmp);
void tri_rapide(void *base, size_t n, size_t sz, cmp_fn cmp);

extern const char    *ALGO_NOMS[NB_ALGOS];
extern const sort_fn  ALGO_FONCTIONS[NB_ALGOS];
extern const int      ALGO_QUADRATIQUE[NB_ALGOS]; /* 1 si O(n^2) en moyenne */
extern const char    *ALGO_COMPLEXITE_MOY[NB_ALGOS];   /* complexite theorique moyenne */
extern const char    *ALGO_COMPLEXITE_PIRE[NB_ALGOS];  /* complexite theorique pire cas */

#endif
