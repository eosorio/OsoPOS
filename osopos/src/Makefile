all: invent remision factur corte impresor imprrem osopos

CC = gcc -g -Wall -m486

PGLIBS = -lpq -lcrypt

CURSESLIBS = -lcurses

GTKLIBS = `gtk-config --cflags` `gtk-config --libs`

FORMSLIBS = -lform

C_REMISION = -Dmxmembarra=1000

C_OSOPOS = -Dmaxspc=70 -Dmaxart=30 -Dmaxarts=1000 -Dmaxdepto=100

HEADER_FACTUR = include/factur.h
HEADER_REMISION = include/rchapa-remis.h

IMPRESIONES_USUARIO = include/linucs.h

DIR_BIN = /home/OsoPOS/bin

SUPERVISOR_BINDIR = /home/OsoPOS/scaja/bin

remision: remision.c
	${CC} -o ${DIR_BIN}/remision remision.c ${PGLIBS} ${CURSESLIBS} ${C_REMISION} \
	${C_OSOPOS}	-include ${HEADER_REMISION} -ggdb
	chmod u+s ${DIR_BIN}/remision

factur: factur.c
	${CC} -o $(DIR_BIN)/factur factur.c ${PGLIBS} ${CURSESLIBS} ${FORMSLIBS} \
	-D_MOD_FACTUR -ggdb
	chmod u+s ${DIR_BIN}/factur

invent: invent.c
	${CC} -o ${SUPERVISOR_BINDIR}/invent invent.c ${PGLIBS} ${CURSESLIBS} ${FORMSLIBS} \
	${C_OSOPOS} -ggdb

corte: corte.c
	${CC} -o ${SUPERVISOR_BINDIR}/corte corte.c ${PGLIBS} ${CURSESLIBS} ${FORMSLIBS}

impresor: impresor.c
	${CC} -o $(DIR_BIN)/imprime impresor.c ${CURSESLIBS} ${PGLIBS}

osopos: osopos.c
	${CC} -o $(DIR_BIN)/osopos osopos.c ${CURSESLIBS}

imprrem: imprrem.c
	${CC} -o $(DIR_BIN)/imprrem imprrem.c ${CURSESLIBS} ${PGLIBS}

gtk_invent: gtk_invent.c
	${CC} -o ${SUPERVISOR_BINDIR}/gtk_invent gtk_invent.c ${GTKLIBS} \
        ${PGLIBS} ${C_OSOPOS}
