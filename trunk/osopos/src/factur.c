/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

 Facturación 1.16. Módulo de facturación de OsoPOS.

        Copyright (C) 1999-2008 Eduardo Israel Osorio Hernández

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


#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <termios.h>
#include <panel.h>
#include <time.h>
#include <values.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>

#include "include/factur-curses.h"
#include "include/print-func.h"

//#include "include/linucs.h"
#include "include/electroh.h"

#define vers "1.16"
/*
#ifdef maxspc
#undef maxspc
#endif
*/

#define mxchcant 50


#ifndef CTRL
#define CTRL(x)         ((x) & 0x1f)
#endif

#define QUIT            CTRL('Q')
#define ESCAPE          CTRL('[')
#define ENTER		10
#define BLANK           ' '        /* caracter de fondo */

#define normal 1
#define verde_sobre_negro 2
#define amarillo_sobre_azul 3


int   captura_articulos();
int   CaptObserv();
void  imprime_factura();

/* Funciones que usan form.h */
void  AjustaModoTerminal(void);
/* FIELD *CreaEtiqueta(int frow, int fcol, NCURSES_CONST char *label); */
//FIELD *CreaCampo(int pren, int pcol, int ren, int cols);
//void  MuestraForma(FORM *f, unsigned ren, unsigned col);
//void  BorraForma(FORM *f);
int   obs_form_virtualize(WINDOW *w);
int   form_virtualize(WINDOW *w, int readchar, int c);
//int   my_form_driver(FORM *form, int c);


int form_virtualize(WINDOW *w, int readchar, int c)
{
  int  mode = REQ_INS_MODE;

  if (readchar)
    c = wgetch(w);

  switch(c) {
    case QUIT:
    case ESCAPE:
        return(MAX_FORM_COMMAND + 1);

    case KEY_NEXT:
    case CTRL('I'):
    case CTRL('N'):
    case CTRL('M'):
    case KEY_ENTER:
    case ENTER:
        return(REQ_NEXT_FIELD);
    case KEY_PREVIOUS:
    case CTRL('P'):
        return(REQ_PREV_FIELD);

    case KEY_HOME:
        return(REQ_FIRST_FIELD);
    case KEY_END:
    case KEY_LL:
        return(REQ_LAST_FIELD);

    case CTRL('L'):
        return(REQ_LEFT_FIELD);
    case CTRL('R'):
        return(REQ_RIGHT_FIELD);
    case CTRL('U'):
        return(REQ_UP_FIELD);
    case CTRL('D'):
        return(REQ_DOWN_FIELD);

   case CTRL('W'):
        return(REQ_NEXT_WORD);
/*    case CTRL('B'):
        return(REQ_PREV_WORD);*/
    case CTRL('B'):
      return(c);
      break;
    case CTRL('S'):
        return(REQ_BEG_FIELD);
    case CTRL('E'):
        return(REQ_END_FIELD);

    case KEY_LEFT:
        return(REQ_LEFT_CHAR);
    case KEY_RIGHT:
        return(REQ_RIGHT_CHAR);
    case KEY_UP:
        return(REQ_UP_CHAR);
    case KEY_DOWN:
        return(REQ_DOWN_CHAR);

/*    case CTRL('M'):
        return(REQ_NEW_LINE);*/
    /* case CTRL('I'):
        return(REQ_INS_CHAR); */
    case CTRL('O'):
        return(REQ_INS_LINE);
    case CTRL('V'):
        return(REQ_DEL_CHAR);

    case CTRL('H'):
    case KEY_BACKSPACE:
        return(REQ_DEL_PREV);
    case CTRL('Y'):
        return(REQ_DEL_LINE);
    case CTRL('G'):
        return(REQ_DEL_WORD);

    case CTRL('C'):
        return(REQ_CLR_EOL);
    case CTRL('K'):
        return(REQ_CLR_EOF);
    case CTRL('X'):
        return(REQ_CLR_FIELD);
/*  case CTRL('A'):
        return(REQ_NEXT_CHOICE); */
    case CTRL('Z'):
        return(REQ_PREV_CHOICE);

    case 331: /* Insert en teclado para PC */
    case CTRL(']'):
        if (mode == REQ_INS_MODE)
            return(mode = REQ_OVL_MODE);
        else
            return(mode = REQ_INS_MODE);

    default:
        return(c);
    }
}

int obs_form_virtualize(WINDOW *w)
{
  int  mode = REQ_INS_MODE;
  int         c = wgetch(w);

  switch(c) {
    case KEY_DOWN:
      return(REQ_NEXT_FIELD);
    case KEY_UP:
      return(REQ_PREV_FIELD);
    default:
        return(form_virtualize(w, FALSE, c));
    }
}





/**********************************************************************/

void muestra_ayuda(int ren, int col) {
  mvaddstr(ren,col,
         "<Ctrl-Q> Continua ");
  addstr("<Ctrl-A> Agrega ");
  addstr("<Intro> Sig. campo ");
  addstr("<Ctrl-X> Borra campo");
}

int captura_articulos() {
  WINDOW *ventana;
  FORM   *forma;
  FIELD  *campo[11];
  char   *etiqueta;
  int    finished = 0, c;
  int    tam_ren, tam_col, pos_ren, pos_col;
  int    i = 0;
  char buff[30];

  pos_ren = 5;
  pos_col = 0;

  etiqueta = "Introduzca los articulos";

  /* describe la forma */
  campo[0] = CreaEtiqueta(1, 0, "Cant");
  campo[1] = CreaCampo(2, 0, 1, maxexistlong, amarillo_sobre_azul);
  campo[2] = CreaEtiqueta(1, maxexistlong+1, "Descripcion:");
  campo[3] = CreaCampo(2, maxexistlong+1, 1, maxdes, amarillo_sobre_azul);
  campo[4] = CreaEtiqueta(1, maxexistlong+maxdes+2, "P.U.:");
  campo[5] = CreaCampo(2, maxexistlong+maxdes+2, 1, maxpreciolong, amarillo_sobre_azul);
  campo[6] = CreaEtiqueta(1, maxexistlong+maxdes+maxpreciolong+3,
                          "IVA");
  campo[7] = CreaCampo(2,maxexistlong+maxdes+maxpreciolong+3, 1, 3, amarillo_sobre_azul);
  campo[8] = CreaEtiqueta(0, 6, etiqueta);
  campo[9] = CreaEtiqueta(1, maxexistlong+maxdes+maxpreciolong+7,
                          "%");
  campo[10] = (FIELD *)0;

  set_field_type(campo[1], TYPE_NUMERIC, 0, 0, MAXDOUBLE);
  set_field_type(campo[5], TYPE_NUMERIC, 2, 0, MAXDOUBLE);
  set_field_type(campo[7], TYPE_NUMERIC, 0, 0, MAXDOUBLE);

  forma = new_form(campo);

  scale_form(forma, &tam_ren, &tam_col);
  campo[8]->fcol = (unsigned) ((tam_col - strlen(etiqueta)) / 2);
  sprintf(buff, "%f", iva_porcentaje);
  set_field_buffer(campo[7], 0, buff);

  muestra_ayuda(getmaxy(stdscr)-1,0);
  refresh();

  MuestraForma(forma, pos_ren, pos_col);
  ventana = form_win(forma);
  raw();
  noecho();

  /* int form_driver(FORM forma, int cod) */
  /* acepta el código cod, el cual indica la acción a tomar en la forma */
  /* hay algunos codigos en la función form_virtualize(WINDOW w) */

  while (!finished)
  {
    switch(form_driver(forma, c = form_virtualize(ventana, TRUE, 0))) {
    case E_OK:
      break;
    case E_UNKNOWN_COMMAND:
      if (c == CTRL('A')) {
        art[i].cant = atof(campo[1]->buf);
        strncpy(art[i].desc, campo[3]->buf, maxdes-1);
        limpiacad(art[i].desc, TRUE);
        art[i].pu = atof(campo[5]->buf);
        art[i].iva_porc = atof(campo[7]->buf);
        mvprintw(pos_ren+tam_ren+i+2, 1, "%-.2f", art[i].cant);
        mvprintw(pos_ren+tam_ren+i+2, maxexistlong+2, "%s", art[i].desc);
        if (maxdes > 65) {
          mvprintw(pos_ren+tam_ren+i+2, maxexistlong+65+2, " %8.2f",
                   art[i].pu);
          mvprintw(pos_ren+tam_ren+i+2, maxexistlong+73+3, " %3.2f",
                   art[i].iva_porc);
        }
        else {
          mvprintw(pos_ren+tam_ren+i+2, maxexistlong+maxdes+2, "%8.2f",
                   art[i].pu);
          mvprintw(pos_ren+tam_ren+i+2, maxexistlong+maxdes+8+2, " %3.2f",
                   art[i].iva_porc);
        }
        refresh();
        i++;
      }
      else if (!EsEspaniol(c))
        finished = my_form_driver(forma, c);
      else
        waddch(ventana, c);
      break;
    default:

        beep();
      break;
    }
  }

  BorraForma(forma);

  free_form(forma);
  for (c = 0; campo[c] != 0; c++)
      free_field(campo[c]);
  noraw();
  echo();

  attrset(COLOR_PAIR(normal));
  clrtobot();
  raw();
  return(i);
}

int CaptObserv(char *obs[maxobs], char *garantia) {
  int  i,
       c,
       finished = 0;
  WINDOW *w_obs;
  PANEL  *pan_obs;
  FORM   *f_obs;
  FIELD  *fld_obs[maxobs+1];
  int    num_rows,num_cols;

  for (i=0; i<=maxobs; i++)
    fld_obs[i] = CreaCampo(i, 1, 1, maxdes, amarillo_sobre_azul);

  fld_obs[maxobs] = (FIELD *)0;

  f_obs = new_form(fld_obs);

  scale_form(f_obs, &num_rows, &num_cols);

  w_obs = newwin(maxobs+2, getmaxx(stdscr), 6, 0);

  if ( w_obs  !=  0  ) {
    if ((pan_obs = new_panel(w_obs)) == 0) {
      free_form(f_obs);
    }
    else {
      set_form_win(f_obs, w_obs);
      set_form_sub(f_obs, derwin(w_obs, num_rows, num_cols, 1, 1));
      box(w_obs, 0, 0);
      keypad(w_obs, TRUE);
      wattrset(w_obs, COLOR_PAIR(verde_sobre_negro));
      mvwprintw(w_obs, 0, 2, "Observaciones:");
      wattrset(w_obs, COLOR_PAIR(normal));
      strncpy(fld_obs[1]->buf, "Pago en una sola exhibicion", maxdes);

      if (post_form(f_obs) != E_OK)
        wrefresh(w_obs);
      noecho();
      raw();


      while (!finished)  {
        switch(form_driver(f_obs, c = obs_form_virtualize(w_obs))) {
        case E_OK:
          break;
        case E_UNKNOWN_COMMAND:
          finished = my_form_driver(f_obs, c);
          break;
        default:
          beep();
          break;
        }
      }

      unpost_form(f_obs);
      echo();
      for (i=0; i<maxobs; i++) {
        strncpy(obs[i], fld_obs[i]->buf, maxdes);
        limpiacad(obs[i], TRUE);
      }
      /*
      for (i=0; (i<maxobs && !finished); ++i) {
        obs[i] = malloc(maxdes);
        mvwprintw(panel_window(pan_obs), i+1, 1, "%i: ",i+1);
        wgetstr(panel_window(pan_obs), obs[i]);
        finished = !strlen(obs[i]);
      }
      if (finished)
      i--;
      */
      werase(w_obs);
      box(w_obs, 0, 0);
      wattrset(w_obs, COLOR_PAIR(verde_sobre_negro));
      mvwprintw(w_obs, 0, 2, "Garantía:");
      wattrset(w_obs, COLOR_PAIR(normal));
      wmove(w_obs, 2, 1);
      wgetstr(panel_window(pan_obs), garantia);
      clear();
      del_panel(pan_obs);
      werase(w_obs);
      delwin(w_obs);
      refresh();

      Crea_Factura(cliente, fecha, art, numarticulos, subtotal, iva, total,
                   garantia, obs, nmfact, tipoimp);
      return(i);
    }
  }
  return(ERROR_DIVERSO);
}


void Muestra_Factura(struct fech fecha,
                     int numart,
                     int numobs, 
                     char *obs[maxobs],
                     char *garantia)
{
  int i;
  int centavos;

  attrset(COLOR_PAIR(normal));
  mvprintw(0, (getmaxx(stdscr)-27)/2, "Vista preliminar de factura");
  mvprintw(0, getmaxx(stdscr)-10, "%2d/%2d/%2d",
                                  fecha.dia, fecha.mes, fecha.anio);
  muestra_cliente(1,0, cliente);

  for (i=0; i<numart; i++) {
    mvprintw(i+6, 1, "%f", art[i].cant);
    mvprintw(i+6, maxexistlong+2, "%s", art[i].desc);
    if (maxdes > 70)
      mvprintw(i+6, maxexistlong+70+2, " %8.2f", art[i].pu);
    else
      mvprintw(i+6, maxexistlong+maxdes+2, "%8.2f", art[i].pu);
  }

  for (i=0; i<numobs; ++i)
    mvprintw(i+7+numarticulos, 0, "%s", obs[i]);

  if (strlen(garantia))
    mvprintw(i+8+numarticulos+numobs, 0, "%s de garantía", garantia);

  mvprintw(i+10+numarticulos+numobs, 0, "%s--", str_cant(total,&centavos));
  mvprintw(i+11+numarticulos+numobs, 0, "pesos %2d/100 M.N.", centavos);
  if (centavos<10)
    mvprintw(i+11+numarticulos+numobs, 6, "0");

  mvprintw(i+12+numarticulos, getmaxx(stdscr)-12, "%8.2f", subtotal);
  mvprintw(i+13+numarticulos, getmaxx(stdscr)-12, "%8.2f", iva);
  mvprintw(i+14+numarticulos, getmaxx(stdscr)-12, "%8.2f", total);
  refresh();
}

void imprime_factura() {
  char *comando;
  char buffer;

  mvprintw(getmaxy(stdscr)-1, 0, "¿Imprimir factura (S/N)? S\b");
  buffer = toupper(getch());
  if ((buffer != 'S') && (buffer != '\n'))
    return;

  comando = calloc(1, mxbuff);
  sprintf(comando, "lpr -Fb -P %s %s",  lprimp, nmfact);
  system(comando);
  sprintf(comando, "rm -rf %s", nmfact);
  system(comando);
  free(comando);

}

void AjustaModoTerminal(void)
{
  noraw();
  cbreak();
  noecho();
  scrollok(stdscr, TRUE);
  idlok(stdscr, TRUE);
  keypad(stdscr, TRUE);
}

/* Se incluye esta función en pos-curses.h 
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


FIELD *CreaCampo(int frow, int fcol, int ren, int cols)
{
    FIELD *f = new_field(ren, cols, frow, fcol, 0, 0);

    if (f)
        set_field_back(f,COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
    return(f);
}*/

/* void MuestraForma(FORM *f, unsigned pos_ren, unsigned pos_col)
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
} */

/*void BorraForma(FORM *f)
{
    WINDOW      *w = form_win(f);
    WINDOW      *s = form_sub(f);

    unpost_form(f);
    werase(w);
    wrefresh(w);
    delwin(s);
    delwin(w);
} */

/*int my_form_driver(FORM *form, int c)
{
    if (c == (MAX_FORM_COMMAND + 1)
                && form_driver(form, REQ_VALIDATION) == E_OK)
        return(TRUE);
    else
    {
        beep();
        return(FALSE);
    }
}*/


/***************************************************************************
***************************************************************************/

int main(int argc, char *argv[]) {
/*   char encabezado1[mxbuff] = "Sistema OsoPOS - Programa Facturar",
        encabezado2[mxbuff] = "E. Israel Osorio H., 1999-2001 linucs@punto-deventa.com"; */
  char   *obs[maxobs],
         *garantia;
  int    i;
  unsigned num_venta=0;
  unsigned folio_fact=0;
  PGconn *con;
  time_t tiempo;

  initscr();
  start_color();
  init_config();
  read_global_config();
  read_config();

  init_pair(amarillo_sobre_azul, COLOR_YELLOW, COLOR_BLUE);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(normal, COLOR_WHITE, COLOR_BLACK);

  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd); 
  if (con == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }

  tiempo = time(NULL);
  f = localtime(&tiempo);
  fecha.dia = f->tm_mday;
  fecha.mes = f->tm_mon + 1;
  fecha.anio = f->tm_year + 1900;
  if (argc>1)
    if (!strcmp(argv[1],"-r")) {
      num_venta = atoi(argv[2]);      
      //      numarticulos = LeeVenta(nmdatos, art);
    }
  clear();
/*  printw("RFC: ");
  getstr(cliente.rfc);
  clear(); */

  AjustaModoTerminal();
  captura_cliente(con, &num_venta, &folio_fact, fecha, cliente);
  muestra_cliente(0,0,cliente);
  if (num_venta)
    numarticulos = lee_venta(con, num_venta, art);
  else
    numarticulos = captura_articulos();
  CalculaIVA();
  for (i=0; i<maxobs; i++) {
    obs[i] = calloc(1,mxbuff);
  }
  garantia = calloc(1,maxdes);
  i = CaptObserv(obs, garantia);
  Muestra_Factura(fecha, numarticulos, i, obs, garantia);
  imprime_factura();
  endwin();
  RegistraFactura(folio_fact, art, con);
  for (i=0; i<maxobs; i++) {
    free(obs[i]);
  }
  free(garantia);
  free(home_directory);
  PQfinish(con);
  return(OK);
}

/* BUGS:

* Se puede provocar overflow en captura de descripcion de artículo
* Overflow en observaciones (mismo error)

PENDIENTES:
- Modificar la lectura de archivo de venta, para adaptrase al nuevo formato
*/

