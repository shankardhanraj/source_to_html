#include "s2h_event.h"
#include "s2h_conv.h"

/* start_or_end_conv function definitation */
void html_begin(FILE* dest_fp, int type) /* type => not used, but can be used to add differnet HTML tags */
{
	/* Add HTML begining tags into destination file */
	fprintf(dest_fp, "<!DOCTYPE html>\n");
	fprintf(dest_fp, "<html lang=\"en-US\">\n");
	fprintf(dest_fp, "<head>\n");
	fprintf(dest_fp, "<title>%s</title>\n", "source_to_html");
	fprintf(dest_fp, "<meta charset=\"UTF-8\">\n");
	fprintf(dest_fp, "<link rel=\"stylesheet\" href=\"styles.css\">\n");
	fprintf(dest_fp, "</head>\n");
	fprintf(dest_fp, "<body style=\"background-color:white;\">\n");
	fprintf(dest_fp, "<pre>\n");
}
void html_end(FILE* dest_fp, int type) /* type => not used, but can be used to add differnet HTML tags */
{
	/* Add HTML closing tags into destination file */
	fprintf(dest_fp, "</pre>\n");
	fprintf(dest_fp, "</body>\n");
	fprintf(dest_fp, "</html>\n");
}


/* sourc_to_html function definitation */
void source_to_html(FILE* fp, pevent_t *event)
{

#if DEBUG
	printf("%s", event->data);
#endif

	switch(event->type)
	{
		case PEVENT_REGULAR_EXP:
			fprintf(fp,"<span class=\"regular_exp\">%s</span>",event->data);
			break;
		case PEVENT_PREPROCESSOR_DIRECTIVE:
			fprintf(fp,"<span class=\"preprocess_dir\">%s</span>",event->data);
			break;
		case PEVENT_MULTI_LINE_COMMENT:
		case PEVENT_SINGLE_LINE_COMMENT:
			fprintf(fp,"<span class=\"comment\">%s</span>",event->data);
			break;
		case PEVENT_STRING:
			fprintf(fp,"<span class=\"string\">%s</span>",event->data);
			break;
		case PEVENT_HEADER_FILE:
			fprintf(fp,"<span class=\"header_file\">%s</span>",event->data);
			//printf(fp,"<span class=\"header_file\">&lt;%s&gt;</span>",event->data);
			break;
		case PEVENT_NUMERIC_CONSTANT:
			fprintf(fp,"<span class=\"numeric_constant\">%s</span>",event->data);
			break;
		case PEVENT_RESERVE_KEYWORD_DATA:
				fprintf(fp,"<span class=\"reserved_key1\">%s</span>",event->data);
			break;
		case PEVENT_RESERVE_KEYWORD_NON_DATA:
				fprintf(fp,"<span class=\"reserved_key2\">%s</span>",event->data);
			break;
		case PEVENT_ASCII_CHAR:
			fprintf(fp,"<span class=\"ascii_char\">%s</span>",event->data);
			break;
		case PEVENT_CODE:
			fprintf(fp,"<span class=\"code\">%s</span>",event->data);
			break;
	}
}
