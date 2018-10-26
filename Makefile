CFLAGS=-Wall -std=c99 -g
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

mdcc: $(OBJS)
	cc -o $@ $(OBJS) $(LDFLAGS)

test: mdcc
	./test.sh


$(OBJS): mdcc.h

clean:
	rm -f mdcc *.o a.out tmp*

.PHONY: clean test
