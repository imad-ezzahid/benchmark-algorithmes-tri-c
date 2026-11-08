/* Test en console : tous les tris, toutes les configurations, resultats dans resultats.csv
   Usage : ./tri_console [int|float|char|double] [taille_max]            */
#include <stdio.h>
#include <stdlib.h>
#include "sorts.h"
#include "generate.h"
#include "bench.h"


int main(int argc, char **argv) {
    TypeDonnee type = T_INT;
    size_t taille_max = 100000;
    if (argc > 1) { int t = type_depuis_nom(argv[1]); if (t < 0) { fprintf(stderr, "Type inconnu: %s\n", argv[1]); return 1; } type = (TypeDonnee)t; }
    if (argc > 2) taille_max = (size_t)atol(argv[2]);

    const size_t tailles[] = { 100, 500, 1000, 5000, 10000, 50000, 100000 };
    const int nb_tailles = sizeof tailles / sizeof tailles[0];
    size_t sz = type_taille(type);
    cmp_fn cmp = type_cmp(type);

    FILE *csv = fopen("resultats.csv", "w");
    if (!csv) { perror("resultats.csv"); return 1; }
    csv_entete(csv);
    srand(42);                                    /* graine fixe : resultats reproductibles */

    for (int c = 0; c < 3; c++) {
        printf("\n=== Type %s, tableau %s ===\n", TYPE_NOMS[type], CFG_NOMS[c]);
        printf("%-20s %10s %14s %6s\n", "Algorithme", "Taille", "Temps (s)", "Trie");
        for (int s = 0; s < nb_tailles; s++) {
            size_t n = tailles[s];
            if (n > taille_max) break;
            void *tab = generer_tableau(type, n, (Configuration)c);
            if (!tab) { fprintf(stderr, "Memoire insuffisante\n"); return 1; }
            int reps = repetitions_pour(n);
            for (int a = 0; a < NB_ALGOS; a++) {
                if (ALGO_QUADRATIQUE[a] && n > SEUIL_QUADRATIQUE) continue;
                int ok;
                double t = mesurer(ALGO_FONCTIONS[a], tab, n, sz, cmp, reps, &ok);
                printf("%-20s %10zu %14.6f %6s\n", ALGO_NOMS[a], n, t, ok ? "OK" : "ERREUR");
                csv_ligne(csv, ALGO_NOMS[a], type, (Configuration)c, n, t, ok);
            }
            free(tab);
        }
    }
    fclose(csv);
    printf("\nResultats ecrits dans resultats.csv\n");
    return 0;
}
