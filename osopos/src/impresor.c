/* Para generar el binario ejecutable usar				
   gcc -O ~/fuente/impresor.c -lcurses -o ~/bin/imprime -lpq */

/* OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa Impresor 0.0.1-1 1999 E. Israel Osorio H., linucs@starmedia.com
   con licencia GPL.
   Lea el archivo README que contiene los términos de la licencia */

#include <stdio.h>
#include "include/pos-curses.h"
#include <time.h>

#define blanco_sobre_negro 1
#define amarillo_sobre_negro 2
#define verde_sobre_negro 3
#define azul_sobre_blanco 4
#define cyan_sobre_negro 5


char *nm_disp_ticket   = "/dev/lp0";
char *nm_disp_facturas = "/dev/lp0";
char *nm_disp_notas    = "/dev/lp0";


int main()
{
  char opcion;

  initscr();
  if (!has_colors()) {
    mvprintw(LINES/2,0,
      "Este equipo no puede producir colores, pulse una tecla para abortar...");
    getch();
    printw("Abortando");
    endwin();
    exit(10);
  }

  start_color();
  init_pair(blanco_sobre_negro, COLOR_WHITE, COLOR_BLACK);
  init_pair(amarillo_sobre_negro, COLOR_YELLOW, COLOR_BLACK);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(azul_sobre_blanco, COLOR_BLUE, COLOR_WHITE);
  init_pair(cyan_sobre_negro, COLOR_CYAN, COLOR_BLACK);

  do {
    clear();
    printw("Seleccione el documento a imprimir:");
    mvprintw(2,2, "Corte de caja");
    mvprintw(3,2, "Factura");
    mvprintw(4,2, "Nota de mostrador");

    attrset(COLOR_PAIR(azul_sobre_blanco));
    mvprintw(2,2, "C");
    mvprintw(3,2, "F");
    mvprintw(4,2, "N");
    attrset(COLOR_PAIR(blanco_sobre_negro));
    move(7,2);

    opcion = toupper(getch());
    switch (opcion) {
      case 'C':
        opcion = imprime_doc("/home/OsoPOS/log/tmp/impresion.ticket",
                             nm_disp_ticket);
        if (opcion == ERROR_ARCHIVO_1)
          ErrorArchivo("/home/OsoPOS/log/tmp/impresion.ticket");
        else if (opcion == ERROR_ARCHIVO_2)
          ErrorArchivo(nm_disp_ticket);
        else
          opcion = 'C';
      break;
      case 'F':
        opcion = imprime_doc("/home/OsoPOS/log/tmp/impresion.factura",
                      nm_disp_facturas);
        if (opcion == ERROR_ARCHIVO_1)
          ErrorArchivo("/home/OsoPOS/log/tmp/impresion.factura");
        else if (opcion == ERROR_ARCHIVO_2)
          ErrorArchivo(nm_disp_facturas);
        else
          opcion = 'F';
      break;
      case 'N':
        opcion = imprime_doc("/home/OsoPOS/log/tmp/impresion.nota",
                      nm_disp_notas);
        if (opcion == ERROR_ARCHIVO_1)
          ErrorArchivo("/home/OsoPOS/log/tmp/impresion.nota");
        else if (opcion == ERROR_ARCHIVO_2)
          ErrorArchivo(nm_disp_notas);
        else
          opcion = 'F';
      break;
      default: fprintf(stderr, "\a");
    }
  }
  while (opcion!='C' && opcion!='F' && opcion!='N');
  endwin();
  return(OK);
}
