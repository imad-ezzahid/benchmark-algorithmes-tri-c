#include "courbes_dessin.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "state.h"

#define PI 3.14159265358979323846
#define Y_MIN_LOG 1e-9

static const double COULEURS[NB_ALGOS][3] = {
    {0.84, 0.15, 0.16}, {0.12, 0.47, 0.71}, {0.17, 0.63, 0.17},
    {1.00, 0.50, 0.05}, {0.58, 0.40, 0.74}, {0.09, 0.75, 0.81}
};

typedef struct { double x, y; } Pt;

static void texte(cairo_t *cr, double x, double y, const char *s, int align) {  /* 0 gauche, 1 centre, 2 droite */
    cairo_text_extents_t te;
    cairo_text_extents(cr, s, &te);
    if (align == 1) x -= te.x_advance / 2.0;
    else if (align == 2) x -= te.x_advance;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, s);
}

void courbes_dessiner(cairo_t *cr, int w, int h, const ParamsCourbes *p) {
    /* fond blanc (le PNG exporte n'est pas transparent) */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    const double ml = 85, mr = 45, mt = 50, mb = 65;
    double px = ml, py = mt, pw = w - ml - mr, ph = h - mt - mb;
    if (pw < 80 || ph < 80) return;

    /* titre */
    char titre[200];
    snprintf(titre, sizeof titre, "Temps d'exécution en fonction de la taille n  (type %s, tableau %s)",
             TYPE_NOMS[p->type], CFG_NOMS[p->cfg]);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 15);
    texte(cr, w / 2.0, 28, titre, 1);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    /* collecte des points : un tableau de points par algorithme */
    Pt pts[NB_ALGOS][NB_TAILLES];
    int np[NB_ALGOS] = { 0 };
    double xmin = 1e300, xmax = 0, ymin = 1e300, ymax = 0;
    for (int a = 0; a < NB_ALGOS; a++) {
        if (!p->algo[a]) continue;
        for (int s = 0; s < NB_TAILLES; s++) {
            if (!p->taille[s]) continue;
            for (int i = 0; i < NB_RESULTATS; i++) {
                const Resultat *r = &RESULTATS[i];
                if (r->algo == a && r->type == p->type && r->cfg == p->cfg && r->n == TAILLES_STD[s]) {
                    double y = r->temps > Y_MIN_LOG ? r->temps : Y_MIN_LOG;
                    pts[a][np[a]++] = (Pt){ (double)r->n, y };
                    if (r->n < xmin) xmin = (double)r->n;
                    if (r->n > xmax) xmax = (double)r->n;
                    if (y < ymin) ymin = y;
                    if (y > ymax) ymax = y;
                    break;
                }
            }
        }
    }

    cairo_set_font_size(cr, 12);
    int total = 0;
    for (int a = 0; a < NB_ALGOS; a++) total += np[a];
    if (total == 0) {
        cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
        texte(cr, w / 2.0, h / 2.0, "Aucune mesure pour cette sélection (lancez des tris ou une campagne).", 1);
        return;
    }

    /* echelles */
    double x0, x1, y0, y1;
    if (p->log) {
        x0 = floor(log10(xmin)); x1 = ceil(log10(xmax)); if (x1 <= x0) x1 = x0 + 1;
        y0 = floor(log10(ymin)); y1 = ceil(log10(ymax)); if (y1 <= y0) y1 = y0 + 1;
    } else {
        x0 = 0; x1 = xmax * 1.05;
        y0 = 0; y1 = ymax * 1.05; if (y1 <= 0) y1 = 1;
    }
#define FX(x) (px + ((p->log ? log10(x) : (x)) - x0) / (x1 - x0) * pw)
#define FY(y) (py + ph - ((p->log ? log10((y) > Y_MIN_LOG ? (y) : Y_MIN_LOG) : (y)) - y0) / (y1 - y0) * ph)

    /* grille et graduations */
    char buf[32];
    cairo_set_line_width(cr, 1.0);
    if (p->log) {
        for (int k = (int)x0; k <= (int)x1; k++) {
            double gx = px + (k - x0) / (x1 - x0) * pw;
            cairo_set_source_rgb(cr, 0.88, 0.88, 0.88); cairo_move_to(cr, gx, py); cairo_line_to(cr, gx, py + ph); cairo_stroke(cr);
            snprintf(buf, sizeof buf, "1e%d", k);
            cairo_set_source_rgb(cr, 0, 0, 0); texte(cr, gx, py + ph + 18, buf, 1);
        }
        for (int k = (int)y0; k <= (int)y1; k++) {
            double gy = py + ph - (k - y0) / (y1 - y0) * ph;
            cairo_set_source_rgb(cr, 0.88, 0.88, 0.88); cairo_move_to(cr, px, gy); cairo_line_to(cr, px + pw, gy); cairo_stroke(cr);
            snprintf(buf, sizeof buf, "1e%d", k);
            cairo_set_source_rgb(cr, 0, 0, 0); texte(cr, px - 8, gy + 4, buf, 2);
        }
    } else {
        for (int i = 0; i <= 5; i++) {
            double gx = px + pw * i / 5.0, gy = py + ph - ph * i / 5.0;
            cairo_set_source_rgb(cr, 0.88, 0.88, 0.88);
            cairo_move_to(cr, gx, py); cairo_line_to(cr, gx, py + ph); cairo_stroke(cr);
            cairo_move_to(cr, px, gy); cairo_line_to(cr, px + pw, gy); cairo_stroke(cr);
            cairo_set_source_rgb(cr, 0, 0, 0);
            snprintf(buf, sizeof buf, "%.3g", x0 + (x1 - x0) * i / 5.0); texte(cr, gx, py + ph + 18, buf, 1);
            snprintf(buf, sizeof buf, "%.3g", y0 + (y1 - y0) * i / 5.0); texte(cr, px - 8, gy + 4, buf, 2);
        }
    }

    /* cadre */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.5);
    cairo_rectangle(cr, px, py, pw, ph);
    cairo_stroke(cr);

    /* courbes */
    for (int a = 0; a < NB_ALGOS; a++) {
        if (np[a] == 0) continue;
        cairo_set_source_rgb(cr, COULEURS[a][0], COULEURS[a][1], COULEURS[a][2]);
        cairo_set_line_width(cr, 2.0);
        for (int k = 0; k < np[a]; k++) {
            if (k == 0) cairo_move_to(cr, FX(pts[a][k].x), FY(pts[a][k].y));
            else        cairo_line_to(cr, FX(pts[a][k].x), FY(pts[a][k].y));
        }
        cairo_stroke(cr);
        for (int k = 0; k < np[a]; k++) {
            cairo_arc(cr, FX(pts[a][k].x), FY(pts[a][k].y), 3.5, 0, 2 * PI);
            cairo_fill(cr);
        }
    }

    /* legende */
    int nl = 0;
    for (int a = 0; a < NB_ALGOS; a++) if (np[a]) nl++;
    double lx = px + 12, ly = py + 10, lw = 170, lh = nl * 20 + 8;
    cairo_set_source_rgba(cr, 1, 1, 1, 0.92);
    cairo_rectangle(cr, lx - 6, ly - 6, lw, lh);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
    int row = 0;
    for (int a = 0; a < NB_ALGOS; a++) {
        if (!np[a]) continue;
        double yy = ly + row * 20 + 8;
        cairo_set_source_rgb(cr, COULEURS[a][0], COULEURS[a][1], COULEURS[a][2]);
        cairo_set_line_width(cr, 2.5);
        cairo_move_to(cr, lx, yy); cairo_line_to(cr, lx + 26, yy); cairo_stroke(cr);
        cairo_set_source_rgb(cr, 0, 0, 0);
        texte(cr, lx + 34, yy + 4, ALGO_NOMS[a], 0);
        row++;
    }

    /* libelles des axes (avec unites) */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_font_size(cr, 13);
    texte(cr, px + pw / 2, h - 14, p->log ? "Taille du tableau n (échelle log)" : "Taille du tableau n", 1);
    cairo_save(cr);
    cairo_translate(cr, 20, py + ph / 2);
    cairo_rotate(cr, -PI / 2);
    texte(cr, 0, 0, p->log ? "Temps d'exécution (s) (échelle log)" : "Temps d'exécution (s)", 1);
    cairo_restore(cr);
#undef FX
#undef FY
}
