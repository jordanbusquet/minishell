#include "headers/minishell_gui.h"

// Fonction "main" qui génère la fenêtre du MiniShell
int main(int argc, char **argv) {
    // Crée une nouvelle application GTK, avec un identifiant unique
    GtkApplication *app = gtk_application_new("com.JordanBusquet.MiniShell", G_APPLICATION_DEFAULT_FLAGS);

    // Connecte le signal "activate" (déclenché quand l'application démarre) à ta fonction personnalisée app_activate
    g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);

    // Lance l’application GTK
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app); // Libère la mémoire quand l'application se termine

    // Retourne le code de sortie du programme (0 si tout s’est bien passé)
    return status;
}