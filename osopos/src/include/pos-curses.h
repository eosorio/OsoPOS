/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 pos-func.h 0.23-1 Biblioteca de funciones de OsoPOS.
        Copyright (C) 1999,2000,2004 Eduardo Israel Osorio Hernández

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
#include <form.h>
#define _form

#include <panel.h>
#define _panel

#define normal 1
#define verde_sobre_negro 2
#define amarillo_sobre_azul 3

char scan_terminator='\n';

int ErrorArchivo(char *nmarch);
/* Reporta errores en apertura de archivo y termina el programa */

int wgetkeystrokes(WINDOW *, char *, int);
/* Captura una secuencia de teclas o teclas de función */

FIELD *CreaEtiqueta(int frow, int fcol, NCURSES_CONST char *label);
FIELD *CreaCampo(int frow, int fcol, int ren, int cols, int colores);
int my_form_driver(FORM *form, int c);
char *obten_login();
char *obten_passwd(char *usuario);
void mensaje_v(char *texto, int colores_txt, int t);  /* Abre una ventana con mensaje */

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

  /* Si wait_flag == 0, algun otro dispositivo se esta comunicando */
  int ch = 0;
  short finished = 0;
  int i;
#ifdef _SERIAL_COMM
  extern int wait_flag;
#else
  int wait_flag = 1;
#endif

  //  noraw();
  cbreak();
  noecho();
  //  scrollok(w, TRUE);
  idlok(w, TRUE);
  keypad(w, TRUE);

  for (i=0; finished==0 && i<str_len && wait_flag; i++) {
    ch = wgetch(w);
    if (!wait_flag)
      ungetch(ch);
    if (ch == ERR) {
    /* Probablemente se produjo una interrupción del signal handler */
      i--;
      continue;
    }
    if (ch>KEY_F0 && ch<KEY_F(13)) {
      echo();
      raw();
      return(ch - KEY_F0);
    }

    if ((ch >= '0' && ch <= '9') || (ch>='A' && ch<='z') || ch=='-') {
      input_str[i+1] = 0;
      input_str[i] = ch;
      wprintw(w, "%c", ch);
    }
    //        case '\n':
    else if (ch == scan_terminator) {
      finished = 1;
      echo();
      raw();
      return(0);
      break;
    }
    else
      switch(ch) {
        case 127:
        case 263: /* Backspace */
        case '\b':
          if (i) {
            input_str[--i] = 0;
            wmove(w, w->_cury, w->_curx-1);
            wclrtoeol(w);
          }
          i--;
          break;
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
        case '/':
        case '.':
          input_str[i+1] = 0;
          input_str[i] = ch;
          wprintw(w, "%c", ch);
          break;
        default:
          wprintw(w, "%c", 9);
      }
  }  /* for */
  echo();
  return(0);
}

FIELD *CreaEtiqueta(int pren, int pcol, NCURSES_CONST char *etiqueta)
{
    FIELD *f = new_field(1, strlen(etiqueta), pren, pcol, 0, 0);

    if (f)
    {
        set_field_buffer(f, 0, etiqueta);
        set_field_opts(f, field_opts(f) & ~O_ACTIVE);
    }
    return(f);
}

FIELD *CreaCampo(int frow, int fcol, int ren, int cols, int colores)
{
    FIELD *f = new_field(ren, cols, frow, fcol, 0, 0);

    if (f)
        set_field_back(f,COLOR_PAIR(colores) | A_BOLD);
    return(f);
}


int my_form_driver(FORM *form, int c)
{
    if (c == (MAX_FORM_COMMAND + 1)
                && form_driver(form, REQ_VALIDATION) == E_OK)
        return(TRUE);
    else
    {
        beep();
        return(FALSE);
    }
}


void MuestraForma(FORM *f, unsigned pos_ren, unsigned pos_col)
{
    WINDOW      *w;
    int ren, col;

    scale_form(f, &ren, &col);

    if ((w =newwin(ren+2, col+2, pos_ren, pos_col)) != (WINDOW *)0)
    {
        set_form_win(f, w);
        set_form_sub(f, derwin(w, ren, col, 1, 1));
        box(w, 0, 0);
        keypad(w, TRUE);
    }

    if (post_form(f) != E_OK)
        wrefresh(w);
}

void BorraForma(FORM *f)
{
    WINDOW      *w = form_win(f);
    WINDOW      *s = form_sub(f);

    unpost_form(f);
    werase(w);
    wrefresh(w);
    delwin(s);
    delwin(w);
}

/*+-------------------------------------------------------------------------
	mkpanel(rows,cols,tly,tlx) - alloc a win and panel and associate them
--------------------------------------------------------------------------*/
static PANEL *
mkpanel(int color, int rows, int cols, int tly, int tlx)
{
    WINDOW *win;
    PANEL *pan = 0;

    if ((win = newwin(rows, cols, tly, tlx)) != 0) {
	if ((pan = new_panel(win)) == 0) {
	    delwin(win);
	} else if (has_colors()) {
	    int fg = (color == COLOR_BLUE) ? COLOR_WHITE : COLOR_BLACK;
	    int bg = color;
	    init_pair(color, fg, bg);
	    wbkgdset(win, COLOR_PAIR(color) | ' ');
	} else {
	    wbkgdset(win, A_BOLD | ' ');
	}
    }
    return pan;
}				/* end of mkpanel */

/*+-------------------------------------------------------------------------
	rmpanel(pan)
--------------------------------------------------------------------------*/
static void
rmpanel(PANEL * pan)
{
    WINDOW *win = panel_window(pan);
    del_panel(pan);
    delwin(win);
}				/* end of rmpanel */


char *obten_login()
{
  char *u;

  u = (char *)calloc(1,mxbuff);

  mvprintw(getmaxy(stdscr)-3,0, "Usuario: ");
  clrtoeol();
  noecho();
  getnstr(u, mxbuff-1);
  echo();
  return(u);
}

char *obten_passwd(char *login)
{
  char *p;

  p = (char *)calloc(1,mxbuff);

  mvprintw(getmaxy(stdscr)-3,0, "Contraseña: ");
  clrtoeol();
  noecho();
  getnstr(p, mxbuff-1);
  echo();
  move(getmaxy(stdscr)-3,0);
  clrtoeol();
  return(p);
}

void mensaje_v(char *texto, int txt_colores, int tecla)
{
  WINDOW *v_mens;
  PANEL  *pan = 0;
  attr_t v_attr;
  short  v_colores;

  int vx = 70;
  int vy = 5; /* Dimensiones horizontal y vertical de la ventana */
  int t=0;

  pan = mkpanel(COLOR_BLACK, vy, vx, (getmaxy(stdscr)-vy)/2, (getmaxx(stdscr)-vx)/2);
//  set_panel_userptr(pan, "pan");  <--Invalid. The second argument has to be a pointer to another panel

  v_mens = panel_window(pan);

  wattr_get(v_mens, &v_attr, &v_colores, NULL);
  wattrset(v_mens, COLOR_PAIR(txt_colores));
  mvwprintw(v_mens, 2,1, texto);
  wattrset(v_mens, v_attr);
  wclrtoeol(v_mens);
  box(v_mens, 0, 0);
  show_panel(pan);

  while (tecla!=t)
    t = wgetch(v_mens);

  wrefresh(v_mens);
  rmpanel(pan);
}

