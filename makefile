#CFLAGS = -O2 -Wall -ansi -pedantic -static --std=c18
CFLAGS = -O2 -Wall --std=c18 -pedantic -static
LDLIBS := -lm

abcplay: source/abcplay.c

%: source/%.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDLIBS)

clean:
	rm -f abcplay
