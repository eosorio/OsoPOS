/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa ImprRem 0.17 (C) 1999-2003 E. Israel Osorio H.
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
#include "include/print-func.h"

#define _IMPRREM
#define vers "0.17-1"

#define maxrenglon 8
#define maxdes 39

#define ESC 27
#define FF 12
#define SI 15

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* máximo de items que contiene el renglon de articulos */
#define mxrenartsitem 10

#include "include/minegocio-remis.h"

struct articulos art[maxart];
char nmimpre[mxbuff];
char *lp_printer, *prt_type; /* Tipo de impresora */
char *log_name;
char *cmd_lp; /* Comando de impresión */
int numarts, dgaran=-1;
short unsigned maxitem;
int iva_incluido;
short unsigned desglose_iva;
short unsigned ajuste_microv, ajuste_v, ajuste_h;
short unsigned price_pos[3];
unsigned total_price_pos, desc_pos, desc_len, code_len, qt_pos, code_pos;
short unsigned date_posv, date_posh;
short date_pitch = 12;
short article_pitch = 17;
char *home_directory;
struct db_data db;
struct divisas divisa[mxmemdivisa];
char   s_divisa[MX_LON_DIVISA];       /* Designación de la divisa que se usa para cobrar en la base de datos */
int mxrenarts_long;  /* longitud máxima del renglon de articulos al imprimir */

int read_config();
int CalculaIVA();
int read_general_config();
int imprime(char *nmimpre, struct articulos art[maxart], int numart);

/***************************************************************************/

int init_config()
{


  home_directory = (char *)calloc(1, 255);
  strcpy(home_directory, getenv("HOME"));

  log_name = (char *)calloc(1, 255);
  strcpy(log_name, getenv("LOGNAME"));

  cmd_lp = (char *)calloc(1, strlen("lpr -P ")+1);
  strcpy(cmd_lp, "lpr -P ");

  db.name= NULL;
  db.user = NULL;
  db.passwd = NULL;
  db.sup_user = NULL;
  db.sup_passwd = NULL;
  db.hostport = NULL;
  db.hostname = NULL;

  db.hostname = (char *)calloc(1, strlen("255.255.255.255"));
  db.name = (char *)calloc(1, strlen("elpuntodeventa.com"));
  db.user = (char *)calloc(1, strlen(log_name)+1);
  strcpy(db.user, log_name);
  db.sup_user = (char *)calloc(1, strlen("scaja")+1);

  TAX_PERC_DEF = 10;
  strcpy(s_divisa, "MXP");

  iva_incluido = 1;
  desglose_iva = 0;
  date_posh = 0;
  date_posv = 0;
  code_len  = 15;
  desc_len  = 30;
  mxrenarts_long = 50;

  return(OK);
}

/***************************************************************************/

int read_general_config()
{
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;

  
  nmconfig = (char *)calloc(1, 255);
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


      if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = (char *)realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.port")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = (char *)realloc(db.hostport, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostport,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.nombre")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = (char *)realloc(db.name, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.name,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.sup_usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = (char *)realloc(db.sup_user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.sup_user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.sup_passwd")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_passwd = (char *)calloc(1, strlen(buf)+1);
        if (db.sup_passwd  != NULL) {
          strcpy(db.sup_passwd,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"porcentaje_iva")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        TAX_PERC_DEF = atoi(buf);
      }
      else if(!strcmp(b, "divisa")) {
        strncpy(buf, strtok(NULL,"="), MX_LON_DIVISA);
        strcpy(s_divisa, buf);
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

int read_config() {
  char       *nmconfig;
  FILE       *config;
  char       buff[mxbuff],buf[mxbuff];
  char       *b;
  char       *aux = NULL;
  static int i;


  home_directory = (char *)calloc(1, 255);
  nmconfig = (char *)calloc(1, 255);
  config = popen("printenv HOME", "r");
  fgets(home_directory, 255, config);
  home_directory[strlen(home_directory)-1] = 0;
  pclose(config);

  strcpy(nmconfig, home_directory);
  strcat(nmconfig, "/.osopos/imprrem.config");

  strcpy(nmimpre,"/tmp/osopos_nota");
  prt_type = (char *)calloc(1, strlen("EPSON"));
  strcpy(prt_type,"EPSON");
  lp_printer = (char *)calloc(1, strlen("facturas")+1);
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
        aux = (char *)realloc(lp_printer, strlen(buf)+1);
        if (aux != NULL)
          strcpy(lp_printer,buf);
        else
          fprintf(stderr,"imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"impresora.tipo")) {
        strcpy(buf, strtok(NULL,"="));
        aux = (char *)realloc(prt_type, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(prt_type,buf);
          for (i=0; i<strlen(prt_type); i++)
            prt_type[i] = toupper(prt_type[i]);
        }
        else
          fprintf(stderr,"imprrem. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = (char *)realloc(db.user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "imprrem. Error de memoria en argumento de configuracion %s\n",
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
        date_posv = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"fecha.pos_h")) {
        strcpy(buf, strtok(NULL,"="));
        date_posh = (unsigned) atoi(buf);
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
      else if (!strcmp(b,"codigo.long")) {
        strcpy(buf, strtok(NULL,"="));
        code_len = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"articulos.tamaño")) {
        strcpy(buf, strtok(NULL,"="));
        article_pitch = (unsigned) atoi(buf);
      }
      else if (!strcmp(b,"articulos.maxcols")) {
        strcpy(buf, strtok(NULL,"="));
        mxrenarts_long = atoi(buf);
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

  init_config();
  read_general_config();
  read_config();

  if (argc>1) {
    dgaran = atoi(argv[2]);
    num_venta = atoi(argv[1]);
  }

  conn = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd);
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
