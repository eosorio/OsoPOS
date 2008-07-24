#ifndef _form
#include <form.h>
#define _form
#endif

#include "pos-curses.h"
#define _pos_curses

#ifndef CTRL
#define CTRL(x)         ((x) & 0x1f)
#endif

#define QUIT            CTRL('Q')
#define ESCAPE          CTRL('[')
#define ENTER		10
#define BLANK           ' '        /* caracter de fondo */

#define CAMPO_RFC      1
#define CAMPO_NOMBRE   2
#define CAMPO_CALLE    4
#define CAMPO_NEXT     6   /* Domicilio->Num. Ext */
#define CAMPO_NINT     8   /* Domicilio->Num. Int */
#define CAMPO_COLONIA 10   /* Domicilio->Colonia */
#define CAMPO_CIUDAD  12   /* Domicilio->Ciudad */
#define CAMPO_EDO     14
#define CAMPO_CP      16
#define CAMPO_CURP    18
#define CAMPO_FOLIO   22
#define CAMPO_NUMVEN  24
#define CAMPO_DIA     26
#define CAMPO_MES     28
#define CAMPO_ANIO    30

struct datoscliente captura_cliente(PGconn *con,
				    unsigned* num_venta,
				    unsigned *folio_fact,
				    struct fech fecha);

void  muestra_ayuda_cliente(int ren, int col);

void  muestra_cliente(int renglon, int columna, struct datoscliente cliente);

bool   EsEspaniol(char c);

int   form_virtualize(WINDOW *w, int readchar, int c);

struct datoscliente captura_cliente(PGconn *con,
				    unsigned* num_venta,
				    unsigned *folio_fact,
				    struct fech fecha) {
  struct datoscliente cliente;
  WINDOW *ven;
  FORM *forma;
  FIELD *campo[31];
  char etiqueta[mxbuff];
  int  finished = 0, c, i;
  int tam_ren, tam_col, pos_ren, pos_col;
  char scp[16];
  GString *buffer;

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
  campo[CAMPO_NINT] = CreaCampo(7, maxspcalle+maxspext+1, 1, maxspint-1, amarillo_sobre_azul);
  campo[9] = CreaEtiqueta(8, 0, "Colonia:");
  campo[CAMPO_COLONIA] = CreaCampo(9, 0, 1, maxspcol-1, amarillo_sobre_azul);
  campo[11] = CreaEtiqueta(10, 0, "Ciudad:");
  campo[CAMPO_CIUDAD] = CreaCampo(11, 0, 1, maxspcd-1, amarillo_sobre_azul);
  campo[13] = CreaEtiqueta(10, maxspcd+1, "Estado:");
  campo[CAMPO_EDO] = CreaCampo(11, maxspcd+1, 1, maxspedo-1, amarillo_sobre_azul);
  campo[15] = CreaEtiqueta(10, maxspcd+maxspedo+2, "C.P.:");
  campo[CAMPO_CP] = CreaCampo(11, maxspcd+maxspedo+2, 1, 5, amarillo_sobre_azul);
  campo[17] = CreaEtiqueta(12, 0, "C.U.R.P.");
  campo[CAMPO_CURP] = CreaCampo(13, 0, 1, maxcurp-1, amarillo_sobre_azul);
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

  set_field_type(campo[CAMPO_DIA], TYPE_INTEGER, 2, 1, 31);
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
    switch(form_driver(forma, c = form_virtualize(ven, TRUE, 0))) {
    case E_OK:
      break;
    case E_UNKNOWN_COMMAND:
      if (c == CTRL('B')) {
        strcpy(cliente.rfc, field_buffer(campo[CAMPO_RFC], 0));
        if (!BuscaCliente(cliente.rfc, &cliente, con)) {
          set_field_buffer(campo[CAMPO_NOMBRE], 0, cliente.nombre);
          set_field_buffer(campo[CAMPO_CALLE], 0, cliente.dom_calle);
          set_field_buffer(campo[CAMPO_NEXT], 0, cliente.dom_numero);
          set_field_buffer(campo[CAMPO_NINT], 0, cliente.dom_inter);
          set_field_buffer(campo[CAMPO_COLONIA], 0, cliente.dom_col);
          set_field_buffer(campo[CAMPO_CIUDAD], 0, cliente.dom_ciudad);
          set_field_buffer(campo[CAMPO_EDO], 0, cliente.dom_edo);
          sprintf(scp, "%5u", cliente.cp);
          set_field_buffer(campo[CAMPO_RFC], 0, scp);

          set_field_buffer(campo[CAMPO_CURP], 0, cliente.curp);
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

  for (i=2; i<=8; i+=2) {
    buffer = g_string_new(field_buffer(campo[i], 0));

    if (buffer->str[ buffer->len - 1 ] == ' ') {
      buffer->str[ buffer->len-1 ] = 0;
      set_field_buffer(campo[i], 0, buffer->str);
    }
    g_string_free(buffer, TRUE);
  }

  strncpy(cliente.nombre, field_buffer(campo[CAMPO_NOMBRE], 0), maxspc);
  limpiacad(cliente.nombre, TRUE);
  strncpy(cliente.dom_calle, field_buffer(campo[CAMPO_CALLE], 0), maxspcalle);
  limpiacad(cliente.dom_calle, TRUE);
  strncpy(cliente.dom_numero,field_buffer(campo[CAMPO_NEXT], 0), maxspext);
  limpiacad(cliente.dom_numero, TRUE);
  strncpy(cliente.dom_inter,field_buffer(campo[CAMPO_NINT], 0), maxspint);
  limpiacad(cliente.dom_inter, TRUE);
  strncpy(cliente.dom_col,field_buffer(campo[CAMPO_COLONIA], 0), maxspcol);
  limpiacad(cliente.dom_col, TRUE);
  strncpy(cliente.dom_ciudad,field_buffer(campo[CAMPO_CIUDAD], 0), maxspcd);
  limpiacad(cliente.dom_ciudad, TRUE);
  strncpy(cliente.dom_edo,field_buffer(campo[CAMPO_EDO], 0), maxspedo);
  limpiacad(cliente.dom_edo, TRUE);
  cliente.cp = atoi(field_buffer(campo[CAMPO_RFC], 0));
  strncpy(cliente.curp,field_buffer(campo[CAMPO_CURP], 0), maxcurp);
  limpiacad(cliente.curp, TRUE);
  strncpy(cliente.rfc,field_buffer(campo[1], 0), maxrfc);
  limpiacad(cliente.rfc, TRUE);
  *folio_fact = atoi(field_buffer(campo[CAMPO_FOLIO], 0));
  *num_venta = atoi(field_buffer(campo[CAMPO_NUMVEN], 0));
  fecha.dia = atoi(field_buffer(campo[CAMPO_DIA], 0));
  fecha.mes = atoi(field_buffer(campo[CAMPO_MES], 0));
  fecha.anio = atoi(field_buffer(campo[CAMPO_ANIO], 0))+2000;

  BorraForma(forma);
  free_form(forma);
  for (c = 0; campo[c] != 0; c++)
      free_field(campo[c]);
  //  noraw();
  echo();

  attrset(COLOR_PAIR(normal));
  move(5,0);
  clrtobot();
  raw();
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

bool EsEspaniol(char c) {
  /* Lo siguiente ya no es válido con UTF-8 */
  /*  return(c=='á' || c=='é' || c== 'í' || c=='ó' ||
      c=='ú' || c=='ñ' || c=='Ñ' || c=='ü' || c=='Ü');*/
  return(false);
}



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

