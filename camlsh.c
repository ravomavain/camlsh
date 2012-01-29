/*  Camlsh - a camllight wrapper to improve shell-mode usage.
 *  Copyright (C) 2012  Romain Labolle <ravomavain@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * 
 *
 *  Compile : gcc -o camlsh camlsh.c -lreadline
 *  Usage : camlsh [options] [--] [camlrun options]
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <wordexp.h>
#include <regex.h>
#include <sys/types.h>
#include <readline/readline.h>
#include <readline/history.h>

static struct option long_options[] = {
	{"color",   required_argument, NULL, 'c'},
	{"noargs",  no_argument,       NULL, 'n'},
	{"camlrun", required_argument, NULL, 'r'},
	{"camltop", required_argument, NULL, 't'},
	{"stdlib",  required_argument, NULL, 'l'},
	{"open",	required_argument, NULL, 'o'},
	{"help",	no_argument,       NULL, 'h'},
	{NULL, 0, NULL, 0}
};

typedef struct list{
	char *hd;
	struct list *tl;
} list;

void error(char *s)
{
	perror(s);
	exit(1);
}

void* xmalloc(size_t num)
{
	void *new = calloc(1, num);
	if (!new)
		exit(1);
	return new;
}

void xfree(void *p)
{
	if(p)
		free(p);
	p=NULL;
}

char* xstrdup(const char *str)
{
	if (str)
		return strcpy(xmalloc(strlen(str) + 1), str);
	return NULL;
}

char* xstrndup(const char *str, size_t n)
{
	if (str)
		return strncpy(xmalloc(n+1), str, n);
	return NULL;
}

void usage(char **argv)
{
	printf("usage: %s [options] [--] [camlrun options]\n", argv[0]);
	printf("Options:\n"
			"\t-c|--color n\t\tSet color code for error messages.0 to disable.\n"
			"\t\t\t\tSee VT100 colors attributes for more informations. Default: 31 (red).\n"
			"\t-o|--open file.ml\tRun file.ml and give back console access to the use.\n"
			"\t\t\t\tUsefull for testing purpose.\n"
			"\t-t|--camltop <path>\tSet camltop path. This should be a custom camltop,\n"
			"\t\t\t\tcompiled with toplevel_input_prompt = \"\\014\" and error_prompt = \"\\021>\".\n"
			"\t\t\t\tDefault to ./camltop, ~/.camlsh/camltop or */lib/caml-light/caml{sh,}top\n"
			"\t-l|--stdlib <path>\tSet Caml stdlib path for camltop (can be standard one).\n"
			"\t\t\t\tDefault to /usr/lib/caml-light/ or /usr/local/lib/caml-light/\n"
			"\t-r|--camlrun <path>\tSet path to camlrun. Default: camlrun (from PATH).\n"
			"\t-n|--noargs\t\tDon't pass default arguments to camlrun, only [camlrun options] are passed.\n"
			"\t-h|--help\t\tDisplay this message.\n"
	);
	exit(0);
}

char *expand_first(char *filename)
{
	char *result;
	wordexp_t exp_result;
	if(0!=(wordexp(filename, &exp_result, 0)))
		return NULL;
	wordexp(filename, &exp_result, 0);
	result = xstrdup(exp_result.we_wordv[0]);
	wordfree(&exp_result);
	return result;
}

list *expand(char *filename)
{
	wordexp_t exp_result;
	list *files=NULL, *tmp=NULL;
	int i;

	if(0!=(wordexp(filename, &exp_result, 0)))
		return NULL;
	for (i=exp_result.we_wordc-1; i >= 0 ; i--)
	{
		tmp = xmalloc(sizeof(list));
		tmp->hd = xstrdup(exp_result.we_wordv[i]);
		tmp->tl = files;
		files = tmp;
	}
	wordfree(&exp_result);

	return files;
}

char *find_camlstdlib(void)
{
	if(access("/usr/lib/caml-light/", R_OK) == 0)
		return "/usr/lib/caml-light/";
	if(access("/usr/local/lib/caml-light/", R_OK) == 0)
		return "/usr/local/lib/caml-light/";
	error("Cannot find caml stdlibs");
}

char *find_camltop(void)
{
	char *filename;

	if(access("./camltop", R_OK) == 0)
		return "./camltop";

	filename = expand_first("~/.camlsh/camltop");
	if(access(filename, R_OK) == 0)
		return filename;
	xfree(filename);

	if(access("/usr/lib/caml-light/camlshtop", R_OK) == 0)
		return "/usr/lib/caml-light/camlshtop";
	if(access("/usr/local/lib/caml-light/camlshtop", R_OK) == 0)
		return "/usr/local/lib/caml-light/camlshtop";
	if(access("/usr/lib/caml-light/camltop", R_OK) == 0)
		return "/usr/lib/caml-light/camltop";
	if(access("/usr/local/lib/caml-light/camltop", R_OK) == 0)
		return "/usr/local/lib/caml-light/camltop";
	error("Cannot find camltop");
}

void camlbackend(int in[2], int out[2], int argc, char *argv[])
{
	int c, i, option_index=0, noargs=0;
	char *camlrun = NULL;
	char *camltop = NULL;
	char *stdlib = NULL;
	char **camlargv = NULL;

	/* Close stdin, stdout, stderr */
	close(0);
	close(1);
	close(2);
	/* make our pipes, our new stdin,stdout and stderr */
	dup2(in[0],0);
	dup2(out[1],1);
	dup2(out[1],2);

	/* Close unused ends of the pipes */
	close(in[1]);
	close(out[0]);

	/* Parse command line arguments */
	opterr=0;
	while ((c = getopt_long(argc, argv, "c:nr:t:l:o:h", long_options, &option_index)) != -1)
	{
		switch (c)
		{
			case 'c':
			case 'o':
				break;
			case 'n':
				noargs = 1;
				break;
			case 'r':
				camlrun = optarg;
				break;
			case 't':
				camltop = optarg;
				break;
			case 'l':
				stdlib = optarg;
				break;
			case '?':
			case 'h':
				exit(0);
				break; /* Yes, I know... */
			default:
				printf ("?? getopt returned character code 0%o ??\n", c);
		}
	}

	camlargv = xmalloc(sizeof(char*)*( 1 + (noargs?0:3) + (argc - optind) + 1)); /* "camlrun" + ("/path/camltop" + "-stdlib" + "/path/stdlib/") + extras + NULL */

	if(camlrun == NULL)
		camlrun = "camlrun";
	camlargv[0] = camlrun;
	i=1;

	if(!noargs)
	{
		if(camltop)
			camlargv[1] = camltop;
		else
			camlargv[1] = find_camltop();
		camlargv[2] = "-stdlib";
		if(stdlib)
			camlargv[3] = stdlib;
		else
			camlargv[3] = find_camlstdlib();
		i=4;
	}

	while (optind < argc)
	{
		camlargv[i++] = argv[optind++];
	}

	camlargv[i] = (char*)NULL;

	/* Over-write the child process with camllight */
	execvp(camlrun, camlargv);
	error("Could not exec camllight");
}

void camlfrontend(int in[2], int out[2], int argc, char *argv[])
{
	int o=0, i=0, n=0, option_index=0, com=0, sqt=0, dqt=0, inlen=0, color=31;
	char c=0;
	char *input=NULL;
	char *str=NULL;
	char *histfile=NULL;
	char prompt[3] = "# ";
	char *set_color=NULL, *reset_color=NULL;
	FILE *output=NULL;
	list *includes=NULL, *tmp=NULL;
	FILE *inc=NULL;
	regex_t regex;
	size_t nmatch = 2;
	regmatch_t pmatch[2];

	/* Close unused ends of the pipes */
	close(in[0]);
	close(out[1]);

	/* Open caml output for reading */
	output = fdopen(out[0], "rb");
	if(output == NULL)
		error("Could not open pipe");
	opterr=0;
	while ((o = getopt_long(argc, argv, "c:nr:t:l:o:h", long_options, &option_index)) != -1)
	{
		switch (o)
		{
			case 'c':
				color = strtol(optarg, &optarg+strlen(optarg), 10);
				break;
			case 'o':
				if(includes == NULL)
					includes = expand(optarg);
				else
				{
					tmp = includes;
					while(tmp->tl != NULL)
						tmp = tmp->tl;
					tmp->tl = expand(optarg);
				}
				break;
			case 'n':
			case 'r':
			case 't':
			case 'l':
				break;
			case 'h':
				usage(argv);
				return;
			case '?':
				if (optopt == 'c' || optopt == 'r' || optopt == 't' || optopt == 'l' || optopt == 'o')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				usage(argv);
				return;
			default:
				printf ("?? getopt returned character code 0%o ??\n", o);
		}
	}

	if ( (1 <= color && color <=8) || (30 <= color && color <=37) || (40 <= color && color <=47))
	{
		set_color = xmalloc(sizeof(char)*6);
		snprintf(set_color, 6, "\033[%dm", color);
		reset_color = "\033[m";
	}
	else
		set_color = reset_color = "";

	while((c = getc(output))!=(char)14)
	{
		if(c == EOF)
			return;
		putchar(c);
	}

	if(0 != (regcomp(&regex, "^(\\*include \\(.*\\)\\*)$", 0)))
		error("regcomp() failed");

	histfile = expand_first("~/.camlsh/history");
	read_history(histfile);
	rl_bind_key ('\t', rl_insert);

	for(;;)
	{
		if(includes != NULL && inc == NULL)
		{
			inc=fopen(includes->hd, "rb");
			if(inc)
				rl_instream = inc;
			else
			{
				if(includes->hd != NULL)
					xfree(includes->hd);
				tmp = includes;
				includes = includes->tl;
				xfree(tmp);
				if(includes == NULL)
				{
					 rl_instream = stdin;
					 continue;
				}
			}
		}

		printf(reset_color);
		input = readline(prompt);

		if (!input)
		{
			if(includes != NULL && inc)
			{
				rl_instream = stdin;
				fclose(inc);
				inc = NULL;
				if(includes->hd != NULL)
					xfree(includes->hd);
				tmp = includes;
				includes = includes->tl;
				xfree(tmp);
				putchar('\n');
				continue;
			}
			else
				break;
		}
		inlen = strlen(input);
		if(inlen == 0)
			continue;
		if(regexec(&regex, input, nmatch, pmatch, 0)==0)
		{
			str = xstrndup(&input[pmatch[1].rm_so], pmatch[1].rm_eo-pmatch[1].rm_so);
			if(includes == NULL)
				includes = expand(str);
			else
			{
				tmp = includes;
				while(tmp->tl != NULL)
					tmp = tmp->tl;
				tmp->tl = expand(str);
			}
			xfree(str);
		}

		add_history(input);

		write(in[1], input, inlen);
		write(in[1], "\n", 1);

		prompt[0]='\0';
		n=0;
		for(i=0;i<inlen-1;i++)
		{
			if(!sqt && !com && input[i] == '"' && (i==0 || input[i-1]!='\\'))
			{
				dqt ^= 1;
				continue;
			}
			if(!dqt && !com && input[i] == '\'' && (i==0 || input[i-1]!='\\'))
			{
				sqt ^= 1;
				continue;
			}
			if(!dqt && !sqt && input[i] == '(' && input[i+1] == '*')
			{
				com++;
				i++;
				continue;
			}
			if(!dqt && !sqt && input[i] == '*' && input[i+1] == ')' && com)
			{
				com--;
				i++;
				continue;
			}
			if(!dqt && !sqt && !com && input[i] == ';' && input[i+1] == ';')
			{
				n++;
				i++;
				if(i==inlen-1)
					prompt[0]='#';
			}
		}
		while(n>0)
		{
			c = getc(output);
			if(c == EOF)
				return;
			else if(c == (char)14)
				n--;
			else if(c == (char)21)
				printf(set_color);
			else
				putchar(c);
		}
	}
	putchar('\n');
	mkdir(expand_first("~/.camlsh"));
	write_history(histfile);
	xfree(histfile);
	regfree(&regex);
	close(in[1]);
	close(out[0]);
	return;
}

int main(int argc, char *argv[])
{
	int in[2], out[2], pid, status;

	/* In a pipe, xx[0] is for reading, xx[1] is for writing */
	if (pipe(in) < 0) error("pipe in");
	if (pipe(out) < 0) error("pipe out");

	pid=fork();
	if (pid == -1)
		error("Could not fork");
	else if (pid == 0)
		camlbackend(in, out, argc, argv);
	else
		camlfrontend(in, out, argc, argv);
	if(waitpid(pid, &status, 0)==-1)
		error("waitpid()");
	exit(status>>8);
}

