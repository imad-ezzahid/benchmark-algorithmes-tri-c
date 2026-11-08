#include "bench.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

double maintenant(void) {
#ifdef _WIN32
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)f.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

int est_trie(const void *base, size_t n, size_t sz, cmp_fn cmp) {
    const char *b = base;
    for (size_t i = 1; i < n; i++)
        if (cmp(b + (i - 1) * sz, b + i * sz) > 0) return 0;
    return 1;
}

double mesurer(sort_fn f, const void *initial, size_t n, size_t sz, cmp_fn cmp,
               int repetitions, int *ok) {
    char *copie = malloc(n * sz);
    if (!copie) { if (ok) *ok = 0; return -1.0; }
    double total = 0.0;
    int tout_ok = 1;
    for (int r = 0; r < repetitions; r++) {
        memcpy(copie, initial, n * sz);          /* meme tableau de depart */
        double t0 = maintenant();
        f(copie, n, sz, cmp);
        double t1 = maintenant();
        total += t1 - t0;
        if (!est_trie(copie, n, sz, cmp)) tout_ok = 0;
    }
    free(copie);
    if (ok) *ok = tout_ok;
    return total / repetitions;
}

void csv_entete(FILE *f) { fprintf(f, "algorithme,type,configuration,taille,temps_moyen_s,trie_ok\n"); }

void csv_ligne(FILE *f, const char *algo, TypeDonnee t, Configuration cfg,
               size_t n, double temps, int ok) {
    fprintf(f, "%s,%s,%s,%zu,%.9f,%d\n", algo, TYPE_NOMS[t], CFG_NOMS[cfg], n, temps, ok);
}

int repetitions_pour(size_t n) { return n <= 1000 ? 20 : (n <= 10000 ? 5 : 3); }

void csv_ajouter(const char *chemin, const char *algo, TypeDonnee t, Configuration cfg,
                 size_t n, double temps, int ok) {
    FILE *f = fopen(chemin, "a");
    if (!f) return;
    fseek(f, 0, SEEK_END);
    if (ftell(f) == 0) csv_entete(f);
    csv_ligne(f, algo, t, cfg, n, temps, ok);
    fclose(f);
}
