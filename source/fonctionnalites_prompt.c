#include "../headers/minishell_gui.h"

// Fonction appelée quand l'utilisateur valide une ligne
void validation(GtkEntry *entry, gpointer user_data) {
    GtkWidget *vbox = GTK_WIDGET(user_data);

    const gchar *texte = gtk_entry_buffer_get_text(gtk_entry_get_buffer(entry));
    analyse(texte);

    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(entry), FALSE);

    // Création du label " "
    GtkWidget *espace = gtk_label_new(" ");
    gtk_widget_set_name(espace, "espace");
    gtk_box_append(GTK_BOX(vbox), espace);

    creation_gtkentry(vbox);

    GtkWidget *scrolled_window = gtk_widget_get_ancestor(vbox, GTK_TYPE_SCROLLED_WINDOW);
    if (scrolled_window) {
        t_scroll_context *ctx = g_new(t_scroll_context, 1);
        ctx->scrolled_window = GTK_SCROLLED_WINDOW(scrolled_window);

        ctx->offset = -5;  // Décale vers le haut de 50 pixels (ou +50 vers le bas)

        g_timeout_add(30, scroll_to_position, ctx);
    }
}