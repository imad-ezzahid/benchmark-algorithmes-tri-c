/* Fenetre 4 : comparaison des performances (tableau rempli a partir des mesures reelles) */
#include <stdio.h>
#include <stdlib.h>
#include "gui.h"
#include "state.h"

static const char *TYPES_FILTRE[] = { "Tous", "int", "float", "char", "double", NULL };
static const char *CFG_FILTRE[]   = { "Toutes", "aleatoire", "croissant", "decroissant", NULL };

static GtkWidget *fenetre = NULL;
static GtkWidget *dd_type, *dd_cfg, *defil, *compte;

static int cmp_res(const void *a, const void *b) {
    const Resultat *x = a, *y = b;
    if (x->cfg != y->cfg)   return (int)x->cfg - (int)y->cfg;
    if (x->type != y->type) return (int)x->type - (int)y->type;
    if (x->n != y->n)       return (x->n > y->n) - (x->n < y->n);
    return x->algo - y->algo;
}

static GtkWidget *cellule(const char *txt, int gras, float xalign) {
    GtkWidget *l = gtk_label_new(NULL);
    if (gras) {
        char *m = g_markup_printf_escaped("<b>%s</b>", txt);
        gtk_label_set_markup(GTK_LABEL(l), m);
        g_free(m);
    } else {
        gtk_label_set_text(GTK_LABEL(l), txt);
    }
    gtk_label_set_xalign(GTK_LABEL(l), xalign);
    return l;
}

static void remplir(void) {
    guint ft = gtk_drop_down_get_selected(GTK_DROP_DOWN(dd_type));
    guint fc = gtk_drop_down_get_selected(GTK_DROP_DOWN(dd_cfg));
    if (ft == GTK_INVALID_LIST_POSITION) ft = 0;
    if (fc == GTK_INVALID_LIST_POSITION) fc = 0;

    Resultat *copie = g_new(Resultat, NB_RESULTATS > 0 ? NB_RESULTATS : 1);
    int m = 0;
    for (int i = 0; i < NB_RESULTATS; i++) {
        const Resultat *r = &RESULTATS[i];
        if (ft > 0 && (int)r->type != (int)ft - 1) continue;
        if (fc > 0 && (int)r->cfg  != (int)fc - 1) continue;
        copie[m++] = *r;
    }
    qsort(copie, (size_t)m, sizeof(Resultat), cmp_res);

    GtkWidget *g = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(g), 20);
    gtk_grid_set_row_spacing(GTK_GRID(g), 4);
    gtk_widget_set_margin_start(g, 8);  gtk_widget_set_margin_end(g, 8);
    gtk_widget_set_margin_top(g, 8);    gtk_widget_set_margin_bottom(g, 8);

    const char *entetes[] = { "Algorithme", "Type", "Configuration", "Taille", "Temps d'exécution (s)", "Complexité moyenne", "Trié" };
    for (int c = 0; c < 7; c++)
        gtk_grid_attach(GTK_GRID(g), cellule(entetes[c], 1, (c == 3 || c == 4) ? 1.0f : 0.0f), c, 0, 1, 1);

    for (int i = 0; i < m; i++) {
        const Resultat *r = &copie[i];
        char taille[32], temps[32];
        snprintf(taille, sizeof taille, "%zu", r->n);
        snprintf(temps, sizeof temps, "%.6f", r->temps);
        gtk_grid_attach(GTK_GRID(g), cellule(ALGO_NOMS[r->algo], 0, 0.0f), 0, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(g), cellule(TYPE_NOMS[r->type], 0, 0.0f), 1, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(g), cellule(CFG_NOMS[r->cfg], 0, 0.0f), 2, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(g), cellule(taille, 0, 1.0f), 3, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(g), cellule(temps, 0, 1.0f), 4, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(g), cellule(ALGO_COMPLEXITE_MOY[r->algo], 0, 0.0f), 5, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(g), cellule(r->ok ? "✔" : "✘", 0, 0.5f), 6, i + 1, 1, 1);
    }
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(defil), g);   /* remplace l'ancien tableau */

    char s[128];
    if (NB_RESULTATS == 0)
        snprintf(s, sizeof s, "Aucune mesure : lancez des tris dans « Exécution des tris ».");
    else
        snprintf(s, sizeof s, "%d mesure(s) affichée(s) sur %d.", m, NB_RESULTATS);
    gtk_label_set_text(GTK_LABEL(compte), s);
    g_free(copie);
}

void gui_resultats_rafraichir(void) { if (fenetre) remplir(); }

static void on_filtre(GObject *o, GParamSpec *p, gpointer d) { (void)o; (void)p; (void)d; remplir(); }
static void on_actualiser(GtkButton *b, gpointer d) { (void)b; (void)d; remplir(); }
static void on_effacer(GtkButton *b, gpointer d) { (void)b; (void)d; etat_effacer_resultats(); remplir(); }
static void on_fermer(GtkButton *b, gpointer win) { (void)b; gtk_window_close(GTK_WINDOW(win)); }

void gui_resultats_ouvrir(GtkWindow *parent) {
    if (fenetre) { remplir(); gtk_window_present(GTK_WINDOW(fenetre)); return; }

    GtkWidget *win = gtk_window_new();
    fenetre = win;
    g_object_add_weak_pointer(G_OBJECT(win), (gpointer *)&fenetre);
    gtk_window_set_title(GTK_WINDOW(win), "Comparaison des performances");
    gtk_window_set_default_size(GTK_WINDOW(win), 820, 520);
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_application(GTK_WINDOW(win), gtk_window_get_application(parent));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 16);  gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);    gtk_widget_set_margin_bottom(box, 16);

    GtkWidget *filtres = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(filtres), gtk_label_new("Type :"));
    dd_type = gtk_drop_down_new_from_strings(TYPES_FILTRE);
    gtk_box_append(GTK_BOX(filtres), dd_type);
    gtk_box_append(GTK_BOX(filtres), gtk_label_new("Configuration :"));
    dd_cfg = gtk_drop_down_new_from_strings(CFG_FILTRE);
    gtk_box_append(GTK_BOX(filtres), dd_cfg);
    gtk_box_append(GTK_BOX(box), filtres);

    defil = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(defil, TRUE);
    gtk_widget_set_hexpand(defil, TRUE);
    gtk_box_append(GTK_BOX(box), defil);

    g_signal_connect(dd_type, "notify::selected", G_CALLBACK(on_filtre), NULL);
    g_signal_connect(dd_cfg,  "notify::selected", G_CALLBACK(on_filtre), NULL);

    compte = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(compte), 0.0f);
    gtk_box_append(GTK_BOX(box), compte);

    GtkWidget *bas = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *bact = gtk_button_new_with_label("Actualiser");
    GtkWidget *beff = gtk_button_new_with_label("Effacer les mesures");
    GtkWidget *bfer = gtk_button_new_with_label("Fermer");
    gtk_widget_set_hexpand(bact, TRUE); gtk_widget_set_hexpand(beff, TRUE); gtk_widget_set_hexpand(bfer, TRUE);
    g_signal_connect(bact, "clicked", G_CALLBACK(on_actualiser), NULL);
    g_signal_connect(beff, "clicked", G_CALLBACK(on_effacer), NULL);
    g_signal_connect(bfer, "clicked", G_CALLBACK(on_fermer), win);
    gtk_box_append(GTK_BOX(bas), bact);
    gtk_box_append(GTK_BOX(bas), beff);
    gtk_box_append(GTK_BOX(bas), bfer);
    gtk_box_append(GTK_BOX(box), bas);

    gtk_window_set_child(GTK_WINDOW(win), box);
    remplir();
    gtk_window_present(GTK_WINDOW(win));
}
