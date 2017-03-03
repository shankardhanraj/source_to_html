FLAGS := s2h_conv.h s2h_event.h
SRCS := s2h_conv.c s2h_event.c s2h_main.c
TRGT := source_to_html

${TRGT} : ${SRCS}
	gcc $^ -o $@

clean :
	rm -f ${TRGT}

