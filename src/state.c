#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EtatApp ETAT = { NULL, 0, T_INT, CFG_ALEATOIRE, 0 };

Resultat RESULTATS[MAX_RESULTATS];
int      NB_RESULTATS = 0;

void etat_ajouter_resultat(int algo, TypeDonnee type, Configuration cfg, size_t n, double temps, int ok) {
    for (int i = 0; i < NB_RESULTATS; i++) {
        Resultat *r = &RESULTATS[i];
        if (r->algo == algo && r->type == type && r->cfg == cfg && r->n == n) {
            r->temps = temps; r->ok = ok; return;
        }
    }
    if (NB_RESULTATS >= MAX_RESULTATS) return;
    RESULTATS[NB_RESULTATS++] = (Resultat){ algo, type, cfg, n, temps, ok };
}

void etat_effacer_resultats(void) { NB_RESULTATS = 0; }

int etat_a_donnees(void) { return ETAT.tab != NULL; }

void etat_reset(void) {
    free(ETAT.tab);
    ETAT.tab = NULL; ETAT.n = 0; ETAT.manuel = 0;
}

void etat_definir(void *tab, size_t n, TypeDonnee type, Configuration cfg, int manuel) {
    free(ETAT.tab);
    ETAT.tab = tab; ETAT.n = n; ETAT.type = type; ETAT.cfg = cfg; ETAT.manuel = manuel;
}

#define SAISIE_MIN 10
#define SAISIE_MAX 30

void *parser_saisie(const char *txt, TypeDonnee t, size_t *n_out, char *err, size_t errsz) {
    if (!txt) { snprintf(err, errsz, "Veuillez entrer des valeurs."); return NULL; }
    size_t sz = type_taille(t);
    char *copie = strdup(txt);
    char *tab = malloc(SAISIE_MAX * sz);
    if (!copie || !tab) { snprintf(err, errsz, "Mémoire insuffisante."); free(copie); free(tab); return NULL; }

    size_t n = 0;
    for (char *tok = strtok(copie, " ,;\t\n"); tok; tok = strtok(NULL, " ,;\t\n")) {
        if (n == SAISIE_MAX) {
            snprintf(err, errsz, "Trop de valeurs : maximum %d autorisées.", SAISIE_MAX);
            free(copie); free(tab); return NULL;
        }
        char *fin = NULL;
        int ok = 1;
        switch (t) {
            case T_INT:    { long v = strtol(tok, &fin, 10); ((int *)tab)[n] = (int)v; ok = (*fin == '\0'); break; }
            case T_FLOAT:  { float v = strtof(tok, &fin);    ((float *)tab)[n] = v;     ok = (*fin == '\0'); break; }
            case T_DOUBLE: { double v = strtod(tok, &fin);   ((double *)tab)[n] = v;    ok = (*fin == '\0'); break; }
            case T_CHAR:   { ok = (strlen(tok) == 1); if (ok) ((char *)tab)[n] = tok[0]; break; }
        }
        if (!ok) {
            snprintf(err, errsz, "Valeur invalide : \"%s\" (type attendu : %s).", tok, TYPE_NOMS[t]);
            free(copie); free(tab); return NULL;
        }
        n++;
    }
    free(copie);
    if (n < SAISIE_MIN || n > SAISIE_MAX) {
        snprintf(err, errsz, "Veuillez saisir entre %d et %d valeurs (%zu fournie(s)).", SAISIE_MIN, SAISIE_MAX, n);
        free(tab); return NULL;
    }
    *n_out = n;
    return tab;
}

void etat_charger_csv(const char *chemin) {
    FILE *f = fopen(chemin, "r");
    if (!f) {
        f = fopen("../resultats.csv", "r");
        if (!f) f = fopen("triii/resultats.csv", "r");
        if (!f) f = fopen("../triii/resultats.csv", "r");
        if (!f) return;
    }

    char ligne[512];
    /* Ignorer l'en-tete */
    if (!fgets(ligne, sizeof ligne, f)) { fclose(f); return; }

    while (fgets(ligne, sizeof ligne, f)) {
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (strlen(ligne) == 0) continue;

        char algo_str[64] = "", type_str[32] = "", cfg_str[32] = "";
        size_t n = 0;
        double temps = 0.0;
        int ok = 1;

        char *toks[10];
        int ntoks = 0;
        char *p = strtok(ligne, ",");
        while (p && ntoks < 10) {
            toks[ntoks++] = p;
            p = strtok(NULL, ",");
        }

        if (ntoks == 6) {
            /* Format avec point décimal : algo,type,cfg,taille,temps,ok */
            strncpy(algo_str, toks[0], sizeof(algo_str) - 1);
            strncpy(type_str, toks[1], sizeof(type_str) - 1);
            strncpy(cfg_str, toks[2], sizeof(cfg_str) - 1);
            n = (size_t)strtoull(toks[3], NULL, 10);
            temps = strtod(toks[4], NULL);
            ok = atoi(toks[5]);
        } else if (ntoks == 7) {
            /* Format Windows locale française avec virgule : algo,type,cfg,taille,temps_int,temps_frac,ok */
            strncpy(algo_str, toks[0], sizeof(algo_str) - 1);
            strncpy(type_str, toks[1], sizeof(type_str) - 1);
            strncpy(cfg_str, toks[2], sizeof(cfg_str) - 1);
            n = (size_t)strtoull(toks[3], NULL, 10);
            char tbuf[64];
            snprintf(tbuf, sizeof tbuf, "%s.%s", toks[4], toks[5]);
            temps = strtod(tbuf, NULL);
            ok = atoi(toks[6]);
        } else {
            continue;
        }

        int algo_idx = -1;
        for (int i = 0; i < NB_ALGOS; i++) {
            if (strcmp(ALGO_NOMS[i], algo_str) == 0) {
                algo_idx = i; break;
            }
        }
        if (algo_idx == -1) continue;

        int type_idx = type_depuis_nom(type_str);
        if (type_idx == -1) continue;

        int cfg_idx = -1;
        for (int i = 0; i < 3; i++) {
            if (strcmp(CFG_NOMS[i], cfg_str) == 0) {
                cfg_idx = i; break;
            }
        }
        if (cfg_idx == -1) continue;

        etat_ajouter_resultat(algo_idx, (TypeDonnee)type_idx, (Configuration)cfg_idx, n, temps, ok);
    }
    fclose(f);
}
