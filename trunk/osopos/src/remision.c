/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa Remision 1.0.7-1 1999, 2000 E. Israel Osorio H., licencia GPL.
   linucs@punto-deventa.com
   Lea el archivo README, COPYING y LEAME que contienen información
   sobre la licencia de uso de este programa

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
#include <time.h>

#define vers "1.0.8-1"
#define release "Ricardo Chapa"

#ifndef maxdes
#define maxdes 39
#endif

#ifndef maxcod
#define maxcod 20
#endif

#ifndef mxmembarra
#define mxmembarra 100
#endif

#define blanco_sobre_negro 1
#define amarillo_sobre_negro 2
#define verde_sobre_negro 3
#define azul_sobre_blanco 4
#define cyan_sobre_negro 5


/* Códigos de impresora */
#define abre_cajon_star 7
#ifndef ESC
#define ESC 27
#endif

#define _PAGO_TARJETA    1
#define _PAGO_CREDITO    2
#define _PAGO_EFECTIVO  20
#define _PAGO_CHEQUE    21

#define _NOTA_MOSTRADOR   1
#define _FACTURA         2
#define _TEMPORAL        5

int forma_de_pago();
PGresult *registra_venta(PGconn *base,
                         char   *tabla,
                         int    num_venta,
                         double total,
                         double utilidad,
                         int    tipo_pago,
                         int    tipo_fact,
                         bool   corte_parcial);

void Termina(PGconn *con, int error);
int LeeConfig(char*, char*);
double capt_remision(char *nm_reg_diario, char *nm_reg_temp, int *numart, double *util);
void imp_ticket_arts();
void ImpTicketPie(struct tm fecha, unsigned numvents);
/* Función que reemplaza a ActualizaEx */
int actualiza_existencia(PGconn *con);
/* Esta función reemplaza a leebarras */
int lee_articulos(PGconn *base_inventario);
void MuestraArtLista(WINDOW *vent, int i, short cr);
void muestra_subtotal(double);
void mensaje(char *texto);
void aviso_existencia(struct articulos art);

FILE *impresora;
double a_pagar; 
int numbarras=0;
/* struct barras barra[mxmembarra];  */
struct articulos articulo[maxart], barra[mxmembarra];
int numarts=0;		/* Número de items capturados	*/
short unsigned maxitemr;

  /* Variables de configuración */
char *nm_disp_ticket,		/* Nombre de impresora de ticket */
  *nmfpie,		/* Pie de página de ticket */
  *nmfenc,		/* Archivo de encabezado de ticket */
  *nmtickets,	/* Registro de tickets impresos */
  *tipo_disp_ticket, /* Tipo de miniprinter */
  *nm_num_venta,	/* Nombre del archivo con el número de venta */
  *nmimprrem,		/* Ubicación de imprrem */
  *nm_avisos,    /* Nombre del archivo de avisos */
  *nm_sendmail,  /* Ruta completa de sendmail */
  *dir_avisos,   /* email de notificación */
  *cc_avisos,    /* con copia para */
  *asunto_avisos; /* Asunto del correo de avisos */



int LeeConfig(char *nmdatos,
              char *nmdiario)
{
  static char nmconfig[] = "remision.config";
  FILE *config;
  static char buff[mxbuff],buf[mxbuff];
  static char *b;

  nm_disp_ticket = calloc(1, strlen("/dev/lpx")+1);
  strcpy(nm_disp_ticket,  "/dev/lp0");

  tipo_disp_ticket = calloc(1, strlen("STAR")+1);
  strcpy(tipo_disp_ticket, "STAR");

  strcpy(nmdatos,  "/home/OsoPOS/caja/venta.ultima");

  strcpy(nmdiario, "/home/OsoPOS/scaja/log/ventas.hoy");

  nmfpie = calloc(1, strlen("/home/OsoPOS/caja/pie_pagina.txt")+1);
  strcpy(nmfpie,   "/home/OsoPOS/caja/pie_pagina.txt");

  nmfenc = calloc(1, strlen("/home/OsoPOS/caja/encabezado.txt")+1);
  strcpy(nmfenc,   "/home/OsoPOS/caja/encabezado.txt");
  
  nmtickets = calloc(1, strlen("/home/OsoPOS/scaja/log/venta")+1);
  strcpy(nmtickets,"/home/OsoPOS/scaja/log/venta");
  
  nmimprrem = calloc(1, strlen("~/bin/imprem")+1);
  strcpy(nmimprrem,"~/bin/imprem");

  nm_num_venta = calloc(1, strlen("/home/OsoPOS/scaja/log/.numventa")+1);
  strcpy(nm_num_venta, "/home/OsoPOS/scaja/log/.numventa");

  nm_avisos = calloc(1, strlen("/home/OsoPOS/log/avisos.txt")+1);
  strcpy(nm_avisos, "/home/OsoPOS/log/avisos.txt");
  
  dir_avisos = calloc(1, strlen("scaja@osopos")+1);
  strcpy(nm_avisos, "scaja@osopos");
  
  cc_avisos = "";

  asunto_avisos = calloc(1, strlen("Aviso de baja existencia")+1);
  strcpy(asunto_avisos, "Aviso de baja existencia");
  
  nm_sendmail = calloc(1, strlen("/usr/sbin/sendmail -t")+1);
  strcpy(nm_avisos, "/usr/sbin/sendmail -t");

  maxitemr = 6;

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuración */
    b = buff;
    fgets(buff,sizeof(buff),config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;
      if (!strlen(buff) || buff[0] == '#') {
	fgets(buff,sizeof(buff),config);
	continue;
      }
      strcpy(buf, strtok(buff,"="));
        /* La función strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

        /* Definición de impresora de ticket */
      if (!strcmp(b,"ticket")) {
        strcpy(buf, strtok(NULL,"="));
        free(nm_disp_ticket);
        nm_disp_ticket = NULL;
        nm_disp_ticket = calloc(1, strlen(buf)+1);
        strcpy(nm_disp_ticket,buf);
      }
      else if (!strcmp(b,"registro.ticket")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmtickets, strlen(buf)+1);
        strcpy(nmtickets,buf);
      }
       /* Archivo de última venta */
      else if (!strcmp(b,"datos")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmdatos, strlen(buf)+1);
        strcpy(nmdatos,buf);
      }
      else if (!strcmp(b,"num_venta")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nm_num_venta, strlen(buf)+1);
        strcpy(nm_num_venta,buf);
      }
	/* Registro del diario */
      else if (!strcmp(b,"registro")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmdiario,strlen(buf)+1);
        strcpy(nmdiario,buf);
      }
      else if (!strcmp(b,"ticket.pie")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmfpie, strlen(buf)+1);
        strcpy(nmfpie,buf);
      }
      else if (!strcmp(b,"ticket.encabezado")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmfenc, strlen(buf)+1);
        strcpy(nmfenc,buf);
      }
      else if (!strcmp(b,"miniimpresora.tipo")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(tipo_disp_ticket, strlen(buf)+1);
        strcpy(tipo_disp_ticket,buf);
      }
      else if (!strcmp(b,"programa.imprem")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nmimprrem, strlen(buf)+1);
        strcpy(nmimprrem,buf);
      }
      else if (!strcmp(b,"renglones.articulos")) {
        strcpy(buf, strtok(NULL,"="));
        maxitemr = atoi(buf);
      }
      else if (!strcmp(b,"avisos")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nm_avisos, strlen(buf)+1);
        strcpy(nm_avisos,buf);
      }
      else if (!strcmp(b,"email.avisos")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(dir_avisos, strlen(buf)+1);
        strcpy(dir_avisos,buf);
      }
      else if (!strcmp(b,"asunto.email")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(asunto_avisos, strlen(buf)+1);
        strcpy(asunto_avisos,buf);
      }
      else if (!strcmp(b,"cc.avisos")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(cc_avisos, strlen(buf)+1);
        strcpy(cc_avisos,buf);
      }
      else if (!strcmp(b,"programa.sendmail")) {
        strcpy(buf, strtok(NULL,"="));
        realloc(nm_sendmail, strlen(buf)+1);
        strcpy(nm_sendmail,buf);
      }
      fgets(buff,sizeof(buff),config);
    }
    fclose(config);
    return(0);
  }
  return(1);
}

/***************************************************************************/

int lee_articulos(PGconn *base_inventario)
{
  char *comando;
/*  char *datos[7]; */
  int  i;
  int   nCampos;
  PGresult* res;

  res = PQexec(base_inventario,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando BEGIN al tratar de leer artículos para colocar en memoria\n");
    PQclear(res);
    return(ERROR_SQL);
  }
  PQclear(res);

  comando = "DECLARE cursor_arts CURSOR FOR SELECT * FROM articulos";
  res = PQexec(base_inventario, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Comando DECLARE CURSOR falló\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base_inventario));
    PQclear(res);
    return(ERROR_SQL);
  }
  PQclear(res);

  comando = "FETCH ALL in cursor_arts";
  res = PQexec(base_inventario, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
    PQclear(res);
    return(ERROR_SQL);
  }

  nCampos = PQnfields(res);

  for (i=0; i < PQntuples(res) && i<mxmembarra; i++) {
    strcpy(barra[i].codigo , PQgetvalue(res,i,0));
    strcpy(barra[i].desc, PQgetvalue(res,i,1));
    barra[i].pu = atof(PQgetvalue(res,i,2));
    barra[i].disc = atof(PQgetvalue(res,i,3));
    barra[i].exist = atoi(PQgetvalue(res,i,4));
    barra[i].exist_min = atoi(PQgetvalue(res,i,5));
    barra[i].p_costo = atof(PQgetvalue(res,i,9));
	barra[i].iva_porc = atof(PQgetvalue(res,i,11));
  }

  if (PQntuples(res) >= mxmembarra)  {
    comando = NULL;
    comando = calloc(1, mxbuff);
    fprintf(stderr, "ADVERTENCIA: Se ha excedido el máximo de artículos en memoria, no fueron\n");
    fprintf(stderr, "             cargados todos los que existen en la base");
    sprintf(comando,
            "Se ha excedido el máximo de artículos en memoria, se leyeton los primeros %d",
            mxmembarra);
    mensaje(comando);
    free(comando);
    comando = NULL;
  }
  PQclear(res);


  res = PQexec(base_inventario, "CLOSE cursor_arts");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Comando CLOSE falló\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base_inventario));
    PQclear(res);
    Termina(base_inventario,-1);
  }
  PQclear(res);

  /* finaliza la transacción */
  res = PQexec(base_inventario, "END");
  PQclear(res);
  return(i);
}

/***************************************************************************/

/*double busca_precio(char *cod, char *descrip, double *p_costo) { */
int busca_precio(char *cod, struct articulos *art) {
/* Busca el código de barras en la memoria y devuelve el precio */
/* del producto. */
int i;
int salir=0; /* falso si hay que salirse */

  for (i=0; (i<numbarras); ++i) {
    salir = strcmp(cod, barra[i].codigo);
    if (!salir) {
      strcpy(art->codigo, barra[i].codigo);
      strcpy(art->desc, barra[i].desc);
      art->p_costo = barra[i].p_costo;
      art->pu = barra[i].pu;
      art->iva_porc = barra[i].iva_porc;
      return(i);
    }
  }
  return(0);
}    

/***************************************************************************/

int cancela_articulo(WINDOW *vent, int *reng, double *subtotal, double *iva)
{
  int i,j;
  int num_item;
  char codigo[maxcod];
  double iva_articulo;

  mvprintw(getmaxy(stdscr)-3,0,"Código del(los) artículo(s) a cancelar: ");
  clrtoeol();
  getstr(codigo);
  move(getmaxy(stdscr)-3,0);
  clrtoeol();

  for (i=0; strcmp(codigo, articulo[i].codigo) && i<*reng; i++);
  num_item = i;

  iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
  (*iva)  -=  (articulo[i].cant * iva_articulo);
  (*subtotal) -= (articulo[i].cant * articulo[i].pu);
  /* Desplaza los artículos a una posición inferior */
  for (; i<*reng; i++) {
    strcpy(articulo[i].codigo, articulo[i+1].codigo);
    articulo[i].pu = articulo[i+1].pu;
    strcpy(articulo[i].desc, articulo[i+1].desc);
    articulo[i].cant = articulo[i+1].cant;
  }
  (*reng)--;

  wclear(vent);
  if (numarts>vent->_maxy)
    for (i=*reng, j=vent->_maxy; j>=0; j--, i-- ) {
      wmove(vent, j,0);
      wclrtoeol(vent);
      MuestraArtLista(vent, i, FALSE);
    }
  else
    for (i=0; i<*reng && i<vent->_maxy; i++)
      MuestraArtLista(vent, i, TRUE);

  wrefresh(vent);
  return(num_item);
}


void MuestraArtLista(WINDOW *vent, int i, short cr) {
  mvwprintw(vent, vent->_cury,  0, "%3d %15s", articulo[i].cant, articulo[i].codigo);
  mvwprintw(vent, vent->_cury, 22, "%-39s", articulo[i].desc);
  mvwprintw(vent, vent->_cury, 62, "$%8.2f", articulo[i].pu);
  if (!articulo[i].iva_porc)
	wprintw(vent, "E");
  if (cr)
    wprintw(vent, "\n");
  wrefresh(vent);
}

void muestra_subtotal(double subtotal) {

  attrset(COLOR_PAIR(verde_sobre_negro));
  mvprintw(getmaxy(stdscr)-5,0,"Subtotal acumulado: $%8.2f",subtotal);
  attrset(COLOR_PAIR(blanco_sobre_negro));
  clrtoeol();
}

/***************************************************************************/

double capt_remision(char   *nm_reg_diario,
                     char   *nm_reg_temp,
                     int    *numart,
                     double *util) {
  double  subtotal = 0.0;
  double  iva_articulo;
  FILE    *registro,*regtemp;
  int     i=0,
          j,k;
  char    *buff;
  WINDOW  *v_arts;
  *util = 0;

  iva = 0.0;
  registro = fopen(nm_reg_diario,"a");
  if (registro == NULL)
    ErrorArchivo(nm_reg_diario);

  regtemp = fopen(nm_reg_temp,"w");
  if (regtemp == NULL)
    ErrorArchivo(nm_reg_temp);

  v_arts = newwin(getmaxy(stdscr)-8, getmaxx(stdscr)-1, 4, 0);
  scrollok(v_arts, TRUE);
  buff = calloc(1,mxbuff);
  mvprintw(3,0,
	"Cant.       Clave                Descripción                 P. Unitario");
  articulo[0].cant = 1;

  do {
	iva_articulo = 0.0;
    articulo[i].pu = 0;
    articulo[i].p_costo = 0;
    mvprintw(getmaxy(stdscr)-3,0,"Código de barras, descripción o cantidad:\n");
    getstr(buff);
    move(getmaxy(stdscr)-2,0);
    deleteln();

	/* Sección de multiplicador */
    if ( ((toupper(buff[0])=='X') || (buff[0]=='*')) && i) {
      for (j=0; !isdigit(buff[j]); j++);
      for (k=0; isdigit(buff[j]) && j<strlen(buff); j++,k++)
        articulo[i].desc[k] = buff[j];
      articulo[i].desc[k]=0;
      articulo[i].cant = atoi(articulo[i].desc);

      if (!articulo[i].cant) {
         /* Cancela el último artículo */
        wmove(v_arts, v_arts->_cury-1,0);
        wclrtoeol(v_arts);
        wrefresh(v_arts);
        subtotal -= (articulo[--i].pu * articulo[i].cant);
		iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
		iva  -= (iva_articulo * articulo[i].cant);
      }
      else {
		iva_articulo = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].iva_porc/100 + 1);
        subtotal += ((articulo[i].cant-articulo[i-1].cant) * articulo[i-1].pu);
		iva  += (iva_articulo * (articulo[i].cant-articulo[i-1].cant));
        articulo[i-1].cant = articulo[i].cant;
      }
      if (i) {
        wmove(v_arts, v_arts->_cury-1, v_arts->_curx);
        MuestraArtLista(v_arts, i-1, TRUE);
      }
      muestra_subtotal(subtotal);

      articulo[i].desc[0] = 0;
      articulo[i].cant = 1;
      continue;
    }

    if (strlen(buff) > 1) {
    /* Si la clave es mayor de 1 caracteres, busca un código o repetición */

	/* Repetición de artículo */
	/* ¿ No debería de ser && en lugar de || ? */
      if (i && (!strcmp(buff,articulo[i-1].desc) || !strcmp(buff,articulo[i-1].codigo)) ) {
        (articulo[i-1].cant)++;
        subtotal += articulo[i-1].pu;
		iva_articulo = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].iva_porc/100 + 1);
		iva += iva_articulo;
        wmove(v_arts, v_arts->_cury-1, v_arts->_curx);
        MuestraArtLista(v_arts, i-1, TRUE);
        muestra_subtotal(subtotal);
        continue;
      }

      if (busca_precio(buff, &articulo[i]))
		strncpy(articulo[i].codigo, buff, maxcod);
      else
		strncpy(articulo[i].desc, buff, maxcod);
    }

    /* El artículo no tiene código, puede ser un art. no registrado */
    /* o un descuento */
    if (!articulo[i].pu) {
      if (strlen(buff) <= 1) {
        articulo[i].cant=0;
        continue;
      }
      if (strstr(articulo[i].desc,"ancela") && i) {
        cancela_articulo(v_arts, &i, &subtotal, &iva);
		muestra_subtotal(subtotal);
        continue;
      }
      strcpy(articulo[i].codigo,"Sin codigo");
      mvprintw(getmaxy(stdscr)-3,0,"Introduce precio unitario: ");
      clrtoeol();
      scanw("%lf",&articulo[i].pu);
      deleteln();
      mvprintw(getmaxy(stdscr)-3,0,"Introduce piorcentaje de I.V.A.: ");
      clrtoeol();
      scanw("%lf",&articulo[i].iva_porc);
      deleteln();
      if ( strstr(articulo[i].desc,"escuento") && i) {
        articulo[i-1].pu += articulo[i].pu;
        articulo[i].codigo[0] = 0;
        articulo[i].desc[0] = 0;

		iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
        subtotal += (articulo[i-1].cant * articulo[i].pu);
		iva += (iva_articulo * articulo[i-1].cant);
        wmove(v_arts, v_arts->_cury-1, 0);
		MuestraArtLista(v_arts, i-1, TRUE);
		muestra_subtotal(subtotal);
        continue;
      }
      /* Artículo no registrado */
      MuestraArtLista(v_arts, i, TRUE);
	  iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
      subtotal += (articulo[i].pu * articulo[i].cant);
	  iva += (iva_articulo * articulo[i].cant);
      muestra_subtotal(subtotal);
      articulo[++i].cant = 1;
    }
    else {  /* Articulo registrado */
      MuestraArtLista(v_arts, i, TRUE);
	  iva_articulo =  articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
      subtotal += (articulo[i].pu * articulo[i].cant);
	  iva += (iva_articulo * articulo[i].cant);
      muestra_subtotal(subtotal);
      articulo[++i].cant = 1;
    }
  }
  while (i<maxitemr &&  articulo[i].cant);
  move(getmaxy(stdscr)-2,0);
  deleteln();
  if (i && !articulo[i-1].cant)
    i--;
  *numart = i;
  for (j=0; j<(*numart); j++) {
    *util += ((articulo[j].pu - articulo[j].p_costo) * articulo[j].cant);
	iva_articulo = articulo[j].pu - articulo[j].pu / (articulo[j].iva_porc/100 + 1);

    fprintf(registro,"%s\n",articulo[j].codigo);
    fprintf(registro,"%d\n",articulo[j].cant);
    fprintf(registro,"%.2f\n",articulo[j].pu*articulo[j].cant);
    fprintf(registro,"%.2f\n",iva_articulo*articulo[j].cant);
    /* * * * * * * * * * * * * CAMBIAR POR BASE DE DATOS * * * * * * * */
    fprintf(regtemp, "%s\n", articulo[i].codigo);
    fprintf(regtemp, "%s\n", articulo[j].desc);
    fprintf(regtemp, "%d\n", articulo[j].cant);
    fprintf(regtemp, "%.2f\n" ,articulo[j].pu);
	fprintf(regtemp, "%.2f\n", articulo[j].iva_porc);
  }
  fclose(regtemp);

  if (i) {
    attrset(COLOR_PAIR(amarillo_sobre_negro));
    attron(A_BOLD);
    mvprintw(getmaxy(stdscr)-5,0, "Total a pagar: %.2f", subtotal);
    attroff(A_BOLD);
    attrset(COLOR_PAIR(blanco_sobre_negro));
    clrtoeol();
    fprintf(registro,"\n");
  }
  fclose(registro);

  free(buff);
  return(subtotal);
}

/***************************************************************************/

int forma_de_pago()
{
  static double recibo = 0.0;
  static double dar;
  char opcion;
  int tipo;

  move(getmaxy(stdscr)-3, 0);
  clrtoeol();

  do {
    mvprintw(getmaxy(stdscr)-2, 0, "¿Forma de pago (Efectivo/Tarjeta/Credito/cHeque)? E");
    clrtoeol();
    attrset(COLOR_PAIR(azul_sobre_blanco)); /* Resalta las opciones */
    if (recibo) {
      attron(A_BLINK);
      fprintf(stderr, "\a");  /* Beep */
    }
    mvprintw(getmaxy(stdscr)-2, 16, "E");
    mvprintw(getmaxy(stdscr)-2, 25, "T");
    mvprintw(getmaxy(stdscr)-2, 33, "C");
    mvprintw(getmaxy(stdscr)-2, 42, "H");
    if (recibo) {
      recibo = 0.0;
      attroff(A_BLINK);
    }
    attrset(COLOR_PAIR(blanco_sobre_negro));
    move(getmaxy(stdscr)-2, 50);
    opcion = toupper(getch());
    switch (opcion) {
      case '\n':
      case 'E':
        tipo = _PAGO_EFECTIVO;
      break;
      case 'H':
        tipo = _PAGO_CHEQUE;
      break;
      case 'C':
        tipo = _PAGO_CREDITO;
      break;
      case 'T':
        tipo = _PAGO_TARJETA;
      break;
      default: recibo++;
    }
  }
  while (recibo);

  if (tipo >= 20) {
    mvprintw(getmaxy(stdscr)-2,0,"Se recibe: ");
    clrtoeol();
    scanw("%lf",&recibo);
    if (recibo >= a_pagar)
      dar = recibo - a_pagar;
    else
      dar = 0;
    mvprintw(getmaxy(stdscr)-2, getmaxx(stdscr)/2, "Cambio de: %.2f", dar);
  }
  return(tipo);
}

/***************************************************************************/

PGresult *registra_venta(PGconn *base,
                         char   *tabla,
                         int    num_venta,
                         double total,
                         double utilidad,
                         int    tipo_pago,
                         int    tipo_fact,
                         bool   corte_parcial)
{
  char     *comando_sql;
  PGresult *resultado;
  char     exp_boleana = 'F';

  if (corte_parcial)
    exp_boleana = 'T';
  comando_sql = calloc(1,mxbuff);
  sprintf(comando_sql, "INSERT INTO %s VALUES(%d, %.2f, %d, %d, '%c', %.2f)",
                        tabla, num_venta, total,
                        tipo_pago, tipo_fact, exp_boleana, utilidad);
  resultado = PQexec(base, comando_sql);
  if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Comando INSERT INTO falló\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
  }
  free(comando_sql);
  return(resultado);
}


void Cambio() {
  static double recibo;
  static double dar;

  mvprintw(getmaxy(stdscr)-2,0,"Se recibe: ");
  clrtoeol();
  scanw("%lf",&recibo);
  if (recibo >= a_pagar)
    dar = recibo - a_pagar;
  else
    dar = 0;
  mvprintw(getmaxy(stdscr)-2,getmaxx(stdscr)/2,"Cambio de: %.2f\n",dar);
}

void imp_ticket_arts() {
  FILE *impr;
  static int i;

  impr = fopen(nm_disp_ticket,"a");
  if (impr == NULL)
	ErrorArchivo(nm_disp_ticket);

  for (i=0; i<numarts; i++) {
    fprintf(impr," -> %s\n",articulo[i].desc);
    fprintf(impr," %5d x %10.2f = %10.2f",
       articulo[i].cant,articulo[i].pu,articulo[i].pu*articulo[i].cant);
	if (!articulo[i].iva_porc)
	  fputs(" E", impr);
	fputs("\n", impr);
  }
  fprintf(impr, "\n     Total: %10.2f\n", a_pagar);
  fprintf(impr, "     I.V.A.: %10.2f\n", iva);
  fprintf(impr, "E = Exento\n\n");
  fclose(impr);
}

void ImpTicketPie(struct tm fecha, unsigned numventa) {
  FILE *impr,*fpie;
  static char *s;

  impr = fopen(nm_disp_ticket,"a");
  if (impr == NULL)
        ErrorArchivo(nm_disp_ticket);

  s = calloc(1,mxbuff);
  /* Fecha y hora local */
  fprintf(impr,"  %2d/%2d/%2d a las %2d:%2d:%2d hrs.\n",
        fecha.tm_mday, (fecha.tm_mon)+1, fecha.tm_year, fecha.tm_hour,
        fecha.tm_min, fecha.tm_sec);
  fprintf(impr, "  Venta num. %u\n", numventa);
  fpie = fopen(nmfpie,"r");
  if (!fpie) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede leer el pie de ticket\n\r");
    fprintf(stderr,"del archivo %s\n",nmfpie);
    fprintf(impr,"\nInformes y ventas de este sistema:\n");
    fprintf(impr,"e-mail: linucs@punto-deventa.com\n");
  }
  else {
    do { 
      fgets(s, sizeof(s), fpie);
      if (!feof(fpie))
	fprintf(impr, "%s", s);
    }
    while (!feof(fpie));
    fclose(fpie);
  }
  fprintf(impr,"\n\n\n");
  imprime_razon_social(impr);
  free(s);
  fclose(impr);
}

void ImpTicketEncabezado() {
  FILE *impr,*fenc;
  char *s;

  impr = fopen(nm_disp_ticket,"a");
  if (impr == NULL)
        ErrorArchivo(nm_disp_ticket);

  s = calloc(1, mxbuff);
  fenc = fopen(nmfenc,"r");
  if (!fenc) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede leer el encabezado de ticket\n\r");
    fprintf(stderr,"del archivo %s\n",nmfenc);
    fprintf(impr,
     "Sistema OsoPOS, programa Remision %s en\n",vers);
    fprintf(impr,"un sistema Linux linucs@punto-deventa.com\n");
  }
  else {
    do {
      fgets(s, sizeof(s), fenc);
      if (!feof(fenc))
        fprintf(impr, "%s", s);
    }
    while (!feof(fenc));
    fclose(fenc);
  }
/*  fprintf(impr,"Tapachula, Chiapas a %u/%u/%u\n",
               fecha.tm_mday, (fecha.tm_mon)+1, fecha.tm_year); */
  fprintf(impr,"---------------------------------------\n");
  fclose(impr);
  free(s);
}

int AbreCajon(char *tipo_miniimp) {
  FILE *impr;

  impr = fopen(nm_disp_ticket,"a");
  if (!impr) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede activar el cajón de dinero\n\r");
    fprintf(stderr,"operado por %s\n",nm_disp_ticket);
    return(0);
  }
  if (!strcpy(tipo_miniimp, "STAR"))
    fprintf(impr, "%c", abre_cajon_star);
  else
    fprintf(impr,"%c%c%c", ESC,'p', 4);
  fclose(impr);
  return(1);
}

void RegistraRem(unsigned num_venta) {
  FILE *log, *ar_num_venta;
  static int i;

  ar_num_venta = fopen(nm_num_venta, "w");
  if (ar_num_venta == NULL)
    ErrorArchivo(nm_num_venta);
  fprintf(ar_num_venta, "%u", num_venta);
  fclose(ar_num_venta);

  log = fopen(nmtickets,"a");
  if (log == NULL)
        ErrorArchivo(nmtickets);

  for (i=0; i<numarts; i++) {
    fprintf(log," -> %s\n",articulo[i].desc);
    fprintf(log, "%20s %3d x %8.2f = %8.2f\n",
	articulo[i].codigo,articulo[i].cant,articulo[i].pu,
	articulo[i].pu*articulo[i].cant);
  }
  fprintf(log,"                  TOTAL: %8.2f\n",a_pagar);
  fclose(log);
}


void aviso_existencia(struct articulos art)
{
  FILE *arch;
  char aviso[255];


  sprintf(aviso, "Baja existencia en producto de proveedor num. %u\n",
                 art.id_prov);
  sprintf(aviso, "%s %s quedan %d\n",
                 art.codigo, art.desc, art.exist);

  arch = fopen(nm_avisos, "a");
  if (!arch) {
    mensaje("No puedo escribir al archivo de mensajes");
    getch();
  }
  else {
	fprintf(arch, "%s", aviso);
	fclose(arch);
  }

  arch = popen(nm_sendmail, "w");
  if (!arch) {
    mensaje("No puedo escribir al programa de envío de mensajes");
    getch();
  }
  else {
	fprintf(arch, "To: %s\n", dir_avisos);
	fprintf(arch, "From: %s\n", "scaja");
	fprintf(arch, "Subject: %s\n", asunto_avisos);
	if (strlen(cc_avisos))
	  fprintf(arch, "Cc: %s\n", cc_avisos);
	fprintf(arch, "\n\n%s\n", aviso);
	fclose(arch);
  }
}

/* Actualiza las existencias de los artículos que se vendieron en una sola operación */
int actualiza_existencia(PGconn *base_inv) {
int i,
    resurtir = 0;
PGresult *res;

  for(i=0; i<numarts; i++) {
    articulo[i].exist = -1;
    Busca_en_Inventario(base_inv, "articulos", "codigo", articulo[i].codigo, &articulo[i]);
    if (articulo[i].exist < 0) /* No existe en la base de existencias */
      continue;
    if (articulo[i].exist > articulo[i].exist_min  &&
		articulo[i].exist-articulo[i].cant <= articulo[i].exist_min) {
      aviso_existencia(articulo[i]);
      resurtir++;
    }
    articulo[i].exist -= articulo[i].cant;
    res = Modifica_en_Inventario(base_inv, "articulos", articulo[i]);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      fprintf(stderr, "Error al actualizar artículo %s %s\n",
                       articulo[i].codigo, articulo[i].desc);
    PQclear(res);
  }
  return(resurtir);
}

void Termina(PGconn *con, int error)
{
  fprintf(stderr, "Error número %d encontrado, abortando...",errno);
  PQfinish(con);
  exit(errno);
}


void mensaje(char *texto)
{
	attrset(COLOR_PAIR(azul_sobre_blanco));
        mvprintw(getmaxy(stdscr)-1,0, texto);
	attrset(COLOR_PAIR(blanco_sobre_negro));
	clrtoeol();
}

unsigned obten_num_venta(char *nm_archivo)
{
  FILE *archivo;
  char buff[50];

  archivo = fopen(nm_archivo, "r");
  if (archivo == NULL) {
    ErrorArchivo(nm_archivo);
    return(0);
  }
  fgets(buff,(sizeof(buff)), archivo);
  fclose(archivo);
  return(atoi(buff));
}


int main() {
  static char buffer, buf[50];
  time_t tiempo;	
  static char encabezado1[mxbuff],
      encabezado2[mxbuff] = "E. Israel Osorio H., 1999,2000 linucs@punto-deventa.com";
  static int dgar;
  PGconn *con;
  unsigned num_venta = 0;
  double utilidad;
  struct tm *fecha;	/* Hora y fecha local	*/

  char *nmdatos,   	/* Nombre del archivo de datos de la venta */
       *nmdiario;   /* Nombre del registro del diario */

  nmdatos  = calloc(1, 31);
  nmdiario = calloc(1, 34);

  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fecha->tm_year += 1900;

  initscr();
  LeeConfig(nmdatos, nmdiario);
  if (!has_colors()) {
    aborta("Este equipo no puede producir colores, pulse una tecla para abortar...",
            10);
  }

  start_color();
  init_pair(blanco_sobre_negro, COLOR_WHITE, COLOR_BLACK);
  init_pair(amarillo_sobre_negro, COLOR_YELLOW, COLOR_BLACK);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(azul_sobre_blanco, COLOR_BLUE, COLOR_WHITE);
  init_pair(cyan_sobre_negro, COLOR_CYAN, COLOR_BLACK);

  con = Abre_Base(NULL, NULL, NULL, NULL, "osopos", "scaja", "");
  if (con == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }
  num_venta = obten_num_venta(nm_num_venta);

  numbarras = lee_articulos(con);
  if (!numbarras) {
    mensaje("No hay artículos en la base de datos. Use el programa de inventarios primero");
    getch();
  }
  else if (numbarras == ERROR_SQL) {
    mensaje("Error al leer base de datos, búsqueda de artículos deshabilitada.");
    getch();
    numbarras = 0;
  }

  sprintf(encabezado1, "Sistema OsoPOS - Programa Remision %s R.%s", vers, release);
  do {
    clear();
    mvprintw(0,0,"%u/%u/%u",
               fecha->tm_mday, (fecha->tm_mon)+1, fecha->tm_year);
    attrset(COLOR_PAIR(cyan_sobre_negro));
    mvprintw(0,(getmaxx(stdscr)-strlen(encabezado1))/2,
	     "%s",encabezado1);
    mvprintw(1,(getmaxx(stdscr)-strlen(encabezado2))/2,
	     "%s",encabezado2);
    attrset(COLOR_PAIR(blanco_sobre_negro));
    a_pagar = capt_remision(nmdiario, nmdatos, &numarts, &utilidad);

    /* esto pretende "despertar" a la impresora para agilizar el proceso, pero en
       realidad no se ha probado aún   */
    fclose(fopen(nm_disp_ticket, "a"));

    if (numarts && a_pagar) {
      attron(A_BOLD);
      mvprintw(getmaxy(stdscr)-2,0,"¿Imprimir Remision, Factura o Ticket (R,F,T)? T\b");
      attroff(A_BOLD);
      buffer = toupper(getch());
      RegistraRem(++num_venta);

      switch (buffer) {
        case 'R':
          attron(A_BOLD);
	      mvprintw(getmaxy(stdscr)-2,0,"¿Días de garantía? : ");
          attroff(A_BOLD);
	      clrtoeol();
     	  scanw("%d",&dgar);
 	      sprintf(buf,"%s %d &",nmimprrem,dgar);
          system(buf);
          buffer = forma_de_pago();
          if (buffer >=20)
            AbreCajon(tipo_disp_ticket);
        	registra_venta(con, "ventas", num_venta, a_pagar, utilidad,
        	               buffer, _NOTA_MOSTRADOR, FALSE);
	        mensaje("Aprieta una tecla para continuar (t para terminar)...");
            buffer = toupper(getch());
        break;
        case 'F':
	      buffer = forma_de_pago();
	      if (buffer >=20)
            AbreCajon(tipo_disp_ticket);
          mensaje("Aprieta una tecla para capturar factura...");
		  getch();
		  system("~/bin/facturar -r");
		  clear();
          registra_venta(con, "ventas", num_venta, a_pagar, utilidad,
                         buffer, _FACTURA, FALSE);
          mensaje("Aprieta una tecla para continuar (t para terminar)...");
          buffer = toupper(getch());
        break;
        case 'T':
        default:
          buffer = forma_de_pago();
          imp_ticket_arts();
	      if (buffer >=20)
            AbreCajon(tipo_disp_ticket);
		  ImpTicketPie(*fecha, num_venta);
          registra_venta(con, "ventas", num_venta, a_pagar, utilidad,
                         buffer, _TEMPORAL, FALSE);
	      mensaje("Corta el papel y aprieta una tecla para continuar (t para terminar)...");
          buffer = toupper(getch());
		  ImpTicketEncabezado();
        break;
      }
      if (actualiza_existencia(con)) {
        mensaje("Baja existencia de productos. Aprieta una tecla para seguir...");
        getch();
      }
    }
    else {
      buffer = 0;
      mensaje("Venta cancelada, apriete una tecla para continuar (t para terminar)...");
      buffer = toupper(getch());
    }
  }
  while (buffer!='T');
  clear();
  refresh();

  free(nm_disp_ticket);
  free(nmdatos);
  free(nmdiario);
  free(nm_avisos);
  free(nmfpie);
  free(nmfenc);
  free(nmtickets);
  free(nmimprrem);
  free(nm_num_venta);

  PQfinish(con);
  endwin();
  return(OK);
}

/* Pendientes:

- Optimizar la funcion busca_precio para que solo devuelva el indice del código buscado y
  posteriormente se haga una copia de registro a registro
- Revisar manejo de articulos exentos de IVA, y sumatoria de IVA
- Hacer que se imprima el IVA en tickets

  */
