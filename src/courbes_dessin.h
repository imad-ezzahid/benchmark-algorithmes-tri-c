#ifndef COURBES_DESSIN_H
#define COURBES_DESSIN_H
#include <cairo.h>
#include "sorts.h"
#include "generate.h"

/* Parametres du graphique (independants de l'interface) */
typedef struct {
    int           algo[NB_ALGOS];     /* 1 = courbe affichee */
    int           taille[NB_TAILLES]; /* 1 = taille comparee */
    int           log;                /* 1 = echelle logarithmique (deux axes) */
    TypeDonnee    type;
    Configuration cfg;
} ParamsCourbes;

/* Dessine le graphique dans cr (zone w x h pixels) a partir des mesures de RESULTATS.
   Utilise pour l'affichage a l'ecran ET pour l'export PNG. */
void courbes_dessiner(cairo_t *cr, int w, int h, const ParamsCourbes *p);

#endif
