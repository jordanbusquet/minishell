/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_heredoc.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jbusquet <jbusquet@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/11 13:55:15 by jbossuyt          #+#    #+#             */
/*   Updated: 2026/08/12 13:18:06 by jbusquet         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/minishell.h"

#define MINISHELL_GUI_HEREDOC_MARKER "\036MINISHELL_HEREDOC\037"

static int	is_delim(char *line, char *delim)
{
	return (line && ft_strlen(line) == ft_strlen(delim)
		&& !ft_strncmp(line, delim, ft_strlen(delim)));
}

static void	heredoc_loop(int fd_write, char *delim)
{
	char	*line;
	char	*gui_line;
	size_t	gui_line_size;
	ssize_t	line_length;
	int		gui_mode;

	gui_mode = (getenv("MINISHELL_GUI") != NULL);
	gui_line = NULL;
	gui_line_size = 0;
	signal(SIGINT, handle_ctrlc_heredoc);
	while (1)
	{
		if (gui_mode)
		{
			fflush(stdout);
			fflush(stderr);
			write(STDOUT_FILENO, MINISHELL_GUI_HEREDOC_MARKER,
				sizeof(MINISHELL_GUI_HEREDOC_MARKER) - 1);
			line_length = getline(&gui_line, &gui_line_size, stdin);
			if (line_length >= 0 && line_length > 0
				&& gui_line[line_length - 1] == '\n')
				gui_line[--line_length] = '\0';
			line = (line_length < 0) ? NULL : strdup(gui_line);
		}
		else
			line = readline("heredoc> ");
		if (!line || is_delim(line, delim))
		{
			free(line);
			break ;
		}
		write(fd_write, line, ft_strlen(line));
		write(fd_write, "\n", 1);
		free(line);
	}
	free(gui_line);
}

static void	exec_heredoc_child(t_ast *ast, char ***env, int *fd)
{
	heredoc_loop(fd[1], ast->right->args[0]);
	close(fd[1]);
	dup2(fd[0], STDIN_FILENO);
	close(fd[0]);
	exec_ast(ast->left, env);
	free_ast(ast);
	exit(g_exit_status);
}

void	exec_heredoc(t_ast *ast, char ***env)
{
	int		fd[2];
	pid_t	pid;
	int		status;

	if (pipe(fd) == -1)
	{
		perror("error pipe");
		g_exit_status = 1;
		return ;
	}
	pid = fork();
	if (pid == -1)
	{
		perror("erreur fork");
		g_exit_status = 1;
		return ;
	}
	if (pid == 0)
		exec_heredoc_child(ast, env, fd);
	close(fd[1]);
	close(fd[0]);
	waitpid(pid, &status, 0);
	g_exit_status = status_to_exit_code(status);
}
