#ifndef BENCH_H
#define BENCH_H
#include <stdio.h>
#include "sorts.h"
#include "generate.h"

double maintenant(void);   /* temps en secondes (horloge haute resolution) */
int    est_trie(const void *base, size_t n, size_t sz, cmp_fn cmp);

/* Trie `repetitions` copies identiques de `initial` avec f et renvoie le temps
   moyen en secondes. *ok vaut 1 si le tableau est correctement trie.
   Seul l'appel au tri est chronometre (pas la copie, pas l'affichage). */
double mesurer(sort_fn f, const void *initial, size_t n, size_t sz, cmp_fn cmp,
               int repetitions, int *ok);

void csv_entete(FILE *f);
void csv_ligne(FILE *f, const char *algo, TypeDonnee t, Configuration cfg,
               size_t n, double temps, int ok);

/* Nombre de repetitions conseille selon la taille (moyenne de plusieurs essais) */
int repetitions_pour(size_t n);

/* Ajoute une ligne a un fichier CSV (cree l'en-tete si le fichier est vide) */
void csv_ajouter(const char *chemin, const char *algo, TypeDonnee t, Configuration cfg,
                 size_t n, double temps, int ok);

/* Au-dela de cette taille, les campagnes sautent les tris O(n^2) (trop longs) */
#define SEUIL_QUADRATIQUE 50000

#endif
