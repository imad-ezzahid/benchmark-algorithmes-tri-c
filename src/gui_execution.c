/* Page 2 : Execution des algorithmes et benchmark */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gui.h"
#include "state.h"
#include "bench.h"

#define FICHIER_CSV "resultats.csv"

/* ---------- travail du thread ---------- */
typedef struct {
    int           campagne;
    void         *tab;            /* mode simple : COPIE du tableau */
    size_t        n, sz;
    size_t        taille_max;     /* mode campagne */
    TypeDonnee    type;
    Configuration cfg;
    cmp_fn        cmp;
    int           algos[NB_ALGOS];
    int           nb;
    volatile gint stop;           /* demande d'arret */
} Job;

static Job JOB;
static volatile gint EN_COURS = 0;

enum { MSG_PROGRES, MSG_RESULTAT, MSG_FIN, MSG_LOG };
typedef struct {
    int kind;
    double frac;
    int algo;
    TypeDonnee type;
    Configuration cfg;
    size_t n;
    double temps;
    int ok;
    int err;
    int arrete;
    char texte_log[256];
} Msg;

static gboolean ui_msg(gpointer p);

static void envoyer(Msg m) {
    Msg *c = g_new(Msg, 1);
    *c = m;
    g_idle_add(ui_msg, c);
}

static void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[256];
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char horodatage[32];
    strftime(horodatage, sizeof horodatage, "%H:%M:%S", t);

    Msg m;
    memset(&m, 0, sizeof m);
    m.kind = MSG_LOG;
    snprintf(m.texte_log, sizeof m.texte_log, "[%s] %s\n", horodatage, buf);
    envoyer(m);
}

/* Une mesure complete (algo x tableau) : 0 = ok, 1 = arret demande, 2 = erreur memoire */
static int executer(Job *j, const void *tab, size_t n, Configuration cfg, int algo, int *fait, int total) {
    int reps = repetitions_pour(n), ok_tout = 1;
    double somme = 0.0;
    for (int r = 0; r < reps; r++) {
        if (g_atomic_int_get(&j->stop)) return 1;
        int ok = 0;
        double t = mesurer(ALGO_FONCTIONS[algo], tab, n, j->sz, j->cmp, 1, &ok);
        if (t < 0) return 2;
        somme += t;
        if (!ok) ok_tout = 0;
        (*fait)++;
        envoyer((Msg){ .kind = MSG_PROGRES, .frac = (double)(*fait) / total, .algo = algo });
    }
    double temps_moyen = somme / reps;
    envoyer((Msg){ .kind = MSG_RESULTAT, .algo = algo, .type = j->type, .cfg = cfg,
                   .n = n, .temps = temps_moyen, .ok = ok_tout });
    return 0;
}

static int saute(const Job *j, int algo, size_t n) {
    (void)j;
    return ALGO_QUADRATIQUE[algo] && n > SEUIL_QUADRATIQUE;
}

static gpointer worker(gpointer data) {
    Job *j = data;
    int fait = 0, code = 0;
    srand(42); /* graine fixe pour reproductibilite */

    if (!j->campagne) {
        int total = j->nb * repetitions_pour(j->n);
        log_info("▶ Démarrage du tri du tableau (taille: %zu, type: %s, config: %s)",
                 j->n, TYPE_NOMS[j->type], CFG_NOMS[j->cfg]);
        for (int a = 0; a < j->nb && !code; a++) {
            code = executer(j, j->tab, j->n, j->cfg, j->algos[a], &fait, total);
        }
    } else {
        int total = 0;
        for (int c = 0; c < 3; c++)
            for (int s = 0; s < NB_TAILLES; s++) {
                size_t n = TAILLES_STD[s];
                if (n > j->taille_max) continue;
                for (int a = 0; a < j->nb; a++)
                    if (!saute(j, j->algos[a], n)) total += repetitions_pour(n);
            }
        if (total == 0) total = 1;

        log_info("🚀 Démarrage de la campagne automatique (max: %zu, type: %s)",
                 j->taille_max, TYPE_NOMS[j->type]);

        for (int c = 0; c < 3 && !code; c++)
            for (int s = 0; s < NB_TAILLES && !code; s++) {
                size_t n = TAILLES_STD[s];
                if (n > j->taille_max) continue;
                void *tab = generer_tableau(j->type, n, (Configuration)c);
                if (!tab) { code = 2; break; }
                for (int a = 0; a < j->nb && !code; a++) {
                    if (!saute(j, j->algos[a], n)) {
                        code = executer(j, tab, n, (Configuration)c, j->algos[a], &fait, total);
                    }
                }
                free(tab);
            }
    }

    free(j->tab);
    j->tab = NULL;
    g_atomic_int_set(&EN_COURS, 0);
    envoyer((Msg){ .kind = MSG_FIN, .err = (code == 2), .arrete = (code == 1) });
    return NULL;
}

/* ---------- interface ---------- */
static const char  *MAXI_TXT[] = { "1 000", "5 000", "10 000", "50 000", "100 000", NULL };
static const size_t MAXI[]     = { 1000, 5000, 10000, 50000, 100000 };
static const char  *TYPES_TXT[] = { "int", "float", "char", "double", NULL };

static struct {
    GtkWidget *lbl_info_memoire;
    GtkWidget *cases[NB_ALGOS];
    GtkWidget *btn_lancer, *btn_campagne, *btn_arret;
    GtkWidget *dd_type, *dd_max;
    GtkWidget *barre, *lbl_statut, *lbl_erreur;
    GtkWidget *texte_console;
} E;

static void maj_info_memoire(void) {
    if (!E.lbl_info_memoire) return;
    char s[256];
    if (!etat_a_donnees()) {
        snprintf(s, sizeof s, "⚠️ Aucun tableau en mémoire : générez-en un dans l'onglet « Tableaux » ou utilisez la Campagne automatique ci-dessous.");
    } else {
        snprintf(s, sizeof s, "✅ Tableau prêt : %zu éléments | Type : %s | Configuration : %s",
                 ETAT.n, TYPE_NOMS[ETAT.type], ETAT.manuel ? "Saisie manuelle" : CFG_NOMS[ETAT.cfg]);
    }
    gtk_label_set_text(GTK_LABEL(E.lbl_info_memoire), s);
}

static void ajouter_log(const char *s) {
    if (!E.texte_console) return;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(E.texte_console));
    GtkTextIter fin;
    gtk_text_buffer_get_end_iter(buf, &fin);
    gtk_text_buffer_insert(buf, &fin, s, -1);
}

static void mode_calcul(int en_cours) {
    if (!E.btn_lancer) return;
    gtk_widget_set_sensitive(E.btn_lancer, !en_cours);
    gtk_widget_set_sensitive(E.btn_campagne, !en_cours);
    gtk_widget_set_sensitive(E.btn_arret, en_cours);
    gui_maj_statut(en_cours ? "Benchmark en cours..." : "Prêt", en_cours);
}

static gboolean ui_msg(gpointer p) {
    Msg *m = p;
    if (m->kind == MSG_RESULTAT) {
        etat_ajouter_resultat(m->algo, m->type, m->cfg, m->n, m->temps, m->ok);
        csv_ajouter(FICHIER_CSV, ALGO_NOMS[m->algo], m->type, m->cfg, m->n, m->temps, m->ok);
        gui_resultats_rafraichir();
        gui_courbes_rafraichir();
    }

    char s[256];
    switch (m->kind) {
    case MSG_LOG:
        ajouter_log(m->texte_log);
        break;
    case MSG_PROGRES:
        if (E.barre) {
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(E.barre), m->frac);
            snprintf(s, sizeof s, "%s  —  %.0f %%", ALGO_NOMS[m->algo], m->frac * 100.0);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(E.barre), s);
        }
        break;
    case MSG_RESULTAT: {
        char temps_str[32];
        if (m->temps < 0.001) snprintf(temps_str, sizeof temps_str, "%.1f µs", m->temps * 1e6);
        else if (m->temps < 1.0) snprintf(temps_str, sizeof temps_str, "%.2f ms", m->temps * 1e3);
        else snprintf(temps_str, sizeof temps_str, "%.3f s", m->temps);

        snprintf(s, sizeof s, "✔ %-18s [%s, n=%zu] : %10.6f s (%s)  %s\n",
                 ALGO_NOMS[m->algo], CFG_NOMS[m->cfg], m->n, m->temps, temps_str,
                 m->ok ? "✔ Tri validé" : "❌ ERREUR DE TRI");
        ajouter_log(s);
        break;
    }
    case MSG_FIN:
        mode_calcul(0);
        if (m->err) {
            if (E.lbl_statut) gtk_label_set_text(GTK_LABEL(E.lbl_statut), "❌ Erreur : mémoire insuffisante.");
            ajouter_log("❌ Erreur fatale : allocation mémoire impossible.\n");
        } else if (m->arrete) {
            if (E.lbl_statut) gtk_label_set_text(GTK_LABEL(E.lbl_statut), "⏹ Arrêté par l'utilisateur.");
            ajouter_log("⏹ Benchmark interrompu par l'utilisateur.\n");
        } else {
            if (E.lbl_statut) gtk_label_set_text(GTK_LABEL(E.lbl_statut), "✅ Terminé avec succès. Résultats enregistrés.");
            if (E.barre) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(E.barre), 1.0);
            ajouter_log("★ Session de calcul terminée. Résultats ajoutés à resultats.csv.\n");
        }
        break;
    }

    g_free(m);
    return G_SOURCE_REMOVE;
}

static int preparer_job(int *quadratique) {
    if (E.lbl_erreur) gtk_label_set_text(GTK_LABEL(E.lbl_erreur), "");
    memset(&JOB, 0, sizeof JOB);
    *quadratique = 0;
    for (int i = 0; i < NB_ALGOS; i++) {
        if (gtk_check_button_get_active(GTK_CHECK_BUTTON(E.cases[i]))) {
            JOB.algos[JOB.nb++] = i;
            if (ALGO_QUADRATIQUE[i]) *quadratique = 1;
        }
    }
    if (JOB.nb == 0) {
        if (E.lbl_erreur) gtk_label_set_text(GTK_LABEL(E.lbl_erreur), "Veuillez sélectionner au moins un algorithme.");
        return 0;
    }
    return 1;
}

static void demarrer(void) {
    if (E.barre) gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(E.barre), 0.0);
    mode_calcul(1);
    g_atomic_int_set(&EN_COURS, 1);
    GThread *th = g_thread_new("tri", worker, &JOB);
    g_thread_unref(th);
}

static void on_lancer(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    if (g_atomic_int_get(&EN_COURS)) return;
    maj_info_memoire();
    if (!etat_a_donnees()) {
        if (E.lbl_erreur) gtk_label_set_text(GTK_LABEL(E.lbl_erreur), "Aucun tableau en mémoire : générez-en un d'abord dans l'onglet « Tableaux ».");
        return;
    }
    int quadratique;
    if (!preparer_job(&quadratique)) return;
    JOB.campagne = 0;
    JOB.n = ETAT.n; JOB.sz = type_taille(ETAT.type); JOB.type = ETAT.type; JOB.cfg = ETAT.cfg;
    JOB.cmp = type_cmp(ETAT.type);
    JOB.tab = malloc(JOB.n * JOB.sz);
    if (!JOB.tab) {
        if (E.lbl_erreur) gtk_label_set_text(GTK_LABEL(E.lbl_erreur), "Mémoire insuffisante pour la copie de travail.");
        return;
    }
    memcpy(JOB.tab, ETAT.tab, JOB.n * JOB.sz);

    char s[256];
    snprintf(s, sizeof s, "\n======================================================\n"
                          "Tri du tableau courant (%zu éléments, %s, %s)\n"
                          "Moyenne sur %d répétitions haute précision\n"
                          "======================================================\n",
             JOB.n, TYPE_NOMS[JOB.type], ETAT.manuel ? "manuel" : CFG_NOMS[JOB.cfg], repetitions_pour(JOB.n));
    ajouter_log(s);

    if (E.lbl_statut) {
        gtk_label_set_text(GTK_LABEL(E.lbl_statut), (quadratique && JOB.n >= 50000)
            ? "En cours… Les tris quadratiques O(n²) sur grandes tailles peuvent prendre plusieurs secondes."
            : "Benchmark en cours…");
    }
    demarrer();
}

static void on_campagne(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    if (g_atomic_int_get(&EN_COURS)) return;
    guint t = gtk_drop_down_get_selected(GTK_DROP_DOWN(E.dd_type));
    guint m = gtk_drop_down_get_selected(GTK_DROP_DOWN(E.dd_max));
    if (t == GTK_INVALID_LIST_POSITION || m == GTK_INVALID_LIST_POSITION) {
        if (E.lbl_erreur) gtk_label_set_text(GTK_LABEL(E.lbl_erreur), "Sélectionnez un type et une taille maximale.");
        return;
    }
    int quadratique;
    if (!preparer_job(&quadratique)) return;
    JOB.campagne = 1;
    JOB.type = (TypeDonnee)t;
    JOB.sz = type_taille(JOB.type);
    JOB.cmp = type_cmp(JOB.type);
    JOB.taille_max = MAXI[m];

    char s[256];
    snprintf(s, sizeof s, "\n======================================================\n"
                          "Campagne Complète : Type %s | Tailles jusqu'à %zu\n"
                          "3 configurations testées (aléatoire, croissant, décroissant)\n"
                          "======================================================\n",
             TYPE_NOMS[JOB.type], JOB.taille_max);
    ajouter_log(s);

    if (E.lbl_statut) {
        gtk_label_set_text(GTK_LABEL(E.lbl_statut), "Campagne en cours… (tris O(n²) ignorés au-delà de 50 000 éléments)");
    }
    demarrer();
}

static void on_arret(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    g_atomic_int_set(&JOB.stop, 1);
    if (E.lbl_statut) gtk_label_set_text(GTK_LABEL(E.lbl_statut), "Arrêt demandé… Pris en compte dès la fin de l'itération en cours.");
}

static void on_select_tous(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    for (int i = 0; i < NB_ALGOS; i++) gtk_check_button_set_active(GTK_CHECK_BUTTON(E.cases[i]), TRUE);
}

static void on_select_rapides(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    for (int i = 0; i < NB_ALGOS; i++) gtk_check_button_set_active(GTK_CHECK_BUTTON(E.cases[i]), !ALGO_QUADRATIQUE[i]);
}

static void on_select_quadratiques(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    for (int i = 0; i < NB_ALGOS; i++) gtk_check_button_set_active(GTK_CHECK_BUTTON(E.cases[i]), ALGO_QUADRATIQUE[i]);
}

static void on_select_aucun(GtkButton *b, gpointer data) {
    (void)b; (void)data;
    for (int i = 0; i < NB_ALGOS; i++) gtk_check_button_set_active(GTK_CHECK_BUTTON(E.cases[i]), FALSE);
}

static void on_aller_resultats(GtkButton *b, gpointer data) { (void)b; (void)data; gui_afficher_page("resultats"); }
static void on_aller_courbes(GtkButton *b, gpointer data)   { (void)b; (void)data; gui_afficher_page("courbes"); }

/* Couleurs des pastilles pour chaque algo */
static const char *PASTILLES[NB_ALGOS] = {
    "<span foreground='#ef4444'>●</span> ",  /* Tri par selection (rouge) */
    "<span foreground='#3b82f6'>●</span> ",  /* Tri par insertion (bleu) */
    "<span foreground='#22c55e'>●</span> ",  /* Tri a bulles (vert) */
    "<span foreground='#f97316'>●</span> ",  /* Tri fusion (orange) */
    "<span foreground='#a855f7'>●</span> ",  /* Tri Shell (violet) */
    "<span foreground='#06b6d4'>●</span> "   /* Tri rapide (cyan) */
};

GtkWidget *gui_execution_page_creer(void) {
    GtkWidget *racine = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(racine, TRUE);
    gtk_widget_set_hexpand(racine, TRUE);

    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_add_css_class(page, "content-area");

    /* En-tete */
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(header, "page-header");
    GtkWidget *titre = gtk_label_new("Exécution & Benchmark des Tris");
    gtk_widget_add_css_class(titre, "page-title");
    gtk_label_set_xalign(GTK_LABEL(titre), 0.0f);
    GtkWidget *sous = gtk_label_new("Mesurez les temps d'exécution réels sur des copies mémoires isolées avec vérification de conformité.");
    gtk_widget_add_css_class(sous, "page-description");
    gtk_label_set_xalign(GTK_LABEL(sous), 0.0f);
    gtk_box_append(GTK_BOX(header), titre);
    gtk_box_append(GTK_BOX(header), sous);
    gtk_box_append(GTK_BOX(page), header);

    /* Statut memoire */
    E.lbl_info_memoire = gtk_label_new("");
    gtk_widget_add_css_class(E.lbl_info_memoire, "card");
    gtk_label_set_xalign(GTK_LABEL(E.lbl_info_memoire), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(E.lbl_info_memoire), TRUE);
    gtk_box_append(GTK_BOX(page), E.lbl_info_memoire);

    /* CARTE 1 : Sélection des algorithmes */
    GtkWidget *card_algos = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(card_algos, "card");

    GtkWidget *titre_algos = gtk_label_new("⚙️ Algorithmes à Comparer");
    gtk_widget_add_css_class(titre_algos, "card-title");
    gtk_label_set_xalign(GTK_LABEL(titre_algos), 0.0f);
    gtk_box_append(GTK_BOX(card_algos), titre_algos);

    /* Grille pour les algorithmes (2 colonnes) */
    GtkWidget *grille_chks = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grille_chks), 24);
    gtk_grid_set_row_spacing(GTK_GRID(grille_chks), 8);

    /* Colonne 1 : Tris optimaux O(n log n) */
    GtkWidget *lbl_col1 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_col1), "<b>Tris Rapides O(n log n) & O(n log² n)</b>");
    gtk_label_set_xalign(GTK_LABEL(lbl_col1), 0.0f);
    gtk_grid_attach(GTK_GRID(grille_chks), lbl_col1, 0, 0, 1, 1);

    /* Colonne 2 : Tris quadratiques O(n²) */
    GtkWidget *lbl_col2 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_col2), "<b>Tris Quadratiques O(n²)</b>");
    gtk_label_set_xalign(GTK_LABEL(lbl_col2), 0.0f);
    gtk_grid_attach(GTK_GRID(grille_chks), lbl_col2, 1, 0, 1, 1);

    int row_fast = 1, row_quad = 1;
    for (int i = 0; i < NB_ALGOS; i++) {
        char label_buf[128];
        snprintf(label_buf, sizeof label_buf, "%s %s (%s)", PASTILLES[i], ALGO_NOMS[i], ALGO_COMPLEXITE_MOY[i]);
        E.cases[i] = gtk_check_button_new();
        GtkWidget *l = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(l), label_buf);
        GtkWidget *box_chk = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_box_append(GTK_BOX(box_chk), E.cases[i]);
        gtk_box_append(GTK_BOX(box_chk), l);
        gtk_check_button_set_active(GTK_CHECK_BUTTON(E.cases[i]), TRUE);

        if (!ALGO_QUADRATIQUE[i]) {
            gtk_grid_attach(GTK_GRID(grille_chks), box_chk, 0, row_fast++, 1, 1);
        } else {
            gtk_grid_attach(GTK_GRID(grille_chks), box_chk, 1, row_quad++, 1, 1);
        }
    }
    gtk_box_append(GTK_BOX(card_algos), grille_chks);

    /* Boutons de sélection rapide */
    GtkWidget *barre_sel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *b_tous = gtk_button_new_with_label("Tout cocher");
    GtkWidget *b_rap = gtk_button_new_with_label("Tris rapides seuls");
    GtkWidget *b_quad = gtk_button_new_with_label("Tris O(n²) seuls");
    GtkWidget *b_non = gtk_button_new_with_label("Tout décocher");
    gtk_widget_add_css_class(b_tous, "btn-chip");
    gtk_widget_add_css_class(b_rap, "btn-chip");
    gtk_widget_add_css_class(b_quad, "btn-chip");
    gtk_widget_add_css_class(b_non, "btn-chip");
    g_signal_connect(b_tous, "clicked", G_CALLBACK(on_select_tous), NULL);
    g_signal_connect(b_rap, "clicked", G_CALLBACK(on_select_rapides), NULL);
    g_signal_connect(b_quad, "clicked", G_CALLBACK(on_select_quadratiques), NULL);
    g_signal_connect(b_non, "clicked", G_CALLBACK(on_select_aucun), NULL);
    gtk_box_append(GTK_BOX(barre_sel), b_tous);
    gtk_box_append(GTK_BOX(barre_sel), b_rap);
    gtk_box_append(GTK_BOX(barre_sel), b_quad);
    gtk_box_append(GTK_BOX(barre_sel), b_non);
    gtk_box_append(GTK_BOX(card_algos), barre_sel);
    gtk_box_append(GTK_BOX(page), card_algos);

    /* CARTE 2 : Lancement des tris */
    GtkWidget *card_launch = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_widget_add_css_class(card_launch, "card");

    GtkWidget *titre_launch = gtk_label_new("🚀 Lancement des Mesures");
    gtk_widget_add_css_class(titre_launch, "card-title");
    gtk_label_set_xalign(GTK_LABEL(titre_launch), 0.0f);
    gtk_box_append(GTK_BOX(card_launch), titre_launch);

    GtkWidget *grid_modes = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid_modes), 16);
    gtk_grid_set_row_spacing(GTK_GRID(grid_modes), 12);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid_modes), TRUE);

    /* Mode A : Tri du tableau courant */
    GtkWidget *box_mode_a = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *lbl_a = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_a), "<b>Option 1 : Tableau actuel en mémoire</b>\n<span size='small' foreground='#94a3b8'>Exécute les tris cochés sur le tableau généré</span>");
    gtk_label_set_xalign(GTK_LABEL(lbl_a), 0.0f);
    E.btn_lancer = gtk_button_new_with_label("▶ Lancer le tri du tableau");
    gtk_widget_add_css_class(E.btn_lancer, "btn-primary");
    g_signal_connect(E.btn_lancer, "clicked", G_CALLBACK(on_lancer), NULL);
    gtk_box_append(GTK_BOX(box_mode_a), lbl_a);
    gtk_box_append(GTK_BOX(box_mode_a), E.btn_lancer);

    /* Mode B : Campagne complète */
    GtkWidget *box_mode_b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *lbl_b = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_b), "<b>Option 2 : Campagne automatique complète</b>\n<span size='small' foreground='#94a3b8'>Toutes tailles × 3 configurations (aléatoire, croissant, décroissant)</span>");
    gtk_label_set_xalign(GTK_LABEL(lbl_b), 0.0f);

    GtkWidget *box_params = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl_t = gtk_label_new("Type :");
    E.dd_type = gtk_drop_down_new_from_strings(TYPES_TXT);
    GtkWidget *lbl_m = gtk_label_new("Taille max :");
    E.dd_max = gtk_drop_down_new_from_strings(MAXI_TXT);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(E.dd_max), 2); /* 10 000 par défaut */
    E.btn_campagne = gtk_button_new_with_label("🚀 Lancer campagne");
    gtk_widget_add_css_class(E.btn_campagne, "btn-secondary");
    g_signal_connect(E.btn_campagne, "clicked", G_CALLBACK(on_campagne), NULL);

    gtk_box_append(GTK_BOX(box_params), lbl_t);
    gtk_box_append(GTK_BOX(box_params), E.dd_type);
    gtk_box_append(GTK_BOX(box_params), lbl_m);
    gtk_box_append(GTK_BOX(box_params), E.dd_max);
    gtk_box_append(GTK_BOX(box_params), E.btn_campagne);

    gtk_box_append(GTK_BOX(box_mode_b), lbl_b);
    gtk_box_append(GTK_BOX(box_mode_b), box_params);

    gtk_grid_attach(GTK_GRID(grid_modes), box_mode_a, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_modes), box_mode_b, 1, 0, 1, 1);
    gtk_box_append(GTK_BOX(card_launch), grid_modes);

    /* Bouton d'arret */
    GtkWidget *row_stop = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    E.btn_arret = gtk_button_new_with_label("⏹ Arrêter l'exécution en cours");
    gtk_widget_add_css_class(E.btn_arret, "btn-danger");
    g_signal_connect(E.btn_arret, "clicked", G_CALLBACK(on_arret), NULL);
    gtk_box_append(GTK_BOX(row_stop), E.btn_arret);
    gtk_box_append(GTK_BOX(card_launch), row_stop);

    /* Barre de progression et Statut */
    E.barre = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(E.barre), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(E.barre), "En attente");
    gtk_box_append(GTK_BOX(card_launch), E.barre);

    E.lbl_statut = gtk_label_new("Prêt.");
    gtk_label_set_xalign(GTK_LABEL(E.lbl_statut), 0.0f);
    gtk_box_append(GTK_BOX(card_launch), E.lbl_statut);

    E.lbl_erreur = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(E.lbl_erreur), 0.0f);
    gtk_widget_add_css_class(E.lbl_erreur, "badge-danger");
    gtk_box_append(GTK_BOX(card_launch), E.lbl_erreur);

    gtk_box_append(GTK_BOX(page), card_launch);

    /* CARTE 3 : Console de logs */
    GtkWidget *card_log = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(card_log, "card");

    GtkWidget *titre_log = gtk_label_new("💻 Journal des Mesures en Direct");
    gtk_widget_add_css_class(titre_log, "card-title");
    gtk_label_set_xalign(GTK_LABEL(titre_log), 0.0f);
    gtk_box_append(GTK_BOX(card_log), titre_log);

    E.texte_console = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(E.texte_console), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(E.texte_console), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(E.texte_console), TRUE);
    gtk_widget_add_css_class(E.texte_console, "terminal-view");

    GtkWidget *defil_log = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(defil_log), E.texte_console);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(defil_log), 150);
    gtk_widget_add_css_class(defil_log, "terminal-card");
    gtk_box_append(GTK_BOX(card_log), defil_log);

    /* Raccourcis de navigation */
    GtkWidget *nav_bas = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *b_res = gtk_button_new_with_label("📋 Voir le tableau des résultats ➔");
    GtkWidget *b_cour = gtk_button_new_with_label("📈 Visualiser les courbes graphiques ➔");
    gtk_widget_add_css_class(b_res, "btn-secondary");
    gtk_widget_add_css_class(b_cour, "btn-secondary");
    g_signal_connect(b_res, "clicked", G_CALLBACK(on_aller_resultats), NULL);
    g_signal_connect(b_cour, "clicked", G_CALLBACK(on_aller_courbes), NULL);
    gtk_box_append(GTK_BOX(nav_bas), b_res);
    gtk_box_append(GTK_BOX(nav_bas), b_cour);
    gtk_box_append(GTK_BOX(card_log), nav_bas);

    gtk_box_append(GTK_BOX(page), card_log);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(racine), page);
    mode_calcul(0);
    maj_info_memoire();
    return racine;
}

void gui_execution_ouvrir(GtkWindow *parent) {
    (void)parent;
    maj_info_memoire();
    gui_afficher_page("execution");
}
