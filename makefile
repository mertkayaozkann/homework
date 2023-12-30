CC = gcc
CFLAGS = -Wall -Werror

homework: homework.c
    $(CC) $(CFLAGS) -o homework homework.c

clean:
    rm -f homework