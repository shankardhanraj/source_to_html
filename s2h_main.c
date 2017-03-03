#include "s2h_event.h"
#include <stdlib.h>
#include "s2h_conv.h"

int main (int argc, char *argv[])
{
	FILE *fsp, *dfp; // source and destination file descriptors 
	char dest_file[100];
	char open_file_command[100];
	pevent_t *event;

	if(argc < 2)
	{
		printf("\nError ! please enter file name\n");
		printf("Usage: <executable> <file name> \n");
		printf("Example : ./a.out abc.txt\n\n");
		return 1;
	}

#ifdef DEBUG
	printf("File to be opened : %s\n", argv[1]);
#endif

	/* opnen the file */

	if(NULL == (fsp = fopen(argv[1], "r")))
	{
		printf("Error! File %s could not be opened\n", argv[1]);
		return 2;
	}
	/* Check for output file */
	if (argc > 2)
	{
		sprintf(dest_file, "%s.html", argv[2]);
	}
	else
	{
		sprintf(dest_file, "%s.html", argv[1]);
	}
	/* open dest file */
	if (NULL == (dfp = fopen(dest_file, "w")))
	{
		printf("Error! could not create %s output file\n", dest_file);
		return 3;
	}

	/* write HTML starting Tags */
	html_begin(dfp, HTML_OPEN);

	/* Read from src file convert into html and write to dest file */

	do
	{
		event = get_parser_event(fsp);

		/* call sourc_to_html */
        if (!(event->type == PEVENT_NULL || event->type == PEVENT_EOF))
		source_to_html(dfp, event);

//		printf("In main : Event = %d\n", event);
	} while (event->type != PEVENT_EOF);

	/* Call start_or_end_conv function for ending the convertation */
	html_end(dfp, HTML_CLOSE);

/* close file */
	fclose(fsp);
	fclose(dfp);

	printf("Output file %s generated\n", dest_file);
	sprintf(open_file_command, "firefox %s", dest_file);
	system(open_file_command);

	return 0;
}
