/* Fenetre 1 : menu principal */
#include "gui.h"

static void on_tableaux(GtkButton *b, gpointer win) { (void)b; gui_tableaux_ouvrir(GTK_WINDOW(win)); }

static void on_execution(GtkButton *b, gpointer win) { (void)b; gui_execution_ouvrir(GTK_WINDOW(win)); }

static void on_resultats(GtkButton *b, gpointer win) { (void)b; gui_resultats_ouvrir(GTK_WINDOW(win)); }

static void on_courbes(GtkButton *b, gpointer win) { (void)b; gui_courbes_ouvrir(GTK_WINDOW(win)); }
static void on_aide(GtkButton *b, gpointer win) { (void)b; gui_aide_ouvrir(GTK_WINDOW(win)); }

static void on_quitter(GtkButton *b, gpointer win) {
    (void)b;
    g_application_quit(G_APPLICATION(gtk_window_get_application(GTK_WINDOW(win))));
}

static GtkWidget *bouton(const char *texte, GCallback cb, gpointer win, const char *titre_placeholder) {
    GtkWidget *b = gtk_button_new_with_label(texte);
    if (titre_placeholder) g_object_set_data(G_OBJECT(b), "titre", (gpointer)titre_placeholder);
    g_signal_connect(b, "clicked", cb, win);
    return b;
}

void gui_menu_ouvrir(GtkApplication *app) {
    GtkWidget *win = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(win), "Comparaison des algorithmes de tri");
    gtk_window_set_default_size(GTK_WINDOW(win), 420, 460);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 24);  gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 20);    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *titre = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(titre), "<span size='x-large' weight='bold'>Menu principal</span>");
    GtkWidget *sous = gtk_label_new("Comparaison expérimentale des algorithmes de tri");
    gtk_widget_set_margin_bottom(sous, 10);

    gtk_box_append(GTK_BOX(box), titre);
    gtk_box_append(GTK_BOX(box), sous);
    gtk_box_append(GTK_BOX(box), bouton("Gestion des tableaux",         G_CALLBACK(on_tableaux), win, NULL));
    gtk_box_append(GTK_BOX(box), bouton("Exécution des tris",           G_CALLBACK(on_execution), win, NULL));
    gtk_box_append(GTK_BOX(box), bouton("Comparaison des performances", G_CALLBACK(on_resultats), win, NULL));
    gtk_box_append(GTK_BOX(box), bouton("Visualisation des courbes",    G_CALLBACK(on_courbes), win, NULL));
    gtk_box_append(GTK_BOX(box), bouton("Aide et informations",         G_CALLBACK(on_aide), win, NULL));
    gtk_box_append(GTK_BOX(box), bouton("Quitter",                      G_CALLBACK(on_quitter), win, NULL));

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_present(GTK_WINDOW(win));
}

/* Fenetre provisoire pour les fenetres 3 a 6 (ajoutees aux etapes suivantes) */
void gui_placeholder_ouvrir(GtkWindow *parent, const char *titre) {
    GtkWidget *w = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(w), titre ? titre : "Fenêtre");
    gtk_window_set_default_size(GTK_WINDOW(w), 360, 140);
    gtk_window_set_transient_for(GTK_WINDOW(w), parent);
    gtk_window_set_application(GTK_WINDOW(w), gtk_window_get_application(parent));
    GtkWidget *l = gtk_label_new("Cette fenêtre sera ajoutée à l'étape suivante.");
    gtk_window_set_child(GTK_WINDOW(w), l);
    gtk_window_present(GTK_WINDOW(w));
}
