all: invent remision carrito factur corte imprrem osopos
install: install_invent install_remision install_factur install_corte install_imprrem install_osopos

CC = gcc -g -Wall 

PGLIBS = -lpq

CRYPTLIBS = -lssl -lcrypto

CURSESLIBS = -lncurses

PANELLIBS = -lpanel

GTKLIBS = `gtk-config --cflags` `gtk-config --libs`

FORMSLIBS = -lform

GLIBLIBS = `pkg-config --cflags --libs glib-2.0`

C_REMISION = -Dmxmembarra=8000 -Dmxmemdivisa=5 -Dmaxitem_busqueda=100 -D_SERIAL_COMM

C_OSOPOS = -Dmaxspc=70 -Dmaxart=30 -Dmaxarts=6000 -Dmaxdepto=500

C_IMPRREM = -Dmxmemdivisa=5

HEADER_FACTUR = include/factur.h
HEADER_REMISION = include/minegocio-remis.h

IMPRESIONES_USUARIO = include/minegocio.h

DIR_BIN = /usr/bin

BUILD_BINDIR = ../bin

remision: remision.c
	${CC} -o ${BUILD_BINDIR}/remision.o remision.c ${PGLIBS} ${FORMSLIBS} \
	${PANELLIBS} ${CURSESLIBS} ${GLIBLIBS} ${CRYPTLIBS} \
	${C_REMISION} ${C_OSOPOS} -ggdb

carrito: carrito.c
	${CC} -o ${BUILD_BINDIR}/carrito.o carrito.c ${PGLIBS} ${FORMSLIBS} \
	${PANELLIBS} ${CURSESLIBS} ${GLIBLIBS} ${CRYPTLIBS} \
	${C_REMISION} ${C_OSOPOS} -ggdb

factur: factur.c
	${CC} -o ${BUILD_BINDIR}/factur.o factur.c ${PGLIBS} ${FORMSLIBS} ${PANELLIBS} \
	${CURSESLIBS} -D_MOD_FACTUR -ggdb

invent: invent.c
	${CC} -o ${BUILD_BINDIR}/invent.o invent.c ${PGLIBS} ${FORMSLIBS} ${CURSESLIBS} \
	${C_OSOPOS} -ggdb

movinvent: mov_invent.c
	${CC} -o ${BUILD_BINDIR}/movinvent.o mov_invent.c ${PGLIBS} ${FORMSLIBS} ${CURSESLIBS} \
	${C_OSOPOS} -ggdb

corte: corte.c
	${CC} -o ${BUILD_BINDIR}/corte.o corte.c ${PGLIBS} ${FORMSLIBS} ${CURSESLIBS} \
	${CRYPTLIBS} ${PANELLIBS} ${GLIBLIBS} -ggdb

osopos: osopos.c
	${CC} -o ${BUILD_BINDIR}/osopos.o osopos.c ${CURSESLIBS}

imprrem: imprrem.c
	${CC} -o ${BUILD_BINDIR}/imprrem.o imprrem.c ${CURSESLIBS} ${PGLIBS} ${C_IMPRREM} -ggdb

gtk_invent: gtk_invent.c
	${CC} -o ${BUILD_BINDIR}/gtk_invent gtk_invent.c ${GTKLIBS} \
        ${PGLIBS} ${C_OSOPOS}

install_remision:
	cp ${BUILD_BINDIR}/remision.o $(DIR_BIN)/remision
	chmod u+s ${DIR_BIN}/remision

install_factur:
	cp ${BUILD_BINDIR}/factur.o $(DIR_BIN)/factur
	chmod u+s ${DIR_BIN}/factur

install_osopos:
	cp ${BUILD_BINDIR}/osopos.o $(DIR_BIN)/osopos

install_imprrem:
	cp ${BUILD_BINDIR}/imprrem.o $(DIR_BIN)/imprrem

install_invent:
	cp ${BUILD_BINDIR}/invent.o ${DIR_BIN}/invent

install_corte:
	cp ${BUILD_BINDIR}/corte.o ${DIR_BIN}/corte

clean:
	rm -f ${BUILD_BINDIR}/*.o
