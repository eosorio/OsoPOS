#include <ncurses.h>

#include "pos-func.h"


int ErrorArchivo(char *nmarch);
/* Reporta errores en apertura de archivo y termina el programa */

/*********************************************************************/

int ErrorArchivo(char *nmarch) {

  clear();
  mvprintw(LINES/2,0,"ERROR: En la integridad del archivo %s",nmarch);
  mvprintw(LINES/2+1,0,"abortando, pulsa una tecla para terminar...");
  getch();
  endwin();
  exit(1);
}

void aborta(char *mensaje, int cod)
{

  mvprintw(LINES/2,0, "%s", mensaje);
  getch();
  endwin();
  exit(cod);
}
