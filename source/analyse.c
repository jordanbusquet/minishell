#include "../headers/minishell_gui.h"

#include <string.h>

void analyse(const gchar *texte) {
    g_print("Texte saisi : %s\n", texte);

    // Partie qui gère les changements de chemin avec "cd"
    if (strncmp(texte, "cd ", 3) == 0) {
        const char *chemin = texte + 3;
        change_dossier(chemin);
    }
}