#include "../headers/minishell_gui.h"

#include <math.h>

// Callback pour le scroll par la molette
gboolean on_scroll(GtkEventControllerScroll *controller, double scroll_dx, double scroll_dy, gpointer user_data) {
    (void)controller;
    (void)scroll_dx;
    GtkWidget *scrolled_window = GTK_WIDGET(user_data);
    GtkWidget *child = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scrolled_window));

    if (!GTK_IS_SCROLLABLE(child))
        return FALSE;

    GtkAdjustment *vadjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(child));
    const double step = 19.0;
    double scroll_pixels = scroll_dy * step;

    if (fabs(scroll_pixels) < step)
        return FALSE;

    double value = gtk_adjustment_get_value(vadjustment);
    double lower = gtk_adjustment_get_lower(vadjustment);
    double upper = gtk_adjustment_get_upper(vadjustment) - gtk_adjustment_get_page_size(vadjustment);
    if (upper < lower)
        upper = lower;

    // Bloque le scroll hors limites
    if ((scroll_pixels > 0 && value >= upper) || (scroll_pixels < 0 && value <= lower))
        return TRUE;

    double new_value = value + scroll_pixels;

    // Clamp et arrondi au multiple de step
    if (new_value < lower) new_value = lower;
    else if (new_value > upper) new_value = upper;
    new_value = round(new_value / step) * step;

    if (new_value == value)
        return TRUE;

    gtk_adjustment_set_value(vadjustment, new_value);
    return TRUE;
}

// Callback pour suivre les changements de la scrollbar (drag, etc.)
void on_adjustment_value_changed(GtkAdjustment *adjustment, gpointer user_data) {
    const double step = 19.0;

    double value = gtk_adjustment_get_value(adjustment);
    double lower = gtk_adjustment_get_lower(adjustment);
    double upper = gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment);
    if (upper < lower)
        upper = lower;

    // Clamp la valeur dans les bornes
    if (value < lower) value = lower;
    else if (value > upper) value = upper;

    double new_value = round(value / step) * step;

    if (new_value != value) {
        // Bloquer pour éviter récursion infinie
        g_signal_handlers_block_by_func(adjustment, G_CALLBACK(on_adjustment_value_changed), user_data);
        gtk_adjustment_set_value(adjustment, new_value);
        g_signal_handlers_unblock_by_func(adjustment, G_CALLBACK(on_adjustment_value_changed), user_data);
    }
}