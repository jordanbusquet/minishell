#include "../headers/minishell_gui.h"

// Cette fonction sert à scroller la fenêtre vers une position calculée
gboolean scroll_to_position(gpointer data) {
    t_scroll_context *ctx = (t_scroll_context *)data;

    GtkAdjustment *vadjustment = gtk_scrolled_window_get_vadjustment(ctx->scrolled_window);

    // Valeurs importantes
    double lower = gtk_adjustment_get_lower(vadjustment);
    double upper = gtk_adjustment_get_upper(vadjustment);
    double page_size = gtk_adjustment_get_page_size(vadjustment);

    // Position max (scroll le plus bas possible)
    double max_scroll = upper - page_size;

    // Calcul de la nouvelle position avec offset
    double target = max_scroll + ctx->offset;

    // Clamp pour rester dans les bornes
    if (target < lower)
        target = lower;
    if (target > max_scroll)
        target = max_scroll;

    gtk_adjustment_set_value(vadjustment, target);

    g_free(ctx);
    return G_SOURCE_REMOVE;
}