/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa Corte 0.05 (C) 1999-2001 E. Israel Osorio H.
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

#define VERSION "0.05"
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

double suma_pagos(PGconn *base, int tipo_pago, bool parcial, double *utilidad);
double muestra_comprobantes(PGconn *base, FILE *disp, int tipo_factur, bool parcial,
                            unsigned cashier_id);
void genera_corte(int parcial, PGconn *con, FILE *disp, unsigned cashier_number);
void   clean_records(PGconn *base);
char *procesa(char opcion, PGconn *con, FILE *disp);
int read_config();

char buff[maxbuf];
char nm_file[maxbuf];
char lp_printer[maxbuf];
char tipo_imp[maxbuf];
char msg[mxbuff];

double suma_pagos(PGconn *base, int tipo_pago, bool parcial, double *utilidad)
{
  char      *comando;
  int       i;
  double    total;
  PGresult* res;

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando BEGIN al buscar en registro de ventas\n");
    PQclear(res);
    return(-1);
  }
  PQclear(res);

  comando = calloc(1,mxbuff);
  sprintf(comando,
          "DECLARE cursor_ventas CURSOR FOR SELECT monto,utilidad FROM ventas WHERE (tipo_pago=%d",
          tipo_pago);
  if (parcial)
    strcat(comando, " AND (corte & B'10000000')!=B'10000000'");
  strcat(comando, ")");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando DECLARE CURSOR al buscar los registro de ventas\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    PQclear(res);
    return(-1);
  }
  PQclear(res);

  comando = "FETCH ALL in cursor_ventas";
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
    PQclear(res);
    return(-1);
  }


/*  strcpy(art.codigo,PQgetvalue(res,registro,campo));  */

  for (i=0, total=0; i<PQntuples(res); i++) {
    total += atof(PQgetvalue(res, i, 0));
    (*utilidad) += atof(PQgetvalue(res, i, 1));
  }

  PQclear(res);

  res = PQexec(base, "CLOSE cursor_ventas");
  PQclear(res);

  res = PQexec(base, "END");
  PQclear(res);
  return(total);
}

double muestra_comprobantes(PGconn *base, FILE *disp, int tipo_factur, bool parcial,
                            unsigned cashier_id)
{
  char      *comando;
  int       i;
  double    total;
  PGresult* res;

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr,"Falló comando BEGIN al buscar en registro de ventas\n");
   PQclear(res);
   return(-1);
  }
  PQclear(res);

  /* fetch instances from the pg_database, the system catalog of databases*/
  comando = calloc(1,mxbuff);
  if (cashier_id)
    sprintf(comando,
            "DECLARE cursor_ventas CURSOR FOR SELECT * FROM ventas WHERE (id_cajero=%d AND tipo_factur=%d",
            cashier_id, tipo_factur);
  else
    sprintf(comando,
            "DECLARE cursor_ventas CURSOR FOR SELECT * FROM ventas WHERE (tipo_factur=%d",
            tipo_factur);
    
  if (parcial)
    strcat(comando, " AND (corte & B'10000000')!=B'10000000'");
  else
    strcat(comando, " AND (corte & B'01000000')!=B'01000000'");
  strcat(comando, ")");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Falló comando DECLARE CURSOR al buscar los registro de ventas\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    free(comando);
    PQclear(res);
   return(-1);
  }
  PQclear(res);

  strcpy(comando, "FETCH ALL in cursor_ventas");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
    free(comando);
    PQclear(res);
    return(-1);
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

  fprintf(disp, "Num. de  Monto    Forma de  Num. de\n");
  fprintf(disp, " venta              pago    cajero\n");
  for (i=0, total=0; i<PQntuples(res); i++) {
    total += atof(PQgetvalue(res, i, 1));
    sprintf(comando, "%5s $%10.2f ",
            PQgetvalue(res, i, 0),
            atof(PQgetvalue(res, i, 1)) );
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
    fprintf(disp, "%6d\n", atoi(PQgetvalue(res, i, 7)));
  }
  fprintf(disp, "--------------------------------------\n");
  fprintf(disp, "   TOTAL: $%10.2f\n", total);
  fprintf(disp, "**************************************\n");
  PQclear(res);

  res = PQexec(base, "CLOSE cursor_ventas");
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

  PQexec(base, "UPDATE ventas set corte=(corte | B'01000000')");
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

void marca_revisados(PGconn *base)
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

  res = PQexec(base, "UPDATE ventas SET corte=(corte | B'10000000')");
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

void genera_corte(int parcial, PGconn *con, FILE *disp, unsigned cashier_id)
{
  time_t    tiempo;
  struct tm *fecha;
  double    gran_total = 0.0,
            subtotal   = 0.0,
            util       = 0.0;

  disp = fopen(nm_file, "w");
  if (disp==NULL) {
    ErrorArchivo(nm_file);
    exit(-1);
  }

  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fprintf(disp, "Corte ");
  if (parcial)
    fprintf(disp, "parcial ");

  fprintf(disp,"realizado el dia %d/%d/%d\n a las %d:",
        fecha->tm_mday, (fecha->tm_mon)+1, fecha->tm_year+1900, fecha->tm_hour);
  if (fecha->tm_min<10)
    fprintf(disp, "0");
  fprintf(disp, "%d:", fecha->tm_min);
  if (fecha->tm_sec<10)
    fprintf(disp, "0");
  fprintf(disp, "%d hrs.\n\n", fecha->tm_sec);

  muestra_comprobantes(con, disp, _NOTA_MOSTRADOR, parcial, cashier_id);
  muestra_comprobantes(con, disp, _FACTURA, parcial, cashier_id);
  muestra_comprobantes(con, disp, _TEMPORAL, parcial, cashier_id);

  subtotal = suma_pagos(con, _PAGO_EFECTIVO, parcial, &util);
  fprintf(disp, "\nTotal ingresos efectivo: %12.2f\n", subtotal);
  gran_total += subtotal;
  subtotal = suma_pagos(con, _PAGO_CHEQUE, parcial, &util);
  fprintf(disp, "Total ingresos cheques:  %12.2f\n", subtotal);
  gran_total += subtotal;
  subtotal = suma_pagos(con, _PAGO_CREDITO, parcial, &util);
  fprintf(disp, "Total ingresos credito:  %12.2f\n", subtotal);
  gran_total += subtotal;
  subtotal = suma_pagos(con, _PAGO_TARJETA, parcial, &util);
  fprintf(disp, "Total ingresos tarjetas: %12.2f\n", subtotal);
  gran_total += subtotal;
  fprintf(disp, "             GRAN TOTAL: %12.2f\n", gran_total);
  fprintf(disp, "     Utilidades del día: %12.2f\n\n\n\n\n", util);
  fclose(disp);

  if (parcial)
    marca_revisados(con);
  else
    clean_records(con);

}

char *procesa(char opcion, PGconn *con, FILE *disp)
{
  int cashier_num;

  move(getmaxy(stdscr),0);
  clrtoeol();
  switch (opcion) {
  case '1':
    genera_corte(FALSE, con, disp, 0);
    sprintf(msg, "Corte de día generado.");
    break;
  case '2':
    genera_corte(TRUE, con, disp, 0);
    sprintf(msg,  "Corte parcial generado.");
    break;
  case '3':
    printw("Indique el número del cajero a reportar: ");
    wgetnstr(stdscr, msg, mxbuff-1);
    /****** Catch the user error ******/
    cashier_num = atoi(msg);
    genera_corte(TRUE, con, disp, cashier_num);
    sprintf(msg,  "Corte parcial de cajero %d generado.", cashier_num);
    break;
  case '4':
    sprintf(buff, "less -rf %s", nm_file);
    system(buff);
    break;
  case '5': 
    sprintf(buff, "lpr -P %s %s", lp_printer, nm_file);
    system(buff);
    sprintf(msg, "El corte se mandó a la cola de impresión %s.",
             lp_printer);
    break;
  case '6': 
    sprintf(buff, "pico %s", nm_file);
    system(buff);
    break;
  case '7':
    clean_records(con);
    sprintf(msg, "Registros eliminados.");
    break;
  case '8' :
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
  char       buf[maxbuf];
  char       *b;
  char       home_directory[maxbuf];
  static int i;


  /*process = popen("printenv HOME", "r");
  if (process != NULL) {
    fgets(home_directory, maxbuf, config);
    home_directory[strlen(home_directory)-1] = 0;
    pclose(process);
  }
  else*/
    sprintf(home_directory, "/home/scaja");

  sprintf(nmconfig, "%s", home_directory);
  strcat(nmconfig, "/.osopos/corte.config");

  //  nm_file = calloc(1, strlen("/tmp/osopos_corte")+1);
  sprintf(nm_file,"/tmp/osopos_corte");

  //  tipo_imp = calloc(1, strlen("EPSON")+1);
  sprintf(tipo_imp,"EPSON");

  //  lp_printer = calloc(1, strlen("ticket")+1);
  strncpy(lp_printer, "ticket", maxbuf);


  config = fopen(nmconfig,"r");
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
      fgets(buff,sizeof(buff),config);
    }
    fclose(config);
    return(0);
  }
  return(1);  
}


int main()
{
  PGconn    *con;
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

  read_config();


  con = Abre_Base(NULL, NULL, NULL, NULL, "osopos", NULL, NULL);
  if (!con) {
    mvprintw(getmaxy(stdscr)/2, 0,
      "ERROR FATAL: No se puede abrir la base de datos. Pulse una tecla para abortar...");
    getch();
    exit(-1);
  }

  do {
    clear();
    printw("Programa de corte de caja, v. %s. (C) 1999-2001. E. Israel Osorio H.\n\n",
           VERSION);
    mvprintw(getmaxy(stdscr),0, msg);
    printw("Indique la operación a realizar:");
    mvprintw(4,3,"1. Corte de dia");
    mvprintw(5,3,"2. Corte parcial de ventas en general");
    mvprintw(6,3,"3. Corte parcial por cajero");
    mvprintw(7,3,"4. Ver los cortes generados");
    mvprintw(8,3,"5. Imprimir los cortes generados");
    mvprintw(9,3,"6. Editar los cortes generados");
    mvprintw(10,3,"7. Limpiar todos los registros de ventas (iniciar nuevo día)");
    mvprintw(12,3,"8. Ver licencia de uso");
    mvprintw(15,3,"0. Salir");
    mvprintw(17,1,"Opción: ");

    opcion = getch();
    if (opcion != '0')
      strcpy(msg, procesa(opcion, con, disp));
  }
  while (opcion != '0');

  PQfinish(con);
  endwin();
  //  free(lp_printer);
  //  free(nm_file);
  //  free(tipo_imp);
  return(OK);
}
