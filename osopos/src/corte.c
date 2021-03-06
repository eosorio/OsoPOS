/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa Corte 0.14 (C) 1999-2009,2017 E. Israel Osorio H.
   eduardo.osorio.ti@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>

#include "include/pos-curses.h"

#define VERSION "0.14"
#define blanco_sobre_negro 1
#define amarillo_sobre_negro 2
#define verde_sobre_negro 3
#define azul_sobre_blanco 4
#define cyan_sobre_negro 5

#define _PAGO_TARJETA    1
#define _PAGO_CREDITO    2
#define _PAGO_EFECTIVO  20
#define _PAGO_CHEQUE    21

#define _NOTA_MOSTRADOR  1
#define _FACTURA         2
#define _TEMPORAL        5

#ifndef maxbuf
#define maxbuf 255
#endif

#define blanco_sobre_negro       1
#define amarillo_sobre_negro     2
#define verde_sobre_negro        3
#define azul_sobre_blanco        4
#define cyan_sobre_negro         5
#define normal                   6
#define amarillo_sobre_azul      7
#define inverso                  8

#include "include/pos-bd-func.h"
#include "include/common.h"

double suma_pagos(PGconn *base, int tipo_pago, bool parcial, double *utilidad, double *iva,
                  double tax[maxtax], unsigned cashier_id, unsigned first_sale, unsigned last_sale);
double muestra_comprobantes(PGconn *base, FILE *disp, int tipo_factur, 
                            double *iva, double *ieps, bool parcial,
                            unsigned cashier_id, long f_sale, long l_sale);
void genera_corte(int parcial, PGconn *con, FILE *disp, unsigned cashier_number,
                  long first_sale, long last_sale);
void   clean_records(PGconn *base, long f_sale);
char *procesa(char opcion, PGconn *con, FILE *disp);
int read_config();
int init_config();
int strToUpper(char *strOrigin);
int verif_passwd(PGconn *con, gchar *login, gchar *passwd);

char buff[maxbuf];
char nm_file[maxbuf];
char lp_printer[maxbuf];
char tipo_imp[maxbuf];
char msg[mxbuff];
struct db_data db;
char *home_directory;
char *log_name;
char *nm_disp_ticket,           /* Nombre de impresora de ticket */
  *lp_disp_ticket,      /* Definición de miniprinter en /etc/printcap */
  *disp_lector_serie,  /* Ruta al scanner de c. de barras serial */
  *nmfpie,              /* Pie de página de ticket */
  *nmfenc,              /* Archivo de encabezado de ticket */
  *nmtickets,   /* Registro de tickets impresos */
  *tipo_disp_ticket, /* Tipo de miniprinter */
  *nmimprrem,           /* Ubicación de imprrem */
  *nm_factur,    /* Nombre del progreama de facturación */
  *nm_avisos,    /* Nombre del archivo de avisos */
  *nm_journal,   /* Nombre actual del archivo del diario de marcaje */
  *nm_orig_journal, /* Nombre del archivo del diario original */
  *nm_sendmail,  /* Ruta completa de sendmail */
  *dir_avisos,   /* email de notificación */
  *asunto_avisos, /* Asunto del correo de avisos */
  *cc_avisos;     /* con copia para */
char   s_divisa[3];       /* Designación de la divisa que se usa para cobrar en la base de datos */
double TAX_PERC_DEF; /* Porcentaje de IVA por omisión */
short unsigned search_2nd;  /* ¿Buscar código alternativo al mismo tiempo que el primario ? */ 
int id_teller = 0;
short unsigned manual_discount; /* Aplicar el descuento manual de precio en la misma línea */
int iva_incluido;
int listar_neto;

double suma_pagos(PGconn *base, int tipo_pago, bool parcial, double *utilidad, double *iva,
                  double tax[maxtax], unsigned cashier_id, unsigned f_sale, unsigned l_sale)
{
  char      *comando;
  int       i,j;
  double    total;
  PGresult* res;

  comando = calloc(1,mxbuff*2);

  sprintf(comando,
          "SELECT sum(v.monto), sum(v.utilidad), sum(v.iva), ");

  if (f_sale) {
    sprintf(comando, "%ssum(v.tax_0), sum(v.tax_1), sum(v.tax_2), sum(v.tax_3), sum(v.tax_4), sum(v.tax_5) FROM ventas v, corte c WHERE v.numero=c.numero AND v.tipo_pago=%d AND v.numero>=%u" ,
            comando, tipo_pago, f_sale);
    if (l_sale)
      sprintf(comando+strlen(comando), " AND v.numero<=%u", l_sale);
  }
  else {
    sprintf(comando, "%s sum(v.tax_0), sum(v.tax_1), sum(v.tax_2), sum(v.tax_3), ", comando);
    strcat(comando, "sum(v.tax_4), sum(v.tax_5) FROM ventas v, corte c ");

    if (cashier_id)
      sprintf(comando+strlen(comando), "WHERE (v.id_cajero=%d AND v.tipo_pago=%d ",
              cashier_id, tipo_pago);
    else
      sprintf(comando+strlen(comando),
              "WHERE (v.tipo_pago=%d ", tipo_pago);

    strcat(comando, "AND c.numero=v.numero AND ");
 
   if (parcial)
      strcat(comando, "(c.bandera & B'10000000')!=B'10000000') ");
    else
      strcat(comando, "(c.bandera & B'01000000')!=B'01000000')");
  }
    
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"Error al consultar registros de ventas (suma_pagos)\n");
    PQclear(res);
    return(SQL_ERROR);
  }


  for (i=0, total=0; i<PQntuples(res); i++) {
    total += atof(PQgetvalue(res, i, 0));
    (*utilidad) += atof(PQgetvalue(res, i, 1));
    (*iva) += atof(PQgetvalue(res, i, 2));
    for (j=0; j<maxtax; j++)
      tax[j] += atof(PQgetvalue(res, i, 3+j));
  }

  PQclear(res);

  return(total);
}

double muestra_comprobantes(PGconn *base, FILE *disp, int tipo_factur,
                            double *iva, double *isuntuario, bool parcial,
                            unsigned cashier_id, long f_sale, long l_sale)
{
  char      *comando = NULL;
  int       i;
  double    total;
  PGresult* res;


  comando = calloc(1,mxbuff*2);
  if (f_sale) {
    sprintf(comando,
            "SELECT v.numero,v.monto,v.tipo_pago,v.iva,v.tax_0,v.id_cajero,CASE WHEN (c.bandera&B'00010000'=B'00010000') THEN '*' ELSE ' ' END AS x FROM ventas v, corte c WHERE c.numero=v.numero AND v.tipo_factur=%d AND v.numero>=%ld" ,
            tipo_factur, f_sale);
    if (l_sale)
      sprintf(comando+strlen(comando), " AND v.numero<=%ld", l_sale);
  }
  else {
    if (cashier_id)
      sprintf(comando,
              "SELECT v.numero,v.monto,v.tipo_pago,v.iva,v.tax_0,v.id_cajero,CASE WHEN (c.bandera&B'00010000'=B'00010000') THEN '*' ELSE ' ' END AS x FROM ventas v, corte c WHERE (v.numero=c.numero AND v.id_cajero=%d AND v.tipo_factur=%d",
              cashier_id, tipo_factur);
    else
      sprintf(comando,
              "SELECT v.numero,v.monto,v.tipo_pago,v.iva,v.tax_0,v.id_cajero,CASE WHEN (c.bandera&B'00010000'=B'00010000') THEN '*' ELSE ' ' END AS x FROM ventas v, corte c WHERE (v.numero=c.numero AND v.tipo_factur=%d",
              tipo_factur);
    
    if (parcial)
      strcat(comando, " AND (c.bandera & B'10000000')!=B'10000000'");
    else
      strcat(comando, " AND (c.bandera & B'01000000')!=B'01000000'");
    strcat(comando, ")");
  }
  strcat(comando, " ORDER BY v.numero");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"Error al consultar registros de ventas (muestra_comprobantes)\n");
    free(comando);
    PQclear(res);
    return(SQL_ERROR);
  }


/*  strcpy(art.codigo,PQgetvalue(res,registro,campo));  */

  switch (tipo_factur) {
    case _NOTA_MOSTRADOR:
      fprintf(disp, "Ventas con NOTA de mostrador:\n");
    break;
    case _FACTURA:
      fprintf(disp, "Ventas con FACTURA:\n");
    break;
    case _TEMPORAL:
    default:
      fprintf(disp, "Ventas con TICKET:\n");
  }

  fprintf(disp, " Num. de    Monto    Forma de  Num. de\n");
  fprintf(disp, "  venta                pago    cajero\n");
  for (i=0, total=0; i<PQntuples(res); i++) {
    total   += atof(PQgetvalue(res, i,  1));
    (*iva)  += atof(PQgetvalue(res, i, 3));
    (*isuntuario) += atof(PQgetvalue(res, i, 4));
    sprintf(comando, "%1s%5s $%10.2f ",
            PQgetvalue(res, i, 6), PQgetvalue(res, i, 0), atof(PQgetvalue(res, i, 1)) );
    fprintf(disp, comando);
    switch ( atoi(PQgetvalue(res, i, 2)) ) {
      case _PAGO_CHEQUE:
        fprintf(disp, "Cheque   ");
      break;
      case _PAGO_TARJETA:
        fprintf(disp, "Tarjeta  ");
      break;
      case _PAGO_CREDITO:
        fprintf(disp, "Credito  ");
      break;
      case _PAGO_EFECTIVO:
      default:
        fprintf(disp, "Efectivo ");
    }
    fprintf(disp, "%6d\n", atoi(PQgetvalue(res, i, 5)));
  }
  if (PQntuples(res)) {
    fprintf(disp, "--------------------------------------\n");
    fprintf(disp, "   TOTAL: $%10.2f\n", total);
    fprintf(disp, "  I.V.A.: $%10.2f\n", *iva);
    fprintf(disp, " Imp. S.: $%10.2f\n", *isuntuario);
  }
  else
    fprintf(disp, "No hay ventas a reportar\n");

  fprintf(disp, "  ====================================\n\n");
  PQclear(res);

  return(total);
}

void clean_records(PGconn *base, long f_sale)
{
  PGresult *res;
  char     mensaje[255];

  clear();

  if (f_sale == 0) {
    sprintf(mensaje, "SELECT min(numero) FROM corte WHERE (bandera & B'01000000')!=B'01000000' ");
    res = PQexec(base, mensaje);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr, "Error, no puedo consultar número de primera venta\n");
      fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
      return;
    }
    else {
      if (PQntuples(res))
        f_sale = atol(PQgetvalue(res, 0, 0));
      else
        f_sale = 0;

      PQclear(res);
    }
  }
  if (!f_sale) {
    printw("No hay ventas en el día de hoy\n");
    /* There are no records today */
  }

  sprintf(mensaje,
          "UPDATE corte SET bandera=(bandera | B'01000000') WHERE numero>=%ld AND (bandera & B'01000000')!=B'01000000'",
          f_sale);

  res = PQexec(base, mensaje);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    strcpy(mensaje, PQerrorMessage(base));
    if (strlen(mensaje)) {
      printw("No puedo modificar los registros de ventas.\n");
      /* Can't modify day records */
      printw("Mensaje de error: %s\n", mensaje);
      /* Error message: */
      PQclear(res);
    }
  }

  mvprintw(getmaxy(stdscr)-1, 0, "Presione una tecla para continuar...");
  /* Press any key */
  getch();
  clear();
  PQclear(res);
}

void marca_revisados(PGconn *base, int num_cajero, long f_sale)
{
  PGresult *res;
  char     *mensaje;

  mensaje = calloc(1,255);

  if (f_sale == 0) {
    sprintf(mensaje, "SELECT min(numero) FROM corte WHERE (bandera & B'10000000')!=B'10000000' ");
    res = PQexec(base, mensaje);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr, "Error, no puedo consultar número de primera venta\n");
      fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
      return;
    }
    else {
      if (PQntuples(res))
        f_sale = atol(PQgetvalue(res, 0, 0));
      PQclear(res);
    }
  }

  res = PQexec(base, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error, no se pudo comenzar la transacción para actualizar\n");
    fprintf(stderr, "       registros de ventas\n");
    fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
    return;
  }
  PQclear(res);


  sprintf(mensaje, "UPDATE corte SET bandera=(bandera | B'10000000')");
  if (num_cajero>=0)
    sprintf(mensaje, "%s FROM ventas v WHERE corte.numero>=%ld AND v.numero=corte.numero AND v.id_cajero=%d ",
            mensaje, f_sale, num_cajero);
  else
    sprintf(mensaje, "%s WHERE corte.numero>=%ld AND bandera&B'10000000'=B'10000000' ", mensaje, f_sale);

  res = PQexec(base, mensaje);
  mensaje[0]=0;
  
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    strcpy(mensaje, PQerrorMessage(base));
    clear();
    move(getmaxy(stdscr)/2, 0);
    if (strlen(mensaje)) {
      printw("No se pudieron actualizar los registros del día.\n");
      /* Can't update daily records */
      printw("Mensaje de error: %s\n", mensaje);
      /* Error message: */
    }
    else {
      printw("Se produjo un error inesperado al actualizar los registros del día...");
    }
    PQclear(res);
    mvprintw(getmaxy(stdscr)-1, 0, "Presione una tecla para continuar...");
    getch();
    clear();
  }
  free(mensaje);

  res = PQexec(base, "END");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error, no se pudo terminar la transacción para actualizar\n");
    fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
    return;
  }
  PQclear(res);
}

void genera_corte(int parcial, PGconn *con, FILE *disp, unsigned cashier_id,
                  long f_sale, long l_sale)
{
  int i;
  time_t    tiempo;
  struct tm *fecha;
  double    gran_total = 0.0,
    subtotal   = 0.0,
    util       = 0.0,
    iva_total  = 0.0,
    iva[3], /* Iva de ticket, factura y nota de mostrador */
    isuntuario[3], /* idem */
    tax[5];

  disp = fopen(nm_file, "w");
  if (disp==NULL) {
    ErrorArchivo(nm_file);
    exit(FILE_1_ERROR);
  }

  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  for (i=0; i<maxtax; i++)
    tax[i]=0.0;
  for (i=0; i<3; i++) {
    iva[i] = 0.0;
    isuntuario[i] = 0.0;
  }

  if (parcial<=1) {
    fprintf(disp, "Corte ");
    if (parcial) {
      fprintf(disp, "parcial ");
      if (cashier_id)
        fprintf(disp, "de cajero num. %u\n", cashier_id);
      else
        fprintf(disp, "de todos los cajeros\n");
    }
  }
  else if(parcial==2) {
    fprintf(disp, "Reporte de ventas por n%cmero\n", 163);
  }

  fprintf(disp,"realizado el d%ca %d/%d/%d\n a las %d:", 161,
          fecha->tm_mday, (fecha->tm_mon)+1, fecha->tm_year+1900, fecha->tm_hour);
  if (fecha->tm_min<10)
    fprintf(disp, "0");
  fprintf(disp, "%d:", fecha->tm_min);
  if (fecha->tm_sec<10)
    fprintf(disp, "0");
  fprintf(disp, "%d hrs.\n\n", fecha->tm_sec);

  muestra_comprobantes(con, disp, _NOTA_MOSTRADOR, &iva[0], &isuntuario[0],
                       parcial, cashier_id, f_sale, l_sale);
  muestra_comprobantes(con, disp, _FACTURA, &iva[1], &isuntuario[1],
                       parcial, cashier_id, f_sale, l_sale);
  muestra_comprobantes(con, disp, _TEMPORAL, &iva[2], &isuntuario[2],
                       parcial, cashier_id, f_sale, l_sale);

  fprintf(disp, "\n**** INGRESOS ****\n");
  subtotal = suma_pagos(con, _PAGO_EFECTIVO, parcial, &util, &iva_total, tax, cashier_id, f_sale, l_sale);
  fprintf(disp, "         Efectivo: %12.2f\n", subtotal);
  gran_total += subtotal;
  subtotal = suma_pagos(con, _PAGO_CHEQUE, parcial, &util, &iva_total, tax, cashier_id, f_sale, l_sale);
  fprintf(disp, "          Cheques: %12.2f\n", subtotal);
  gran_total += subtotal;
  subtotal = suma_pagos(con, _PAGO_CREDITO, parcial, &util, &iva_total, tax, cashier_id, f_sale, l_sale);
  fprintf(disp, "          Cr%cdito: %12.2f\n", 130, subtotal);
  gran_total += subtotal;
  subtotal = suma_pagos(con, _PAGO_TARJETA, parcial, &util, &iva_total, tax, cashier_id, f_sale, l_sale);
  fprintf(disp, "         Tarjetas: %12.2f\n", subtotal);
  gran_total += subtotal;
  fprintf(disp, "       GRAN TOTAL: %12.2f\n\n", gran_total);

  fprintf(disp, "**** IMPUESTOS ****\n");
  fprintf(disp, "     I.V.A. total: %12.2f\n", iva_total);
  fprintf(disp, "Imp. Suntu. total: %12.2f\n", tax[0]);
  fprintf(disp, "       Impuesto 1: %12.2f\n", tax[1]);
  fprintf(disp, "       Impuesto 2: %12.2f\n", tax[2]);
  fprintf(disp, "       Impuesto 3: %12.2f\n", tax[3]);
  fprintf(disp, "       Impuesto 4: %12.2f\n", tax[4]);
  fprintf(disp, "       Impuesto 5: %12.2f\n\n", tax[5]);

  fprintf(disp, "Utilidades brutas: %12.2f\n\n\n\n\n\n", util);
  fclose(disp);

  if (parcial)
    marca_revisados(con, cashier_id, f_sale);
  else
    clean_records(con, f_sale);

}

char *procesa(char opcion, PGconn *con, FILE *disp)
{
  int cashier_num;
  unsigned first_sale=0, last_sale=0;
  char query[maxbuf*2];
  PGresult* res;

  move(getmaxy(stdscr),0);
  clrtoeol();
  switch (opcion) {
  case '1':
    sprintf(query, "SELECT min(numero) FROM corte WHERE bandera&B'01000000'!=B'01000000' ");
    res = PQexec(con, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      strcpy(msg, PQerrorMessage(con));
      if (strlen(msg)) {
        printw("No puedo obtener numero de primera venta.\n");
        /* Can't modify day records */
        printw("Mensaje de error: %s\n", msg);
        /* Error message: */
        PQclear(res);
      }
    }
    else
      if (PQntuples(res))
        first_sale = atoi(PQgetvalue(res, 0, 0));
    genera_corte(FALSE, con, disp, 0, first_sale, 0);
    sprintf(msg, "Corte de día generado.");
    break;
  case '2':
    sprintf(query, "SELECT min(numero) from corte where bandera&B'10000000'!=B'\10000000' ");
    res = PQexec(con, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      strcpy(msg, PQerrorMessage(con));
      if (strlen(msg)) {
        printw("No puedo obtener numero de primera.\n");
        /* Can't modify day records */
        printw("Mensaje de error: %s\n", msg);
        /* Error message: */
        PQclear(res);
      }
    }
    else
      if (PQntuples(res))
        first_sale = atoi(PQgetvalue(res, 0, 0));
     genera_corte(TRUE, con, disp, 0, first_sale, 0);
    sprintf(msg,  "Corte parcial generado.");
    break;
  case '3':
    mvprintw(getmaxy(stdscr)-2, 0, "Indique el número del cajero a reportar: ");
    wgetnstr(stdscr, msg, mxbuff-1);
    /****** Catch the user error ******/
    cashier_num = atoi(msg);
    genera_corte(TRUE, con, disp, cashier_num, 0, 0);
    sprintf(msg,  "Corte parcial de cajero %d generado.", cashier_num);
    break;
  case '4':
    printw("\n Número de primera venta: ");
    wgetnstr(stdscr, msg, mxbuff-1);
    first_sale = atoi(msg);
    printw("Última venta (0 para todas las restantes): ");
    wgetnstr(stdscr, msg, mxbuff-1);
    last_sale = atoi(msg);
    genera_corte(2, con, disp, 0, first_sale, last_sale);
    break;
  case '5':
    sprintf(buff, "less -rf %s", nm_file);
    system(buff);
    break;
  case '6': 
    sprintf(buff, "lpr -P %s %s", lp_printer, nm_file);
    system(buff);
    sprintf(msg, "El corte se mandó a la cola de impresión %s.",
             lp_printer);
    break;
  case '7': 
    sprintf(buff, "pico %s", nm_file);
    system(buff);
    break;
  case '8':
    clean_records(con, first_sale);
    sprintf(msg, "Registros eliminados.");
    break;
  case 'V' :
    sprintf(buff, "less -rf /usr/share/doc/osopos/LICENCIA");
    system(buff);
    break;
  default:
    beep();
  }
  return(msg);
}


int read_config() {
  gchar      *nmconfig;
  FILE       *config;
  char       buf[maxbuf];
  char       *b;
  char       home_directory[maxbuf];
  char       *aux = NULL;
  static int i;


  strcpy(home_directory, getenv("HOME"));

  nmconfig = g_strdup_printf("%s/.osopos/corte.config", home_directory);

  config = fopen(nmconfig,"r");
  if (!config) { /* No existe archivo de configuración de usuario */
    sprintf(nmconfig, "/etc/osopos/corte.config");
    config = fopen(nmconfig,"r");
  }


  if (config) {		/* Si existe archivo de configuración */
    b = buff;
    fgets(buff,sizeof(buff),config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;

      if (!strlen(buff) || buff[0] == '#') { /* Linea vacía o coment. */
        fgets(buff,sizeof(buff),config);
        continue;
      }

      strcpy(buf, strtok(buff,"="));
	/* La función strtok modifica el contenido de la cadena buff	*/
	/* remplazando con NULL el argumento divisor (en este caso "=") */
	/* por lo que b queda apuntando al primer token			*/

	/* Busca parámetros de impresora */
      if (!strcmp(b,"archivo_corte")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(nm_file,buf);
      }
      else if (!strcmp(b,"impresora.lpr")) {
        strncpy(buf, strtok(NULL,"="), maxbuf);
        strncpy(lp_printer,buf, maxbuf);
      }
      else if (!strcmp(b,"impresora.tipo")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(tipo_imp,buf);
        strToUpper(tipo_imp);
        //~ for (i=0; i<strlen(tipo_imp); i++)
          //~ tipo_imp[i] = toupper(tipo_imp[i]);
      }
      else if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.usuario")) {
        g_free(db.user);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.user = g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.passwd")) {
        g_free(db.passwd);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.passwd =  g_strdup_printf("%s", buf);
      }
      if (!feof(config))
        fgets(buff,mxbuff,config);
    }
    fclose(config);
    return(0);
  }
  return(1);  
}

int read_global_config()
{
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;

  
  nmconfig = calloc(1, 255);
  strncpy(nmconfig, "/etc/osopos/corte.config", 255);

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuración */
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
        /* La función strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

        /* Definición de impresora de ticket */
      if (!strcmp(b,"ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b, "lp_ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(lp_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(lp_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"miniimpresora.tipo")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(tipo_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(tipo_disp_ticket,buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"porcentaje_iva")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        TAX_PERC_DEF = atoi(buf);
      }
      else if (!strcmp(b,"avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"email.avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(dir_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(dir_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"asunto.email")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(asunto_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(asunto_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"cc.avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        free(cc_avisos);
        cc_avisos = NULL;
        cc_avisos = calloc(1, strlen(buf)+1);
        if (cc_avisos != NULL) {
          strcpy(cc_avisos, buf);
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"programa.sendmail")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_sendmail, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_sendmail,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
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

/***************************************************************************/

int read_general_config()
{
  gchar *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;

  
  nmconfig = g_strdup_printf("/etc/osopos/general.config");

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuración */
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
        /* La función strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

        /* Definición de impresora de ticket */
      if (!strcmp(b,"ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b, "lp_ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(lp_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(lp_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"miniimpresora.tipo")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(tipo_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(tipo_disp_ticket,buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"programa.factur")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_factur, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_factur, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "corte. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.port")) {
        g_free(db.hostport);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.hostport =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.nombre")) {
        g_free(db.name);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.name =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.sup_usuario")) {
        g_free(db.sup_user);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_user =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.sup_passwd")) {
        g_free(db.sup_passwd);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_passwd =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"porcentaje_iva")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        TAX_PERC_DEF = atoi(buf);
      }
      else if (!strcmp(b,"avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"email.avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(dir_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(dir_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"asunto.email")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(asunto_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(asunto_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"cc.avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        free(cc_avisos);
        cc_avisos = NULL;
        cc_avisos = calloc(1, strlen(buf)+1);
        if (cc_avisos != NULL) {
          strcpy(cc_avisos, buf);
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"programa.sendmail")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_sendmail, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_sendmail,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
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
    g_free(nmconfig);
    b = NULL;
    return(0);
  }
  if (aux != NULL)
    aux = NULL;
  g_free(nmconfig);
  b = NULL;
  return(1);
}

/***************************************************************************/

int init_config()
{
  home_directory = calloc(1, 255);
  strcpy(home_directory, getenv("HOME"));

  log_name = calloc(1, 255);
  strcpy(log_name, getenv("LOGNAME"));

  sprintf(nm_file,"/tmp/osopos_corte");

  sprintf(tipo_imp,"EPSON");

  strncpy(lp_printer, "ticket", maxbuf);


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
  return(OK);
}

int main()
{
  PGconn    *con, *con_s;
  char      opcion;
  FILE      *disp;
  struct usuario usr;
  short i=0;

  init_config();
  read_general_config();
  read_global_config();
  read_config();
  initscr();
  if (!has_colors()) {
    mvprintw(getmaxy(stdscr)/2,0,
      "Este equipo no puede producir colores, pulse una tecla para abortar...");
    getch();
    printw("Abortando");
    endwin();
    exit(10);
  }
  msg[0]=0;

  start_color();
  init_pair(blanco_sobre_negro, COLOR_WHITE, COLOR_BLACK);
  init_pair(amarillo_sobre_negro, COLOR_YELLOW, COLOR_BLACK);
  init_pair(verde_sobre_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(azul_sobre_blanco, COLOR_BLUE, COLOR_WHITE);
  init_pair(cyan_sobre_negro, COLOR_CYAN, COLOR_BLACK);
  init_pair(amarillo_sobre_azul, COLOR_YELLOW, COLOR_BLUE);
  init_pair(normal, COLOR_WHITE, COLOR_BLACK);
  init_pair(inverso, COLOR_BLACK, COLOR_WHITE);

  con_s = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd);
  if (con_s == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }
  usr.login = g_strdup(obten_login());
  usr.passwd = g_strdup(obten_passwd(usr.login));
  i = verif_passwd(con_s, usr.login, usr.passwd);

  if (i == FALSE) {
    aborta("Nombre de usuario/contraseña incorrecto\n", ERROR_DIVERSO);
  }

/*   con = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.user, db.passwd);  */

/*  if (!con) { */
/*     mvprintw(getmaxy(stdscr)/2, 0, */
/*       "ERROR FATAL: No se puede abrir la base de datos. Pulse una tecla para abortar..."); */
/*     getch(); */
/*     exit(SQL_ERROR); */
/*   } */

  if (!puede_hacer(con_s, usr.login, "caja_corte")) {
    mensaje_v("No esta autorizado para hacer corte de caja. <Esc>", 
              azul_sobre_blanco, ESC);
    PQfinish(con_s);
    exit(ERROR_DIVERSO);   
  }

  mvprintw(getmaxy(stdscr)-2, 0, "\"corte\" se brinda SIN GARANTIA. Presione V para más información");
  move(0, 0);
  do {
    printw("Programa de corte de caja, v. %s. (C) 1999-2009. E. Israel Osorio H.\n",
           VERSION);
    printw("elpuntodeventa.com\n\n");
    mvprintw(getmaxy(stdscr)-1, 0, msg);
    mvprintw( 3,1,"Indique la operación a realizar:");
    mvprintw( 5,3,"1. Corte de dia");
    mvprintw( 6,3,"2. Corte parcial de ventas en general");
    mvprintw( 7,3,"3. Corte parcial por cajero");
    mvprintw( 8,3,"4. Resumen general de ventas por número");
    mvprintw( 9,3,"5. Ver los reportes generados");
    mvprintw(11,3,"6. Imprimir los reportes generados");
    mvprintw(12,3,"7. Editar los reportes generados");
    mvprintw(13,3,"8. Limpiar todos los registros de ventas (iniciar nuevo día)");
    mvprintw(15,3,"V. Ver licencia de uso");
    mvprintw(18,3,"S. Salir");
    mvprintw(20,1,"Opción: ");

    msg[0] = 0;
    opcion = toupper(getch());
    if (opcion != 'S') {
      strcpy(msg, procesa(opcion, con_s, disp));
      clear();
    }
  }
  while (opcion != 'S');

  PQfinish(con);
  PQfinish(con_s);
  endwin();
  //  free(lp_printer);
  //  free(nm_file);
  //  free(tipo_imp);
  free(nm_avisos);
  nm_avisos = NULL;
  free(nm_sendmail);
  nm_sendmail = NULL;
  free(dir_avisos);
  dir_avisos = NULL;
  free(db.hostname);
  db.hostname = NULL;
  free(db.hostport);
  db.hostport = NULL;
  /*if (cc_avisos != NULL)
    free(cc_avisos);*/
  free(asunto_avisos);
  asunto_avisos = NULL;
  free(tipo_disp_ticket);
  tipo_disp_ticket = NULL;
  free(lp_disp_ticket);
  free(nm_avisos);
  nm_avisos = NULL;
  free(nmfpie);
  nmfpie = NULL;
  free(nmfenc);
  nmfenc = NULL;
  free(nmimprrem);
  nmimprrem = NULL;
  free(log_name);
  log_name = NULL;
  free(home_directory);
  free(nm_disp_ticket);
  free(nm_journal);
  free(nm_orig_journal);

  return(OK);
}
