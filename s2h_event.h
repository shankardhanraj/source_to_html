#ifndef S2C_EVENT_C
#define S2C_EVENT_C

#define PEVENT_DATA_SIZE	2048
#define WORD_BUFF_SIZE	500

typedef enum
{
    PEVENT_NULL,
    PEVENT_PREPROCESSOR_DIRECTIVE,
    PEVENT_RESERVE_KEYWORD_DATA,
    PEVENT_RESERVE_KEYWORD_NON_DATA,
    PEVENT_NUMERIC_CONSTANT,
    PEVENT_STRING,
    PEVENT_HEADER_FILE,
    PEVENT_REGULAR_EXP,
    PEVENT_SINGLE_LINE_COMMENT,
    PEVENT_MULTI_LINE_COMMENT,
    PEVENT_EOF,
    PEVENT_ASCII_CHAR,
    PEVENT_CODE,
}pevent_e;

/* event variable to store event and related properties */
typedef struct
{
    pevent_e type; // event type
    int length; // data length
    char data[PEVENT_DATA_SIZE]; // cwparsed string
}pevent_t;

#endif
