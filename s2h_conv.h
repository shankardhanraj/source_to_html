#include <stdio.h>
#include "s2h_event.h"

#ifndef S2C_CONV_H
#define S2C_CONV_H

#define HTML_OPEN	1
#define HTML_CLOSE	0

/* start_or_end_conv function definitation */
void html_begin(FILE* dest_fp, int type); /* type => not used, but can be used to add differnet HTML tags */
void html_end(FILE* dest_fp, int type); /* type => not used, but can be used to add differnet HTML tags */

/* sourc_to_html function definitation */
void source_to_html(FILE* fp, pevent_t *event);

/* get parser event */
pevent_t *get_parser_event(FILE *fs);

#endif
