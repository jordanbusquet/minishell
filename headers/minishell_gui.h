#ifndef MINISHELL_GUI_H
#define MINISHELL_GUI_H

#include <gtk/gtk.h>

typedef struct s_scroll_context
{
	GtkScrolledWindow	*scrolled_window;
	double				offset;
}	t_scroll_context;

extern char	repertoire_actuel[1024];

void		app_activate(GApplication *app, gpointer user_data);
void		load_css(void);
GtkWidget	*message_start(void);
GtkWidget	*terminal(GtkWindow *window);
GtkWidget	*create_main_vbox(GtkWindow *window);

void		analyse(const gchar *texte);
void		change_dossier(const char *chemin);
void		validation(GtkEntry *entry, gpointer user_data);
void		creation_gtkentry(GtkWidget *vbox);
gboolean	scroll_to_position(gpointer data);
gboolean	on_scroll(GtkEventControllerScroll *controller, double scroll_dx,
				double scroll_dy, gpointer user_data);
void		on_adjustment_value_changed(GtkAdjustment *adjustment,
				gpointer user_data);

char		*infos(void);
char		*path_pc(void);
char		*name_pc(void);
char		*name_user(void);

#endif
