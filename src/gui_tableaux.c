/* Page 1 : Gestion des tableaux et saisie manuelle */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gui.h"
#include "state.h"

#define APERCU_MAX 50   /* nombre maximum d'elements affiches dans l'apercu */

static const char  *TAILLES_TXT[] = { "100", "500", "1 000", "5 000", "10 000", "50 000", "100 000", NULL };
static const size_t TAILLES[]     = { 100, 500, 1000, 5000, 10000, 50000, 100000 };
static const char  *TYPES_TXT[]   = { "int", "float", "char", "double", NULL };
static const char  *CFG_TXT[]     = { "Aléatoire", "Trié croissant", "Trié décroissant", NULL };

static struct {
    GtkWidget *dd_taille, *dd_type, *dd_cfg;
    GtkWidget *entry, *texte, *erreur;
    GtkWidget *lbl_stat_taille, *lbl_stat_type, *lbl_stat_cfg, *lbl_stat_mem;
    GtkWidget *lbl_info_apercu;
} F;

static void maj_statistiques(void) {
    if (!F.lbl_stat_taille) return;

    if (!etat_a_donnees()) {
        gtk_label_set_text(GTK_LABEL(F.lbl_stat_taille), "Aucun");
        gtk_label_set_text(GTK_LABEL(F.lbl_stat_type), "—");
        gtk_label_set_text(GTK_LABEL(F.lbl_stat_cfg), "—");
        gtk_label_set_text(GTK_LABEL(F.lbl_stat_mem), "0 Ko");
        gtk_label_set_text(GTK_LABEL(F.lbl_info_apercu), "Aucun tableau en mémoire. Générez-en un ou effectuez une saisie manuelle.");
        GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(F.texte));
        gtk_text_buffer_set_text(buf, "", -1);
        return;
    }

    char s_taille[64], s_type[64], s_cfg[64], s_mem[64], s_info[256];
    snprintf(s_taille, sizeof s_taille, "%zu éléments", ETAT.n);
    snprintf(s_type, sizeof s_type, "%s (%zu octets)", TYPE_NOMS[ETAT.type], type_taille(ETAT.type));
    snprintf(s_cfg, sizeof s_cfg, "%s", ETAT.manuel ? "Saisie manuelle" : CFG_TXT[ETAT.cfg]);

    double ko = (double)(ETAT.n * type_taille(ETAT.type)) / 1024.0;
    if (ko >= 1024.0) snprintf(s_mem, sizeof s_mem, "%.2f Mo", ko / 1024.0);
    else              snprintf(s_mem, sizeof s_mem, "%.1f Ko", ko);

    gtk_label_set_text(GTK_LABEL(F.lbl_stat_taille), s_taille);
    gtk_label_set_text(GTK_LABEL(F.lbl_stat_type), s_type);
    gtk_label_set_text(GTK_LABEL(F.lbl_stat_cfg), s_cfg);
    gtk_label_set_text(GTK_LABEL(F.lbl_stat_mem), s_mem);

    size_t apercu_n = ETAT.n < APERCU_MAX ? ETAT.n : APERCU_MAX;
    snprintf(s_info, sizeof s_info, "Tableau actif en mémoire (%s%s).",
             ETAT.n > APERCU_MAX ? "aperçu des 50 premiers éléments affiché ci-dessous" : "l'intégralité du tableau est affichée",
             ETAT.manuel ? " - issu d'une saisie manuelle" : "");
    gtk_label_set_text(GTK_LABEL(F.lbl_info_apercu), s_info);

    char tampon[4096];
    apercu_tableau(ETAT.tab, ETAT.type, ETAT.n, APERCU_MAX, tampon, sizeof tampon);
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(F.texte));
    gtk_text_buffer_set_text(buf, tampon, -1);
}

static void afficher_erreur(const char *msg) {
    if (!F.erreur) return;
    gtk_label_set_text(GTK_LABEL(F.erreur), msg ? msg : "");
    if (msg && strlen(msg) > 0) {
        gtk_widget_add_css_class(F.erreur, "badge-danger");
    } else {
        gtk_widget_remove_css_class(F.erreur, "badge-danger");
    }
}

static void on_generer(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    guint s = gtk_drop_down_get_selected(GTK_DROP_DOWN(F.dd_taille));
    guint t = gtk_drop_down_get_selected(GTK_DROP_DOWN(F.dd_type));
    guint c = gtk_drop_down_get_selected(GTK_DROP_DOWN(F.dd_cfg));
    if (s == GTK_INVALID_LIST_POSITION || t == GTK_INVALID_LIST_POSITION || c == GTK_INVALID_LIST_POSITION) {
        afficher_erreur("Veuillez sélectionner une taille, un type et une configuration.");
        return;
    }
    srand((unsigned)time(NULL));
    void *tab = generer_tableau((TypeDonnee)t, TAILLES[s], (Configuration)c);
    if (!tab) { afficher_erreur("Mémoire insuffisante pour allouer ce tableau."); return; }
    etat_definir(tab, TAILLES[s], (TypeDonnee)t, (Configuration)c, 0);
    afficher_erreur("");
    maj_statistiques();
}

static void on_saisie(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    guint t = gtk_drop_down_get_selected(GTK_DROP_DOWN(F.dd_type));
    if (t == GTK_INVALID_LIST_POSITION) { afficher_erreur("Veuillez choisir un type de données."); return; }
    char err[256] = "";
    size_t n = 0;
    void *tab = parser_saisie(gtk_editable_get_text(GTK_EDITABLE(F.entry)), (TypeDonnee)t, &n, err, sizeof err);
    if (!tab) { afficher_erreur(err); return; }
    etat_definir(tab, n, (TypeDonnee)t, CFG_ALEATOIRE, 1);
    afficher_erreur("");
    maj_statistiques();
}

static void on_preset_alea(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(F.dd_type), 0);
    gtk_editable_set_text(GTK_EDITABLE(F.entry), "42, 17, 88, 3, 65, 91, 12, 54, 76, 29, 38, 5, 80, 23, 67");
    on_saisie(NULL, NULL);
}

static void on_preset_inverse(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(F.dd_type), 0);
    gtk_editable_set_text(GTK_EDITABLE(F.entry), "100, 92, 85, 78, 64, 53, 47, 36, 25, 14, 8, 2");
    on_saisie(NULL, NULL);
}

static void on_preset_trie(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    gtk_drop_down_set_selected(GTK_DROP_DOWN(F.dd_type), 0);
    gtk_editable_set_text(GTK_EDITABLE(F.entry), "3, 8, 14, 21, 35, 48, 56, 62, 79, 83, 91, 99");
    on_saisie(NULL, NULL);
}

static void on_reset(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    etat_reset();
    gtk_editable_set_text(GTK_EDITABLE(F.entry), "");
    afficher_erreur("");
    maj_statistiques();
}

static void on_aller_execution(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    gui_afficher_page("execution");
}

static GtkWidget *creer_stat_pill(const char *label, GtkWidget **lbl_val) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_add_css_class(box, "stat-pill");
    gtk_widget_set_hexpand(box, TRUE);

    GtkWidget *l_titre = gtk_label_new(label);
    gtk_widget_add_css_class(l_titre, "stat-pill-label");
    gtk_label_set_xalign(GTK_LABEL(l_titre), 0.0f);

    GtkWidget *l_val = gtk_label_new("—");
    gtk_widget_add_css_class(l_val, "stat-pill-val");
    gtk_label_set_xalign(GTK_LABEL(l_val), 0.0f);

    gtk_box_append(GTK_BOX(box), l_titre);
    gtk_box_append(GTK_BOX(box), l_val);
    if (lbl_val) *lbl_val = l_val;
    return box;
}

GtkWidget *gui_tableaux_page_creer(void) {
    GtkWidget *racine = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(racine, TRUE);
    gtk_widget_set_hexpand(racine, TRUE);

    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_add_css_class(page, "content-area");

    /* En-tete de la page */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(header, "page-header");
    GtkWidget *titre = gtk_label_new("Gestion des Données & Tableaux");
    gtk_widget_add_css_class(titre, "page-title");
    gtk_label_set_xalign(GTK_LABEL(titre), 0.0f);
    GtkWidget *sous_titre = gtk_label_new("Générez des jeux de données d'essais ou saisissez vos propres valeurs pour tester les tris.");
    gtk_widget_add_css_class(sous_titre, "page-description");
    gtk_label_set_xalign(GTK_LABEL(sous_titre), 0.0f);
    gtk_box_append(GTK_BOX(header), titre);
    gtk_box_append(GTK_BOX(header), sous_titre);
    gtk_box_append(GTK_BOX(page), header);

    /* Ligne des statistiques d'etat */
    GtkWidget *stats_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_append(GTK_BOX(stats_row), creer_stat_pill("Taille en mémoire", &F.lbl_stat_taille));
    gtk_box_append(GTK_BOX(stats_row), creer_stat_pill("Type de données", &F.lbl_stat_type));
    gtk_box_append(GTK_BOX(stats_row), creer_stat_pill("Configuration", &F.lbl_stat_cfg));
    gtk_box_append(GTK_BOX(stats_row), creer_stat_pill("Empreinte mémoire", &F.lbl_stat_mem));
    gtk_box_append(GTK_BOX(page), stats_row);

    /* Deux colonnes : Generation automatique & Saisie manuelle */
    GtkWidget *grille_actions = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grille_actions), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grille_actions), 16);
    gtk_grid_set_column_homogeneous(GTK_GRID(grille_actions), TRUE);

    /* --- CARTE 1 : Génération automatique --- */
    GtkWidget *card_gen = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(card_gen, "card");

    GtkWidget *titre_gen = gtk_label_new("🎲 Génération Automatique");
    gtk_widget_add_css_class(titre_gen, "card-title");
    gtk_label_set_xalign(GTK_LABEL(titre_gen), 0.0f);
    gtk_box_append(GTK_BOX(card_gen), titre_gen);

    GtkWidget *sous_gen = gtk_label_new("Générez des tableaux de 100 à 100 000 éléments avec distribution personnalisée.");
    gtk_widget_add_css_class(sous_gen, "card-subtitle");
    gtk_label_set_xalign(GTK_LABEL(sous_gen), 0.0f);
    gtk_box_append(GTK_BOX(card_gen), sous_gen);

    GtkWidget *grid_form = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_form), 12);
    gtk_grid_set_row_spacing(GTK_GRID(grid_form), 10);

    GtkWidget *l1 = gtk_label_new("Taille du tableau :");
    gtk_label_set_xalign(GTK_LABEL(l1), 0.0f);
    F.dd_taille = gtk_drop_down_new_from_strings(TAILLES_TXT);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(F.dd_taille), 2); /* 1000 par defaut */
    gtk_widget_set_hexpand(F.dd_taille, TRUE);

    GtkWidget *l2 = gtk_label_new("Type de données :");
    gtk_label_set_xalign(GTK_LABEL(l2), 0.0f);
    F.dd_type = gtk_drop_down_new_from_strings(TYPES_TXT);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(F.dd_type), 0); /* int */
    gtk_widget_set_hexpand(F.dd_type, TRUE);

    GtkWidget *l3 = gtk_label_new("Configuration :");
    gtk_label_set_xalign(GTK_LABEL(l3), 0.0f);
    F.dd_cfg = gtk_drop_down_new_from_strings(CFG_TXT);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(F.dd_cfg), 0); /* aléatoire */
    gtk_widget_set_hexpand(F.dd_cfg, TRUE);

    gtk_grid_attach(GTK_GRID(grid_form), l1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_form), F.dd_taille, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_form), l2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_form), F.dd_type, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_form), l3, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_form), F.dd_cfg, 1, 2, 1, 1);
    gtk_box_append(GTK_BOX(card_gen), grid_form);

    GtkWidget *btn_gen = gtk_button_new_with_label("🎲 Générer le tableau");
    gtk_widget_add_css_class(btn_gen, "btn-primary");
    g_signal_connect(btn_gen, "clicked", G_CALLBACK(on_generer), NULL);
    gtk_box_append(GTK_BOX(card_gen), btn_gen);

    /* --- CARTE 2 : Saisie manuelle --- */
    GtkWidget *card_man = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(card_man, "card");

    GtkWidget *titre_man = gtk_label_new("✏️ Saisie Manuelle Personnalisée");
    gtk_widget_add_css_class(titre_man, "card-title");
    gtk_label_set_xalign(GTK_LABEL(titre_man), 0.0f);
    gtk_box_append(GTK_BOX(card_man), titre_man);

    GtkWidget *sous_man = gtk_label_new("Saisissez entre 10 et 30 valeurs séparées par des espaces ou virgules.");
    gtk_widget_add_css_class(sous_man, "card-subtitle");
    gtk_label_set_xalign(GTK_LABEL(sous_man), 0.0f);
    gtk_box_append(GTK_BOX(card_man), sous_man);

    F.entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(F.entry), "ex : 42, 17, 88, 3, 65, 91, 12, 54, 76, 29, 38");
    gtk_box_append(GTK_BOX(card_man), F.entry);

    /* Boutons pré-remplis pratiques */
    GtkWidget *chips_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *chip1 = gtk_button_new_with_label("🎲 Aléatoire");
    GtkWidget *chip2 = gtk_button_new_with_label("📉 Inversé");
    GtkWidget *chip3 = gtk_button_new_with_label("📈 Trié");
    gtk_widget_add_css_class(chip1, "btn-chip");
    gtk_widget_add_css_class(chip2, "btn-chip");
    gtk_widget_add_css_class(chip3, "btn-chip");
    g_signal_connect(chip1, "clicked", G_CALLBACK(on_preset_alea), NULL);
    g_signal_connect(chip2, "clicked", G_CALLBACK(on_preset_inverse), NULL);
    g_signal_connect(chip3, "clicked", G_CALLBACK(on_preset_trie), NULL);
    gtk_box_append(GTK_BOX(chips_box), chip1);
    gtk_box_append(GTK_BOX(chips_box), chip2);
    gtk_box_append(GTK_BOX(chips_box), chip3);
    gtk_box_append(GTK_BOX(card_man), chips_box);

    GtkWidget *btn_man = gtk_button_new_with_label("✏️ Valider la saisie manuelle");
    gtk_widget_add_css_class(btn_man, "btn-secondary");
    g_signal_connect(btn_man, "clicked", G_CALLBACK(on_saisie), NULL);
    gtk_box_append(GTK_BOX(card_man), btn_man);

    F.erreur = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(F.erreur), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(F.erreur), TRUE);
    gtk_box_append(GTK_BOX(card_man), F.erreur);

    gtk_grid_attach(GTK_GRID(grille_actions), card_gen, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grille_actions), card_man, 1, 0, 1, 1);
    gtk_box_append(GTK_BOX(page), grille_actions);

    /* --- CARTE 3 : Aperçu en mémoire --- */
    GtkWidget *card_prev = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(card_prev, "card");

    GtkWidget *prev_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *titre_prev = gtk_label_new("📋 Aperçu du Tableau en Mémoire");
    gtk_widget_add_css_class(titre_prev, "card-title");
    gtk_label_set_xalign(GTK_LABEL(titre_prev), 0.0f);
    gtk_widget_set_hexpand(titre_prev, TRUE);
    gtk_box_append(GTK_BOX(prev_header), titre_prev);
    gtk_box_append(GTK_BOX(card_prev), prev_header);

    F.lbl_info_apercu = gtk_label_new("Aucun tableau en mémoire.");
    gtk_widget_add_css_class(F.lbl_info_apercu, "card-subtitle");
    gtk_label_set_xalign(GTK_LABEL(F.lbl_info_apercu), 0.0f);
    gtk_box_append(GTK_BOX(card_prev), F.lbl_info_apercu);

    F.texte = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(F.texte), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(F.texte), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(F.texte), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(F.texte), GTK_WRAP_WORD_CHAR);
    gtk_widget_add_css_class(F.texte, "terminal-view");

    GtkWidget *defil = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(defil), F.texte);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(defil), 100);
    gtk_widget_add_css_class(defil, "terminal-card");
    gtk_box_append(GTK_BOX(card_prev), defil);

    /* Barre d'action inférieure */
    GtkWidget *barre_action = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *b_reset = gtk_button_new_with_label("🗑️ Réinitialiser");
    gtk_widget_add_css_class(b_reset, "btn-secondary");
    g_signal_connect(b_reset, "clicked", G_CALLBACK(on_reset), NULL);

    GtkWidget *b_suivant = gtk_button_new_with_label("⚡ Passer au Benchmark ➔");
    gtk_widget_add_css_class(b_suivant, "btn-success");
    g_signal_connect(b_suivant, "clicked", G_CALLBACK(on_aller_execution), NULL);

    gtk_box_append(GTK_BOX(barre_action), b_reset);
    GtkWidget *sp = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(sp, TRUE);
    gtk_box_append(GTK_BOX(barre_action), sp);
    gtk_box_append(GTK_BOX(barre_action), b_suivant);
    gtk_box_append(GTK_BOX(card_prev), barre_action);

    gtk_box_append(GTK_BOX(page), card_prev);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(racine), page);
    maj_statistiques();
    return racine;
}

void gui_tableaux_ouvrir(GtkWindow *parent) {
    (void)parent;
    gui_afficher_page("tableaux");
}
