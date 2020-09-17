CFLAGS+=-Wall -Werror -std=gnu99 -O2 -DJSMN_PARENT_LINKS
LDFLAGS+=`curl-config --libs`

TARGETS=json_parser_full json_parser_with_keys
OBJS_COMMON=buf.o log.o json.o jsmn.o
OBJS_FULL=json_parser_full.o
OBJS_KEYS=json_parser_with_keys.o

all: $(TARGETS)
	$(RM) $(OBJS_COMMON)

clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS_COMMON) $(OBJS_FULL) $(OBJS_KEYS)

json_parser_full: $(OBJS_COMMON) $(OBJS_FULL)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(RM) $(OBJS_FULL)

json_parser_with_keys: $(OBJS_COMMON) $(OBJS_KEYS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(RM) $(OBJS_KEYS)
