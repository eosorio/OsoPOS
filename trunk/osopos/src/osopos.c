/* -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*- 

  OsoPOS - Sistema de punto de venta, inventarios y facturación
  Copyright (C) 1999,2000 Eduardo Israel Osorio Hernández
  israel@punto-deventa.com
  Tel +52(3)328-2928

        Este programa es un software libre; puede usted redistribuirlo y/o
modificarlo de acuerdo con los términos de la Licencia Pública General GNU
publicada por la Free Software Foundation: ya sea en la versión 2 de la
Licencia, o (a su elección) en una versión posterior.

        Este programa es distribuido con la esperanza de que sea útil, pero
SIN GARANTIA ALGUNA; incluso sin la garantía implícita de COMERCIABILIDAD o
DE ADECUACION A UN PROPOSITO PARTICULAR. Véase la Licencia Pública General
GNU para mayores detalles.

        Debería usted haber recibido una copia de la Licencia Pública General
GNU junto con este programa; de no ser así, escriba a Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA02139, USA.
 
*/

/* front-end v. 0.0.2 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <string.h>

void despliega(short unsigned item);
int teclazo(char tecla);


void despliega(short unsigned item)
{
  mvprintw( 4,3,"1. Operación de caja (venta)");
  mvprintw( 5,3,"2. Facturación");
  mvprintw( 6,3,"3. Cortes de caja");
  mvprintw( 7,3,"4. Control de inventarios");
  mvprintw( 8,3,"5. Lista de precios");
  mvprintw(10,3,"8. Ver licencia GPL del programa");
  mvprintw(12,3,"0. Salir");
  mvprintw(14,1,"Opción: ");
}

int ejecuta(char tecla)
{
  unsigned opcion;
  int      salida;
  FILE     *ar_temp;

  opcion = atoi(&tecla);

  switch (opcion) {
    case 1 : salida = system("venta");
             break;
    case 2 : salida = system("facturar");
             break;
    case 3 : salida = system("corte");
             break;
    case 4 : salida = system("inventario");
             break;
    case 5 : ar_temp = fopen("/home/OsoPOS/precios.txt", "r");
             if (ar_temp != NULL) {
               fclose(ar_temp);
               salida = system("less -r -f /home/OsoPOS/precios.txt");
             }
             else
               salida = 0;
             break;
    case 8 : salida = system("less -r -f /home/OsoPOS/LICENCIA");
             break;
    default: beep();
             salida = 0;
  }
  return(salida);
}


int main() {
  char teclazo;

  initscr();
  do {
    clear();
    teclazo = strlen("OsoPOS, sistema de punto de venta");
    mvprintw(0,(COLS-teclazo)/2, "OsoPOS, sistema de punto de venta");
    teclazo = strlen("(C) 1999,2000 E. Israel Osorio H.  linucs@punto-deventa.com");
    mvprintw(1,(COLS-teclazo)/2, "(C) 1999,2000 E. Israel Osorio H.  linucs@punto-deventa.com");
    mvprintw(3,1,"Indique la operación a realizar:");
    despliega(0);
    raw();
    teclazo = getch();
    if (teclazo != '0')
      ejecuta(teclazo);
  }
  while (teclazo !='0');
  endwin();
  return(0);
}
