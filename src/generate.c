#include "generate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *TYPE_NOMS[4] = { "int", "float", "char", "double" };
const char *CFG_NOMS[3]  = { "aleatoire", "croissant", "decroissant" };
const size_t TAILLES_STD[NB_TAILLES] = { 100, 500, 1000, 5000, 10000, 50000, 100000 };

static int cmp_int(const void *a, const void *b)    { int x = *(const int *)a, y = *(const int *)b; return (x > y) - (x < y); }
static int cmp_float(const void *a, const void *b)  { float x = *(const float *)a, y = *(const float *)b; return (x > y) - (x < y); }
static int cmp_char(const void *a, const void *b)   { char x = *(const char *)a, y = *(const char *)b; return (x > y) - (x < y); }
static int cmp_double(const void *a, const void *b) { double x = *(const double *)a, y = *(const double *)b; return (x > y) - (x < y); }

size_t type_taille(TypeDonnee t) {
    switch (t) { case T_INT: return sizeof(int); case T_FLOAT: return sizeof(float);
                 case T_CHAR: return sizeof(char); default: return sizeof(double); }
}
cmp_fn type_cmp(TypeDonnee t) {
    switch (t) { case T_INT: return cmp_int; case T_FLOAT: return cmp_float;
                 case T_CHAR: return cmp_char; default: return cmp_double; }
}
int type_depuis_nom(const char *nom) {
    for (int i = 0; i < 4; i++) if (strcmp(nom, TYPE_NOMS[i]) == 0) return i;
    return -1;
}

/* rand() sous Windows ne donne que 15 bits : on en assemble deux */
static unsigned rand31(void) { return ((unsigned)rand() << 15) ^ (unsigned)rand(); }

void *generer_tableau(TypeDonnee t, size_t n, Configuration cfg) {
    size_t sz = type_taille(t);
    char *tab = malloc(n * sz);
    if (!tab) return NULL;
    for (size_t i = 0; i < n; i++) {
        switch (t) {
            case T_INT:    ((int *)tab)[i]    = (int)(rand31() % 1000000); break;
            case T_FLOAT:  ((float *)tab)[i]  = (float)(rand31() % 1000000) / 100.0f; break;
            case T_CHAR:   ((char *)tab)[i]   = (char)('a' + rand() % 26); break;
            case T_DOUBLE: ((double *)tab)[i] = (double)(rand31() % 1000000) / 100.0; break;
        }
    }
    if (cfg != CFG_ALEATOIRE) {
        qsort(tab, n, sz, type_cmp(t));            /* trie croissant (non chronometre) */
        if (cfg == CFG_DECROISSANT) {              /* puis inverse */
            for (size_t i = 0; i < n / 2; i++) {
                char *a = tab + i * sz, *b = tab + (n - 1 - i) * sz;
                for (size_t k = 0; k < sz; k++) { char x = a[k]; a[k] = b[k]; b[k] = x; }
            }
        }
    }
    return tab;
}

void apercu_tableau(const void *tab, TypeDonnee t, size_t n, size_t max, char *buf, size_t taille_buf) {
    size_t m = n < max ? n : max, pos = 0;
    buf[0] = '\0';
    for (size_t i = 0; i < m && pos + 24 < taille_buf; i++) {
        int w = 0;
        switch (t) {
            case T_INT:    w = snprintf(buf + pos, taille_buf - pos, "%d ", ((const int *)tab)[i]); break;
            case T_FLOAT:  w = snprintf(buf + pos, taille_buf - pos, "%.2f ", ((const float *)tab)[i]); break;
            case T_CHAR:   w = snprintf(buf + pos, taille_buf - pos, "%c ", ((const char *)tab)[i]); break;
            case T_DOUBLE: w = snprintf(buf + pos, taille_buf - pos, "%.2f ", ((const double *)tab)[i]); break;
        }
        if (w > 0) pos += (size_t)w;
    }
    if (n > m && pos + 5 < taille_buf) snprintf(buf + pos, taille_buf - pos, "...");
}
