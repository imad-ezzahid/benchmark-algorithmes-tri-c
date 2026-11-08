#ifndef STATE_H
#define STATE_H
#include <stddef.h>
#include "generate.h"

/* Donnees partagees par toutes les pages */
typedef struct {
    void         *tab;      /* tableau initial (NULL si aucun) */
    size_t        n;
    TypeDonnee    type;
    Configuration cfg;
    int           manuel;   /* 1 si saisi a la main */
} EtatApp;

extern EtatApp ETAT;

/* Resultats mesures (lus par resultats et courbes) */
#define MAX_RESULTATS 5000
typedef struct {
    int           algo;     /* indice dans ALGO_NOMS */
    TypeDonnee    type;
    Configuration cfg;
    size_t        n;
    double        temps;    /* secondes, moyenne */
    int           ok;       /* 1 si trie correctement */
} Resultat;

extern Resultat RESULTATS[MAX_RESULTATS];
extern int      NB_RESULTATS;

void etat_effacer_resultats(void);
/* Ajoute un resultat (remplace l'ancien si meme algo/type/config/taille) */
void etat_ajouter_resultat(int algo, TypeDonnee type, Configuration cfg, size_t n, double temps, int ok);

int  etat_a_donnees(void);
void etat_reset(void);
void etat_definir(void *tab, size_t n, TypeDonnee type, Configuration cfg, int manuel);

/* Analyse une saisie manuelle (10 a 30 valeurs separees par espaces, virgules
   ou points-virgules). Renvoie un tableau alloue, ou NULL avec un message dans err. */
void *parser_saisie(const char *txt, TypeDonnee t, size_t *n_out, char *err, size_t errsz);

/* Charge les resultats existants depuis un fichier CSV */
void etat_charger_csv(const char *chemin);

#endif
