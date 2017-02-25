/*    -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

OsoPOS - Programa de inventario 1999,2003 E. Israel Osorio 
   soporte@elpuntodeventa.com

     Este programa es un software libre; puede usted redistribuirlo y/o
modificarlo de acuerdo con los t�rminos de la Licencia P�blica General GNU
publicada por la Free Software Foundation: ya sea en la versi�n 2 de la
Licencia, o (a su elecci�n) en una versi�n posterior.

     Este programa es distribuido con la esperanza de que sea �til, pero
SIN GARANTIA ALGUNA; incluso sin la garant�a impl�cita de COMERCIABILIDAD o
DE ADECUACION A UN PROPOSITO PARTICULAR. V�ase la Licencia P�blica General
GNU para mayores detalles.

     Deber�a usted haber recibido una copia de la Licencia P�blica General
GNU junto con este programa; de no ser as�, escriba a Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA02139, USA.
*/



#include <stdio.h>
#include <values.h>
#include <unistd.h>
#ifndef _pos
#include "include/pos-curses.h"
#define _pos
#endif
#include "include/print-func.h"

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
#define CAMPO_COD2           25
#define CAMPO_DESCR           3
#define CAMPO_PU              5
#define CAMPO_PU2            27 
#define CAMPO_PU3            29 
#define CAMPO_PU4            31 
#define CAMPO_PU5            33
#define CAMPO_DIVISA         35
#define CAMPO_EXIS           10
#define CAMPO_DISC            7
#define CAMPO_EXMIN          12
#define CAMPO_EXMAX          14
#define CAMPO_CODPROV        16
#define CAMPO_DEPTO          18
#define CAMPO_PCOSTO         20
#define CAMPO_IVA            22

#define version "0.33-1"

#define maxitemlista    4096

WINDOW *v_arts;
char   *nminvent;
char   *nmdisp;
char   *tipo_imp;
char   *lp_printer;
char   *home_directory; 
unsigned almacen;
struct db_data db;
char   *item[maxitemlista];
struct articulos art;
PGconn *con, *con_s;

int    iva_incluido;

int read_config();
int imprime_lista(PGconn *con, char *campo_orden);

void muestra_renglon(unsigned renglon, unsigned num_items);
int quita_renglon(unsigned renglon, unsigned num_items);

int form_virtualize(WINDOW *w);
//FIELD *CreaEtiqueta(int pren, int pcol, NCURSES_CONST char *etiqueta);

int inicializa_lista(PGconn *base, char *campo_orden);

/* Muestra el detalle del art�culo */
int fill_form(FIELD *campo[35], unsigned i, PGconn *base);

/***************************************************************************/

int init_config()
{
  FILE *env_process;
  char *log_name;

  home_directory = calloc(1, 255);
  log_name = calloc(1, 255);

  if (!(env_process = popen("printenv HOME", "r"))) {
    free(log_name);
    free(home_directory);
    return(PROCESS_ERROR);
  }
  fgets(home_directory, 255, env_process);
  home_directory[strlen(home_directory)-1] = 0;
  pclose(env_process);

  if (!(env_process = popen("printenv LOGNAME", "r"))) {
    free(log_name);
    free(home_directory);
    return(PROCESS_ERROR);
  }
  fgets(log_name, 255, env_process);
  log_name[strlen(log_name)-1] = 0;
  pclose(env_process);

  
  sprintf(tipo_imp,"EPSON");

  strncpy(lp_printer, "facturas", maxbuf);


  db.name= NULL;
  db.user = NULL;
  db.passwd = NULL;
  db.sup_user = NULL;
  db.sup_passwd = NULL;
  db.hostport = NULL;
  db.hostname = NULL;

  db.hostname = calloc(1, strlen("255.255.255.255"));
  db.name = calloc(1, strlen("elpuntodeventa.com"));
  db.user = calloc(1, strlen(log_name)+1);
  strcpy(db.user, log_name);

  db.sup_user = calloc(1, strlen("scaja")+1);
  return(0);
}

/***************************************************************************/

int read_general_config()
{
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;

  
  nmconfig = calloc(1, 255);
  strncpy(nmconfig, "/etc/osopos/general.config", 255);

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuraci�n */
    b = buff;
    fgets(buff,mxbuff,config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;
      if (!strlen(buff) || buff[0] == '#') {
        if (!feof(config))
          fgets(buff,mxbuff,config);
        continue;
      }
      strncpy(buf, strtok(buff,"="), mxbuff);
        /* La funci�n strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

      if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.port")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostport, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostport,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.nombre")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.name, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.name,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.sup_usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.sup_user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.sup_user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.sup_passwd")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_passwd = calloc(1, strlen(buf)+1);
        if (db.sup_passwd  != NULL) {
          strcpy(db.sup_passwd,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"porcentaje_iva")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        TAX_PERC_DEF = atoi(buf);
      }
      else if (!strcmp(b,"iva_incluido")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        iva_incluido = atoi(buf);
      }
      if (!feof(config))
        fgets(buff,mxbuff,config);
    }
  
    fclose(config);
    if (aux != NULL)
      aux = NULL;
    free(nmconfig);
    b = NULL;
    return(0);
  }
  if (aux != NULL)
    aux = NULL;
  free(nmconfig);
  b = NULL;
  return(1);
}

int read_config() {
  char *nmconfig;
  FILE *config;
  char buff[mxbuff], buf[mxbuff];
  char *b;
  char *aux = NULL;
  int i;

  for (i=0; i<mxbuff; buff[i]=0, buf[i++]=0);
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
  if (config) {         /* Si existe archivo de configuraci�n */
    b = buff;
    fgets(buff,sizeof(buff),config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;

      if (!strlen(buff) || buff[0] == '#') { /* Linea vac�a o coment. */
        fgets(buff,sizeof(buff),config);
        continue;
      }

      strncpy(buf, strtok(buff,"="), mxbuff);
        /* La funci�n strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

        /* Busca par�metros de operaci�n */
      if (!strcmp(b,"base.datos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nminvent, strlen(buf+1));
        if (aux != NULL)
          strncpy(nminvent,buf, mxbuff);
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"impresion.salida")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmdisp, strlen(buf+1));
        if (aux != NULL)
          strncpy(nmdisp, buf, strlen(buf));
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL)
          strncpy(db.hostname,buf, strlen(buf));
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.port")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostport, strlen(buf)+1);
        if (aux != NULL)
          strncpy(db.hostport,buf, strlen(buf));
        else
          fprintf(stderr, "invent. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      fgets(buff,sizeof(buff),config);
    }
    fclose(config);
    return(OK);
  }
  else {
    fprintf(stderr, "No encuentro el archivo de configuraci�n en %s,\n",
            nmconfig);
    fprintf(stderr, "usando datos por omisi�n...\n");
  }
  return(1);
}


int inicializa_lista(PGconn *base, char *campo_orden)
{
  int       i, j;
  PGresult  *res;
  char *comando;
  char str_aux[255];

  v_arts =  newwin(getmaxy(stdscr)-12,getmaxx(stdscr), 9,0);
/* Los par�metros anteriores causan un SIGSEGV y los posteriores no */
/*  v_arts = newwin (LINES-4, COLS-1, 4,0); */
  scrollok(v_arts, TRUE);

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr,"Fall� comando BEGIN al buscar en inventario\n");
   return(ERROR_SQL);
  }
  PQclear(res);

  comando = calloc(1,mxbuff);
  snprintf(comando, mxbuff,
      "DECLARE cursor_arts CURSOR FOR SELECT codigo,descripcion,pu,cant FROM articulos ORDER BY \"%s\"",
	campo_orden);
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Fall� comando DECLARE CURSOR al buscar art�culos\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    free(comando);
    return(ERROR_SQL);
  }
  PQclear(res);

  strcpy(comando, "FETCH ALL in cursor_arts");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regres� registros apropiadamente\n");
    free(comando);
    return(ERROR_SQL);
  }

  /*nCampos = PQnfields(res); */

/*  strcpy(art.codigo,PQgetvalue(res,registro,campo));  */

  for (j=0; j<getmaxx(stdscr) && j<254; j++)
    str_aux[j] = ' ';
  str_aux[j] = 0;

  for (i=0; i<PQntuples(res) && i<maxitemlista; i++) {
    item[i] = calloc(1,getmaxx(stdscr));
    strcpy(item[i], str_aux);

    if (maxdes+maxcod+maxpreciolong+maxexistlong+3 > getmaxx(stdscr)-1) {
       /* C�digo */
      strncpy(comando, PQgetvalue(res,i,0), maxcod);
      if (strlen(comando)>16)
        comando[16] = 0;
      memcpy(&item[i][0], comando, strlen(comando));

       /* Descripci�n */
      strncpy(comando, PQgetvalue(res,i,1), maxdes);
      if (strlen(comando)>31)
        comando[31] = 0;
      memcpy(&item[i][16], comando, strlen(comando));

      /* Precio */
      sprintf(comando, "%10.2f", atof(PQgetvalue(res,i,2)) );
      memcpy(&item[i][16+31], comando, strlen(comando));

      /* Existencia */
      sprintf(comando, "%4.2f", atof(PQgetvalue(res,i,3)) );
      memcpy(&item[i][16+31+maxpreciolong+1], comando, strlen(comando));
    }
    else {
      strncpy(comando, PQgetvalue(res,i,0), maxcod);
      if (strlen(comando)>maxcod)
        comando[maxcod-1] = 0;
      memcpy(&item[i][0], comando, strlen(comando));

      strncpy(comando, PQgetvalue(res,i,1), maxdes);
      if (strlen(comando)>maxdes)
        comando[maxdes-1] = 0;
      memcpy(&item[i][maxcod+1], comando, strlen(comando));

      sprintf(comando, "%10.2f", atof(PQgetvalue(res,i,2)) );
      memcpy(&item[i][maxcod+maxdes+2], comando, strlen(comando));

      sprintf(comando, "%4.2f", atof(PQgetvalue(res,i,3)) );
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
       /* C�digo */
      strncpy(aux, art.codigo, maxcod);
      if (strlen(aux)>16)
        aux[16] = 0;
      memcpy(&item[i][0], aux, strlen(aux));

       /* Descripci�n */
      strncpy(aux, art.desc, maxdes);
      if (strlen(aux)>31)
        aux[31] = 0;
      memcpy(&item[i][16], aux, strlen(aux));

      /* Precio */
      sprintf(aux, "%10.2f", art.pu);
      memcpy(&item[i][16+31], aux, strlen(aux));

      /* Existencia */
      sprintf(aux, "%-2.1f", art.exist );
      memcpy(&item[i][16+31+maxpreciolong+1], aux, strlen(aux));
    }
    else {
      strncpy(aux, art.codigo, maxcod);
      if (strlen(aux)>maxcod)
        aux[maxcod-1] = 0;
      memcpy(&item[i][0], aux, strlen(aux));

      strncpy(aux, art.desc, maxdes);
      if (strlen(aux)>maxdes)
        aux[maxdes-1] = 0;
      memcpy(&item[i][maxcod+1], aux, strlen(aux));

      sprintf(aux, "%10.2f", art.pu );
      memcpy(&item[i][maxcod+maxdes+2], aux, strlen(aux));

      sprintf(aux, "%-2.1f", art.exist );
      memcpy(&item[i][maxpreciolong+maxdes+maxcod+3], aux, strlen(aux));
    }
    item[i][getmaxx(stdscr)-1] = 0;

}


/*FIELD *CreaEtiqueta(int pren, int pcol, NCURSES_CONST char *etiqueta)
{
    FIELD *f = new_field(1, strlen(etiqueta), pren, pcol, 0, 0);

    if (f)
    {
        set_field_buffer(f, 0, etiqueta);
        set_field_opts(f, field_opts(f) & ~O_ACTIVE);
    }
    return(f);
}*/

/* funciones agregadas a pos-curses.h 
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
*/

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

int fill_form(FIELD *campo[35], unsigned i, PGconn *base)
{
  PGresult           *res;
  char               codigo[maxcod];
  char               *aux;
  char               *comando;
  struct articulos   art;
  int                j;

  for (j=0; j<maxcod; codigo[j++] = 0);
  strncpy(codigo, item[i], maxcod-1);
  codigo[maxcod-1] = 0;
  limpiacad(codigo, TRUE);
  comando = calloc(1, mxbuff);

  res = search_product(base, "articulos", "codigo", codigo, TRUE, &art);
  if (res == NULL) {
    aux = calloc(1, mxbuff);

    set_field_buffer(campo[CAMPO_COD], 0, codigo);
    set_field_buffer(campo[CAMPO_COD2], 0, art.codigo2);
    set_field_buffer(campo[CAMPO_DESCR], 0, art.desc);

    sprintf(aux, "%.2f", art.pu);
    set_field_buffer(campo[CAMPO_PU], 0, aux);
    sprintf(aux, "%.2f", art.pu2);
    set_field_buffer(campo[CAMPO_PU2], 0, aux);
    sprintf(aux, "%.2f", art.pu3);
    set_field_buffer(campo[CAMPO_PU3], 0, aux);
    sprintf(aux, "%.2f", art.pu4);
    set_field_buffer(campo[CAMPO_PU4], 0, aux);
    sprintf(aux, "%.2f", art.pu5);
    set_field_buffer(campo[CAMPO_PU5], 0, aux);

    set_field_buffer(campo[CAMPO_DIVISA], 0, art.divisa);

    sprintf(aux, "%f", art.exist);
    set_field_buffer(campo[CAMPO_EXIS], 0, aux);

    sprintf(aux, "%.2f", art.disc);
    set_field_buffer(campo[CAMPO_DISC], 0, aux);

    sprintf(aux, "%f", art.exist_min);
    set_field_buffer(campo[CAMPO_EXMIN], 0, aux);

    sprintf(aux, "%f", art.exist_max);
    set_field_buffer(campo[CAMPO_EXMAX], 0, aux);

    sprintf(comando, "SELECT nick FROM proveedores WHERE id=%d",
            art.id_prov);
    res = PQexec(base, comando);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr,"Fall� comando %s\n", comando);
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
      fprintf(stderr,"Fall� comando %s\n", comando);
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

int interpreta_campos(struct articulos *art, FIELD *campo[35])
{
  int aux;
  char str_depto[maxdeptolen+1];
  char str_prov[maxnickprov+1];

  for (aux=0; aux<=maxnickprov; str_prov[aux++] = 0);
  for (aux=0; aux<=maxdeptolen; str_depto[aux++] = 0);

  strncpy(art->codigo, campo[CAMPO_COD]->buf, maxcod-1);
  art->codigo[maxcod-1] = 0;
  limpiacad(art->codigo, TRUE);

  strncpy(art->codigo2, campo[CAMPO_COD2]->buf, maxcod-1);
  art->codigo2[maxcod-1] = 0;
  limpiacad(art->codigo2, TRUE);

  strncpy(art->desc, campo[CAMPO_DESCR]->buf, maxdes);
  art->desc[maxdes-1] = 0;
  limpiacad(art->desc, TRUE);

  art->pu = atof(campo[CAMPO_PU]->buf);
  art->pu2 = atof(campo[CAMPO_PU2]->buf);
  art->pu3 = atof(campo[CAMPO_PU3]->buf);
  art->pu4 = atof(campo[CAMPO_PU4]->buf);
  art->pu5 = atof(campo[CAMPO_PU5]->buf);

  strncpy(art->divisa, campo[CAMPO_DIVISA]->buf, 3);
  //  art->desc[3] = 0;

  art->exist = atof(campo[CAMPO_EXIS]->buf);
  art->disc = atof(campo[CAMPO_DISC]->buf);
  art->exist_min = atof(campo[CAMPO_EXMIN]->buf);
  art->exist_max = atof(campo[CAMPO_EXMAX]->buf);

  strncpy(str_prov, campo[CAMPO_CODPROV]->buf, maxnickprov);
  limpiacad(str_prov, TRUE);
  aux = busca_proveedor(con_s, str_prov);
  art->id_prov = aux<0 ? 0 : aux;

  strncpy(str_depto, campo[CAMPO_DEPTO]->buf, maxdeptolen);
  limpiacad(str_depto, TRUE);
  aux = busca_depto(con, str_depto);
  art->id_depto = aux<0 ? 0 : aux;

  art->p_costo = atof(campo[CAMPO_PCOSTO]->buf);
  art->iva_porc = atof(campo[CAMPO_IVA]->buf);
  return(OK);
}


void modifica_articulo(FIELD *campo[35], PGconn *base, unsigned num_items)
{
  int    item;
  char   codigo[mxbuff];

  for (item=0; item<mxbuff; codigo[item++] = 0);
  strncpy(codigo, campo[CAMPO_COD]->buf, maxcod);
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


void agrega_articulo(FIELD *campo[35], PGconn *base, unsigned *num_items)
{
  int    i;
  char   codigo[mxbuff];

  for (i=0; i<mxbuff; codigo[i++]=0);
  strncpy(codigo, campo[CAMPO_COD]->buf, maxcod);
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

  for (item=0; item<maxcod; codigo[item++] = 0);
   strncpy(codigo, campo->buf, maxcod-1);
   codigo[maxcod-1] = 0;
   limpiacad(codigo, TRUE);
   item = busca_item(codigo, num_items);
   return(item);
}

int forma_articulo(WINDOW *v_forma, unsigned *num_items, PGconn *base)
{
  FORM  *forma;
  FIELD *campo[35];
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
   fprintf(stderr,"Fall� comando SELECT al buscar departamento en inventario\n");
   sleep(5);
   PQclear(res);
   return(ERROR_SQL);
  }

  auxdepto = calloc(1, maxdeptolen+1);
  if (auxdepto == NULL)
    return(ERROR_MEMORIA);
  num_deptos = PQntuples(res);
  for (i=0; i<num_deptos && i<maxdepto; i++) {
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
   fprintf(stderr,"Fall� comando %s\n", comando);
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
  campo [CAMPO_COD-1] = CreaEtiqueta(1, 0, "Codigo");
  campo [CAMPO_COD] = CreaCampo(2, 0, 1, maxcod, amarillo_sobre_azul);
  campo [CAMPO_DESCR-1] = CreaEtiqueta(1, maxcod+1, "Descripcion");
  campo [CAMPO_DESCR] = CreaCampo(2, maxcod+1, 1, maxdes, amarillo_sobre_azul);
  campo [4] = CreaEtiqueta(1, maxcod+maxdes+2, "P. U.");
  campo [CAMPO_PU] = CreaCampo(2, maxcod+maxdes+2, 1, maxpreciolong, amarillo_sobre_azul);
  campo [6] = CreaEtiqueta(1, maxcod+maxdes+maxpreciolong+3, "Dscto");
  campo [CAMPO_DISC] = CreaCampo(2, maxcod+maxdes+maxpreciolong+3, 1, maxdisclong, amarillo_sobre_azul);
  campo [8] = CreaEtiqueta(0, 6, etiqueta);
  campo [9] = CreaEtiqueta(3, 0, "Exis");
  campo[CAMPO_EXIS] = CreaCampo(4, 0, 1, maxexistlong, amarillo_sobre_azul);
  campo[11] = CreaEtiqueta(3, maxexistlong+1, "Ex min");
  campo[CAMPO_EXMIN] = CreaCampo(4, maxexistlong+2, 1, maxexistlong, amarillo_sobre_azul);
  campo[13] = CreaEtiqueta(3, maxexistlong+maxexistlong+4, "Ex max");
  campo[CAMPO_EXMAX] = CreaCampo(4, maxexistlong+maxexistlong+5, 1, maxexistlong, amarillo_sobre_azul);
  campo[15] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+7, "Codigo proveedor");
  campo[CAMPO_CODPROV] = CreaCampo(4, maxexistlong*2+maxexistlong+7, 1, maxcod, amarillo_sobre_azul);
  campo[17] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+maxcod+8,
              "Departamento");
  campo[CAMPO_DEPTO] = CreaCampo(4, maxexistlong*2+maxexistlong+maxcod+8, 1, maxcod, amarillo_sobre_azul);
  campo[19] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+maxcod*2+9,
              "P. Costo");
  campo[CAMPO_PCOSTO] = CreaCampo(4, maxexistlong*2+maxexistlong+maxcod*2+9,
                                  1, maxpreciolong, amarillo_sobre_azul);
  campo[21] = CreaEtiqueta(3, maxexistlong*2+maxexistlong+maxcod*2+maxpreciolong+10, "IVA");
  campo[CAMPO_IVA] = CreaCampo(4, maxexistlong*2+maxexistlong+maxcod*2+maxpreciolong+10,
                               1, maxdisclong, amarillo_sobre_azul);
  campo[23] = CreaEtiqueta(4, maxexistlong*2+maxexistlong+maxcod*2+maxpreciolong+maxdisclong+10, "%");
  campo[CAMPO_COD2-1] = CreaEtiqueta(5, 0, "Cod. alt.");
  campo[CAMPO_COD2] = CreaCampo(6, 0, 1, maxcod, amarillo_sobre_azul);
  campo[CAMPO_PU2-1] = CreaEtiqueta(5, 21, "Precio 2");
  campo[CAMPO_PU2] = CreaCampo(6, 21, 1, maxpreciolong, amarillo_sobre_azul);
  campo[CAMPO_PU3-1] = CreaEtiqueta(5, 22+maxpreciolong, "Precio 3");
  campo[CAMPO_PU3] = CreaCampo(6, 22+maxpreciolong, 1, maxpreciolong, amarillo_sobre_azul);
  campo[CAMPO_PU4-1] = CreaEtiqueta(5, 23+maxpreciolong*2, "Precio 4");
  campo[CAMPO_PU4] = CreaCampo(6, 23+maxpreciolong*2, 1, maxpreciolong, amarillo_sobre_azul);
  campo[CAMPO_PU5-1] = CreaEtiqueta(5, 24+maxpreciolong*3, "Precio 5");
  campo[CAMPO_PU5] = CreaCampo(6, 24+maxpreciolong*3, 1, maxpreciolong, amarillo_sobre_azul);
  campo[CAMPO_DIVISA-1] = CreaEtiqueta(5, 25+maxpreciolong*4, "Div");
  campo[CAMPO_DIVISA] = CreaCampo(6, 25+maxpreciolong*4, 1, 3, amarillo_sobre_azul);
  campo[36] = (FIELD *)0;

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
  /* acepta el c�digo cod, el cual indica la acci�n a tomar en la forma */
  /* hay algunos codigos en la funci�n form_virtualize(WINDOW w) */

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
        /* A�ade art�culo */
        agrega_articulo(campo, base, num_items);
        i = *num_items-1;
        break;
      case CTRL('B'):
        /* Busca art�culo */
        c = busca_articulo(campo[CAMPO_COD], *num_items);
        if (c<0) {
          beep();
          continue;
        }
        i = c;
        muestra_renglon(i, *num_items);
      case ESCAPE:  /* Alt-Enter en PC */
        fill_form(campo, i, base);
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
  for (i=0; i < num_provs;  i++)
    free(prov[i]);

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
  FILE *disp;        /* dispositivo de impresi�n */

  res = PQexec(con,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr, "fall� comando BEGIN\n");
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
    fprintf(stderr, "comando DECLARE CURSOR fall�\n");
    fprintf(stderr, "Error: %s\n",PQerrorMessage(con));
    PQclear(res);
    free(comando);
    return(ERROR_SQL);
  }
  PQclear(res);

  sprintf(comando, "FETCH ALL in cursor_arts");
  res = PQexec(con, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr, "comando FETCH ALL no regres� registros apropiadamente\n");
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
  int    i;
  unsigned  num_items;
  WINDOW *v_forma;
/*         *v_mensaje; */

  //  read_config();

  initscr();
  start_color();
  init_pair(amarillo_sobre_azul, COLOR_YELLOW, COLOR_BLUE);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(normal, COLOR_WHITE, COLOR_BLACK);

  init_config();
  read_general_config();
  //  read_global_config();
  read_config();

  con_s = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd);
  if (con_s == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }
  //  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, "osopos", log_name, "");
  if (db.passwd == NULL) {
    db.passwd = g_strdup(obten_passwd(db.user));
    //obten_passwd(db.user, db.passwd);
  }

  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.user, db.passwd); 

  if (PQstatus(con) == CONNECTION_BAD) {
    printw("FATAL: Fall� la conexi�n a la base de datos %s\n", nminvent);
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

  num_items = (unsigned) inicializa_lista(con, "codigo");

  if (num_items<0) {
    mvprintw(getmaxy(stdscr)/2, 0, "Error al incializar la lista de art�culos");
    mvprintw(getmaxy(stdscr)-2, 0, "Pulse una tecla para salir...");
    clrtoeol();
    getch();
    clear();
    endwin();
    fprintf(stderr, "Error al incializar la lista de art�culos\n");
    exit(ERROR_SQL);
  }
  ajusta_ventana_forma();
  forma_articulo(v_forma, &num_items, con);

  delwin(v_arts);
  clear();
  refresh();
  endwin();
  PQfinish(con);
  PQfinish(con_s);
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
  sea representado en la pantalla al momento de la modificaci�n
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
