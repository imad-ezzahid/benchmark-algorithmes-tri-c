/* Fenetre 5 : visualisation graphique (courbes temps = f(n)) */
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "state.h"
#include "bench.h"
#include "courbes_dessin.h"

#define FICHIER_PNG "courbes.png"
#define FICHIER_EXPORT "export_mesures.csv"

static const char *TYPES_TXT[] = { "int", "float", "char", "double", NULL };
static const char *CFG_TXT[]   = { "aléatoire", "trié croissant", "trié décroissant", NULL };

static struct {
    GtkWidget *dd_type, *dd_cfg, *algo[NB_ALGOS], *taille[NB_TAILLES], *log, *zone, *statut;
} W;
static GtkWidget *fenetre = NULL;

static void lire_params(ParamsCourbes *p) {
    guint t = gtk_drop_down_get_selected(GTK_DROP_DOWN(W.dd_type));
    guint c = gtk_drop_down_get_selected(GTK_DROP_DOWN(W.dd_cfg));
    p->type = (t == GTK_INVALID_LIST_POSITION) ? T_INT : (TypeDonnee)t;
    p->cfg  = (c == GTK_INVALID_LIST_POSITION) ? CFG_ALEATOIRE : (Configuration)c;
    for (int i = 0; i < NB_ALGOS; i++)   p->algo[i]   = gtk_check_button_get_active(GTK_CHECK_BUTTON(W.algo[i]));
    for (int i = 0; i < NB_TAILLES; i++) p->taille[i] = gtk_check_button_get_active(GTK_CHECK_BUTTON(W.taille[i]));
    p->log = gtk_check_button_get_active(GTK_CHECK_BUTTON(W.log));
}

static void draw_cb(GtkDrawingArea *a, cairo_t *cr, int w, int h, gpointer d) {
    (void)a; (void)d;
    ParamsCourbes p;
    lire_params(&p);
    courbes_dessiner(cr, w, h, &p);
}

void gui_courbes_rafraichir(void) { if (fenetre) gtk_widget_queue_draw(W.zone); }

static void on_toggled(GtkCheckButton *b, gpointer d) { (void)b; (void)d; gtk_widget_queue_draw(W.zone); }
static void on_select(GObject *o, GParamSpec *ps, gpointer d) { (void)o; (void)ps; (void)d; gtk_widget_queue_draw(W.zone); }

static void on_png(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    ParamsCourbes p;
    lire_params(&p);
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1000, 650);
    cairo_t *cr = cairo_create(s);
    courbes_dessiner(cr, 1000, 650, &p);
    cairo_destroy(cr);
    cairo_status_t st = cairo_surface_write_to_png(s, FICHIER_PNG);
    cairo_surface_destroy(s);
    gtk_label_set_text(GTK_LABEL(W.statut), st == CAIRO_STATUS_SUCCESS
        ? "Graphique enregistré dans courbes.png (dossier du programme)."
        : "Erreur : impossible d'écrire courbes.png.");
}

static void on_csv(GtkButton *b, gpointer d) {
    (void)b; (void)d;
    FILE *f = fopen(FICHIER_EXPORT, "w");
    if (!f) { gtk_label_set_text(GTK_LABEL(W.statut), "Erreur : impossible d'écrire export_mesures.csv."); return; }
    csv_entete(f);
    for (int i = 0; i < NB_RESULTATS; i++) {
        const Resultat *r = &RESULTATS[i];
        csv_ligne(f, ALGO_NOMS[r->algo], r->type, r->cfg, r->n, r->temps, r->ok);
    }
    fclose(f);
    char s[160];
    snprintf(s, sizeof s, "%d mesure(s) exportée(s) dans export_mesures.csv (dossier du programme).", NB_RESULTATS);
    gtk_label_set_text(GTK_LABEL(W.statut), s);
}

static void on_fermer(GtkButton *b, gpointer win) { (void)b; gtk_window_close(GTK_WINDOW(win)); }

static GtkWidget *titre(const char *markup) {
    GtkWidget *l = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(l), markup);
    gtk_label_set_xalign(GTK_LABEL(l), 0.0f);
    return l;
}

void gui_courbes_ouvrir(GtkWindow *parent) {
    if (fenetre) { gtk_widget_queue_draw(W.zone); gtk_window_present(GTK_WINDOW(fenetre)); return; }

    GtkWidget *win = gtk_window_new();
    fenetre = win;
    g_object_add_weak_pointer(G_OBJECT(win), (gpointer *)&fenetre);
    gtk_window_set_title(GTK_WINDOW(win), "Visualisation graphique");
    gtk_window_set_default_size(GTK_WINDOW(win), 1000, 640);
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_application(GTK_WINDOW(win), gtk_window_get_application(parent));

    GtkWidget *racine = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_start(racine, 12);  gtk_widget_set_margin_end(racine, 12);
    gtk_widget_set_margin_top(racine, 12);    gtk_widget_set_margin_bottom(racine, 12);

    /* ---- panneau de gauche : choix ---- */
    GtkWidget *g = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_size_request(g, 230, -1);

    gtk_box_append(GTK_BOX(g), titre("<b>Données</b>"));
    W.dd_type = gtk_drop_down_new_from_strings(TYPES_TXT);
    W.dd_cfg  = gtk_drop_down_new_from_strings(CFG_TXT);
    gtk_box_append(GTK_BOX(g), W.dd_type);
    gtk_box_append(GTK_BOX(g), W.dd_cfg);

    gtk_box_append(GTK_BOX(g), titre("<b>Algorithmes à afficher</b>"));
    for (int i = 0; i < NB_ALGOS; i++) {
        W.algo[i] = gtk_check_button_new_with_label(ALGO_NOMS[i]);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(W.algo[i]), TRUE);
        gtk_box_append(GTK_BOX(g), W.algo[i]);
    }

    gtk_box_append(GTK_BOX(g), titre("<b>Tailles à comparer</b>"));
    for (int i = 0; i < NB_TAILLES; i++) {
        char s[32];
        snprintf(s, sizeof s, "%zu", TAILLES_STD[i]);
        W.taille[i] = gtk_check_button_new_with_label(s);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(W.taille[i]), TRUE);
        gtk_box_append(GTK_BOX(g), W.taille[i]);
    }

    W.log = gtk_check_button_new_with_label("Échelle logarithmique");
    gtk_box_append(GTK_BOX(g), W.log);

    GtkWidget *bpng = gtk_button_new_with_label("Enregistrer le graphique (PNG)");
    GtkWidget *bcsv = gtk_button_new_with_label("Exporter les mesures (CSV)");
    GtkWidget *bfer = gtk_button_new_with_label("Fermer");
    g_signal_connect(bpng, "clicked", G_CALLBACK(on_png), NULL);
    g_signal_connect(bcsv, "clicked", G_CALLBACK(on_csv), NULL);
    g_signal_connect(bfer, "clicked", G_CALLBACK(on_fermer), win);
    gtk_box_append(GTK_BOX(g), bpng);
    gtk_box_append(GTK_BOX(g), bcsv);
    gtk_box_append(GTK_BOX(g), bfer);

    W.statut = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(W.statut), TRUE);
    gtk_label_set_xalign(GTK_LABEL(W.statut), 0.0f);
    gtk_box_append(GTK_BOX(g), W.statut);

    /* ---- zone de dessin ---- */
    W.zone = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(W.zone), 640);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(W.zone), 460);
    gtk_widget_set_hexpand(W.zone, TRUE);
    gtk_widget_set_vexpand(W.zone, TRUE);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(W.zone), draw_cb, NULL, NULL);

    /* redessin a chaque changement de choix */
    for (int i = 0; i < NB_ALGOS; i++)   g_signal_connect(W.algo[i],   "toggled", G_CALLBACK(on_toggled), NULL);
    for (int i = 0; i < NB_TAILLES; i++) g_signal_connect(W.taille[i], "toggled", G_CALLBACK(on_toggled), NULL);
    g_signal_connect(W.log, "toggled", G_CALLBACK(on_toggled), NULL);
    g_signal_connect(W.dd_type, "notify::selected", G_CALLBACK(on_select), NULL);
    g_signal_connect(W.dd_cfg,  "notify::selected", G_CALLBACK(on_select), NULL);

    gtk_box_append(GTK_BOX(racine), g);
    gtk_box_append(GTK_BOX(racine), W.zone);
    gtk_window_set_child(GTK_WINDOW(win), racine);
    gtk_window_present(GTK_WINDOW(win));
}
