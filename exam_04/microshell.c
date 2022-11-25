#include <unistd.h>
#include <stdlib.h>
#include <string.h>

typedef enum	s_type
{	
	PIPE = 0,
	BREAK = 1,
	END = 2,
	CD = 4
}	t_type;

typedef struct	s_cmd
{
	char	**args;
	int		nb_args;
	int		type;
	int		pipes[2];
}	t_cmd;

typedef struct s_data
{
	t_cmd 	**cmds;
	char	**envp;
	int		nb_cmds;
	int		retval;
}	t_data;

void 	ft_putchar_fd(char c, int fd)
{
	write(fd, &c, 1);
}

void	ft_putstr_fd(char *str, int fd)
{
	int i = 0;
	while (str[i])
	{
		ft_putchar_fd(str[i], fd);
		i++;
	}
}

void	err_fatal(void)
{
	ft_putstr_fd("error: fatal\n", 2);
	exit(EXIT_FAILURE);
}

int	err_msg_ret(char *msg, char *str)
{
	ft_putstr_fd(msg, 2);
	if (str)
		ft_putstr_fd(str, 2);
	ft_putstr_fd("\n", 2);
	return (EXIT_FAILURE);
}

int	count_cmds(char **argv)
{
	int i = 0;
	int count = 0;

	while (argv[i])
	{
		while (argv[i] && !strcmp(argv[i], ";"))
			i++;
		if (argv[i])
			count++;
		while (argv[i] && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
			i++;
		if (argv[i])
			i++;
	}
	return (count);
}

int		count_args(char	**argv, int i)
{
	int count = 0;
	while (argv[i] && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
	{
		count ++;
		i++;
	}
	return (count);
}

t_cmd	*get_cmd(char **argv)
{
	t_cmd	*cmd = malloc (sizeof(t_cmd) * 1);
	if (!cmd)
		err_fatal();
	static int i = 0;
	int j;

	while (argv[i] && !strcmp(argv[i], ";"))
		i++;
	if (argv[i])
	{
		cmd->nb_args = count_args(argv, i);
		cmd->args = malloc(sizeof(char *) * (cmd->nb_args + 1));
		if (!cmd->args)
			err_fatal();
		cmd->args[cmd->nb_args] = NULL;
		j = 0;
		while (argv[i] && strcmp(argv[i], ";") && strcmp(argv[i], "|"))
		{
			cmd->args[j] = argv[i];
			j++;
			i++;
		}
		if (argv[i])
		{
			if (!strcmp(argv[i], "|"))
				cmd->type = PIPE;
			else if (!strcmp(argv[i], ";"))
				cmd->type = BREAK;
			i++;
		}
	}
	return (cmd);
}

int	builtin_cd(t_cmd *cmd)
{
	if (cmd->nb_args != 2)
		err_msg_ret("error: cd: bad arguments", NULL);
	if (chdir(cmd->args[1]) < 0)
		err_msg_ret("error: cd: cannot change directory to ", cmd->args[1]);
	return (0);
}

void	execute(t_data *d)
{
	int i = 0;
	int pid;
	int status;
	int pipe_open;

	d->retval = EXIT_FAILURE;
	while (i < d->nb_cmds)
	{
		pipe_open = 0;
		if (d->cmds[i]->type == CD)
			d->retval = builtin_cd(d->cmds[i]);
		else
		{
			if ((d->cmds[i]->type == PIPE || (i > 0 && d->cmds[i - 1]->type == PIPE)))
			{
				pipe_open = 1;
				if (pipe(d->cmds[i]->pipes) < 0)
					err_fatal();
			}
			pid = fork();
			if (pid < 0)
				err_fatal();
			else if (!pid)
			{
				if (d->cmds[i]->type == PIPE && dup2(d->cmds[i]->pipes[1], STDOUT_FILENO) < 0)
					err_fatal();
				if (i > 0 && d->cmds[i - 1]->type == PIPE && dup2(d->cmds[i - 1]->pipes[0], STDIN_FILENO) < 0)
					err_fatal();
				if (execve(d->cmds[i]->args[0], d->cmds[i]->args, d->envp) < 0)
				{
					ft_putstr_fd("error: cannot execute ", 2);
					ft_putstr_fd(d->cmds[i]->args[0], 2);
					ft_putstr_fd("\n", 2);
				}
				exit(EXIT_FAILURE);
			}
			else
			{
				waitpid(pid, &status, 0);
				if (pipe_open)
				{
					close(d->cmds[i]->pipes[1]);
					if (d->cmds[i]->type == END || d->cmds[i]->type == BREAK)
						close(d->cmds[i]->pipes[0]);
				}
				if (i > 0 && d->cmds[i - 1]->type == PIPE)
					close(d->cmds[i - 1]->pipes[0]);
				if (WIFEXITED(status))
					d->retval = WEXITSTATUS(status); 
			}
		}
		i++;
	}
}

void	free_all(t_data *d)
{
	int i = 0;
	while (d->cmds[i])
	{
		close(d->cmds[i]->pipes[0]);
		close(d->cmds[i]->pipes[1]);
		free(d->cmds[i]);
		i++;
	}
	free(d->cmds);
}

int	main(int argc, char **argv, char **envp)
{
	t_data	d;
	int retval;

	if (argc < 2)
		return (EXIT_SUCCESS);
	d.nb_cmds = count_cmds(&(argv[1]));
	d.cmds = malloc(sizeof(t_cmd *) * (d.nb_cmds + 1));
	if (!d.cmds)
		err_fatal();
	d.cmds[d.nb_cmds] = NULL;
	d.envp = envp;
	int i = 0;
	while (i < d.nb_cmds)
	{
		d.cmds[i] = get_cmd(&(argv[1]));
		i++;
	}
	i = 0;
	while (i < d.nb_cmds)
	{
		if (!d.cmds[i + 1])
			d.cmds[i]->type = END;
		if (!strcmp(d.cmds[i]->args[0], "cd"))
			d.cmds[i]->type = CD;
		i++;
	}
	execute(&d);
	retval = d.retval;
	free_all(&d);
	return (retval);
}