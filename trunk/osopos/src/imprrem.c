/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa ImprRem 0.16 (C) 1999-2001 E. Israel Osorio H.
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
#include <time.h> 
#include "include/pos-func.h" 

#define _IMPRREM
#define vers "0.16-2"

#define maxrenglon 8
#define maxdes 39
#define mxbuff 130
#define ESC 27
#define FF 12
#define SI 15

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "include/minegocio-remis.h"

char buff1[mxbuff],buff2[mxbuff];
struct articulos art[maxart];
char nmimpre[mxbuff];
char *lp_printer, *prt_type; /* Tipo de impresora */
int numarts, dgaran=-1;
short unsigned maxitem;
short iva_incluido;
short desglose_iva;
short unsigned ajuste_microv, ajuste_v, ajuste_h;
short unsigned price_pos[3];
unsigned total_price_pos, desc_pos, desc_len, qt_pos, code_pos;
short unsigned date_pos;
short date_pitch = 12;
short article_pitch = 17;
char *home_directory;

int read_config();
int CalculaIVA();


int read_config() {
  char       *nmconfig;
  FILE       *config;
  char       buff[mxbuff],buf[mxbuff];
  char       *b;
  char       *aux = NULL;
  static int i;


  home_directory = calloc(1, 255);
  nmconfig = calloc(1, 255);
  config = popen("printenv HOME", "r");
  fgets(home_directory, 255, config);
  home_directory[strlen(home_directory)-1] = 0;
  pclose(config);

  strcpy(nmconfig, home_directory);
  strcat(nmconfig, "/.osopos/imprrem.config");

  iva_incluido = 1;
  desglose_iva = 0;
  strcpy(nmimpre,"/tmp/osopos_nota");
  prt_type = calloc(1, strlen("EPSON"));
  strcpy(prt_type,"EPSON");
  lp_printer = calloc(1, strlen("facturas")+1);
  strcpy(lp_printer, "facturas");
  maxitem = 20;

  ajuste_v = 0;
  ajuste_microv = 0;
  ajuste_h = 0;

  price_pos[0] = 30;
  price_pos[1] = 40;
  price_pos[2] = 0;

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
      if (!strcmp(b,"impresora")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(nmimpre,buf);
      }
      else if (!strcmp(b,"impresora.lpr")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(lp_printer, strlen(buf)+1);
        if (aux != NULL)
          strcpy(lp_printer,buf);
        else
          fprintf(stderr,"imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"impresora.tipo")) {
        strcpy(buf, strtok(NULL,"="));
        aux = realloc(prt_type, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(prt_type,buf);
          for (i=0; i<strlen(prt_type); i++)
            prt_type[i] = toupper(prt_type[i]);
        }
        else
          fprintf(stderr,"imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"iva.incluido")) {
        strcpy(buf, strtok(NULL,"="));
        iva_incluido = (buf[0] == 'S');
      }
      else if (!strcmp(b,"iva.desglosado")) {
        strcpy(buf, strtok(NULL,"="));
        desglose_iva = (buf[0] == 'S');
      }
      else if (!strcmp(b,"maxitem")) {
        strcpy(buf, strtok(NULL,"="));
        maxitem = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"ajuste.microv")) {
        strcpy(buf, strtok(NULL,"="));
        ajuste_microv = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"ajuste.v")) {
        strcpy(buf, strtok(NULL,"="));
        ajuste_v = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"ajuste.h")) {
        strcpy(buf, strtok(NULL,"="));
        ajuste_h = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"importe.1")) {
        strcpy(buf, strtok(NULL,"="));
        price_pos[0] = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"importe.2")) {
        strcpy(buf, strtok(NULL,"="));
        price_pos[1] = (unsigned) atoi(buf);
      }
      else if (!strcmp(b, "importe.3")) {
        strcpy(buf, strtok(NULL, "="));
        price_pos[2] = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"importe.total")) {
        strcpy(buf, strtok(NULL,"="));
        total_price_pos = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"fecha.pos_v")) {
        strcpy(buf, strtok(NULL,"="));
        date_pos = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"fecha.tamaño")) {
        strcpy(buf, strtok(NULL,"="));
        date_pitch = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"codigo.pos")) {
        strcpy(buf, strtok(NULL,"="));
        code_pos = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"cantidad.pos")) {
        strcpy(buf, strtok(NULL,"="));
        qt_pos = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"descripcion.pos")) {
        strcpy(buf, strtok(NULL,"="));
        desc_pos = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"descripcion.long")) {
        strcpy(buf, strtok(NULL,"="));
        desc_len = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"articulos.tamaño")) {
        strcpy(buf, strtok(NULL,"="));
        article_pitch = (unsigned) atoi(buf);
      }

      fgets(buff,sizeof(buff),config);
    }
    fclose(config);
    return(0);
  }
  return(1);  
}

int CalculaIVA() {
  static double sumatoria = 0.0;
  static int i;

  for(i=0; i<numarts; i++) {
    if (!iva_incluido && desglose_iva)
      art[i].pu =  art[i].pu / (art[i].iva_porc+1);
    sumatoria += (art[i].pu * art[i].cant);
  }
  subtotal = sumatoria;
  iva = subtotal * iva_porcentaje * iva_incluido * desglose_iva;
  total = subtotal + iva;
  return(i);
}

int main(int argc, char *argv[])
{
  unsigned num_venta = 0;
  PGconn *conn;

  read_config();

  if (argc>1) {
    dgaran = atoi(argv[2]);
    num_venta = atoi(argv[1]);
  }

  conn = Abre_Base(NULL, NULL, NULL, NULL, "osopos", "scaja", "");
  if (conn == NULL) {
    fprintf(stderr, "FATAL: No puedo accesar la base de datos osopos\n. Abortando...");
    return(ERROR_SQL);
  }
  numarts = lee_venta(conn, num_venta, art);

  if (numarts>0) {
    CalculaIVA();
    imprime(nmimpre, art, numarts);
    PQfinish(conn);
    return (OK);
  }
  else {
    PQfinish(conn);
    return (numarts);
  }
}
