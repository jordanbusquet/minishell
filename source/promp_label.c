#include "../headers/minishell_gui.h"

static gboolean grab_focus_idle(gpointer user_data);

// Création du "GtkEntry"
void creation_gtkentry(GtkWidget *vbox) {
    // Crée une box horizontale pour une ligne
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    // Création du label "hostname"
    GtkWidget *hostname = gtk_label_new(infos());
    gtk_widget_set_name(hostname, "hostname");
    gtk_label_set_xalign(GTK_LABEL(hostname), 0.0);

    // Création d'un label "path"
    GtkWidget *path = gtk_label_new(path_pc());
    gtk_widget_set_name(path, "path");
    gtk_label_set_xalign(GTK_LABEL(path), 0.0);

    // Création du label "$>"
    GtkWidget *prompt_label = gtk_label_new("$>");
    gtk_widget_set_name(prompt_label, "prompt-label");

    // Création du "entry"
    GtkWidget *entry = gtk_entry_new();
    gtk_widget_set_name(entry, "entry");
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_entry_set_max_length(GTK_ENTRY(entry), 82);

    // Ajout du label et de l'entrée dans la hbox (ligne)
    gtk_box_append(GTK_BOX(hbox), hostname);
    gtk_box_append(GTK_BOX(hbox), path);

    gtk_box_append(GTK_BOX(hbox2), prompt_label);
    gtk_box_append(GTK_BOX(hbox2), entry);

    // Ajout de la hbox (ligne) dans la vbox (terminal)
    gtk_box_append(GTK_BOX(vbox), hbox);
    gtk_box_append(GTK_BOX(vbox), hbox2);

    // Connexion du signal "activate" sur l'entrée, avec comme user_data la vbox (terminal)
    g_signal_connect(entry, "activate", G_CALLBACK(validation), vbox);

    // Mettre le focus sur le nouvel entry
    g_idle_add(grab_focus_idle, entry);
}

// Fonction qui permet d'attribuer le focus au "GtkEntry"
static gboolean grab_focus_idle(gpointer user_data) {
    GtkWidget *entry = GTK_WIDGET(user_data);
    gtk_widget_grab_focus(entry);
    return G_SOURCE_REMOVE;
}