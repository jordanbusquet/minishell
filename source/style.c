// Inclusion de la bibliothèques nécéssaire
#include "../headers/minishell_gui.h"

void load_css(void) {
    // Création du CSS pour ma fenêtre
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider, "style.css");

    // Récupère l'affichage par défaut (l'écran principal de l'utilisateur)
    GdkDisplay *display = gdk_display_get_default();
    gtk_style_context_add_provider_for_display(display, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);  // Applique le CSS à ma fenêtre avec une priorité élevée
}