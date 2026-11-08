#ifndef GENERATE_H
#define GENERATE_H
#include <stddef.h>
#include "sorts.h"

typedef enum { T_INT, T_FLOAT, T_CHAR, T_DOUBLE } TypeDonnee;
typedef enum { CFG_ALEATOIRE, CFG_CROISSANT, CFG_DECROISSANT } Configuration;

extern const char *TYPE_NOMS[4];
extern const char *CFG_NOMS[3];

size_t type_taille(TypeDonnee t);          /* sizeof du type */
cmp_fn type_cmp(TypeDonnee t);             /* fonction de comparaison du type */
int    type_depuis_nom(const char *nom);   /* "int" -> T_INT, -1 si inconnu */

/* Alloue et remplit un tableau de n elements. A liberer avec free(). */
void *generer_tableau(TypeDonnee t, size_t n, Configuration cfg);

/* Affiche les `max` premiers elements dans buf (pour l'apercu dans l'interface) */
void apercu_tableau(const void *tab, TypeDonnee t, size_t n, size_t max, char *buf, size_t taille_buf);

/* Tailles utilisees pour les campagnes automatiques */
#define NB_TAILLES 7
extern const size_t TAILLES_STD[NB_TAILLES];

#endif
