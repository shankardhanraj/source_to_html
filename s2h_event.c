#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "s2h_event.h"

/********** Internal states and event of parser **********/
typedef enum
{
	PSTATE_IDLE,
	PSTATE_PREPROCESSOR_DIRECTIVE,
	PSTATE_HEADER_FILE,
	PSTATE_RESERVE_KEYWORD,
	PSTATE_NUMERIC_CONSTANT,
	PSTATE_STRING,
	PSTATE_COMMENT,
	PSTATE_ASCII_CHAR,
	PSTATE_CODE,
	PSTATE_DEFINES,
	PSTATE_EOF,
}pstate_e;

/* parser state variable */
pstate_e state = PSTATE_IDLE;

static pevent_t pevent_data;
static int event_data_idx = 0;

static char word[WORD_BUFF_SIZE];
static int word_idx = 0;

static char ch;
static char pre_ch;
static int pre_cur;
static int cur;

static int inside_a_string = 0;

static char* res_kwords_data[] = {"const", "volatile", "extern", "auto", "register",
	"static", "signed", "unsigned", "short", "long", 
	"double", "char", "int", "float", "struct", 
	"union", "enum", "void", "typedef", ""
};

static char* res_kwords_non_data[] = {"goto", "return", "continue", "break", 
	"if", "else", "for", "while", "do", 
	"switch", "case", "default","sizeof", ""
};

static char operators[] = {'/', '+', '*', '-', '%', '=', '<', '>', '~', '&', '.', ',', '!', '^', '|', '\0'};
static char symbols[] = {'(', ')', '{', '[', ':', '}', ']', ';', '\"', '\'', ',', '\\', '\0'};
static char reg_exp[] = {'e', 'u', 'i', 'o', 'p', 'a', 's', 'd', 'f', 'g', 'x', 'c', 'b', 'n', '\0'};

/********** state handlers **********/
pevent_t * pstate_idle_handler(FILE *fs);
pevent_t * pstate_comment_handler(FILE *fs);
pevent_t * pstate_ascii_char_handler(FILE *fs);
pevent_t * pstate_string_handler(FILE *fs);
pevent_t * pstate_reserve_keyword_handler(FILE *fs);
pevent_t * pstate_preprocessor_directive_handler(FILE *fs);
pevent_t * pstate_header_file_handler(FILE *fs);
pevent_t * pstate_numeric_constant_handler(FILE *fs);
pevent_t * pstate_code_handler(FILE *s);
pevent_t * pstate_defines_handler(FILE *s);
/********** Utility functions **********/

/* function to check symbols */
static int is_symbol(char c)
{
	int idx;
	for(idx = 0; symbols[idx] != '\0'; idx++)
	{
		if(symbols[idx] == c)
			return 1;
	}

	return 0;
}

/* function to check operator */
static int is_operator(char c)
{
	int idx;
	for(idx = 0; operators[idx] != '\0'; idx++)
	{
		if(operators[idx] == c)
			return 1;
	}

	return 0;
}

/* to set parser event */
static void set_parser_event(pstate_e s, pevent_e e)
{
	pevent_data.data[event_data_idx] = '\0';
	pevent_data.length = event_data_idx;
	event_data_idx = 0;
	state = s;
	pevent_data.type = e;
}

/* to get parser event */
pevent_t *get_parser_event(FILE *fs)
{
	pevent_t *evptr = NULL;

	switch(state)
	{
		case PSTATE_IDLE :
#ifdef DEBUG  
			puts("idle");
#endif
			if((evptr = pstate_idle_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_COMMENT :
#ifdef DEBUG
			puts("comment");
#endif
			if((evptr = pstate_comment_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_PREPROCESSOR_DIRECTIVE :
#ifdef DEBUG
			puts("pre_proc_dir");
#endif
			if((evptr = pstate_preprocessor_directive_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_RESERVE_KEYWORD :
#ifdef DEBUG
			puts("res_key");
#endif
			if((evptr = pstate_reserve_keyword_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_NUMERIC_CONSTANT :
#ifdef DEBUG
			puts("num_const");
#endif
			if((evptr = pstate_numeric_constant_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_STRING :
#ifdef DEBUG
			puts("string");
#endif
			if((evptr = pstate_string_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_HEADER_FILE :
#ifdef DEBUG
			puts("header");
#endif
			if((evptr = pstate_header_file_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_ASCII_CHAR :
#ifdef DEBUG
			puts("ascii");
#endif
			if((evptr = pstate_ascii_char_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_DEFINES:
#ifdef DEBUG
			puts("defines");
#endif
			if((evptr = pstate_defines_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_CODE:
#ifdef DEBUG
			puts("code");
#endif
			if((evptr = pstate_code_handler(fs)) != NULL)
				return evptr;
			break;
		case PSTATE_EOF :
#ifdef DEBUG
			puts("eof");
#endif
			printf("<< SUCCESS >> ");
			pevent_data.type = PEVENT_EOF;
			return &pevent_data;
			break;
	}
}

pevent_t * pstate_idle_handler(FILE *fs)
{
	cur = (unsigned int)ftell(fs);
	ch = fgetc(fs);

	/* Comment */
	if (ch == '/')
	{
#ifdef DEBUG
		puts("comment");
#endif
		pre_ch = ch;
		if ((ch = fgetc(fs)) == '/' || ch == '*')
		{
			fseek(fs, cur, SEEK_SET);
			set_parser_event(PSTATE_COMMENT, PEVENT_NULL);
			return &pevent_data;
		}
		else
		{
			fseek(fs, cur, SEEK_SET);
			set_parser_event(PSTATE_CODE, PEVENT_NULL);
			return &pevent_data;
		}
	}

	/* Pre-processor directive */
	if (ch == '#')
	{
#ifdef DEBUG
		puts("pre_pro");
#endif
		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_PREPROCESSOR_DIRECTIVE, PEVENT_NULL);
		return &pevent_data;
	}

	/* Reserved keywords */
	if ((ch >= 'a' && ch <= 'z') || ch >= 'A' && ch <= 'Z')
	{
#ifdef DEBUG
		puts("res_key");
#endif
		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_RESERVE_KEYWORD, PEVENT_NULL);
		return &pevent_data;
	}

	/* Numerical constants */
	if (ch >= '0' && ch <= '9')
	{
#ifdef DEBUG
		puts("num_const");
#endif
		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_NUMERIC_CONSTANT, PEVENT_NULL);
		return &pevent_data;
	}

	/* Strings */
	if (ch == '"')
	{
#ifdef DEBUG
		puts("string");
#endif
		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_STRING, PEVENT_NULL);
		return &pevent_data;
	}

	/* ASCII character */
	if (ch == 39)
	{
#ifdef DEBUG
		puts("ascii");
#endif
		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_ASCII_CHAR, PEVENT_NULL);
		return &pevent_data;
	}

	/* EOF */
	if (ch == EOF)
	{
#ifdef DEBUG
		puts("eof");
#endif
		set_parser_event(PSTATE_EOF, PEVENT_NULL);
		return &pevent_data;
	}

	/* Operators */
	if (is_operator(ch))
	{
#ifdef DEBUG
		puts("op");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_IDLE, PEVENT_CODE);
		return &pevent_data;
	}

	/* Symbols */
	if (is_symbol(ch))
	{
#ifdef DEBUG
		puts("sym");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_IDLE, PEVENT_CODE);
		return &pevent_data;
	}

	/* Space */
	if (isspace(ch))
	{
#ifdef DEBUG
		puts("space");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_IDLE, PEVENT_CODE);
		return &pevent_data;
	}
#ifdef DEBUG
	puts("switch end");
#endif
}

pevent_t * pstate_comment_handler(FILE *fs)
{
	int f_star = 1, f_slash = 1;

	/* Single line comment */
	cur = (unsigned int)ftell(fs);
	pre_ch = fgetc(fs);
	ch = fgetc(fs);

	if (ch == '/')
	{
		pevent_data.data[event_data_idx++] = pre_ch;
		pevent_data.data[event_data_idx++] = ch;

		while ((pevent_data.data[event_data_idx]= fgetc(fs)) != '\n' && pevent_data.data[event_data_idx] != EOF)
		{
			if (pevent_data.data[event_data_idx] == EOF)
			{
				set_parser_event(PSTATE_EOF, PEVENT_SINGLE_LINE_COMMENT);
				return &pevent_data;
			}
			event_data_idx++;
		}

		event_data_idx++;
		set_parser_event(PSTATE_IDLE, PEVENT_SINGLE_LINE_COMMENT);
		return &pevent_data;
	}

	/* Multi line comment */
	else 
	{
		pevent_data.data[event_data_idx++] = pre_ch;
		pevent_data.data[event_data_idx++] = ch;
		f_star = 1, f_slash = 1;

		while (f_star || f_slash)
		{
			pevent_data.data[event_data_idx] = fgetc(fs);

			if (pevent_data.data[event_data_idx] == EOF)
			{
				set_parser_event(PSTATE_EOF, PEVENT_MULTI_LINE_COMMENT);
				return &pevent_data;
			}

			pre_ch = pevent_data.data[event_data_idx];
			ch = fgetc(fs);
			fseek(fs, -1, SEEK_CUR);

			if (pre_ch == '*' && ch == '/')
			{
				fseek(fs, 1, SEEK_CUR);
				pevent_data.data[++event_data_idx] = '/';
				event_data_idx++;
				set_parser_event(PSTATE_IDLE, PEVENT_MULTI_LINE_COMMENT);
				return &pevent_data;
			}

			event_data_idx++;
		}

	}
}


pevent_t * pstate_preprocessor_directive_handler(FILE *fs)
{
	int i, j, k;
	char define[] = "efine ";
	char include[] = "nclude";
	char *def[] = {"if", "ifdef", "ifndef", ""};

	cur = (unsigned int) ftell(fs);
	pevent_data.data[event_data_idx++] = fgetc(fs);

	while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF && pevent_data.data[event_data_idx] != '\n')
	{
		if (pevent_data.data[event_data_idx] != ' ')
		{
			break;
		}
		event_data_idx++;
	}
	pre_cur = (unsigned int) ftell(fs);

	if (pevent_data.data[event_data_idx] == 'd')
	{
		event_data_idx++;
		for (i = 0; i < 6; i++)
		{
			if ((pevent_data.data[event_data_idx] = fgetc(fs)) == define[i])
			{
				event_data_idx++; 
				continue;
			}
			break;
		}
		if (i != 6)
		{
			fseek(fs, cur, SEEK_SET);
			set_parser_event(PSTATE_CODE, PEVENT_NULL);
			return &pevent_data;
		}
		else
		{
			set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
			return &pevent_data;
		}

	}
	else if (pevent_data.data[event_data_idx] == 'i')
	{
		event_data_idx++;
		for (i = 0; i < 6; i++)
		{
			if ((pevent_data.data[event_data_idx] = fgetc(fs)) == include[i])
			{
				event_data_idx++; 
				continue;
			}
			break;
		}
		if (i != 6)
		{
			fseek(fs, pre_cur, SEEK_SET);
			word_idx = 0;
			word[word_idx++] = 'i';

			while ((word[word_idx] = fgetc(fs)) != EOF)
			{
				if (word[word_idx] == ' ' || word[word_idx] == '\n')
					break;
				word_idx++;
			}
			word[word_idx] = '\0';

			for (j = 0; strcmp(def[j], "") != 0; j++)
			{	
				if (strcmp(def[j], word) == 0)
				{
					event_data_idx = 0;

					pevent_data.data[event_data_idx++] = '#';
					for (k = 0; (pevent_data.data[event_data_idx++] = word[k]) !=	0; k++)
						;

					fseek(fs, -1, SEEK_CUR);
					word_idx = 0;
					set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
					return &pevent_data;
				}
			}
			fseek(fs, cur, SEEK_SET);
			set_parser_event(PSTATE_CODE, PEVENT_NULL);
			return &pevent_data;
		}
		else
		{
			while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF && pevent_data.data[event_data_idx] != '\n')
			{
				if (pevent_data.data[event_data_idx] != ' ')
					break;
				event_data_idx++;
			}
			if(pevent_data.data[event_data_idx] == '<' || pevent_data.data[event_data_idx] == '"')
			{
				fseek(fs, -1, SEEK_CUR);
				set_parser_event(PSTATE_HEADER_FILE, PEVENT_PREPROCESSOR_DIRECTIVE);
				return &pevent_data;
			}
			else
			{
				fseek(fs, cur, SEEK_SET);
				set_parser_event(PSTATE_CODE, PEVENT_NULL);
				return &pevent_data;
			}
		}
	}
	else if (pevent_data.data[event_data_idx] == 'e')
	{
		fseek(fs, pre_cur, SEEK_SET);
		word_idx = 0;
		word[word_idx++] = 'e';

		while ((word[word_idx] = fgetc(fs)) != EOF)
		{
			if (word[word_idx] == ' ' || word[word_idx] == '\n')
				break;
			word_idx++;
		}
		word[word_idx] = '\0';

		if (strcmp(word, "endif") == 0)
		{
			event_data_idx = 0;

			pevent_data.data[event_data_idx++] = '#';
			for (k = 0; (pevent_data.data[event_data_idx++] = word[k]) !=	0; k++)
				;

			fseek(fs, -1, SEEK_CUR);
			word_idx = 0;
			set_parser_event(PSTATE_IDLE, PEVENT_PREPROCESSOR_DIRECTIVE);
			return &pevent_data;
		}

	}

	fseek(fs, cur, SEEK_SET);
	set_parser_event(PSTATE_CODE, PEVENT_NULL);
	return &pevent_data;
}


pevent_t * pstate_reserve_keyword_handler(FILE *fs)
{
	int i, j;

	cur = (unsigned int) ftell(fs);

	while ((word[word_idx] = fgetc(fs)) != EOF)
	{
		if(word[word_idx] == '\n' || word[word_idx] == ' ')
			break;

		for (i = 0; operators[i] != '\0'; i++)
		{
			if (word[word_idx] == operators[i])
			{
				break;
			}
		}
		if (word[word_idx] == operators[i])
			break;

		for (i = 0; symbols[i] != '\0'; i++)
		{
			if (word[word_idx] == symbols[i])
			{
				break;
			}
		}
		if (word[word_idx] == symbols[i])
			break;

		word_idx++;

	}

	word[word_idx] = '\0';

	for (i = 0; strcmp(res_kwords_data[i], "") != 0; i++) 
	{
		if (strcmp(word, res_kwords_data[i]) == 0)
		{
			for (j = 0; (pevent_data.data[event_data_idx++] = word[j]) != '\0'; j++);

			word_idx = 0;
			fseek(fs, -1, SEEK_CUR);
			set_parser_event(PSTATE_IDLE, PEVENT_RESERVE_KEYWORD_DATA); // to be directed to idle mode
			return &pevent_data;
		}
	}

	for (i = 0; strcmp(res_kwords_non_data[i], "") != 0; i++) 
	{
		if (strcmp(word, res_kwords_non_data[i]) == 0)
		{
			for (j = 0; (pevent_data.data[event_data_idx++] = word[j]) != '\0'; j++);

			word_idx = 0;
			fseek(fs, -1, SEEK_CUR);
			set_parser_event(PSTATE_IDLE, PEVENT_RESERVE_KEYWORD_NON_DATA); // to be directed to idle mode
			return &pevent_data;
		}
	}

	fseek(fs, cur, SEEK_SET);
	word_idx = 0;
	set_parser_event(PSTATE_CODE, PEVENT_NULL); // to be directed to code 
	return &pevent_data;
}

pevent_t * pstate_numeric_constant_handler(FILE *fs)
{
	int i;
	char suffix[] = {' ', 'l', 'f', 'u', 'L', '\0'};

	cur = (unsigned int)ftell(fs);

	while (isdigit(pevent_data.data[event_data_idx] = fgetc(fs)))
	{
		if (pevent_data.data[event_data_idx] == EOF)
			break;
		event_data_idx++;
	}

	if (pevent_data.data[event_data_idx] == EOF)
	{
		set_parser_event(PSTATE_EOF, PEVENT_NUMERIC_CONSTANT); // to be directed to eof
		return &pevent_data;
	}
	else if (pevent_data.data[event_data_idx] == ' ' || pevent_data.data[event_data_idx] == '\n')
	{
		fseek(fs, -1, SEEK_CUR);
		set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT); // to be directed to eof
		return &pevent_data;
	}
	else
	{
		for (i = 0; suffix[i] != '\0'; i++)
		{
			if (pevent_data.data[event_data_idx] == suffix[i])
			{
				event_data_idx++;

				if ((ch = fgetc(fs)) != ' ' && ch != EOF && ch != '\n')
				{
					fseek(fs, cur, SEEK_SET);
					set_parser_event(PSTATE_CODE, PEVENT_NULL); // to be directed to code 
					return &pevent_data;
				}

				fseek(fs, -1, SEEK_CUR);
				set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT); // to be directed to idle
				return &pevent_data;
			}
		}

		for (i = 0; operators[i] != '\0'; i++)
		{
			if (pevent_data.data[event_data_idx] == operators[i])
			{
				fseek(fs, -1, SEEK_CUR);
				set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT); // to be directed to idle
				return &pevent_data;
			}
		}

		for (i = 0; symbols[i] != '\0'; i++)
		{
			if (pevent_data.data[event_data_idx] == symbols[i])
			{
				fseek(fs, -1, SEEK_CUR); 
				set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT); // to be directed to idle
				return &pevent_data;
			}
		}

		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_CODE, PEVENT_NULL); // to be directed to code
		return &pevent_data;
	}
}



pevent_t * pstate_string_handler(FILE *fs)
{
	int i;
	cur = (unsigned int)ftell(fs);

	ch = fgetc(fs);

	switch(ch)
	{
		case EOF:
			set_parser_event(PSTATE_EOF, PEVENT_NULL);
			return &pevent_data;
			break;
		case '%':
			pre_ch = ch;
			ch = fgetc(fs);

			if (ch == EOF)
			{
				event_data_idx = 0;
				pevent_data.data[event_data_idx++] = pre_ch;
				set_parser_event(PSTATE_EOF, PEVENT_REGULAR_EXP);
				return &pevent_data;
				break;
			}

			for (i = 0; reg_exp[i] != '\0'; i++)
			{
				if (ch == reg_exp[i])
					break;
			}

			if (ch == reg_exp[i])
			{
				pevent_data.data[event_data_idx++] = pre_ch;
				pevent_data.data[event_data_idx++] = ch;
				set_parser_event(PSTATE_STRING, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}
			else
			{
				pevent_data.data[event_data_idx++] = pre_ch;
				pevent_data.data[event_data_idx++] = ch;
				set_parser_event(PSTATE_STRING, PEVENT_STRING);
				return &pevent_data;
			}
			break;
		case '\\': 
			pevent_data.data[event_data_idx++] = ch;
			ch = fgetc(fs);

			if (ch == EOF)
			{
				set_parser_event(PSTATE_EOF, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}

			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_STRING, PEVENT_REGULAR_EXP);
			return &pevent_data;
			break;

		default : 
			if (!inside_a_string)
			{
				inside_a_string = 1;
				pevent_data.data[event_data_idx++] = ch;
			}
			else if (ch == '"')
			{
				pevent_data.data[event_data_idx++] = ch;
				inside_a_string = 0;
				set_parser_event(PSTATE_IDLE, PEVENT_STRING);
				return &pevent_data;
			}
			else
			{
				pevent_data.data[event_data_idx++] = ch;
			}

			while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF)
			{
				if (pevent_data.data[event_data_idx] == '"' || pevent_data.data[event_data_idx] == '%' || pevent_data.data[event_data_idx] == '\\')
					break;
				event_data_idx++;
			}

			if (pevent_data.data[event_data_idx] == '"')
			{
				event_data_idx++;
				inside_a_string = 0;
				set_parser_event(PSTATE_IDLE, PEVENT_STRING);
				return &pevent_data;
			}
			else if(pevent_data.data[event_data_idx] == '%' || pevent_data.data[event_data_idx] == '\\')
			{
				fseek(fs, -1, SEEK_CUR);
				set_parser_event(PSTATE_STRING, PEVENT_STRING);
				return &pevent_data;
			}
			else
			{
				inside_a_string = 0;
				set_parser_event(PSTATE_EOF, PEVENT_STRING);
				return &pevent_data;
			}
			break;
	}
}

pevent_t * pstate_header_file_handler(FILE *fs)
{
	cur = (unsigned int) ftell(fs);
	ch = fgetc(fs);

	switch (ch)
	{
		case '<':
			pevent_data.data[event_data_idx++] = '&';
			pevent_data.data[event_data_idx++] = 'l';
			pevent_data.data[event_data_idx++] = 't';
			pevent_data.data[event_data_idx++] = ';';
			while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF)
			{
				if (pevent_data.data[event_data_idx] == '\n')
					break;
				if (pevent_data.data[event_data_idx] == '>')
					break;
				event_data_idx++;
			}

			if (pevent_data.data[event_data_idx] == '>')
			{
				pevent_data.data[event_data_idx++] = '&';
				pevent_data.data[event_data_idx++] = 'g';
				pevent_data.data[event_data_idx++] = 't';
				pevent_data.data[event_data_idx++] = ';';
				pevent_data.data[event_data_idx] = '\0';
				set_parser_event(PSTATE_IDLE, PEVENT_HEADER_FILE);
				return &pevent_data;
			}
			else
			{
				fseek(fs, cur + 1, SEEK_SET);
				event_data_idx = 1;
				set_parser_event(PSTATE_CODE, PEVENT_PREPROCESSOR_DIRECTIVE);
				return &pevent_data;
			}
			break;

		case '"':
			pevent_data.data[event_data_idx++] = ch;

			while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF)
			{
				if (pevent_data.data[event_data_idx] == '\n')
					break;
				if (pevent_data.data[event_data_idx] == ch)
					break;
				event_data_idx++;
			}

			if (pevent_data.data[event_data_idx] == ch)
			{
				event_data_idx++;
				set_parser_event(PSTATE_IDLE, PEVENT_HEADER_FILE);
				return &pevent_data;
			}
			else
			{
				fseek(fs, cur, SEEK_SET);
				set_parser_event(PSTATE_CODE, PEVENT_HEADER_FILE);
				return &pevent_data;
			}
			break;
	}
}


pevent_t * pstate_ascii_char_handler(FILE *fs)
{
	cur = (unsigned int)ftell(fs);
	pre_ch = (pevent_data.data[event_data_idx++] = fgetc(fs));
	ch = pevent_data.data[event_data_idx++] = fgetc(fs);

	if (ch == '\\')
	{
		if ((ch = (pevent_data.data[event_data_idx++] = fgetc(fs))) == '0' || ch == 'a' || ch == 'b' || ch == 't' || ch == 'n' || ch == 'v' || ch == 'f' || ch == 'r' || ch == '\"' || ch == '\'' || ch == '\\')
		{
			ch = (pevent_data.data[event_data_idx++] = fgetc(fs));
			if ( pre_ch == ch)
			{
				set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
				return &pevent_data;
			}
			else
			{
				fseek(fs, cur, SEEK_SET);
				set_parser_event(PSTATE_CODE, PEVENT_NULL);
				return &pevent_data;
			}
		}
		else
		{
			fseek(fs, cur, SEEK_SET);
			set_parser_event(PSTATE_CODE, PEVENT_NULL);
			return &pevent_data;
		}

	}
	ch = (pevent_data.data[event_data_idx++] = fgetc(fs));

	if (pre_ch == ch)
	{
		set_parser_event(PSTATE_IDLE, PEVENT_ASCII_CHAR);
		return &pevent_data;
	}
	else
	{
		fseek(fs, cur, SEEK_SET);
		set_parser_event(PSTATE_CODE, PEVENT_NULL);
		return &pevent_data;
	}
}

pevent_t * pstate_defines_handler(FILE *fs)
{
	int f_star = 1, f_slash = 1;
	int i, j;
	char suffix[] = {' ', 'l', 'f', 'u', 'L', '\0'};
	cur = (unsigned int)ftell(fs);
	ch = fgetc(fs);


	/* Strings */
	if (ch == '"' || inside_a_string)
	{
#ifdef DEBUG
		puts("d_strings");
#endif
		switch(ch)
		{
			case EOF:
				set_parser_event(PSTATE_EOF, PEVENT_NULL);
				return &pevent_data;
				break;
			case '%':
#ifdef DEBUG
				puts("d_reg_exp");
#endif
				pre_ch = ch;
				ch = fgetc(fs);

				if (ch == EOF)
				{
					event_data_idx = 0;
					pevent_data.data[event_data_idx++] = pre_ch;
					inside_a_string = 0;
					set_parser_event(PSTATE_EOF, PEVENT_REGULAR_EXP);
					return &pevent_data;
					break;
				}
				for (i = 0; reg_exp[i] != '\0'; i++)
				{
					if (ch == reg_exp[i])
						break;
				}
				if (ch == reg_exp[i])
				{
					pevent_data.data[event_data_idx++] = pre_ch;
					pevent_data.data[event_data_idx++] = ch;
					inside_a_string = 1;
					set_parser_event(PSTATE_STRING, PEVENT_REGULAR_EXP);
					return &pevent_data;
				}
				else
				{
					pevent_data.data[event_data_idx++] = pre_ch;
					pevent_data.data[event_data_idx++] = ch;
					inside_a_string = 1;
					set_parser_event(PSTATE_STRING, PEVENT_STRING);
					return &pevent_data;
				}
				break;
			case '\\': 
				pevent_data.data[event_data_idx++] = ch;
				ch = fgetc(fs);

				if (ch == EOF);
				{
					set_parser_event(PSTATE_EOF, PEVENT_REGULAR_EXP);
					return &pevent_data;
				}

				pevent_data.data[event_data_idx++] = ch;
				inside_a_string = 1;
				set_parser_event(PSTATE_STRING, PEVENT_REGULAR_EXP);
				return &pevent_data;
				break;

			default : 
				if (!inside_a_string)
				{
					inside_a_string = 1;
					pevent_data.data[event_data_idx++] = ch;
				}
				else if (ch == '"')
				{
					pevent_data.data[event_data_idx++] = ch;
					inside_a_string = 0;
					set_parser_event(PSTATE_IDLE, PEVENT_STRING);
					return &pevent_data;
				}
				else
				{
					pevent_data.data[event_data_idx++] = ch;
				}

				while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF)
				{
					if (pevent_data.data[event_data_idx] == '"' || pevent_data.data[event_data_idx] == '%' || pevent_data.data[event_data_idx] == '\\')
						break;
					event_data_idx++;
				}
				if (pevent_data.data[event_data_idx] == '"')
				{
					event_data_idx++;
					inside_a_string = 0;
					set_parser_event(PSTATE_IDLE, PEVENT_STRING);
					return &pevent_data;
				}
				else if(pevent_data.data[event_data_idx] == '%' || pevent_data.data[event_data_idx] == '\\')
				{
					fseek(fs, -1, SEEK_CUR);
					set_parser_event(PSTATE_STRING, PEVENT_STRING);
					return &pevent_data;
				}
				else
				{
					inside_a_string = 0;
					set_parser_event(PSTATE_EOF, PEVENT_STRING);
				}
				break;
		}
	}

	/* Comment */
	if (ch == '/')
	{
#ifdef DEBUG
		puts("d_comment");
#endif

		/* Single line comment */
		pre_ch = ch;
		ch = fgetc(fs);

		if (ch == '/')
		{
			pevent_data.data[event_data_idx++] = pre_ch;
			pevent_data.data[event_data_idx++] = ch;

			while ((pevent_data.data[event_data_idx]= fgetc(fs)) != '\n' && pevent_data.data[event_data_idx] != EOF)
			{
				if (pevent_data.data[event_data_idx] == EOF)
				{
					set_parser_event(PSTATE_EOF, PEVENT_SINGLE_LINE_COMMENT);
					return &pevent_data;
				}
				event_data_idx++;
			}

			event_data_idx++;
			set_parser_event(PSTATE_IDLE, PEVENT_SINGLE_LINE_COMMENT);
			return &pevent_data;
		}

		/* Multi line comment */
		else if (ch == '*') 
		{
			pevent_data.data[event_data_idx++] = pre_ch;
			pevent_data.data[event_data_idx++] = ch;
			f_star = 1, f_slash = 1;

			while (f_star || f_slash)
			{
				pevent_data.data[event_data_idx] = fgetc(fs);

				if (pevent_data.data[event_data_idx] == EOF)
				{
					set_parser_event(PSTATE_EOF, PEVENT_MULTI_LINE_COMMENT);
					return &pevent_data;
				}

				pre_ch = pevent_data.data[event_data_idx];
				ch = fgetc(fs);
				fseek(fs, -1, SEEK_CUR);

				if (pre_ch == '*' && ch == '/')
				{
					fseek(fs, 1, SEEK_CUR);
					pevent_data.data[++event_data_idx] = '/';
					event_data_idx++;
					set_parser_event(PSTATE_DEFINES, PEVENT_MULTI_LINE_COMMENT);
					return &pevent_data;
				}

				event_data_idx++;
			}

		}
		else
		{
			event_data_idx = 0;
			pevent_data.data[event_data_idx++] = '/';
			fseek(fs, cur + 1, SEEK_SET);
			set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		}
	}

	/* Pre-processor directive */
	if (ch == '#')
	{
#ifdef DEBUG
		puts("d_pre_pro");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = '#';
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}

	/* Reserved keywords */
	if ((ch >= 'a' && ch <= 'z') || ch >= 'A' && ch <= 'Z')
	{
#ifdef DEBUG
		puts("d_res_key");
#endif

		word[word_idx++] = ch;

		while ((word[word_idx] = fgetc(fs)) != EOF)
		{

			if(word[word_idx] == '\n' || word[word_idx] == ' ')
				break;

			for (i = 0; operators[i] != '\0'; i++)
			{
				if (word[word_idx] == operators[i])
				{
					break;
				}
			}

			if (word[word_idx] == operators[i])
				break;

			for (i = 0; symbols[i] != '\0'; i++)
			{
				if (word[word_idx] == symbols[i])
				{
					break;
				}
			}

			if (word[word_idx] == symbols[i])
				break;

			word_idx++;

		}
		word[word_idx] = '\0';

		for (i = 0; strcmp(res_kwords_data[i], "") != 0; i++) 
		{
			if (strcmp(word, res_kwords_data[i]) == 0)
			{
				for (j = 0; (pevent_data.data[event_data_idx++] = word[j]) != '\0'; j++);

				word_idx = 0;
				fseek(fs, -1, SEEK_CUR);
				set_parser_event(PSTATE_DEFINES, PEVENT_RESERVE_KEYWORD_DATA);
				return &pevent_data;
			}
		}

		for (i = 0; strcmp(res_kwords_non_data[i], "") != 0; i++) 
		{
			if (strcmp(word, res_kwords_non_data[i]) == 0)
			{
				for (j = 0; (pevent_data.data[event_data_idx++] = word[j]) != '\0'; j++);

				word_idx = 0;
				fseek(fs, -1, SEEK_CUR);
				set_parser_event(PSTATE_DEFINES, PEVENT_RESERVE_KEYWORD_NON_DATA);
				return &pevent_data;
			}
		}

#ifdef DEBUG
		puts("d_code");
#endif
		for (j = 0; (pevent_data.data[event_data_idx++] = word[j]) != '\0'; j++);

		word_idx = 0;
		fseek(fs, -1, SEEK_CUR);
		set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}

	/* Numerical constants */
	if (ch >= '0' && ch <= '9')
	{
#ifdef DEBUG
		puts("d_num_const");
#endif
		pevent_data.data[event_data_idx++] = ch;

		while (isdigit(pevent_data.data[event_data_idx] = fgetc(fs)))
		{
			if (pevent_data.data[event_data_idx] == EOF)
				break;
			event_data_idx++;
		}

		if (pevent_data.data[event_data_idx] == EOF)
		{
			set_parser_event(PSTATE_EOF, PEVENT_NUMERIC_CONSTANT);
			return &pevent_data;
		}
		else if (pevent_data.data[event_data_idx] == ' ' || pevent_data.data[event_data_idx] == '\n') 
		{
			fseek(fs, -1, SEEK_CUR);
			set_parser_event(PSTATE_DEFINES, PEVENT_NUMERIC_CONSTANT);
			return &pevent_data;
		}
		else
		{
			for (i = 0; suffix[i] != '\0'; i++)
			{
				if (pevent_data.data[event_data_idx] == suffix[i])
				{
					event_data_idx++;
					if ((ch = fgetc(fs)) != ' ' && ch != EOF && ch != '\n')
					{
						fseek(fs, cur, SEEK_SET);
						set_parser_event(PSTATE_DEFINES, PEVENT_NULL); // to be directed to code 
						return &pevent_data;
					}
					fseek(fs, -1, SEEK_CUR);
					set_parser_event(PSTATE_DEFINES, PEVENT_NUMERIC_CONSTANT);
					return &pevent_data;
				}
			}

			for (i = 0; operators[i] != '\0'; i++)
			{
				if (pevent_data.data[event_data_idx] == operators[i])
				{
					fseek(fs, -1, SEEK_CUR);
					set_parser_event(PSTATE_DEFINES, PEVENT_NUMERIC_CONSTANT);
					return &pevent_data;
				}
			}

			for (i = 0; symbols[i] != '\0'; i++)
			{
				if (pevent_data.data[event_data_idx] == symbols[i])
				{
					fseek(fs, -1, SEEK_CUR);
					set_parser_event(PSTATE_DEFINES, PEVENT_NUMERIC_CONSTANT);
					return &pevent_data;
				}
			}
#ifdef DEBUG
			puts("d_code");
#endif
			event_data_idx++;

			while ((pevent_data.data[event_data_idx] = fgetc(fs)) != EOF)
			{
				if(pevent_data.data[event_data_idx] == '\n' || pevent_data.data[event_data_idx] == ' ') 
					break;

				for (i = 0; operators[i] != '\0'; i++)
				{
					if (pevent_data.data[event_data_idx] == operators[i])
					{
						break;
					}
				}

				for (i = 0; symbols[i] != '\0'; i++)
				{
					if (pevent_data.data[event_data_idx] == symbols[i])
					{
						break;
					}
				}
				event_data_idx++;
			}

			fseek(fs, -1, SEEK_CUR);
			set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
			return &pevent_data;
		}
	}

	/* ASCII character */
	if (ch == 39)
	{
#ifdef DEBUG
		puts("d_ascii");
#endif
		pevent_data.data[event_data_idx++] = (pre_ch = ch); 		
		ch = pevent_data.data[event_data_idx++] = fgetc(fs);

		if (ch == '\\')
		{
			if ((ch = (pevent_data.data[event_data_idx++] = fgetc(fs))) == '0' || ch == 'a' || ch == 'b' || ch == 't' || ch == 'n' || ch == 'v' || ch == 'f' || ch == 'r' || ch == '\"' || ch == '\'' || ch == '\\')
			{
				ch = (pevent_data.data[event_data_idx++] = fgetc(fs));
				if ( pre_ch == ch)
				{
					set_parser_event(PSTATE_DEFINES, PEVENT_REGULAR_EXP);
					return &pevent_data;
				}
				else
				{
					fseek(fs, cur + 1, SEEK_SET);
					event_data_idx = 0;
					pevent_data.data[event_data_idx++] = '\'';
					set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
					return &pevent_data;
				}
			}
			else
			{
				fseek(fs, cur + 1, SEEK_SET);
				event_data_idx = 0;
				pevent_data.data[event_data_idx++] = '\'';
				set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
				return &pevent_data;
			}

		}
		ch = (pevent_data.data[event_data_idx++] = fgetc(fs));

		if (ch == pre_ch)
		{
			set_parser_event(PSTATE_DEFINES, PEVENT_STRING);
			return &pevent_data;
		}
#ifdef DEBUG
		puts("d_code");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = pre_ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}

	/* EOF */
	if (ch == EOF)
	{
#ifdef DEBUG
		puts("d_eof");
#endif
		set_parser_event(PSTATE_EOF, PEVENT_NULL);
		return &pevent_data;
	}

	/* Operators */
	if (is_operator(ch))
	{
#ifdef DEBUG
		puts("d_op");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}

	/* Symbols */
	if (is_symbol(ch))
	{
#ifdef DEBUG
		puts("d_sym");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}

	/* Space */
	if (ch == ' ')
	{
#ifdef DEBUG
		puts("d_space");
#endif
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_DEFINES, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}

	/* Space */
	if (ch == '\n')
	{
#ifdef DEBUG
		puts("d_new_line");
#endif
		event_data_idx = 0;
		event_data_idx = 0;
		pevent_data.data[event_data_idx++] = ch;
		fseek(fs, cur + 1, SEEK_SET);
		set_parser_event(PSTATE_IDLE, PEVENT_PREPROCESSOR_DIRECTIVE);
		return &pevent_data;
	}
}
pevent_t *pstate_code_handler(FILE *fs)
{
	int i; 

	cur = (unsigned int) ftell(fs);
	ch = fgetc(fs);

	switch(ch)
	{
		case '#': 
			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_IDLE, PEVENT_CODE);
			return &pevent_data;
			break;
		case '\\': 
			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_IDLE, PEVENT_CODE);
			return &pevent_data;
			break;
		case '\'': 
			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_IDLE, PEVENT_CODE);
			return &pevent_data;
			break;
		case '/': 
			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_IDLE, PEVENT_CODE);
			return &pevent_data;
			break;
		case '\"': 
			fseek(fs, -1, SEEK_CUR);
			set_parser_event(PSTATE_STRING, PEVENT_NULL);
			return &pevent_data;
			break;
		default : 
			pevent_data.data[event_data_idx++] = ch;

			while ((word[word_idx] = fgetc(fs)) != EOF)
			{
				if(word[word_idx] == '\n' || word[word_idx] == ' ') 
					break;

				for (i = 0; operators[i] != '\0'; i++)
				{
					if (word[word_idx] == operators[i])
					{
						break;
					}
				}
				if (word[word_idx] == operators[i])
					break;

				for (i = 0; symbols[i] != '\0'; i++)
				{
					if (word[word_idx] == symbols[i])
					{
						break;
					}
				}
				if (word[word_idx] == symbols[i])
					break;

				word_idx++;
			}

			word[word_idx] = '\0';

			for (i = 0; (pevent_data.data[event_data_idx++] = word[i]) != '\0'; i++);

			word_idx = 0;
			fseek(fs, -1, SEEK_CUR);
			set_parser_event(PSTATE_IDLE, PEVENT_CODE);
			return &pevent_data;
			break;
	}
}
/**** End of file ****/
