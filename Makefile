CC = gcc
CFLAGS = -O2 -Wall -Wextra
LOGIQUE = src/sorts.c src/generate.c src/bench.c src/state.c src/courbes_dessin.c
GUI = src/gui_menu.c src/gui_tableaux.c src/gui_execution.c src/gui_resultats.c src/gui_courbes.c src/gui_aide.c src/main_gui.c

# Interface graphique GTK 4 : make gui
gui: $(LOGIQUE) $(GUI)
	$(CC) $(CFLAGS) $(LOGIQUE) $(GUI) -o tri_gui $$(pkg-config --cflags --libs gtk4) -lm

# Version console (sans interface) : make console
console: src/sorts.c src/generate.c src/bench.c src/main_console.c
	$(CC) $(CFLAGS) src/sorts.c src/generate.c src/bench.c src/main_console.c -o tri_console

clean:
	rm -f tri_gui.exe tri_gui tri_console.exe tri_console resultats.csv
