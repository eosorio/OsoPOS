all: invent remision factur corte impresor imprrem osopos
install: install_invent install_remision install_factur install_corte install_impresor install_imprrem install_osopos

CC = gcc -g -Wall -march=i486

PGLIBS = -lpq -lcrypt

CURSESLIBS = -lncurses

PANELLIBS = -lpanel

GTKLIBS = `gtk-config --cflags` `gtk-config --libs`

FORMSLIBS = -lform

C_REMISION = -Dmxmembarra=3000 -Dmxmemdivisa=5

C_OSOPOS = -Dmaxspc=70 -Dmaxart=30 -Dmaxarts=3000 -Dmaxdepto=100

HEADER_FACTUR = include/factur.h
HEADER_REMISION = include/labotana-remis.h

IMPRESIONES_USUARIO = include/labotana.h

DIR_BIN = /usr/bin

SUPERVISOR_BINDIR = /home/OsoPOS/scaja/bin

remision: remision.c
	${CC} -o ../bin/remision.o remision.c ${PGLIBS} ${FORMSLIBS} \
    ${CURSESLIBS} ${C_REMISION} ${C_OSOPOS} -ggdb

factur: factur.c
	${CC} -o ../bin/factur.o factur.c ${PGLIBS} ${FORMSLIBS} ${PANELLIBS} \
	${CURSESLIBS} -D_MOD_FACTUR -ggdb

invent: invent.c
	${CC} -o ../bin/invent.o invent.c ${PGLIBS} ${FORMSLIBS} ${CURSESLIBS} \
	${C_OSOPOS} -ggdb

movinvent: mov_invent.c
	${CC} -o ../bin/movinvent.o mov_invent.c ${PGLIBS} ${FORMSLIBS} ${CURSESLIBS} \
	${C_OSOPOS} -ggdb

corte: corte.c
	${CC} -o ../bin/corte.o corte.c ${PGLIBS} ${FORMSLIBS} ${CURSESLIBS} \
	-ggdb

impresor: impresor.c
	${CC} -o ../bin/impresor.o impresor.c ${CURSESLIBS} ${PGLIBS}

osopos: osopos.c
	${CC} -o ../bin/osopos.o osopos.c ${CURSESLIBS}

imprrem: imprrem.c
	${CC} -o ../bin/imprrem.o imprrem.c ${CURSESLIBS} ${PGLIBS}

gtk_invent: gtk_invent.c
	${CC} -o ${SUPERVISOR_BINDIR}/gtk_invent gtk_invent.c ${GTKLIBS} \
        ${PGLIBS} ${C_OSOPOS}

install_remision:
	cp ../bin/remision.o $(DIR_BIN)/remision
	chmod u+s ${DIR_BIN}/remision

install_factur:
	cp ../bin/factur.o $(DIR_BIN)/factur
	chmod u+s ${DIR_BIN}/factur

install_osopos:
	cp ../bin/osopos.o $(DIR_BIN)/osopos

install_imprrem:
	cp ../bin/imprrem.o $(DIR_BIN)/imprrem

install_invent:
	cp ../bin/invent.o ${DIR_BIN}/invent

install_corte:
	cp ../bin/corte.o ${DIR_BIN}/corte

install_impresor:
	cp ../bin/impresor.o $(DIR_BIN)/impresor

clean:
	rm -f ../bin/*.o
