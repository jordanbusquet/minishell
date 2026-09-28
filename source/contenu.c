#include "../headers/minishell_gui.h"

// Gestion des éléments dans la page
GtkWidget *create_main_vbox(GtkWindow *window) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(vbox, "vbox");
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);
    gtk_box_append(GTK_BOX(vbox), message_start());
    gtk_box_append(GTK_BOX(vbox), terminal(window));
    return vbox;
}