// Inclusion de la bibliothèques nécéssaire
#include "../headers/minishell_gui.h"

GtkWidget* message_start(void) {
    // Création d'un label d'info fixe en haut
    GtkWidget *hbox_info_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *info_label = gtk_label_new("Minishell 42 - Interface GTK 4\n");
    gtk_widget_set_name(info_label, "info-label");
    gtk_box_append(GTK_BOX(hbox_info_label), info_label);
    return hbox_info_label;
}