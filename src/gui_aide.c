/* Fenetre 6 : aide et informations */
#include "gui.h"

static const char *AIDE =
"DESCRIPTION DES ALGORITHMES\n"
"\n"
"Tri par sélection : cherche le plus petit élément de la partie non triée et le place en tête. "
"Le nombre de comparaisons ne dépend pas de l'ordre initial.\n"
"Tri par insertion : insère chaque élément à sa place dans la partie déjà triée. "
"Très rapide si le tableau est presque trié.\n"
"Tri à bulles : compare les voisins et les échange s'ils sont dans le mauvais ordre. "
"Une version avec arrêt anticipé s'arrête dès qu'un passage ne fait aucun échange.\n"
"Tri fusion : coupe le tableau en deux, trie chaque moitié, puis fusionne. "
"Régulier, mais utilise un tableau auxiliaire.\n"
"Tri Shell : tri par insertion avec des écarts décroissants (suite de Knuth : 1, 4, 13, 40, ...). "
"Entre les tris quadratiques et les tris en n log n.\n"
"Tri rapide : choisit un pivot (médiane de trois), sépare les éléments plus petits et plus grands, "
"puis recommence sur chaque partie.\n"
"\n"
"COMPLEXITÉ TEMPORELLE THÉORIQUE\n"
"\n"
"Algorithme           Moyenne        Pire cas\n"
"Tri par sélection    O(n²)          O(n²)\n"
"Tri par insertion    O(n²)          O(n²)\n"
"Tri à bulles         O(n²)          O(n²)\n"
"Tri fusion           O(n log n)     O(n log n)\n"
"Tri Shell            O(n log² n)    O(n²)   (dépend de la suite d'écarts)\n"
"Tri rapide           O(n log n)     O(n²)\n"
"\n"
"MÉTHODOLOGIE DE MESURE DU TEMPS\n"
"\n"
"- Les algorithmes de tri sont séparés du code de l'interface.\n"
"- Chaque algorithme travaille sur une copie identique du tableau initial.\n"
"- Seul l'appel au tri est chronométré (ni la copie, ni l'affichage).\n"
"- Horloge haute résolution du système ; le temps affiché est une moyenne de plusieurs exécutions "
"(20 jusqu'à 1 000 éléments, 5 jusqu'à 10 000, puis 3).\n"
"- Après chaque exécution, le programme vérifie que le tableau est bien trié.\n"
"- La campagne automatique utilise une graine fixe : les tableaux sont reproductibles. "
"Elle saute les tris quadratiques au-delà de 50 000 éléments (trop longs).\n"
"- Les mesures sont ajoutées à resultats.csv ; l'interface ne se bloque pas car le calcul "
"tourne dans un thread séparé.\n"
"\n"
"CONSEILS D'INTERPRÉTATION DES COURBES\n"
"\n"
"- En échelle logarithmique sur les deux axes, une courbe en n² est une droite de pente environ 2, "
"et une courbe en n log n est une droite de pente un peu supérieure à 1.\n"
"- Si n double : un tri quadratique voit son temps multiplié par environ 4 ; "
"un tri en n log n par un peu plus de 2.\n"
"- Pour les petites tailles, les temps sont très courts : la mesure est bruitée "
"(résolution de l'horloge, cache, charge du système).\n"
"- Comparez les trois configurations : l'insertion et les bulles (avec arrêt anticipé) sont très "
"rapides sur un tableau déjà trié, et lents sur un tableau décroissant.\n"
"- Les écarts avec la théorie viennent des constantes cachées dans la notation O, "
"du cache mémoire, des optimisations du compilateur et du coût des allocations.\n";

static void on_fermer(GtkButton *b, gpointer win) { (void)b; gtk_window_close(GTK_WINDOW(win)); }

void gui_aide_ouvrir(GtkWindow *parent) {
    GtkWidget *win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Aide et informations");
    gtk_window_set_default_size(GTK_WINDOW(win), 680, 580);
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_application(GTK_WINDOW(win), gtk_window_get_application(parent));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 12);  gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12);    gtk_widget_set_margin_bottom(box, 12);

    GtkWidget *tv = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tv), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(tv), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(tv), 8);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv)), AIDE, -1);

    GtkWidget *defil = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(defil), tv);
    gtk_widget_set_vexpand(defil, TRUE);
    gtk_box_append(GTK_BOX(box), defil);

    GtkWidget *bfer = gtk_button_new_with_label("Fermer");
    g_signal_connect(bfer, "clicked", G_CALLBACK(on_fermer), win);
    gtk_box_append(GTK_BOX(box), bfer);

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_present(GTK_WINDOW(win));
}
