/*    -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

OsoPOS - Programa de inventario 1999 E. Israel Osorio 
   ventas@punto-deventa.com

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
#include <values.h>
#include <unistd.h>
#ifndef _pos
#include "include/pos-curses.h"
#define _pos
#endif

#define numdat          8

#include "include/invent.h"

#include <form.h>


#ifndef CTRL
#define CTRL(x)         ((x) & 0x1f)
#endif

#define QUIT            CTRL('Q')
#define ESCAPE          CTRL('[')
#define ENTER           10
#define CTRL_DOWN       258        /* Control y flecha abajo en PC */
#define BLANK           ' '        /* caracter de fondo */

#define normal                1
#define verde_sobre_negro     2
#define amarillo_sobre_azul   3

#define CAMPO_COD             1
#define CAMPO_DESCR           3
#define CAMPO_PU              5
#define CAMPO_EXIS           10
#define CAMPO_DISC            7
#define CAMPO_EXMIN          12
#define CAMPO_EXMAX          14
#define CAMPO_CODPROV        16
#define CAMPO_DEPTO          18
#define CAMPO_PCOSTO         20
#define CAMPO_IVA            22

#define version "0.31-1"

#define maxitemlista    2048

WINDOW *v_arts;
char   *nminvent;
char   *nmdisp;
char   *home_directory; 
char   *item[maxitemlista];
struct articulos art;
PGconn *base_inv;

int read_config();
int imprime_lista(PGconn *con, char *campo_orden);

void muestra_renglon(unsigned renglon, unsigned num_items);
int quita_renglon(unsigned renglon, unsigned num_items);

int form_virtualize(WINDOW *w);
FIELD *CreaEtiqueta(int pren, int pcol, NCURSES_CONST char *etiqueta);


int read_config() {
  char *nmconfig;
  FILE *config;
  char buff[mxbuff], buf[mxbuff];
  char *b;


  home_directory = calloc(1, 255);
  nmconfig = calloc(1, 255);
  config = popen("printenv HOME", "r");
  fgets(home_directory, 255, config);
  home_directory[strlen(home_directory)-1] = 0;
  pclose(config);

  strcpy(nmconfig, home_directory);
  strcat(nmconfig, "/.osopos/invent.config");


  nmdisp = calloc(1, strlen("/tmp/scaja_invent")+1);
  nminvent = calloc(1, 9);

  strcpy(nmdisp, "/tmp/scaja_invent");
  strcpy(nminvent, "osopos");

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuración */
    b = buff;
    fgets(buff,sizeof(buff),config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;

      if (!strlen(buff) || buff[0] == '#') { /* Linea vacía o coment. */
        fgets(buff,sizeof(buff),config);
        continue;
      }

      strcpy(buf, strtok(buff,"="));
        /* La función strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

        /* Busca parámetros de operación */
      if (!strcmp(b,"base.datos")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nminvent, strlen(buf+1));
        strcpy(nminvent,buf);
      }
      else if (!strcmp(b,"impresion.salida")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmdisp, strlen(buf+1));
        strcpy(nmdisp,buf);
      }
      fgets(buff,sizeof(buff),config);
    }
    fclose(config);
    return(OK);
  }
  else {
    fprintf(stderr, "No encuentro el archivo de configuración en %s,\n",
            nmconfig);
    fprintf(stderr, "usando datos por omisión...\n");
  }
  return(1);
}


int inicializa_lista(PGconn *base, char *campo_orden)
{
  int       i, j;
  PGresult  *res;
  char *comando;

  v_arts =  newwin(getmaxy(stdscr)-10,getmaxx(stdscr), 7,0);
/* Los parámetros anteriores causan un SIGSEGV y los posteriores no */
/*  v_arts = newwin (LINES-4, COLS-1, 4,0); */
  scrollok(v_arts, TRUE);

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr,"Falló comando BEGIN al buscar en inventario\n");
   return(ERROR_SQL);
  }
  PQclear(res);

  comando = calloc(1,mxbuff);
  sprintf(comando,
      "DECLARE cursor_arts CURSOR FOR SELECT * FROM articulos ORDER BY \"%s\"",
	campo_orden);
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando DECLARE CURSOR al buscar artículos\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    free(comando);
    return(ERROR_SQL);
  }
  PQclear(res);

  strcpy(comando, "FETCH ALL in cursor_arts");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
    free(comando);
    return(ERROR_SQL);
  }

  /*nCampos = PQnfields(res); */

/*  strcpy(art.codigo,PQgetvalue(res,registro,campo));  */

  for (i=0; i<PQntuples(res); i++) {
    item[i] = calloc(1,getmaxx(stdscr));
    for (j=0; j<getmaxx(stdscr); j++) {
      item[i][j] = ' ';
    }

    if (maxdes+maxcod+maxpreciolong+maxexistlong+3 > getmaxx(stdscr)-1) {
       /* Código */
      strcpy(comando, PQgetvalue(res,i,0));
      if (strlen(comando)>16)
        comando[16] = 0;
      memcpy(&item[i][0], comando, strlen(comando));

       /* Descripción */
      strcpy(comando, PQgetvalue(res,i,1));
      if (strlen(comando)>31)
        comando[31] = 0;
      memcpy(&item[i][16], comando, strlen(comando));

      /* Precio */
      sprintf(comando, "%10.2f", atof(PQgetvalue(res,i,2)) );
      memcpy(&item[i][16+31], comando, strlen(comando));

      /* Existencia */
      sprintf(comando, "%-4d", atoi(PQgetvalue(res,i,4)) );
      memcpy(&item[i][16+31+maxpreciolong+1], comando, strlen(comando));
    }
    else {
      strcpy(comando, PQgetvalue(res,i,0));
      if (strlen(comando)>maxcod)
        comando[maxcod-1] = 0;
      memcpy(&item[i][0], comando, strlen(comando));

      strcpy(comando, PQgetvalue(res,i,1));
      if (strlen(comando)>maxdes)
        comando[maxdes-1] = 0;
      memcpy(&item[i][maxcod+1], comando, strlen(comando));

      sprintf(comando, "%10.2f", atof(PQgetvalue(res,i,2)) );
      memcpy(&item[i][maxcod+maxdes+2], comando, strlen(comando));

      sprintf(comando, "%-4d", atoi(PQgetvalue(res,i,4)) );
      memcpy(&item[i][maxpreciolong+maxdes+maxcod+3], comando, strlen(comando));
    }
    item[i][getmaxx(stdscr)-1] = 0;
  }
  PQclear(res);

  /* close the portal */
  res = PQexec(base, "CLOSE cursor_arts");
  PQclear(res);

  /* end the transaction */
  res = PQexec(base, "END");
  PQclear(res);
  free(comando);
  refresh();
  wrefresh(v_arts);
  return(i);
}

void modifica_item(struct articulos art, int i)
{
  char *aux;

    aux = calloc(1, mxbuff);
    if (maxdes+maxcod+maxpreciolong+maxexistlong+3 > getmaxx(stdscr)-1) {
       /* Código */
      strcpy(aux, art.codigo);
      if (strlen(aux)>16)
        aux[16] = 0;
      memcpy(&item[i][0], aux, strlen(aux));

       /* Descripción */
      strcpy(aux, art.desc);
      if (strlen(aux)>31)
        aux[31] = 0;
      memcpy(&item[i][16], aux, strlen(aux));

      /* Precio */
      sprintf(aux, "%10.2f", art.pu);
      memcpy(&item[i][16+31], aux, strlen(aux));

      /* Existencia */
      sprintf(aux, "%-4d", art.exist );
      memcpy(&item[i][16+31+maxpreciolong+1], aux, strlen(aux));
    }
    else {
      strcpy(aux, art.codigo);
      if (strlen(aux)>maxcod)
        aux[maxcod-1] = 0;
      memcpy(&item[i][0], aux, strlen(aux));

      strcpy(aux, art.desc);
      if (strlen(aux)>maxdes)
        aux[maxdes-1] = 0;
      memcpy(&item[i][maxcod+1], aux, strlen(aux));

      sprintf(aux, "%10.2f", art.pu );
      memcpy(&item[i][maxcod+maxdes+2], aux, strlen(aux));

      sprintf(aux, "%-4d", art.exist );
      memcpy(&item[i][maxpreciolong+maxdes+maxcod+3], aux, strlen(aux));
    }
    item[i][getmaxx(stdscr)-1] = 0;

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

FIELD *CreaCampo(int frow, int fcol, int ren, int cols)
{
    FIELD *f = new_field(ren, cols, frow, fcol, 0, 0);

    if (f)
        set_field_back(f,COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
    return(f);
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

void ajusta_ventana_forma(void)
{
  noraw();
  cbreak();
  noecho();
  scrollok(stdscr, TRUE);
  idlok(stdscr, TRUE);
  keypad(stdscr, TRUE);
}


int form_virtualize(WINDOW *w)
{
  int  mode = REQ_INS_MODE;
  int         c = wgetch(w);

  switch(c) {
    case QUIT:
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
/*    case CTRL('R'):
        return(REQ_RIGHT_FIELD); */
/*    case CTRL('U'):
        return(REQ_UP_FIELD); */
/*    case CTRL('D'):
        return(REQ_DOWN_FIELD); */

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
/*    case KEY_UP:
        return(REQ_UP_CHAR);
    case KEY_DOWN:
        return(REQ_DOWN_CHAR); */

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

int llena_campos(FIELD *campo[25], unsigned i, PGconn *base)
{
  PGresult           *res;
  char               codigo[maxcod];
  char               *aux;
  char               *comando;
  struct articulos   art;


  strncpy(codigo, item[i], maxcod-1);
  codigo[maxcod-1] = 0;
  limpiacad(codigo, TRUE);
  comando = calloc(1, mxbuff);

  res = Busca_en_Inventario(base, "articulos", "codigo", codigo, &art);
  if (res == NULL) {
    aux = calloc(1, mxbuff);

    set_field_buffer(campo[CAMPO_COD], 0, codigo);
    set_field_buffer(campo[CAMPO_DESCR], 0, art.desc);

    sprintf(aux, "%.2f", art.pu);
    set_field_buffer(campo[CAMPO_PU], 0, aux);

    sprintf(aux, "%d", art.exist);
    set_field_buffer(campo[CAMPO_EXIS], 0, aux);

    sprintf(aux, "%.2f", art.disc);
    set_field_buffer(campo[CAMPO_DISC], 0, aux);

    sprintf(aux, "%d", art.exist_min);
    set_field_buffer(campo[CAMPO_EXMIN], 0, aux);

    sprintf(aux, "%d", art.exist_max);
    set_field_buffer(campo[CAMPO_EXMAX], 0, aux);

    sprintf(comando, "SELECT nick FROM proveedores WHERE id=%d",
            art.id_prov);
    res = PQexec(base, comando);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr,"Falló comando %s\n", comando);
      sleep(5);
      free(aux);
      free(comando);
      return(ERROR_SQL);
    }
    strncpy(aux, PQgetvalue(res, 0, 0), maxcod);
    set_field_buffer(campo[CAMPO_CODPROV], 0, aux);

    sprintf(aux, "%.2f", art.p_costo);
    set_field_buffer(campo[CAMPO_PCOSTO], 0, aux);

    sprintf(comando, "SELECT nombre FROM departamento WHERE id=%d",
            art.id_depto);
    res = PQexec(base, comando);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr,"Falló comando %s\n", comando);
      sleep(5);
      free(aux);
      free(comando);
      return(ERROR_SQL);
    }
    strncpy(aux, PQgetvalue(res, 0, 0), maxdeptolen);
    set_field_buffer(campo[CAMPO_DEPTO], 0, aux);

    sprintf(aux, "%.2f", art.iva_porc);
    set_field_buffer(campo[CAMPO_IVA], 0, aux);

    free(aux);
    free(comando);
    return(OK);
  }
  else
    return(ERROR_SQL);
}

int busca_item(char *codigo, unsigned num_items)
{
  char *aux;
  int  i;

  if (strlen(codigo) > maxcod)
    return(-1);
  aux = calloc(1, mxbuff);

  for (i=0; i<num_items; i++) {
    strncpy(aux, item[i], maxcod-1);
    aux[mxbuff-1] = 0;
    limpiacad(aux, TRUE);
    if (!strcmp(aux, codigo)) {
      free(aux);
      return(i);
    }
  }
  free(aux);
  return(-1);
}

int interpreta_campos(struct articulos *art, FIELD *campo[25])
{
  int aux;
  char str_depto[maxdeptolen];
  char str_prov[maxnickprov];

  strncpy(art->codigo, campo[CAMPO_COD]->buf, maxcod-1);
  art->codigo[maxcod-1] = 0;
  limpiacad(art->codigo, TRUE);

  strncpy(art->desc, campo[CAMPO_DESCR]->buf, maxdes);
  art->desc[maxdes-1] = 0;
  limpiacad(art->desc, TRUE);

  art->pu = atof(campo[CAMPO_PU]->buf);
  art->exist = atof(campo[CAMPO_EXIS]->buf);
  art->disc = atof(campo[CAMPO_DISC]->buf);
  art->exist_min = atof(campo[CAMPO_EXMIN]->buf);
  art->exist_max = atof(campo[CAMPO_EXMAX]->buf);

  strcpy(str_prov, campo[CAMPO_CODPROV]->buf);
  limpiacad(str_prov, TRUE);
  aux = busca_proveedor(base_inv, str_prov);
  art->id_prov = aux<0 ? 0 : aux;

  strcpy(str_depto, campo[CAMPO_DEPTO]->buf);
  limpiacad(str_depto, TRUE);
  aux = busca_depto(base_inv, str_depto);
  art->id_depto = aux<0 ? 0 : aux;

  art->p_costo = atof(campo[CAMPO_PCOSTO]->buf);
  art->iva_porc = atof(campo[CAMPO_IVA]->buf);
  return(OK);
}


void modifica_articulo(FIELD *campo[25], PGconn *base, unsigned num_items)
{
  int    item;
  char   codigo[mxbuff];

  strcpy(codigo, campo[CAMPO_COD]->buf);
  if (codigo[strlen(codigo)-1] == '\n')
    codigo[strlen(codigo)-1] = 0;
  limpiacad(codigo, TRUE);
  item = busca_item(codigo, num_items);
  if (item<0) {
    beep();
    return;
  }
  if (!interpreta_campos(&art, campo))
    Modifica_en_Inventario(base, "articulos", art);
  modifica_item(art, item);
  muestra_renglon(item, num_items);
  }


void agrega_articulo(FIELD *campo[25], PGconn *base, unsigned *num_items)
{
  int    i;
  char   codigo[mxbuff];

  strcpy(codigo, campo[CAMPO_COD]->buf);
  if (codigo[strlen(codigo)-1] == '\n')
    codigo[strlen(codigo)-1] = 0;
  limpiacad(codigo, TRUE);
  i = busca_item(codigo, *num_items);
  if (i>=0) {
    beep();
    return;
  }
  if (!interpreta_campos(&art, campo))
    Agrega_en_Inventario(base, "articulos", art);
  item[(*num_items)] = calloc(1, getmaxx(stdscr));
  for (i=0; i<getmaxx(stdscr); item[*num_items][i++]=' ');
  modifica_item(art, *num_items);
  (*num_items)++;
  muestra_renglon(*num_items-1, *num_items);
}

PGresult *borra_articulo(PGconn *base, FIELD *campo, unsigned *num_items)
{
  char     *codigo;
  PGresult *res;
  int      item=-1;


  codigo = calloc(1,maxcod);

  strncpy(codigo, campo->buf, maxcod-1);
  codigo[maxcod-1] = 0;
  limpiacad(codigo, TRUE);

  res = Quita_de_Inventario(base, "articulos", codigo);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Falla al quitar un articulo de la base.\n");
    fprintf(stderr, "Error: %s\n",PQerrorMessage(base));
  }
  else {
    item = busca_item(codigo, *num_items);
    if (item >= 0) {
      if (!quita_renglon(item, *num_items))
        wclear(v_arts);
      (*num_items)--;
    }
  }
  free(codigo);
  codigo = NULL;
  if (item < 0)
    beep();
  return(res);
}


void muestra_renglon(unsigned renglon, unsigned num_items)
{
  int      i;

  wattrset(v_arts, COLOR_PAIR(normal));
  wclear(v_arts);
  for (i=0; i<num_items-1-renglon && i<v_arts->_maxy; i++)
    mvwprintw(v_arts, i+1, 0, "%s", item[renglon+i+1]);

  wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
  mvwprintw(v_arts, 0, 0, "%s", item[renglon]);
  refresh();
  wrefresh(v_arts);
}

int quita_renglon(unsigned renglon, unsigned num_items)
{
  int i;

  for (i=renglon; i<num_items-1; i++)
    strcpy(item[i], item[i+1]);
  free(item[i]);
  item[i] = NULL;
  if (renglon<num_items)
    muestra_renglon(renglon-(renglon==num_items-1), num_items-1);
  return(num_items-1);
}

int busca_articulo(FIELD *campo, unsigned num_items)
{
  int  item;
  char codigo[maxcod];

   strncpy(codigo, campo->buf, maxcod-1);
   codigo[maxcod-1] = 0;
   limpiacad(codigo, TRUE);
   item = busca_item(codigo, num_items);
   return(item);
}

int forma_articulo(WINDOW *v_forma, unsigned *num_items, PGconn *base)
{
  FORM  *forma;
  FIELD *campo[25];
  char  *etiqueta;
  char  *depto[maxdepto];
  char  **deptos = depto;
  char  *auxdepto;
  char  *prov[maxprov];
  char  **provs = prov;
  char  *auxprov;
  unsigned num_deptos;
  unsigned num_provs;
  int   finished = 0, c, i, j;
  int   tam_ren, tam_col, pos_ren, pos_col;
  PGresult *res;
  char  *comando;

  comando = "SELECT nombre from departamento ORDER BY id ASC";
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
   fprintf(stderr,"Falló comando SELECT al buscar departamento en inventario\n");
   sleep(5);
   PQclear(res);
   return(ERROR_SQL);
  }

  auxdepto = calloc(1, maxdeptolen+1);
  num_deptos = PQntuples(res);
  for (i=0; i<num_deptos; i++) {
    strncpy(auxdepto, PQgetvalue(res, i, 0), maxdeptolen);
    depto[i] = calloc(1, strlen(auxdepto)+1);
    if (depto[i] == NULL) {
      free(auxdepto);
      return(ERROR_MEMORIA);
    }
    strncpy(depto[i], auxdepto, strlen(auxdepto));
  }
  free(auxdepto);
  PQclear(res);
  depto[i] = NULL;

  comando = "SELECT nick from proveedores ORDER BY id ASC";
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
   fprintf(stderr,"Falló comando %s\n", comando);
   sleep(5);
   return(ERROR_SQL);
   PQclear(res);
  }

  auxprov = calloc(1, maxnickprov+1);
  num_provs = PQntuples(res);
  for (i=0; i<num_provs; i++) {
    strncpy(auxprov, PQgetvalue(res, i, 0), maxnickprov);
    prov[i] = calloc(1, strlen(auxprov)+1);
    if (prov[i] == NULL) {
      free(auxprov);
      return(ERROR_MEMORIA);
    }
    strncpy(prov[i], auxprov, strlen(auxprov));
  }
  PQclear(res);
  free(auxprov);
  prov[i] = NULL;

  pos_ren = 0;
  pos_col = 0;
  etiqueta = "Introduzca los articulos";

  /* describe la forma */
  campo [0] = CreaEtiqueta(1, 0, "Codigo");
  campo [CAMPO_COD] = CreaCampo(2, 0, 1, maxcod);
  campo [2] = CreaEtiqueta(1, maxcod+1, "Descripcion");
  campo [CAMPO_DESCR] = CreaCampo(2, maxcod+1, 1, maxdes);
  campo [4] = CreaEtiqueta(1, maxcod+maxdes+2, "P. U.");
  campo [CAMPO_PU] = CreaCampo(2, maxcod+maxdes+2, 1, maxpreciolong);
  campo [6] = CreaEtiqueta(1, maxcod+maxdes+maxpreciolong+3, "Dscto");
  campo [CAMPO_DISC] = CreaCampo(2, maxcod+maxdes+maxpreciolong+3, 1, maxdisclong);
  campo [8] = CreaEtiqueta(0, 6, etiqueta);
  campo [9] = CreaEtiqueta(3, 0, "Exis");
  campo[CAMPO_EXIS] = CreaCampo(4, 0, 1, maxexistlong);
  campo[11] = CreaEtiqueta(3, maxexistlong+1, "Ex min");
  campo[CAMPO_EXMIN] = CreaCampo(4, maxexistlong+2, 1, maxexistlong);
  campo[13] = CreaEtiqueta(3, maxexistlong+maxexistlong+4, "Ex max");
  campo[CAMPO_EXMAX] = CreaCampo(4, maxexistlong+maxexistlong+5, 1, maxexistlong);
  campo[15] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+7, "Codigo proveedor");
  campo[CAMPO_CODPROV] = CreaCampo(4, maxexistlong*2+maxexistlong+7, 1, maxcod);
  campo[17] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+maxcod+8,
              "Departamento");
  campo[CAMPO_DEPTO] = CreaCampo(4, maxexistlong*2+maxexistlong+maxcod+8, 1, maxcod);
  campo[19] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+maxcod*2+9,
              "P. Costo");
  campo[CAMPO_PCOSTO] = CreaCampo(4, maxexistlong*2+maxexistlong+maxcod*2+9,
                                  1, maxpreciolong);
  campo[21] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+maxcod*2+maxpreciolong+10, "IVA");
  campo[CAMPO_IVA] = CreaCampo(4, maxexistlong*2+maxexistlong+maxcod*2+maxpreciolong+10,
                               1, maxdisclong);
  campo[23] = CreaEtiqueta(4, maxexistlong*2+maxexistlong+maxcod*2+maxpreciolong+maxdisclong+10, "%");
  campo[24] = (FIELD *)0;

  set_field_pad(campo[CAMPO_COD], 0);
  set_field_pad(campo[CAMPO_DESCR], 0);
  set_field_pad(campo[CAMPO_CODPROV], 0);
  set_field_type(campo[CAMPO_PU], TYPE_NUMERIC, 2, 0, MAXDOUBLE);
  set_field_type(campo[CAMPO_DISC], TYPE_NUMERIC, 0, 0, MAXDOUBLE);
  set_field_type(campo[CAMPO_EXIS], TYPE_NUMERIC, 0, 0, 0, MAXDOUBLE);
  set_field_type(campo[CAMPO_EXMIN], TYPE_NUMERIC, 0, 0, 0, MAXDOUBLE);
  set_field_type(campo[CAMPO_EXMAX], TYPE_NUMERIC, 0, 0, 0, MAXDOUBLE); 
  set_field_type(campo[CAMPO_PCOSTO], TYPE_NUMERIC, 0, 0, MAXDOUBLE);
  set_field_type(campo[CAMPO_CODPROV], TYPE_ENUM, provs, TRUE, FALSE);
  set_field_type(campo[CAMPO_DEPTO], TYPE_ENUM, deptos, TRUE, FALSE);
  set_field_type(campo[CAMPO_IVA], TYPE_NUMERIC, 2, 0, MAXDOUBLE);
  forma = new_form(campo);

  scale_form(forma, &tam_ren, &tam_col);
  campo[8]->fcol = (unsigned) ((tam_col - strlen(etiqueta)) / 2);

  /* Muestra la lista inicial */
  for (i=1; i<=(v_arts->_maxy) && i<*num_items; i++) {
    mvwprintw(v_arts, i, 0, "%s", item[i]);
  }

  MuestraForma(forma, pos_ren, pos_col);
  v_forma = form_win(forma);
  raw();
  noecho();

  /* int form_driver(FORM forma, int cod) */
  /* acepta el código cod, el cual indica la acción a tomar en la forma */
  /* hay algunos codigos en la función form_virtualize(WINDOW w) */

  i = 0;
  wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
  mvwprintw(v_arts, 0, 0, "%s", item[0]);
  wmove(v_arts, 0, 0);
  wrefresh(v_arts);

  while (!finished)
  {
    switch(form_driver(forma, c = form_virtualize(v_forma))) {
    case E_OK:
      break;
    case E_UNKNOWN_COMMAND:
      switch (c) {
      case CTRL('A'):
        /* Añade artículo */
        agrega_articulo(campo, base, num_items);
        i = *num_items-1;
        break;
      case CTRL('B'):
        /* Busca artículo */
        c = busca_articulo(campo[CAMPO_COD], *num_items);
        if (c<0) {
          beep();
          continue;
        }
        i = c;
        muestra_renglon(i, *num_items);
      case ESCAPE:  /* Alt-Enter en PC */
        llena_campos(campo, i, base);
        MuestraForma(forma, pos_ren, pos_col);
        break;
      case CTRL('D'):
        modifica_articulo(campo, base, *num_items);
        break;
      case CTRL('R'):
        imprime_lista(base, "codigo");
        break;
      case CTRL('U'):
        borra_articulo(base, campo[CAMPO_COD], num_items);
        if (i==*num_items)
          i--;
        break;
      case KEY_PPAGE:
        if (i < 10) {
          beep();
          continue;
        }
        wattrset(v_arts, COLOR_PAIR(normal));
        mvwprintw(v_arts, v_arts->_cury, 0, "%s", item[i]);
        i -= 10;
        if (v_arts->_cury < 10) {
          wscrl(v_arts,  v_arts->_cury - 10);
          for (j=10-v_arts->_cury;  j > 0;  j--)
            mvwprintw(v_arts, j, 0, "%s", item[i+j]);
          wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
          mvwprintw(v_arts, j, 0, "%s", item[i]);
        }
        else {
          wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
          mvwprintw(v_arts, v_arts->_cury-10, 0, "%s", item[i]);
        }
        wrefresh(v_arts);
        break;
      case KEY_UP:
        if (field_index(current_field(forma)) == CAMPO_CODPROV  ||
            field_index(current_field(forma)) == CAMPO_DEPTO) {
          form_driver(forma, REQ_PREV_CHOICE);
          MuestraForma(forma, pos_ren, pos_col);
        }
        else {
          if (i <= 0) {
            beep();
            continue;
          }
          wattrset(v_arts, COLOR_PAIR(normal));
          mvwprintw(v_arts, v_arts->_cury, 0, "%s", item[i--]);
          if (v_arts->_cury == 0) {
            wscrl(v_arts, -1);
            wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_arts, 0, 0, "%s", item[i]);
          }
          else {
            wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_arts, v_arts->_cury-1, 0, "%s", item[i]);
          }
          wrefresh(v_arts);
        }
        break;
      case KEY_NPAGE:
        if (i+10 >= *num_items) {
          beep();
          continue;
        }
        wattrset(v_arts, COLOR_PAIR(normal));
        mvwprintw(v_arts, v_arts->_cury, 0, "%s", item[i]);
        i += 10;
        if (v_arts->_cury+10 > v_arts->_maxy) {
          c =  10 - v_arts->_maxy+v_arts->_cury; /* lineas a desplazar */
          wscrl(v_arts,  c);
          for (j=0; j<c;  j++)
            mvwprintw(v_arts, v_arts->_maxy-c+j, 0, "%s", item[i-c+j]);
          wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
          mvwprintw(v_arts, v_arts->_maxy, 0, "%s", item[i]);
        }
        else {
          wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
          mvwprintw(v_arts, v_arts->_cury+10, 0, "%s", item[i]);
        }
        wrefresh(v_arts);
        break;
      case KEY_DOWN:
        if (field_index(current_field(forma)) == CAMPO_CODPROV  ||
            field_index(current_field(forma)) == CAMPO_DEPTO) {
          form_driver(forma, REQ_NEXT_CHOICE);
          MuestraForma(forma, pos_ren, pos_col);
        }
        else {
          if (i+1 >= *num_items) {
            beep();
            continue;
          }
          wattrset(v_arts, COLOR_PAIR(normal));
          mvwprintw(v_arts, v_arts->_cury, 0, "%s", item[i++]);
          if (v_arts->_cury == v_arts->_maxy) {
            wscrl(v_arts, +1);
            wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_arts, v_arts->_maxy, 0, "%s", item[i]);
          }
          else {
            wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_arts, v_arts->_cury+1, 0, "%s", item[i]);
          }        
        }
        wrefresh(v_arts);
        break;
      default:
        finished = my_form_driver(forma, c);
      }
      break;
    default:
      
      beep();
      break;
    }
  }

  for (i=0; i < num_deptos;  i++)
    free(depto[i]);
  BorraForma(forma);

  free_form(forma);
  for (c = 0; campo[c] != 0; c++)
      free_field(campo[c]);
  noraw();
  echo();

  attrset(COLOR_PAIR(normal));
  clrtobot();
  raw();
  return(OK);
}


int imprime_lista(PGconn *con, char *campo_orden)
{

  char *comando;
  char *datos[numdat];
  int  i,j;
  int   nCampos;
  PGresult* res;
  FILE *disp;        /* dispositivo de impresión */

  res = PQexec(con,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr, "falló comando BEGIN\n");
   PQclear(res);
   return(ERROR_SQL);
  }
  PQclear(res);

  comando = calloc(1, mxbuff);
  if (comando == NULL)
    return(ERROR_MEMORIA);

  sprintf(comando, "DECLARE cursor_arts CURSOR FOR SELECT * FROM articulos ORDER BY \"%s\"",
	  campo_orden);
  res = PQexec(con, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "comando DECLARE CURSOR falló\n");
    fprintf(stderr, "Error: %s\n",PQerrorMessage(con));
    PQclear(res);
    free(comando);
    return(ERROR_SQL);
  }
  PQclear(res);

  sprintf(comando, "FETCH ALL in cursor_arts");
  res = PQexec(con, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "comando FETCH ALL no regresó registros apropiadamente\n");
    PQclear(res);
    free(comando);
    return(ERROR_SQL);
  }

  nCampos = PQnfields(res);


    /* print out the instances */
    for (i=0, disp = fopen(nmdisp, "a"); i < PQntuples(res); i++) {
      for (j=0  ; j < nCampos; j++) {
        datos[j] = PQgetvalue(res,i,j);
      }
      imprime_renglon(disp, datos);
    }

  PQclear(res);
  fclose(disp);
  free(comando);

  res = PQexec(con, "CLOSE cursor_arts");
  PQclear(res);

  res = PQexec(con, "END");
  PQclear(res);
  return(0);
}



int main() {
  int    i,
         num_items;
  WINDOW *v_forma;
/*         *v_mensaje; */

  read_config();

  initscr();
  start_color();
  init_pair(amarillo_sobre_azul, COLOR_YELLOW, COLOR_BLUE);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(normal, COLOR_WHITE, COLOR_BLACK);

  base_inv = Abre_Base(NULL, NULL, NULL, NULL, nminvent, "scaja", "");
  if (PQstatus(base_inv) == CONNECTION_BAD) {
    printw("FATAL: Falló la conexión a la base de datos %s\n", nminvent);
    printw("Presione <Intro> para terminar...");
    getch();
    endwin();
    exit(ERROR_SQL);
  }

/*  v_mensaje = newwin(LINES-2, COLS-1, LINES-5, 0);
  scrollok(v_mensaje, FALSE);
  mvwprintw(v_mensaje,0,0, "<Ctrl-A>Agrega  <Ctrl-B>Busca  ");
  wprintw(v_mensaje, "<Ctrl-D>moDifica  <Ctrl-Q>Termina");
  wrefresh(v_mensaje); */
  mvprintw(getmaxy(stdscr)-2, 0, "Ctrl: A-Agrega B-Busca ");
  printw("D-moDifica Q-termina R-impRime U-qUita <Alt-Enter>Muestra");

  num_items = inicializa_lista(base_inv, "codigo");

  if (num_items<0) {
    mvprintw(getmaxy(stdscr)/2, 0, "Error al incializar la lista de artículos");
    mvprintw(getmaxy(stdscr)-2, 0, "Pulse una tecla para salir...");
    clrtoeol();
    getch();
    clear();
    endwin();
    fprintf(stderr, "Error al incializar la lista de artículos\n");
    exit(ERROR_SQL);
  }
  ajusta_ventana_forma();
  forma_articulo(v_forma, &num_items, base_inv);

  delwin(v_arts);
  clear();
  refresh();
  endwin();
  for (i=0; i<num_items; i++)
    free(item[i]);
  free(nmdisp);
  free(nminvent);

  return(OK);
}

/* Pendientes:
- Hacer que despliegue un mensaje de error al no poder abrir un
  archivo
- Hacer que haga un refresh del renglon al actualizar un articulo, en caso de que
  sea representado en la pantalla al momento de la modificaciòn
*/

/* BUGS:
- Al solicitar elementos de campos enumerados que eastn fuera de los limites, no muestra
  nada ni muestra de nuevo las opciones (con las flechas)
- Al quitar el ultimo articulo de la lista, y al volverlo a agregar, no despliega en la lista
  al penultimo elemento (se lo come)
*/

/*
int scroll(win)
 This function will scroll up the window (and the lines in the data structure) one 
 line.

int scrl(n)
 int wscrl(win, n)
 These functions will scroll the window or win up or down depending on the value of the
 integer n. If n is positive the window will be scrolled up n lines, otherwise if n is
 negative the window will be scrolled down n lines.

int setscrreg(t, b)
 int wsetscrreg(win, t, b)
 Set a software scrolling region.

*/
