#ifndef GUI_H
#define GUI_H
#include <gtk/gtk.h>

void gui_menu_ouvrir(GtkApplication *app);
void gui_afficher_page(const char *nom_page);
void gui_maj_statut(const char *texte, int en_cours);

/* Fonctions de transition / compatibilité */
void gui_tableaux_ouvrir(GtkWindow *parent);
void gui_execution_ouvrir(GtkWindow *parent);
void gui_resultats_ouvrir(GtkWindow *parent);
void gui_resultats_rafraichir(void);
void gui_courbes_ouvrir(GtkWindow *parent);
void gui_courbes_rafraichir(void);
void gui_aide_ouvrir(GtkWindow *parent);
void gui_placeholder_ouvrir(GtkWindow *parent, const char *titre);

/* Constructeurs de pages pour le conteneur principal */
GtkWidget *gui_tableaux_page_creer(void);
GtkWidget *gui_execution_page_creer(void);
GtkWidget *gui_resultats_page_creer(void);
GtkWidget *gui_courbes_page_creer(void);
GtkWidget *gui_aide_page_creer(void);

#endif
