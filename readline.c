#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <mlvalues.h>
#include <alloc.h>
#include <memory.h>

value cl_readline(prompt)
     value prompt;
{
	char *output = readline(String_val(prompt));
	if(!output)
		return copy_string("");
	int len = strlen(output);
	if (len>0)
		add_history(output);
	output = realloc(output, len+2);
	output[len] = '\n';
	output[len+1] = '\0';
	value String_out = copy_string(output);
	free(output);
	return String_out;
}

value cl_readline_init (u)
	value u;
{
	rl_bind_key ('\t', rl_insert);
}
