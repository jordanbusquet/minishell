// Inclusion des bibliothèques nécéssairent
#include "../headers/minishell_gui.h"

void app_activate(GApplication *app, gpointer user_data) {
    (void)user_data; // Ignore proprement l'argument inutilisé

    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Mini Shell");             // Nom de la fenêtre
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 445);          // Taille de la fenêtre
    gtk_window_set_icon_name(GTK_WINDOW(window), "utilities-terminal"); // Icon de la fenêtre
    gtk_window_present(GTK_WINDOW(window));                             // Affiche la fenêtre au premier plan
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);                // Empèche le redimensionnement
    gtk_widget_set_name(window, "main-window");                         // Donne un nom à la fenêtre pour le CSS

    // Création d'une vbox pour le contenu de la page
    gtk_window_set_child(GTK_WINDOW(window), create_main_vbox(GTK_WINDOW(window)));

    // Rajoute le fichier CSS "style.css" à ma fenêtre
    load_css();
}