/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

 Facturación 1.16. Módulo de facturación de OsoPOS.

        Copyright (C) 1999-2002 Eduardo Israel Osorio Hernández

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


    
#include <stdio.h>
#include "include/pos-curses.h"
#define _pos_curses
#include <panel.h>
#include <time.h>
#include <values.h>
#include "include/print-func.h"

//#include "include/linucs.h"
#include "include/electroh.h"

#ifndef _form
#include <form.h>
#define _form
#endif

#define vers "1.16"
/*
#ifdef maxspc
#undef maxspc
#endif
*/

#define mxchcant 50

#define CAMPO_RFC      1
#define CAMPO_NOMBRE   2
#define CAMPO_CALLE    4
#define CAMPO_NEXT     6
#define CAMPO_EDO     14
#define CAMPO_CP      16
#define CAMPO_FOLIO   22
#define CAMPO_NUMVEN  24
#define CAMPO_DIA     26
#define CAMPO_MES     28
#define CAMPO_ANIO    30


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


int   EsEspaniol(char c);
void  captura_cliente(PGconn *con, unsigned* num_venta, unsigned *folio_fact);
int   captura_articulos();
int   CaptObserv();
void  imprime_factura();
void  muestra_ayuda_cliente(int ren, int col);
void  muestra_cliente(int renglon, int columna, struct datoscliente cliente);


/* Funciones que usan form.h */
void  AjustaModoTerminal(void);
/* FIELD *CreaEtiqueta(int frow, int fcol, NCURSES_CONST char *label); */
//FIELD *CreaCampo(int pren, int pcol, int ren, int cols);
void  MuestraForma(FORM *f, unsigned ren, unsigned col);
void  BorraForma(FORM *f);
int   form_virtualize(WINDOW *w);
//int   my_form_driver(FORM *form, int c);



int EsEspaniol(char c) {
  return(c=='á' || c=='é' || c== 'í' || c=='ó' ||
  c=='ú' || c=='ñ' || c=='Ñ' || c=='ü' || c=='Ü');
}

void muestra_ayuda_cliente(int ren, int col) {
  mvaddstr(ren,col,
   "Las teclas de flecha mueven el cursor a traves del campo\n");
  addstr("<Ctrl-Q>  Terminar de introducir datos    ");
  addstr("<Ctrl-B>  Busca cliente por su RFC\n");
  addstr("<Inicio>  Primer campo (rfc)           ");
  addstr("<Fin>     Ultimo campo (fecha)\n");
  addstr("<Intro>   Siguiente campo        \n");
  addstr("<Ctrl-X>  Borra el campo                  ");
  addstr("<Insert>  Sobreescribir/insertar\n");
}

int form_virtualize(WINDOW *w)
{
  int  mode = REQ_INS_MODE;
  int         c = wgetch(w);

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

void captura_cliente(PGconn *con, unsigned* num_venta, unsigned *folio_fact) {
   WINDOW *ven;
   FORM *forma;
   FIELD *campo[31];
   char etiqueta[mxbuff];
   int  finished = 0, c, i;
   int tam_ren, tam_col, pos_ren, pos_col;
   char scp[16];

  pos_ren = 1;
  pos_col = 0;
  strcpy(etiqueta,"Datos del cliente");

  /* describe la forma */
  campo[0] = CreaEtiqueta(2, 30, etiqueta);
  campo[20] = CreaEtiqueta(4, 0, "Nombre o razon social:");
  campo[CAMPO_NOMBRE] = CreaCampo(5, 0, 1, maxspc-1, amarillo_sobre_azul);
  campo[3] = CreaEtiqueta(6, 0, "Calle:");
  campo[CAMPO_CALLE] = CreaCampo(7, 0, 1, maxspcalle-1, amarillo_sobre_azul);
  campo[5] = CreaEtiqueta(6, maxspcalle, "Num. Ext.:");
  campo[CAMPO_NEXT] = CreaCampo(7, maxspcalle, 1, maxspext-1, amarillo_sobre_azul);
  campo[7] = CreaEtiqueta(6, maxspcalle+maxspext+1, "Int");
  campo[8] = CreaCampo(7, maxspcalle+maxspext+1, 1, maxspint-1, amarillo_sobre_azul);
  campo[9] = CreaEtiqueta(8, 0, "Colonia:");
  campo[10] = CreaCampo(9, 0, 1, maxspcol-1, amarillo_sobre_azul);
  campo[11] = CreaEtiqueta(10, 0, "Ciudad:");
  campo[12] = CreaCampo(11, 0, 1, maxspcd-1, amarillo_sobre_azul);
  campo[13] = CreaEtiqueta(10, maxspcd+1, "Estado:");
  campo[CAMPO_EDO] = CreaCampo(11, maxspcd+1, 1, maxspedo-1, amarillo_sobre_azul);
  campo[15] = CreaEtiqueta(10, maxspcd+maxspedo+2, "C.P.:");
  campo[CAMPO_CP] = CreaCampo(11, maxspcd+maxspedo+2, 1, 5, amarillo_sobre_azul);
  campo[17] = CreaEtiqueta(12, 0, "C.U.R.P.");
  campo[18] = CreaCampo(13, 0, 1, maxcurp-1, amarillo_sobre_azul);
  campo[19] = CreaEtiqueta(12, maxcurp+1, "R.F.C.:");
  campo[CAMPO_RFC] = CreaCampo(13, maxcurp+1, 1, maxrfc-1, amarillo_sobre_azul);
  campo[21] = CreaEtiqueta(0, 0, "Folio:");
  campo[CAMPO_FOLIO] = CreaCampo(0, 6, 1, 5, amarillo_sobre_azul);
  campo[23] = CreaEtiqueta(0, 24, "# venta:");
  campo[CAMPO_NUMVEN] = CreaCampo(0, 33, 1, 8, amarillo_sobre_azul);
  campo[25] = CreaEtiqueta(0, 55, "Fecha:");
  campo[CAMPO_DIA] = CreaCampo(0, 62, 1, 2, amarillo_sobre_azul);
  campo[27] = CreaEtiqueta(0, 64, "/");
  campo[CAMPO_MES] = CreaCampo(0, 65, 1, 2, amarillo_sobre_azul);
  campo[29] = CreaEtiqueta(0, 67, "/");
  campo[CAMPO_ANIO] = CreaCampo(0, 68, 1, 2, amarillo_sobre_azul);
  campo[31] = (FIELD *)0;

  set_field_type(campo[CAMPO_DIA], TYPE_NUMERIC, 0, 1, 31);
  set_field_type(campo[CAMPO_MES], TYPE_NUMERIC, 0, 1, 12);


  forma = new_form(campo);

  /* Calcula y coloca la etiqueta a la mitad de la forma */
  scale_form(forma, &tam_ren, &tam_col);
  campo[0]->fcol = (unsigned) ((tam_col - strlen(etiqueta)) / 2);
  campo[23]->fcol = (unsigned) ((tam_col - 17) / 2);
  campo[CAMPO_NUMVEN]->fcol = (unsigned) (campo[23]->fcol + 9);
  campo[25]->fcol = (unsigned) (tam_col - 15); /* Fecha */
  campo[CAMPO_DIA]->fcol = (unsigned) (campo[25]->fcol + 7); /* Dia */
  campo[27]->fcol = (unsigned) (campo[CAMPO_DIA]->fcol + 2); 
  campo[CAMPO_MES]->fcol = (unsigned) (campo[27]->fcol + 1); /* Mes */
  campo[29]->fcol = (unsigned) (campo[CAMPO_MES]->fcol + 2);
  campo[CAMPO_ANIO]->fcol = (unsigned) (campo[29]->fcol + 1); /* Año */

  sprintf(scp, "%2d", fecha.dia);
  if (scp[0] == ' ') scp[0] = '0';
  set_field_buffer(campo[CAMPO_DIA], 0, scp);
  sprintf(scp, "%2d",fecha.mes);
  if (scp[0] == ' ') scp[0] = '0';
  set_field_buffer(campo[CAMPO_MES], 0, scp);
  sprintf(scp, "%2d",fecha.anio-2000);
  if (scp[0] == ' ') scp[0] = '0';
  set_field_buffer(campo[CAMPO_ANIO], 0, scp);

  sprintf(scp, "%u", *num_venta);
  set_field_buffer(campo[CAMPO_NUMVEN], 0, scp);
  sprintf(scp, "%u", *folio_fact);
  set_field_buffer(campo[CAMPO_FOLIO], 0, scp);

  muestra_ayuda_cliente(tam_ren+pos_ren+3,0);
  refresh();

  MuestraForma(forma, pos_ren, pos_col);
  ven = form_win(forma);
  raw();

  /* int form_driver(FORM forma, int cod) */
  /* acepta el código cod, el cual indica la acción a tomar en la forma */
  /* hay algunos codigos en la función form_virtualize(WINDOW w) */

  while (!finished)
  {
    switch(form_driver(forma, c = form_virtualize(ven))) {
    case E_OK:
      break;
    case E_UNKNOWN_COMMAND:
      if (c == CTRL('B')) {
        strcpy(cliente.rfc,campo[1]->buf);
        if (!BuscaCliente(cliente.rfc, &cliente, con)) {
          set_field_buffer(campo[CAMPO_NOMBRE], 0, cliente.nombre);
          set_field_buffer(campo[4], 0, cliente.dom_calle);
          set_field_buffer(campo[6], 0, cliente.dom_numero);
          set_field_buffer(campo[8], 0, cliente.dom_inter);
          set_field_buffer(campo[10], 0, cliente.dom_col);
          set_field_buffer(campo[12], 0, cliente.dom_ciudad);
          set_field_buffer(campo[14], 0, cliente.dom_edo);
          sprintf(scp, "%5u", cliente.cp);
          set_field_buffer(campo[16], 0, scp);

          set_field_buffer(campo[18], 0, cliente.curp);
        }
        else
          set_field_buffer(campo[CAMPO_NOMBRE], 0, "Nuevo registro");
      }
      else if (!EsEspaniol(c))
        finished = my_form_driver(forma, c);
      else
        waddch(ven, c);
      break;
    default:

        beep();
      break;
    }
  }

  BorraForma(forma);

  for (i=2; i<=8; i+=2)
    if (campo[i]->buf[ strlen(campo[i]->buf)-1 ] == ' ')
      campo[i]->buf[ strlen(campo[i]->buf)-1 ] = 0;

  strncpy(cliente.nombre, campo[CAMPO_NOMBRE]->buf, maxspc);
  limpiacad(cliente.nombre, TRUE);
  strncpy(cliente.dom_calle,campo[CAMPO_CALLE]->buf, maxspcalle);
  limpiacad(cliente.dom_calle, TRUE);
  strncpy(cliente.dom_numero,campo[6]->buf, maxspext);
  limpiacad(cliente.dom_numero, TRUE);
  strncpy(cliente.dom_inter,campo[8]->buf, maxspint);
  limpiacad(cliente.dom_inter, TRUE);
  strncpy(cliente.dom_col,campo[10]->buf, maxspcol);
  limpiacad(cliente.dom_col, TRUE);
  strncpy(cliente.dom_ciudad,campo[12]->buf, maxspcd);
  limpiacad(cliente.dom_ciudad, TRUE);
  strncpy(cliente.dom_edo,campo[14]->buf, maxspedo);
  limpiacad(cliente.dom_edo, TRUE);
  cliente.cp = atoi(campo[16]->buf);
  strncpy(cliente.curp,campo[18]->buf, maxcurp);
  limpiacad(cliente.curp, TRUE);
  strncpy(cliente.rfc,campo[1]->buf, maxrfc);
  limpiacad(cliente.rfc, TRUE);
  *folio_fact = atoi(campo[CAMPO_FOLIO]->buf);
  *num_venta = atoi(campo[CAMPO_NUMVEN]->buf);
  fecha.dia = atoi(campo[CAMPO_DIA]->buf);
  fecha.mes = atoi(campo[CAMPO_MES]->buf);
  fecha.anio = atoi(campo[CAMPO_ANIO]->buf)+2000;

  free_form(forma);
  for (c = 0; campo[c] != 0; c++)
      free_field(campo[c]);
  noraw();
  echo();

  attrset(COLOR_PAIR(normal));
  move(5,0);
  clrtobot();
  raw();
}

void muestra_cliente(int renglon, int columna, struct datoscliente cliente)
{
  mvprintw(1+renglon, columna, "%s", cliente.nombre);
  mvprintw(2+renglon, columna, "%s", cliente.dom_calle);
  mvprintw(2+renglon, columna+maxspcalle+1, "%s", cliente.dom_numero);
  mvprintw(2+renglon, columna+maxspcalle+maxspext+2, "%s", cliente.dom_inter);
  mvprintw(3+renglon, columna, "%s", cliente.dom_col);
  mvprintw(3+renglon, columna+maxspcol+1, "%s", cliente.dom_ciudad);
  mvprintw(3+renglon, columna+maxspcol+maxspcd+2, "%s", cliente.dom_edo);
  mvprintw(3+renglon, columna+maxspcol+maxspcd+maxspedo+3, "%5u", cliente.cp);
  mvprintw(4+renglon, columna, "%s", cliente.curp);
  mvprintw(4+renglon, columna+maxcurp+1, "%s", cliente.rfc);
  refresh();
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
    switch(form_driver(forma, c = form_virtualize(ventana))) {
    case E_OK:
      break;
    case E_UNKNOWN_COMMAND:
      if (c == CTRL('A')) {
        art[i].cant = atoi(campo[1]->buf);
        strncpy(art[i].desc, campo[3]->buf, maxdes-1);
        limpiacad(art[i].desc, TRUE);
        art[i].pu = atof(campo[5]->buf);
        art[i].iva_porc = atof(campo[7]->buf);
        mvprintw(pos_ren+tam_ren+i+2, 1, "%-d", art[i].cant);
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
      wattrset(w_obs, COLOR_PAIR(verde_sobre_negro));
      mvwprintw(w_obs, 0, 2, "Observaciones:");
      wattrset(w_obs, COLOR_PAIR(normal));

      if (post_form(f_obs) != E_OK)
        wrefresh(w_obs);
      noecho();
      raw();

      while (!finished)  {
        switch(form_driver(f_obs, c = form_virtualize(w_obs))) {
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
    mvprintw(i+6, 1, "%-d", art[i].cant);
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

  mvprintw(getmaxy(stdscr)-1, 0, "¿Imprimir factura (S/N)? S\b");
  buffer = toupper(getch());
  if ((buffer != 'S') && (buffer != '\n'))
    return;

  if (impresion_directa) {
    imprime_doc(nmfact, puerto_imp);
  }
  else {
    comando = calloc(1, mxbuff);
    sprintf(comando, "lpr -Fb -P %s %s",  lprimp, nmfact);
    system(comando);
    sprintf(comando, "rm -rf %s", nmfact);
    system(comando);
    free(comando);
  }
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
  read_config();

  init_pair(amarillo_sobre_azul, COLOR_YELLOW, COLOR_BLUE);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(normal, COLOR_WHITE, COLOR_BLACK);

  con = Abre_Base(NULL, NULL, NULL, NULL, "osopos", "scaja", NULL);
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
  captura_cliente(con, &num_venta, &folio_fact);
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

