/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa Corte 0.10 (C) 1999-2002 E. Israel Osorio H.
   desarrollo@elpuntodeventa.com
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
#include <time.h>
#include <unistd.h>

#define VERSION "0.10"
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

double suma_pagos(PGconn *base, int tipo_pago, bool parcial, double *utilidad, double *iva,
                  double tax[maxtax], unsigned cashier_id, unsigned first_sale, unsigned last_sale);
double muestra_comprobantes(PGconn *base, FILE *disp, int tipo_factur, 
                            double *iva, double *ieps, bool parcial,
                            unsigned cashier_id, long f_sale, long l_sale);
void genera_corte(int parcial, PGconn *con, FILE *disp, unsigned cashier_number,
                  long first_sale, long last_sale);
void   clean_records(PGconn *base);
char *procesa(char opcion, PGconn *con, FILE *disp);
int read_config();
int init_config();

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

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando BEGIN al buscar en registro de ventas\n");
    PQclear(res);
    return(SQL_ERROR);
  }
  PQclear(res);

  comando = calloc(1,mxbuff*2);


  if (f_sale) {
    sprintf(comando,
            "DECLARE cursor_v CURSOR FOR SELECT sum(v.monto), sum(v.utilidad), sum(v.iva), sum(v.tax_0), sum(v.tax_1), sum(v.tax_2), sum(v.tax_3), sum(v.tax_4), sum(v.tax_5) FROM ventas v, corte c WHERE v.tipo_pago=%d AND v.numero>=%u" ,
            tipo_pago, f_sale);
    if (l_sale)
      sprintf(comando+strlen(comando), " AND v.numero<=%u", l_sale);
  }
  else {
    sprintf(comando, "DECLARE cursor_v CURSOR FOR SELECT sum(v.monto), sum(v.utilidad), sum(v.iva), ");
    strcat(comando, "sum(v.tax_0), sum(v.tax_1), sum(v.tax_2), sum(v.tax_3), ");
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
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando DECLARE CURSOR al buscar los registro de ventas\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    PQclear(res);
    return(SQL_ERROR);
  }
  PQclear(res);

  comando = "FETCH ALL in cursor_v";
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
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

  res = PQexec(base, "CLOSE cursor_v");
  PQclear(res);

  res = PQexec(base, "END");
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

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr,"Falló comando BEGIN al buscar en registro de ventas\n");
   PQclear(res);
   return(SQL_ERROR);
  }
  PQclear(res);


  comando = calloc(1,mxbuff*2);
  if (f_sale) {
    sprintf(comando,
            "DECLARE cursor_v CURSOR FOR SELECT v.numero,v.monto,v.tipo_pago,v.iva,v.tax_0,v.id_cajero,CASE WHEN (c.bandera&B'00010000'=B'00010000') THEN '*' ELSE ' ' END AS x FROM ventas v, corte c WHERE c.numero=v.numero AND v.tipo_factur=%d AND v.numero>=%ld" ,
            tipo_factur, f_sale);
    if (l_sale)
      sprintf(comando+strlen(comando), " AND v.numero<=%ld", l_sale);
  }
  else {
    if (cashier_id)
      sprintf(comando,
              "DECLARE cursor_v CURSOR FOR SELECT v.numero,v.monto,v.tipo_pago,v.iva,v.tax_0,v.id_cajero,CASE WHEN (c.bandera&B'00010000'=B'00010000') THEN '*' ELSE ' ' END AS x FROM ventas v, corte c WHERE (v.numero=c.numero AND v.id_cajero=%d AND v.tipo_factur=%d",
              cashier_id, tipo_factur);
    else
      sprintf(comando,
              "DECLARE cursor_v CURSOR FOR SELECT v.numero,v.monto,v.tipo_pago,v.iva,v.tax_0,v.id_cajero,CASE WHEN (c.bandera&B'00010000'=B'00010000') THEN '*' ELSE ' ' END AS x FROM ventas v, corte c WHERE (v.numero=c.numero AND v.tipo_factur=%d",
              tipo_factur);
    
    if (parcial)
      strcat(comando, " AND (c.bandera & B'10000000')!=B'10000000'");
    else
      strcat(comando, " AND (c.bandera & B'01000000')!=B'01000000'");
    strcat(comando, ")");
  }
  strcat(comando, " ORDER BY v.numero");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando DECLARE CURSOR al buscar los registro de ventas\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    free(comando);
    PQclear(res);
   return(SQL_ERROR);
  }
  PQclear(res);

  strcpy(comando, "FETCH ALL in cursor_v");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
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

  res = PQexec(base, "CLOSE cursor_v");
  PQclear(res);

  res = PQexec(base, "END");
  PQclear(res);
  return(total);
}

void clean_records(PGconn *base)
{
  PGresult *res;
  char     mensaje[255];
  FILE     *f_print;

  clear();
  if ((f_print = fopen(nm_file, "r"))) {
    fclose(f_print);
    mvprintw(getmaxy(stdscr)/2, 0,
             "ADVERTENCIA: Ya existe un archivo de impresión de corte.\n");
    printw("             Si continua, este se reemplazará.\n");
    /* trans. WARNING: There is a print file. If you proceed, it will be replaced */
    printw("¿Está seguro de continuar? N\b");
    /* trans. Are you sure? */
    if (toupper(getch()) != 'S')
      return;
    printw("\n");
  }
  res = PQexec(base, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    printw("Error, no se pudo comenzar la transacción para borrar\n");
    printw("       registros de ventas\n");
    printw("Mensaje de error: %s\n", PQerrorMessage(base));
    return;
  }
  PQclear(res);

  PQexec(base, "UPDATE corte SET bandera=(bandera | B'01000000')");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    strcpy(mensaje, PQerrorMessage(base));
    if (strlen(mensaje)) {
      printw("No puedo modificar los registros de ventas.\n");
      /* Can't modify day records */
      printw("Mensaje de error: %s\n", mensaje);
      /* Error message: */
      PQclear(res);
    }
    else  {
      printw("No hay ventas en el día de hoy\n");
      /* There are no records today */
    }
    mvprintw(getmaxy(stdscr)-1, 0, "Presione una tecla para continuar...");
    /* Press any key */
    getch();
    clear();
  }

  res = PQexec(base, "END");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error, no se pudo terminar la transacción para borrar ventas\n");
    fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
    return;
  }

  PQclear(res);
}

void marca_revisados(PGconn *base, int num_cajero)
{
  PGresult *res;
  char     *mensaje;

  res = PQexec(base, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error, no se pudo comenzar la transacción para actualizar\n");
    fprintf(stderr, "       registros de ventas\n");
    fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
    return;
  }
  PQclear(res);

  mensaje = calloc(1,255);

  sprintf(mensaje, "UPDATE corte SET bandera=(bandera | B'10000000')");
  if (num_cajero>=0)
    sprintf(mensaje, "%s FROM ventas v WHERE v.id_cajero=%d AND v.numero=corte.numero ", mensaje, num_cajero);

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

  if (!f_sale) {
    if (parcial)
      marca_revisados(con, cashier_id);
    else
      clean_records(con);
  }
}

char *procesa(char opcion, PGconn *con, FILE *disp)
{
  int cashier_num;
  unsigned first_sale, last_sale;

  move(getmaxy(stdscr),0);
  clrtoeol();
  switch (opcion) {
  case '1':
    genera_corte(FALSE, con, disp, 0, 0, 0);
    sprintf(msg, "Corte de día generado.");
    break;
  case '2':
    genera_corte(TRUE, con, disp, 0, 0, 0);
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
    clean_records(con);
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
  char       nmconfig[maxbuf];
  FILE       *config;
  FILE       *process;
  char       buf[maxbuf];
  char       *b;
  char       home_directory[maxbuf];
  char       *aux = NULL;
  static int i;


  sprintf(home_directory, ".");

  process = popen("printenv HOME", "r");
  if (process != NULL) {
    fgets(home_directory, maxbuf, process);
    home_directory[strlen(home_directory)-1] = 0;
    pclose(process);
  }
  sprintf(nmconfig, "%s/.osopos/corte.config", home_directory);

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
        for (i=0; i<strlen(tipo_imp); i++)
          tipo_imp[i] = toupper(tipo_imp[i]);
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
      else if (!strcmp(b,"db.usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "corte. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.passwd")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.passwd = calloc(1, strlen(buf)+1);
        if (db.passwd != NULL) {
          strcpy(db.passwd,buf);
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
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
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;

  
  nmconfig = calloc(1, 255);
  strncpy(nmconfig, "/etc/osopos/general.config", 255);

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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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

int init_config()
{
  FILE *env_process;

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
}

int main()
{
  PGconn    *con, *con_s;
  char      opcion;
  FILE      *disp;

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

  init_config();
  read_general_config();
  read_global_config();
  read_config();

  con_s = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd);
  if (con_s == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }
  //  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, "osopos", log_name, "");
  if (db.passwd == NULL) {
    db.passwd = calloc(1, mxbuff);
    obten_passwd(db.user, db.passwd);
  }

  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.user, db.passwd); 

 if (!con) {
    mvprintw(getmaxy(stdscr)/2, 0,
      "ERROR FATAL: No se puede abrir la base de datos. Pulse una tecla para abortar...");
    getch();
    exit(SQL_ERROR);
  }

  mvprintw(getmaxy(stdscr)-2, 0, "\"corte\" se brinda SIN GARANTIA. Presione V para más información");
  move(0, 0);
  do {
    printw("Programa de corte de caja, v. %s. (C) 1999-2003. E. Israel Osorio H.\n",
           VERSION);
    printw("soporte@elpuntodeventa.com\n\n");
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
      strcpy(msg, procesa(opcion, con, disp));
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
