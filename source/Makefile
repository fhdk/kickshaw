SOURCES = adding_and_deleting.c auxiliary.c check_for_external_changes_timeout.c context_menu.c \
          drag_and_drop.c editing.c find.c kickshaw.c load_menu.c save_menu.c selecting.c
OBJS    = ${SOURCES:.c=.o}
CFLAGS  = -O2 -pedantic -std=gnu99 -Wall -Wextra -Wmissing-prototypes `pkg-config gtk+-3.0 --cflags`
LDADD   = `pkg-config gtk+-3.0 --libs`
CC      = gcc
PACKAGE = kickshaw
PREFIX 	= /usr/local

all: ${OBJS}
	${CC} -o ${PACKAGE} ${OBJS} ${LDADD}

.c.o:
	${CC} ${CFLAGS} -c $<

clean:
	rm -f *.o *.bak *~ kickshaw

install:
	cp kickshaw $(PREFIX)/bin

uninstall:
	rm $(PREFIX)/bin/kickshaw
	@echo "For every user who has started kickshaw a settings file .kickshawrc was created inside the respective home directory. It is not removed automatically, but can be removed by hand."
