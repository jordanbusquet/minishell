#include "../headers/minishell_gui.h"

#include <string.h>

#define PROMPT_MARKER "\036MINISHELL_PROMPT\037"
#define HEREDOC_MARKER "\036MINISHELL_HEREDOC\037"

typedef struct s_shell_terminal
{
	gatomicrefcount	refs;
	GtkTextView		*output_view;
	GtkTextBuffer	*output_buffer;
	GtkTextTag		*output_tag;
	GtkTextTag		*prompt_tag;
	GSubprocess		*process;
	GOutputStream	*input;
	GInputStream	*output;
	GString			*pending_output;
	GString			*stream_buffer;
	gint			input_start;
	gboolean		input_active;
	gboolean		closed;
	gboolean		process_finished;
	gboolean		output_finished;
	gboolean		status_reported;
}	t_shell_terminal;

static t_shell_terminal	*shell_terminal_ref(t_shell_terminal *terminal)
{
	g_atomic_ref_count_inc(&terminal->refs);
	return (terminal);
}

static void	shell_terminal_unref(t_shell_terminal *terminal)
{
	if (!g_atomic_ref_count_dec(&terminal->refs))
		return ;
	g_clear_object(&terminal->process);
	g_clear_object(&terminal->input);
	g_clear_object(&terminal->output);
	g_clear_object(&terminal->output_view);
	g_clear_object(&terminal->output_buffer);
	g_string_free(terminal->pending_output, TRUE);
	g_string_free(terminal->stream_buffer, TRUE);
	g_free(terminal);
}

static void	shell_terminal_destroy(gpointer data)
{
	shell_terminal_unref(data);
}

static void	scroll_to_end(t_shell_terminal *terminal)
{
	GtkTextIter	end;

	gtk_text_buffer_get_end_iter(terminal->output_buffer, &end);
	gtk_text_view_scroll_to_iter(terminal->output_view, &end, 0.0, FALSE,
		0.0, 1.0);
}

static void	append_text(t_shell_terminal *terminal, const gchar *text,
		gssize length)
{
	GtkTextIter	end;

	if (terminal->closed || length == 0)
		return ;
	gtk_text_buffer_get_end_iter(terminal->output_buffer, &end);
	gtk_text_buffer_insert_with_tags(terminal->output_buffer, &end, text,
		length, terminal->output_tag, NULL);
	scroll_to_end(terminal);
}

static void	append_prompt(t_shell_terminal *terminal, const gchar *prompt)
{
	GtkTextIter	end;
	gint		char_count;

	if (terminal->closed)
		return ;
	char_count = gtk_text_buffer_get_char_count(terminal->output_buffer);
	if (char_count > 0)
	{
		gtk_text_buffer_get_iter_at_offset(terminal->output_buffer, &end,
			char_count - 1);
		if (gtk_text_iter_get_char(&end) != '\n')
			append_text(terminal, "\n", 1);
	}
	gtk_text_buffer_get_end_iter(terminal->output_buffer, &end);
	gtk_text_buffer_insert_with_tags(terminal->output_buffer, &end, prompt,
		-1, terminal->prompt_tag, NULL);
	terminal->input_start = gtk_text_buffer_get_char_count(
			terminal->output_buffer);
	terminal->input_active = TRUE;
	gtk_text_view_set_editable(terminal->output_view, TRUE);
	gtk_text_buffer_get_end_iter(terminal->output_buffer, &end);
	gtk_text_buffer_place_cursor(terminal->output_buffer, &end);
	scroll_to_end(terminal);
	gtk_widget_grab_focus(GTK_WIDGET(terminal->output_view));
}

static void	append_process_output(t_shell_terminal *terminal,
		const gchar *text, gsize length)
{
	GString	*valid;
	gsize	offset;

	g_string_append_len(terminal->pending_output, text, length);
	valid = g_string_new(NULL);
	offset = 0;
	while (offset < terminal->pending_output->len)
	{
		gunichar	character;
		gchar		encoded[6];
		gint		encoded_length;

		character = g_utf8_get_char_validated(
				terminal->pending_output->str + offset,
				terminal->pending_output->len - offset);
		if (character == (gunichar)-2)
			break ;
		if (character == (gunichar)-1)
		{
			g_string_append(valid, "\357\277\275");
			offset++;
			continue ;
		}
		encoded_length = g_unichar_to_utf8(character, encoded);
		g_string_append_len(valid, encoded, encoded_length);
		offset += encoded_length;
	}
	if (valid->len > 0)
		append_text(terminal, valid->str, valid->len);
	g_string_erase(terminal->pending_output, 0, offset);
	g_string_free(valid, TRUE);
}

static void	flush_utf8_output(t_shell_terminal *terminal)
{
	gchar	*valid;

	if (terminal->pending_output->len == 0)
		return ;
	valid = g_utf8_make_valid(terminal->pending_output->str,
			terminal->pending_output->len);
	append_text(terminal, valid, -1);
	g_free(valid);
	g_string_truncate(terminal->pending_output, 0);
}

static gssize	find_marker(const gchar *text, gsize length,
		const gchar **marker, const gchar **prompt)
{
	const gchar	*normal;
	const gchar	*heredoc;

	normal = g_strstr_len(text, length, PROMPT_MARKER);
	heredoc = g_strstr_len(text, length, HEREDOC_MARKER);
	if (!normal && !heredoc)
		return (-1);
	if (!heredoc || (normal && normal < heredoc))
	{
		*marker = PROMPT_MARKER;
		*prompt = "minishell$ ";
		return (normal - text);
	}
	*marker = HEREDOC_MARKER;
	*prompt = "heredoc> ";
	return (heredoc - text);
}

static gsize	pending_marker_suffix(const gchar *text, gsize length)
{
	const gchar	*markers[2];
	gsize		marker_length;
	gsize		suffix_length;
	gsize		i;
	gsize		j;

	markers[0] = PROMPT_MARKER;
	markers[1] = HEREDOC_MARKER;
	suffix_length = 0;
	i = 0;
	while (i < G_N_ELEMENTS(markers))
	{
		marker_length = strlen(markers[i]);
		j = 1;
		while (j < marker_length && j <= length)
		{
			if (j > suffix_length
				&& memcmp(text + length - j, markers[i], j) == 0)
				suffix_length = j;
			j++;
		}
		i++;
	}
	return (suffix_length);
}

static void	consume_process_output(t_shell_terminal *terminal,
		const gchar *text, gsize length)
{
	const gchar	*marker;
	const gchar	*prompt;
	gssize		position;
	gsize		marker_length;
	gsize		safe_length;
	gsize		suffix_length;

	g_string_append_len(terminal->stream_buffer, text, length);
	while (terminal->stream_buffer->len > 0)
	{
		position = find_marker(terminal->stream_buffer->str,
				terminal->stream_buffer->len, &marker, &prompt);
		if (position >= 0)
		{
			append_process_output(terminal, terminal->stream_buffer->str,
					position);
			marker_length = strlen(marker);
			g_string_erase(terminal->stream_buffer, 0,
					position + marker_length);
			flush_utf8_output(terminal);
			append_prompt(terminal, prompt);
			continue ;
		}
		suffix_length = pending_marker_suffix(terminal->stream_buffer->str,
				terminal->stream_buffer->len);
		safe_length = terminal->stream_buffer->len - suffix_length;
		if (safe_length == 0)
			break ;
		append_process_output(terminal, terminal->stream_buffer->str,
				safe_length);
		g_string_erase(terminal->stream_buffer, 0, safe_length);
	}
}

static void	flush_pending_output(t_shell_terminal *terminal)
{
	if (terminal->stream_buffer->len > 0)
	{
		append_process_output(terminal, terminal->stream_buffer->str,
				terminal->stream_buffer->len);
		g_string_truncate(terminal->stream_buffer, 0);
	}
	flush_utf8_output(terminal);
}

static void	report_process_exit(t_shell_terminal *terminal)
{
	gchar	*message;

	if (terminal->closed || !terminal->process_finished
		|| !terminal->output_finished || terminal->status_reported)
		return ;
	terminal->status_reported = TRUE;
	if (g_subprocess_get_if_exited(terminal->process))
		message = g_strdup_printf("\n[Mini Shell arrêté : code %d]\n",
				g_subprocess_get_exit_status(terminal->process));
	else
		message = g_strdup_printf("\n[Mini Shell arrêté : signal %d]\n",
				g_subprocess_get_term_sig(terminal->process));
	append_text(terminal, message, -1);
	g_free(message);
}

static void	read_process_output(t_shell_terminal *terminal);

static void	on_process_output_read(GObject *source, GAsyncResult *result,
		gpointer user_data)
{
	t_shell_terminal	*terminal;
	GBytes				*bytes;
	GError				*error;
	gconstpointer		data;
	gsize				length;

	terminal = user_data;
	error = NULL;
	bytes = g_input_stream_read_bytes_finish(G_INPUT_STREAM(source), result,
			&error);
	if (error)
	{
		if (!terminal->closed)
			append_text(terminal, error->message, -1);
		g_clear_error(&error);
		terminal->output_finished = TRUE;
	}
	else
	{
		data = g_bytes_get_data(bytes, &length);
		if (length > 0)
		{
			consume_process_output(terminal, data, length);
			read_process_output(terminal);
		}
		else
		{
			flush_pending_output(terminal);
			terminal->output_finished = TRUE;
		}
		g_bytes_unref(bytes);
	}
	report_process_exit(terminal);
	shell_terminal_unref(terminal);
}

static void	read_process_output(t_shell_terminal *terminal)
{
	if (terminal->closed)
		return ;
	g_input_stream_read_bytes_async(terminal->output, 4096,
			G_PRIORITY_DEFAULT, NULL, on_process_output_read,
			shell_terminal_ref(terminal));
}

static void	on_process_waited(GObject *source, GAsyncResult *result,
		gpointer user_data)
{
	t_shell_terminal	*terminal;
	GError				*error;

	terminal = user_data;
	error = NULL;
	if (!g_subprocess_wait_finish(G_SUBPROCESS(source), result, &error))
	{
		if (!terminal->closed)
			append_text(terminal, error->message, -1);
		g_clear_error(&error);
	}
	else
	{
		terminal->process_finished = TRUE;
		terminal->input_active = FALSE;
		gtk_text_view_set_editable(terminal->output_view, FALSE);
		report_process_exit(terminal);
	}
	shell_terminal_unref(terminal);
}

static void	on_input_written(GObject *source, GAsyncResult *result,
		gpointer user_data)
{
	t_shell_terminal	*terminal;
	GError				*error;
	gsize				bytes_written;

	terminal = user_data;
	error = NULL;
	if (!g_output_stream_write_all_finish(G_OUTPUT_STREAM(source), result,
			&bytes_written, &error))
	{
		if (!terminal->closed)
			append_text(terminal, error->message, -1);
		g_clear_error(&error);
	}
	shell_terminal_unref(terminal);
}

static void	submit_command(t_shell_terminal *terminal)
{
	GtkTextIter	start;
	GtkTextIter	end;
	gchar		*command;
	gchar		*line;

	if (terminal->closed || terminal->process_finished
		|| !terminal->input_active || !terminal->input)
		return ;
	gtk_text_buffer_get_iter_at_offset(terminal->output_buffer, &start,
			terminal->input_start);
	gtk_text_buffer_get_end_iter(terminal->output_buffer, &end);
	command = gtk_text_buffer_get_text(terminal->output_buffer, &start, &end,
			FALSE);
	line = g_strconcat(command, "\n", NULL);
	gtk_text_buffer_insert(terminal->output_buffer, &end, "\n", 1);
	gtk_text_buffer_get_end_iter(terminal->output_buffer, &end);
	gtk_text_buffer_get_iter_at_offset(terminal->output_buffer, &start,
			terminal->input_start);
	gtk_text_buffer_apply_tag(terminal->output_buffer, terminal->output_tag,
			&start, &end);
	terminal->input_active = FALSE;
	gtk_text_view_set_editable(terminal->output_view, FALSE);
	g_output_stream_write_all_async(terminal->input, line, strlen(line),
			G_PRIORITY_DEFAULT, NULL, on_input_written,
			shell_terminal_ref(terminal));
	g_free(line);
	g_free(command);
	scroll_to_end(terminal);
}

static gboolean	on_terminal_key_pressed(GtkEventControllerKey *controller,
		guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
	t_shell_terminal	*terminal;

	(void)controller;
	(void)keycode;
	(void)state;
	terminal = user_data;
	if (keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter)
	{
		submit_command(terminal);
		return (TRUE);
	}
	if (!terminal->input_active)
		return (TRUE);
	return (FALSE);
}

static gboolean	on_window_close(GtkWindow *window, gpointer user_data)
{
	t_shell_terminal	*terminal;

	(void)window;
	terminal = user_data;
	terminal->closed = TRUE;
	if (terminal->process && !terminal->process_finished)
		g_subprocess_force_exit(terminal->process);
	return (FALSE);
}

static void	start_minishell(t_shell_terminal *terminal, GtkWindow *window)
{
	GSubprocessLauncher	*launcher;
	GError				*error;
	gchar				*current_dir;
	gchar				*executable;

	current_dir = g_get_current_dir();
#ifdef G_OS_WIN32
	executable = g_build_filename(current_dir, "minishell", "minishell.exe",
			NULL);
#else
	executable = g_build_filename(current_dir, "minishell", "minishell", NULL);
#endif
	launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_STDIN_PIPE
			| G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE);
	g_subprocess_launcher_setenv(launcher, "MINISHELL_GUI", "1", TRUE);
	error = NULL;
	terminal->process = g_subprocess_launcher_spawn(launcher, &error,
			executable, NULL);
	g_object_unref(launcher);
	g_free(current_dir);
	g_free(executable);
	if (!terminal->process)
	{
		append_text(terminal, "Impossible de démarrer le minishell : ", -1);
		append_text(terminal, error->message, -1);
		append_text(terminal, "\nCompilez le projet avec make.\n", -1);
		g_clear_error(&error);
		return ;
	}
	terminal->input = g_object_ref(g_subprocess_get_stdin_pipe(
				terminal->process));
	terminal->output = g_object_ref(g_subprocess_get_stdout_pipe(
				terminal->process));
	g_signal_connect(window, "close-request", G_CALLBACK(on_window_close),
			terminal);
	read_process_output(terminal);
	g_subprocess_wait_async(terminal->process, NULL, on_process_waited,
			shell_terminal_ref(terminal));
}

GtkWidget	*terminal(GtkWindow *window)
{
	t_shell_terminal	*state;
	GtkWidget			*scrolled_window;
	GtkWidget			*output_view;
	GtkEventController	*key_controller;

	state = g_new0(t_shell_terminal, 1);
	g_atomic_ref_count_init(&state->refs);
	state->pending_output = g_string_new(NULL);
	state->stream_buffer = g_string_new(NULL);
	scrolled_window = gtk_scrolled_window_new();
	gtk_widget_set_vexpand(scrolled_window, TRUE);
	gtk_widget_set_hexpand(scrolled_window, TRUE);
	output_view = gtk_text_view_new();
	gtk_widget_set_name(output_view, "terminal-output");
	gtk_text_view_set_editable(GTK_TEXT_VIEW(output_view), FALSE);
	gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(output_view), TRUE);
	gtk_text_view_set_monospace(GTK_TEXT_VIEW(output_view), TRUE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(output_view), GTK_WRAP_CHAR);
	gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window),
			output_view);
	state->output_view = g_object_ref(GTK_TEXT_VIEW(output_view));
	state->output_buffer = g_object_ref(gtk_text_view_get_buffer(
				GTK_TEXT_VIEW(output_view)));
	state->output_tag = gtk_text_buffer_create_tag(state->output_buffer,
			"terminal-output-text", "editable", FALSE, NULL);
	state->prompt_tag = gtk_text_buffer_create_tag(state->output_buffer,
			"terminal-prompt", "foreground", "#a078d4", "weight",
			PANGO_WEIGHT_BOLD, "editable", FALSE, NULL);
	key_controller = gtk_event_controller_key_new();
	gtk_event_controller_set_propagation_phase(key_controller, GTK_PHASE_CAPTURE);
	g_signal_connect(key_controller, "key-pressed",
			G_CALLBACK(on_terminal_key_pressed), state);
	gtk_widget_add_controller(output_view, key_controller);
	g_object_set_data_full(G_OBJECT(window), "shell-terminal-state", state,
			shell_terminal_destroy);
	start_minishell(state, window);
	return (scrolled_window);
}
