/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 pos-func.h 0.22-1 Biblioteca de funciones de OsoPOS.
        Copyright (C) 1999,2000 Eduardo Israel Osorio Hernández

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

#include <ncurses.h>

#include "pos-func.h"


int ErrorArchivo(char *nmarch);
/* Reporta errores en apertura de archivo y termina el programa */

int wgetkeystrokes(WINDOW *, char *, int);
/* Captura una secuencia de teclas o teclas de función */

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

int wgetkeystrokes(WINDOW *w, char *input_str, int str_len) {
  int ch = 0;
  short finished = 0;
  int i;

  noraw();
  cbreak();
  noecho();
  //  scrollok(w, TRUE);
  idlok(w, TRUE);
  keypad(w, TRUE);

  for (i=0; finished==0 && i<str_len; i++) {
	ch = wgetch(w);
	if (ch>KEY_F0 && ch<KEY_F(13)) {
	  echo();
	  raw();
	  return(ch - KEY_F0);
	}
	if (ch >= 48 && ch<127) {
      input_str[i+1] = 0;
      input_str[i] = ch;
	  wprintw(w, "%c", ch);
	}
	else
	  switch(ch) {
      case 32: /* Espacio */
        if (!i) {
          i--;
          continue;
        }
      case 42: /* Asterisco */
      case 225: /* a acentuada */
      case 233: /* e acentuada */
      case 237: /* i acentuada */
      case 243: /* o acentuada */
      case 250: /* u acentuada */
      case 209: /* Ñ mayuscula */
      case 241: /* ñ minuscula */
        input_str[i+1] = 0;
        input_str[i] = ch;
        wprintw(w, "%c", ch);
        break;
      case '\n':
        finished = 1;
        echo();
        raw();
        return(0);
        break;
      case 127:
      case 263:
        input_str[--i] = 0;
        wmove(w, w->_cury, w->_curx-1);
        wclrtoeol(w);
        i--;
       break;
      default:
	  }
  }  /* for */
  return(0);
}
