#include "../headers/minishell_gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
    // Corrige la casse du chemin Windows
    void corriger_casse_chemin(const char *chemin_entree, char *chemin_corrige, size_t taille) {
        char temp[MAX_PATH];
        char partie[MAX_PATH];
        char chemin_accumule[MAX_PATH] = "";
        WIN32_FIND_DATA findFileData;
        HANDLE hFind;

        GetFullPathName(chemin_entree, MAX_PATH, temp, NULL);

        const char *p = temp;
        if (temp[1] == ':') {
            // Ajouter la lettre du lecteur
            strncpy(chemin_accumule, temp, 2);
            chemin_accumule[2] = '\0';
            p = temp + 2;
        }

        while (*p != '\0') {
            if (*p == '\\') {
                strncat(chemin_accumule, "\\", MAX_PATH - strlen(chemin_accumule) - 1);
                p++;
            }

            int i = 0;
            while (*p != '\\' && *p != '\0') {
                partie[i++] = *p++;
            }
            partie[i] = '\0';

            char chemin_temp[MAX_PATH];
            size_t espace_dispo = sizeof(chemin_temp) - 1;
            snprintf(chemin_temp, espace_dispo, "%.*s\\%.*s", (int)(espace_dispo / 2), chemin_accumule, (int)(espace_dispo / 2), partie);
            chemin_temp[espace_dispo] = '\0';

            hFind = FindFirstFile(chemin_temp, &findFileData);
            if (hFind != INVALID_HANDLE_VALUE) {
                FindClose(hFind);
                strncat(chemin_accumule, findFileData.cFileName, MAX_PATH - strlen(chemin_accumule) - 1);
            } else {
                strncat(chemin_accumule, partie, MAX_PATH - strlen(chemin_accumule) - 1);
            }
        }

        strncpy(chemin_corrige, chemin_accumule, taille - 1);
        chemin_corrige[taille - 1] = '\0';
    }
#endif

// Fonction : tester si un chemin existe
int chemin_existe(const char *chemin) {
    struct stat buffer;
    return (stat(chemin, &buffer) == 0);
}

// Fonction : concaténer deux chemins (ajoute / ou \ selon OS)
void concat_paths(const char *base, const char *rel, char *result, size_t max_len) {
    size_t len = strlen(base);
    strncpy(result, base, max_len);
    result[max_len - 1] = '\0';

    if (len > 0 && base[len - 1] != '/' && base[len - 1] != '\\') {
#ifdef _WIN32
        strncat(result, "\\", max_len - strlen(result) - 1);
#else
        strncat(result, "/", max_len - strlen(result) - 1);
#endif
    }

    strncat(result, rel, max_len - strlen(result) - 1);
}

// Fonction : retirer un dossier à la fin d’un chemin
void remonter_dossier(char *chemin) {
    size_t len = strlen(chemin);
    if (len == 0) return;

    // Supprimer les slashs à la fin
    while (len > 0 && (chemin[len - 1] == '/' || chemin[len - 1] == '\\')) {
        chemin[--len] = '\0';
    }

    // Trouver le dernier slash
    while (len > 0 && chemin[len - 1] != '/' && chemin[len - 1] != '\\') {
        --len;
    }

    // Couper le chemin
    if (len > 0) {
        chemin[len - 1] = '\0';
    } else {
        chemin[0] = '\0';
    }
}

// Fonction principale
void change_dossier(const char *chemin) {
#ifdef _WIN32
    if (strlen(chemin) == 2 && chemin[1] == ':') {
        if (SetCurrentDirectory(chemin) == 0) {
            DWORD err = GetLastError();
            printf("Erreur changement de lecteur %s (code erreur %lu)\n", chemin, err);
        } else {
            strncpy(repertoire_actuel, chemin, sizeof(repertoire_actuel) - 1);
            repertoire_actuel[sizeof(repertoire_actuel) - 1] = '\0';
            printf("Lecteur changé : %s\n", repertoire_actuel);
        }
        return;
    }
#endif

        // Cas spécial "../" répété uniquement
    if (strncmp(chemin, "../", 3) == 0 || strcmp(chemin, "..") == 0) {
        // Vérifier que le chemin ne contient QUE des ../ valides
        const char *p = chemin;
        while (strncmp(p, "../", 3) == 0) {
            p += 3;
        }

        // Vérifie si la fin est exactement ".." (pour cas comme "../../..")
        if (strcmp(p, "..") == 0) {
            p += 2;
        }

        // Si il reste quelque chose d'autre, on rejette
        if (*p != '\0') {
            printf("Chemin invalide : caractères non autorisés après '../'\n");
            return;
        }

        // Si valide, on remonte les dossiers
        char temp_chemin[1024];
        strncpy(temp_chemin, repertoire_actuel, sizeof(temp_chemin));
        temp_chemin[sizeof(temp_chemin) - 1] = '\0';

        // Recalcul du nombre de "../"
        p = chemin;
        while (strncmp(p, "../", 3) == 0) {
            remonter_dossier(temp_chemin);
            p += 3;
        }
        if (strcmp(p, "..") == 0) remonter_dossier(temp_chemin);

        if (chemin_existe(temp_chemin)) {
            if (chdir(temp_chemin) == 0) {
                strncpy(repertoire_actuel, temp_chemin, sizeof(repertoire_actuel) - 1);
                repertoire_actuel[sizeof(repertoire_actuel) - 1] = '\0';
                printf("Répertoire changé (../) : %s\n", repertoire_actuel);
            } else {
                perror("Erreur lors du changement de répertoire (../)");
            }
        } else {
            printf("Chemin invalide (../): %s\n", temp_chemin);
        }
        return;
    }

    // Si repertoire_actuel contient "~"
    if (strchr(repertoire_actuel, '~') != NULL) {
        if (chemin_existe(chemin)) {
            #ifdef _WIN32
                corriger_casse_chemin(chemin, repertoire_actuel, sizeof(repertoire_actuel));
            #else
                strncpy(repertoire_actuel, chemin, sizeof(repertoire_actuel) - 1);
                repertoire_actuel[sizeof(repertoire_actuel) - 1] = '\0';
            #endif

            if (chdir(chemin) == 0) {
                printf("Répertoire changé (tilde) : %s\n", repertoire_actuel);
            } else {
                perror("Erreur lors du changement de répertoire");
            }
        } else {
            printf("Le chemin %s n'existe pas.\n", chemin);
        }
        return;
    }

    // Chemin relatif à repertoire_actuel
    char chemin_complet[1024];
    concat_paths(repertoire_actuel, chemin, chemin_complet, sizeof(chemin_complet));

    #ifdef _WIN32
        char chemin_corrige[1024];
        corriger_casse_chemin(chemin_complet, chemin_corrige, sizeof(chemin_corrige));
    #else
        char *chemin_corrige = chemin_complet;
    #endif

    if (chemin_existe(chemin_corrige)) {
        if (chdir(chemin_corrige) == 0) {
            strncpy(repertoire_actuel, chemin_corrige, sizeof(repertoire_actuel) - 1);
            repertoire_actuel[sizeof(repertoire_actuel) - 1] = '\0';
            printf("Répertoire changé : %s\n", repertoire_actuel);
        } else {
            perror("Erreur lors du changement de répertoire");
        }
    } else {
        printf("Le chemin %s n'existe pas.\n", chemin_complet);
    }
}