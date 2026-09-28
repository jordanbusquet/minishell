/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jbusquet <jbusquet@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 11:39:02 by jbusquet          #+#    #+#             */
/*   Updated: 2026/07/02 11:39:02 by jbusquet         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/minishell.h"

#define MINISHELL_GUI_PROMPT_MARKER "\036MINISHELL_PROMPT\037"

int	g_exit_status = 0;

static void	shell_loop(char ***parsed_env)
{
	t_token	*tokens;
	t_ast	*ast;
	char	*line;
	char	*gui_line;
	size_t	gui_line_size;
	ssize_t	line_length;
	int		gui_mode;

	gui_mode = (getenv("MINISHELL_GUI") != NULL);
	gui_line = NULL;
	gui_line_size = 0;
	while (1)
	{
		if (gui_mode)
		{
			fflush(stdout);
			fflush(stderr);
			write(STDOUT_FILENO, MINISHELL_GUI_PROMPT_MARKER,
				sizeof(MINISHELL_GUI_PROMPT_MARKER) - 1);
			line_length = getline(&gui_line, &gui_line_size, stdin);
			if (line_length >= 0 && line_length > 0
				&& gui_line[line_length - 1] == '\n')
				gui_line[--line_length] = '\0';
			line = (line_length < 0) ? NULL : strdup(gui_line);
		}
		else
			line = readline("minishell$ ");
		if (!line)
		{
			printf("exit\n");
			break ;
		}
		if (*line == '\0')
		{
			free(line);
			continue ;
		}
		add_history(line);
		if (!gui_mode)
			rl_on_new_line();
		tokens = parsing(line, *parsed_env);
		if (!tokens)
			continue ;
		ast = create_ast(&tokens);
		exec_ast(ast, parsed_env);
		free_ast(ast);
		free_tokens(tokens);
	}
	free(gui_line);
}

int	main(int argc, char **argv, char **env)
{
	char	**parsed_env;

	parsed_env = dup_env(env);
	(void)argv;
	if (argc != 1)
		return (printf("The number of arguments is incorrect\n"), 1);
	signal(SIGINT, handle_ctrlc);
	signal(SIGQUIT, SIG_IGN);
	shell_loop(&parsed_env);
	free_env(parsed_env);
	return (g_exit_status);
}
