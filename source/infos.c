#include "../headers/minishell_gui.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <unistd.h>  // gethostname() sur Unix (Linux/macOS)
#endif

// Récupère le nom de l'ordinateur
char *name_pc(void) {
    static char hostname[256];

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "Erreur";
    }
#endif

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return "Inconnu";
    }

#ifdef _WIN32
    WSACleanup();
#endif

    return hostname;
}

// Récupère le nom de l'utilisateur
char *name_user(void) {
    char *user = NULL;

    #ifdef _WIN32
        user = getenv("USERNAME");
    #else
        user = getenv("USER");
    #endif
        if (user == NULL) {
            user = "inconnu";
        }

    return user;
}

// Récupère le chemin relatif du terminal
char *path_pc(void) {
    return repertoire_actuel;
}

// Gère l'affichage de tout ça dant le terminal
char *infos(void) {
    static char label_text[512];
    snprintf(label_text, sizeof(label_text), "%s@%s", name_user(), name_pc());

    return label_text;
}