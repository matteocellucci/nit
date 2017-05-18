CC = gcc
CFLAGS = -Wall -Wextra
CDIR = src
CFILES = $(wildcard $(CDIR)/*.c)
OFILES = $(patsubst $(CDIR)/%, %, $(patsubst %.c, %.o, $(CFILES)))
MAIN = nit 
BINDIR = /usr/bin
RULES = /etc/udev/rules.d/99-nit.rules

all: $(MAIN)

$(MAIN): $(OFILES)
	$(CC) $(CFLAGS) -o $(MAIN) $(OFILES)

%.o: $(CDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install:
	cp $(MAIN) $(BINDIR)

uninstall:
	$(RM) $(BINDIR)/$(MAIN) $(RULES)

clean:
	$(RM) $(OFILES)

.PHONY: install uninstall clean
