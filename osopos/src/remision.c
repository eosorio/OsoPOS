/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para peque�os negocios
   Programa Remision 1.34 (C) 1999-2003 E. Israel Osorio H.
   desarrollo@elpuntodeventa.com
   Lea el archivo README, COPYING y LEAME que contienen informaci�n
   sobre la licencia de uso de este programa

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


#define MALLOC_CHECK_ 2

//#include <linux/kbd_kern.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>


#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define _SERIAL_COMM   /* Se usar�n rutinas de comunicaci�n serial */

#include "include/pos-curses.h"
#define _pos_curses

#include "include/print-func.h"
#define _printfunc

#define vers "1.34"
#define release "La Botana"

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


/* C�digos de impresora */
#define abre_cajon_star 7
#ifndef ESC
#define ESC 27
#endif

#define _PAGO_TARJETA    1
#define _PAGO_CREDITO    2
#define _PAGO_EFECTIVO  20
#define _PAGO_CHEQUE    21

#define _NOTA_MOSTRADOR  1
#define _FACTURA         2
#define _TEMPORAL        5

#include "include/minegocio-remis.h"

int forma_de_pago();
void Termina(PGconn *con, PGconn *con_s, int error);
//int LeeConfig(char*, char*);
double item_capture(PGconn *, int *numart, double *util, double tax[maxtax], struct tm fecha);
void print_ticket_arts();
void print_ticket_footer(struct tm fecha, unsigned numvents);
int AbreCajon(char *tipo_miniimp);
/* Funci�n que reemplaza a ActualizaEx */
int actualiza_existencia(PGconn *con, struct tm *fecha);
/* Esta funci�n reemplaza a leebarras */
int lee_articulos(PGconn *, PGconn *);
void show_item_list(WINDOW *vent, int i, short cr);
void show_subtotal(double);
void mensaje(char *texto);
void aviso_existencia(struct articulos art);
void switch_pu(WINDOW *, struct articulos *, int, double *, double tax[maxtax], int);
int journal_last_sale(int i, char *last_sale_fname);
int journal_marked_items(char *marked_items_fname, char *code, int exist_previous); 
int clean_journal(char *marked_items_fname);
int check_for_journal(char *dir_name);
int cancela_articulo(WINDOW *vent, int *reng, double *subtotal, double *iva,
                     double tax[maxtax], char *last_sale_fname);
int aborta_remision(PGconn *con, PGconn *con_s, char *mensaje, char tecla, int senial);
void signal_handler_IO (int status);
void captura_serial(char *);
void init_serial(struct sigaction *saio, struct termios *oldtio, struct termios *newtio);
void signal_handler_IO (int status);
void close_serial(int fd, struct termios *tio);

double tipo_cambio(char *);

volatile int STOP=FALSE; 

FILE *impresora;
double a_pagar; 
int numbarras=0;
int numdivisas=0;
/* struct barras barra[mxmembarra];  */
struct articulos articulo[maxart], barra[mxmembarra];
struct divisas divisa[mxmemdivisa];
int numarts=0;          /* N�mero de items capturados   */
short unsigned maxitemr;
pid_t pid;
char *program_name;
unsigned almacen;
struct db_data db;

  /* Variables de configuraci�n */
char *home_directory;
char *log_name;
char *nm_disp_ticket,           /* Nombre de impresora de ticket */
  *lp_disp_ticket,      /* Definici�n de miniprinter en /etc/printcap */
  *disp_lector_serie,  /* Ruta al scanner de c. de barras serial */
  *nmfpie,              /* Pie de p�gina de ticket */
  *nmfenc,              /* Archivo de encabezado de ticket */
  *nmtickets,   /* Registro de tickets impresos */
  *tipo_disp_ticket, /* Tipo de miniprinter */
  *nmimprrem,           /* Ubicaci�n de imprrem */
  *nm_factur,    /* Nombre del progreama de facturaci�n */
  *nm_avisos,    /* Nombre del archivo de avisos */
  *nm_journal,   /* Nombre actual del archivo del diario de marcaje */
  *nm_orig_journal, /* Nombre del archivo del diario original */
  *nm_sendmail,  /* Ruta completa de sendmail */
  *dir_avisos,   /* email de notificaci�n */
  *asunto_avisos, /* Asunto del correo de avisos */
  *cc_avisos;     /* con copia para */
char   s_divisa[3];       /* Designaci�n de la divisa que se usa para cobrar en la base de datos */
double TAX_PERC_DEF; /* Porcentaje de IVA por omisi�n */
short unsigned search_2nd;  /* �Buscar c�digo alternativo al mismo tiempo que el primario ? */ 
int id_teller = 0;
short unsigned manual_discount; /* Aplicar el descuento manual de precio en la misma l�nea */
int iva_incluido;
int listar_neto;

int lector_serial; /* Cierto si se usa un scanner serial */
int serial_crlf=1; /* 1 si el scanner envia CRLF, 0 si solamente LF */
tcflag_t serial_bps=B38400; /* bps por omisi�n */
int wait_flag=TRUE;                    /* TRUE while no signal received */
char s_buf[255];    /* Buffer de puerto serie */
struct termios oldtio,newtio;  /* Parametros anteriores y nuevos de terminal serie */
struct sigaction saio;              /* definition of signal action */
int fd;            /* Descriptor de archivo de puerto serie */

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

  nm_journal = calloc(1, mxbuff);
  nm_orig_journal = calloc(1, strlen(home_directory) + strlen("/.last_items.") + 10);
  /* Dejamos 10 caracteres extras para el PID */
  sprintf(nm_orig_journal, "%s/.last_items.%d", home_directory, pid);
  search_2nd = 1;

  nm_disp_ticket = calloc(1, strlen("/tmp/ticket_")+strlen(log_name)+1);
  strcpy(nm_disp_ticket, "/tmp/ticket_");
  strcat(nm_disp_ticket, log_name);

  lp_disp_ticket = calloc(1, strlen("ticket")+20);
  strcpy(lp_disp_ticket, "ticket");

  disp_lector_serie = calloc(1, strlen("/dev/barcode")+1);
  strcpy(disp_lector_serie, "/dev/barcode");
  lector_serial = 0;

  tipo_disp_ticket = calloc(1, strlen("STAR")+20);
  strcpy(tipo_disp_ticket, "STAR");

  nmfpie = calloc(1, strlen("/pie_pagina.txt")+strlen(home_directory)+20);
  sprintf(nmfpie,   "%s/pie_pagina.txt", home_directory);

  nmfenc = calloc(1, strlen("/etc/osopos/encabezado.txt")+strlen(home_directory)+20);
  sprintf(nmfenc,  "%s/encabezado.txt", home_directory);
  
  nmimprrem = calloc(1, strlen("/usr/local/bin/imprem")+20);
  strcpy(nmimprrem,"/usr/bin/imprem");

  nm_avisos = calloc(1, strlen("/var/log/osopos/avisos.txt")+20);
  strcpy(nm_avisos, "/var/log/osopos/avisos.txt");
  
  dir_avisos = calloc(1, strlen("scaja@localhost.localdomain")+20);
  strcpy(dir_avisos, "scaja@localhost");
  
  cc_avisos = calloc(1, strlen("scaja@localhost.localdomain")+20);
  strcpy(cc_avisos, "scaja@localhost");

  asunto_avisos = calloc(1, strlen("Aviso de baja existencia")+20);
  strcpy(asunto_avisos, "Aviso de baja existencia");
  
  nm_sendmail = calloc(1, strlen("/usr/sbin/sendmail -t")+20);
  strcpy(nm_sendmail, "/usr/sbin/sendmail -t");

  nm_factur = calloc(1, strlen("/usr/local/bin/factur"));
  strcpy(nm_factur, "/usr/bin/factur");

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
  db.passwd = calloc(1, mxbuff);
  db.sup_user = calloc(1, strlen("scaja")+1);

  maxitemr = 6;

  iva_incluido = 0;
  TAX_PERC_DEF = 15;
  strcpy(s_divisa, "MXP");

  manual_discount = 1;

  almacen = 1;

  return(OK);
}

int read_config()
{
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;
  //  struct kbd_struct teclado;

  int i_buf;
  
  //  setledstate(teclado, VC_NUMLOCK); 

  nmconfig = calloc(1, 255);
 
  strncpy(nmconfig, home_directory, 255);
  strcat(nmconfig, "/.osopos/remision.config");


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

        /* Definici�n de impresora de ticket */
      if (!strcmp(b,"ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b,"ticket.pie")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmfpie, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmfpie, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
   /*      else if (!strcmp(b,"programa.factur")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_factur, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_factur, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                 "remision. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b,"programa.imprem")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmimprrem, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmimprrem,buf);
          aux = NULL;
          }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
                  } */
      else if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
     else if (!strcmp(b,"renglones.articulos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        maxitemr = atoi(buf);
      }
      else if (!strcmp(b,"avisos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_avisos, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_avisos,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"busca_codigo_alternativo")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        search_2nd = atoi(buf);
      }
     else if (!strcmp(b,"cajero.numero")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       id_teller = atoi(buf);
     }
     else if (!strcmp(b,"almacen.numero")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       almacen = atoi(buf);
     }
     else if(!strcmp(b, "scanner_serie")) {
       lector_serial = 1;
       strncpy(buf, strtok(NULL,"="), mxbuff);
       aux = realloc(disp_lector_serie, strlen(buf)+1);
       if (aux != NULL) {
         strcpy(disp_lector_serie, buf);
         aux = NULL;
       }
       else
         fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                 b);
     }
     else if(!strcmp(b, "scanner_velocidad")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       i_buf = atoi(buf);
       switch (i_buf) {
       case 2400:
         serial_bps = B2400;
         break;
       case 4800:
         serial_bps = B4800;
         break;
       case 9600:
         serial_bps = B9600;
         break;
       case 19200:
         serial_bps = B19200;
       case 38400:
       default:
         serial_bps = B38400;
       }
     }
     else if (!strcmp(b,"scanner_terminador")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       scan_terminator = atoi(buf);
     }
     else if (!strcmp(b,"iva_incluido")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       iva_incluido = atoi(buf);
     }
      else if (!strcmp(b, "listar_precio_neto")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       listar_neto = atoi(buf);
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

int read_global_config()
{
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;
  int i_buf;
  
  nmconfig = calloc(1, 255);
  strncpy(nmconfig, "/etc/osopos/remision.config", 255);

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

        /* Definici�n de impresora de ticket */
      if (!strcmp(b,"ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n", b);
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
                  "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
       }
      else if (!strcmp(b,"ticket.pie")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmfpie, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmfpie, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"ticket.encabezado")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmfenc, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmfenc,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
                  "remision. Error de memoria en argumento de configuracion %s\n",
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
                 "remision. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b,"programa.imprem")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmimprrem, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmimprrem,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
      /*      else if (!strcmp(b,"db.usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
                  }*/
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"renglones.articulos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        maxitemr = atoi(buf);
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"busca_codigo_alternativo")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        search_2nd = atoi(buf);
      }
      else if (!strcmp(b,"almacen.numero")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        almacen = atoi(buf);
      }
      else if(!strcmp(b, "scanner_serie")) {
        lector_serial = 1;
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(disp_lector_serie, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(disp_lector_serie, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if(!strcmp(b, "scanner_velocidad")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        i_buf = atoi(buf);
        switch (i_buf) {
        case 2400:
          serial_bps = B2400;
          break;
        case 4800:
          serial_bps = B4800;
          break;
        case 9600:
          serial_bps = B9600;
          break;
        case 19200:
          serial_bps = B19200;
          case 38400:
        default:
          serial_bps = B38400;
        }
      }
      else if(!strcmp(b, "divisa")) {
        strncpy(buf, strtok(NULL,"="), MX_LON_DIVISA);
        strcpy(s_divisa, buf);
      }
     else if (!strcmp(b,"iva_incluido")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       iva_incluido = atoi(buf);
     }
      else if (!strcmp(b, "listar_precio_neto")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       listar_neto = atoi(buf);
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
  int i_buf;
  
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

        /* Definici�n de impresora de ticket */
      if (!strcmp(b,"ticket")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nm_disp_ticket, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nm_disp_ticket, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n", b);
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
                  "remision. Error de memoria en argumento de configuracion %s\n",
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
                  "remision. Error de memoria en argumento de configuracion %s\n",
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
                 "remision. Error de memoria en argumento de configuracion %s\n", b);
      }
      else if (!strcmp(b,"programa.imprem")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmimprrem, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmimprrem,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
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
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if(!strcmp(b, "scanner_serie")) {
        lector_serial = 1;
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(disp_lector_serie, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(disp_lector_serie, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if(!strcmp(b, "scanner_velocidad")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        i_buf = atoi(buf);
        switch (i_buf) {
        case 2400:
          serial_bps = B2400;
          break;
        case 4800:
          serial_bps = B4800;
          break;
        case 9600:
          serial_bps = B9600;
          break;
        case 19200:
          serial_bps = B19200;
          case 38400:
        default:
          serial_bps = B38400;
        }
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

int lee_articulos(PGconn *base_inventario, PGconn *con_s)
{
  char comando[1024];
/*  char *datos[7]; */
  int  i;
  int   nCampos;
  PGresult* res;

  res = PQexec(base_inventario,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Fall� comando BEGIN al tratar de leer art�culos para colocar en memoria\n");
    PQclear(res);
    return(ERROR_SQL);
  }
  PQclear(res);

  sprintf(comando, "DECLARE cursor_arts CURSOR FOR SELECT ");
  sprintf(comando, "%s al.codigo, ar.descripcion, al.codigo2, al.pu, ", comando);
  sprintf(comando, "%s al.pu2, al.pu3, al.pu4, al.pu5, al.cant,  al.c_min, ", comando);
  sprintf(comando, "%s al.tax_0, al.tax_1, al.tax_2, al.tax_3, al.tax_4, al.tax_5, ", comando);
  sprintf(comando, "%s ar.iva_porc, ar.p_costo, al.divisa ", comando);
  sprintf(comando, "%s FROM almacen_%d al, articulos ar WHERE al.codigo=ar.codigo;", comando, almacen);
  res = PQexec(base_inventario, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Comando DECLARE CURSOR fall�\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base_inventario));
    PQclear(res);
    return(ERROR_SQL);
  }
  PQclear(res);

  sprintf(comando, "FETCH ALL in cursor_arts");
  res = PQexec(base_inventario, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regres� registros apropiadamente\n");
    PQclear(res);
    return(ERROR_SQL);
  }

  nCampos = PQnfields(res);

  for (i=0; i < PQntuples(res) && i<mxmembarra; i++) {
    strncpy(barra[i].codigo , PQgetvalue(res,i,0), maxcod);
    strncpy(barra[i].codigo2 , PQgetvalue(res,i,2), maxcod);
    strncpy(barra[i].desc, PQgetvalue(res,i,1), maxdes);

    barra[i].pu  = atof(PQgetvalue(res,i, 3));
    barra[i].pu2 = atof(PQgetvalue(res,i, 4));
    barra[i].pu3 = atof(PQgetvalue(res,i, 5));
    barra[i].pu4 = atof(PQgetvalue(res,i, 6));
    barra[i].pu5 = atof(PQgetvalue(res,i, 7));
    //    barra[i].disc = atof(PQgetvalue(res,i,3));
    barra[i].disc = 0;
    barra[i].exist = atof(PQgetvalue(res,i,8));
    barra[i].exist_min = atof(PQgetvalue(res,i,9));
    barra[i].p_costo = atof(PQgetvalue(res,i,17));
    barra[i].iva_porc = atof(PQgetvalue(res,i,16));
    barra[i].tax_0 = atof(PQgetvalue(res,i,10));
    barra[i].tax_1 = atof(PQgetvalue(res,i,11));
    barra[i].tax_2 = atof(PQgetvalue(res,i,12));
    barra[i].tax_3 = atof(PQgetvalue(res,i,13));
    barra[i].tax_4 = atof(PQgetvalue(res,i,14));
    barra[i].tax_5 = atof(PQgetvalue(res,i,15));
    strncpy(barra[i].divisa, PQgetvalue(res,i,18), 3);

  }

  if (PQntuples(res) >= mxmembarra)  {
    fprintf(stderr, "ADVERTENCIA: Se ha excedido el m�ximo de art�culos en memoria, no fueron\n");
    fprintf(stderr, "             cargados todos los que existen en la base");
    sprintf(comando,
            "Se ha excedido el m�ximo de art�culos en memoria, se leyeron los primeros %d",
            mxmembarra);
    mensaje(comando);
  }
  PQclear(res);


  res = PQexec(base_inventario, "CLOSE cursor_arts");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Comando CLOSE fall�\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base_inventario));
    PQclear(res);
    Termina(base_inventario, con_s, ERROR_SQL);
  }
  PQclear(res);

  /* finaliza la transacci�n */
  res = PQexec(base_inventario, "END");
  PQclear(res);
  return(i);
}

/***************************************************************************/

int lee_divisas(PGconn *base_invent)
{
  char comando[256];

  int i;
  int nCampos;
  PGresult* res;
  int div_mxbuff=255;
  char buf[255];

  res = PQexec(base_invent, "SELECT * FROM divisas ");
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"Fall� comando al tratar de leer divisas para colocar en memoria\n");
    PQclear(res);
    return(ERROR_SQL);
  }

  nCampos = PQnfields(res);

  for (i=0; i < PQntuples(res) && i<mxmemdivisa; i++) {
    strncpy(divisa[i].id, PQgetvalue(res,i,0), MX_LON_DIVISA);
    strncpy(buf, PQgetvalue(res,i,2), div_mxbuff);
    divisa[i].tc = atof(buf);
  }

  if (PQntuples(res) >= mxmemdivisa)  {
    fprintf(stderr, "ADVERTENCIA: Se ha excedido el m�ximo de divisas en memoria, no fueron\n");
    fprintf(stderr, "             cargadas todas las que existen en la base");
    sprintf(comando,
            "Se ha excedido el m�ximo de divisas en memoria, se leyeron las primeras %d",
            mxmembarra);
    mensaje(comando);
  }
  PQclear(res);
  return(i);
}

/***************************************************************************/

int find_code(char *cod, struct articulos *art, int search_2nd) {
  //int busca_precio(char *cod, struct articulos *art) {
/* Busca el c�digo de barras en la memoria y devuelve el precio */
/* del producto. */
int i;
int exit=0; /* falso si hay que salirse */
double tc=1;

  for (i=0; (i<numbarras); ++i) {
    exit = strcmp(cod, barra[i].codigo);
    if (exit && search_2nd)
      exit = strcmp(cod, barra[i].codigo2);

    if (!exit) {
      strncpy(art->codigo, barra[i].codigo, maxcod);
      strncpy(art->desc, barra[i].desc, maxdes);

      tc = tipo_cambio(barra[i].divisa);

      art->p_costo = barra[i].p_costo;
      art->pu = barra[i].pu * tc;
      art->pu2= barra[i].pu2 * tc;
      art->pu3= barra[i].pu3 * tc;
      art->pu4= barra[i].pu4 * tc;
      art->pu5= barra[i].pu5 * tc;
      art->id_prov = barra[i].id_prov;
      art->id_depto = barra[i].id_depto;
      art->iva_porc = barra[i].iva_porc;
      strcpy(art->divisa, barra[i].divisa);
      art->tax_0 = barra[i].tax_0;
      art->tax_1 = barra[i].tax_1;
      art->tax_2 = barra[i].tax_2;
      art->tax_3 = barra[i].tax_3;
      art->tax_4 = barra[i].tax_4;
      art->tax_5 = barra[i].tax_5;
      return(i);
    }
  }
  return(ERROR_DIVERSO);
}    

/***************************************************************************/

double tipo_cambio(char *s_divisa) {

int i;
int exit=0; /* falso si hay que salirse */

  for (i=0; (i<numdivisas); ++i) {
    exit = strcmp(s_divisa, divisa[i].id);

    if (!exit) {
      return(divisa[i].tc);
    }
  }
  return(ERROR_DIVERSO);

}    

/***************************************************************************/

int cancela_articulo(WINDOW *vent, int *reng, double *subtotal, double *iva,
                     double tax[maxtax], char *last_sale_fname)
{
  int i,j;
  int num_item;
  char codigo[maxcod];
  double iva_articulo, tax_item[6];
  FILE *f_last_sale;
  FILE *f_last_items;

  mvprintw(getmaxy(stdscr)-3,0,"C�digo del(los) art�culo(s) a cancelar: ");
  clrtoeol();
  getstr(codigo);

  f_last_items = fopen(nm_journal, "a");
  if (f_last_items != NULL) {
    fprintf(f_last_items, "%s\n", codigo);
    fclose(f_last_items);
  }
  move(getmaxy(stdscr)-3,0);
  clrtoeol();

  f_last_sale = fopen(last_sale_fname, "a");
  if (f_last_sale != NULL) {
    fprintf(f_last_sale, "Cancelando articulo: %s\n", codigo);
    fclose(f_last_sale);
  }

  for (i=0; strcmp(codigo, articulo[i].codigo) && i<*reng; i++);
  if (i  ||  strcmp(codigo, articulo[i].codigo) == 0)
    num_item = i;
  else {
    return(-1);
  }

  tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
  tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
  tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
  tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
  tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
  tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);

  if (iva_incluido)
    iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
  else
    iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);

  (*iva)  -=  (articulo[i].cant * iva_articulo);

  for (j=0; j<maxtax; j++)
    tax[j]  -=  (articulo[i].cant * tax_item[j]);

  (*subtotal) -= (articulo[i].cant * articulo[i].pu);
  /* Desplaza los art�culos a una posici�n inferior */
  for (; i<*reng; i++) {
    strncpy(articulo[i].codigo, articulo[i+1].codigo, maxcod);
    articulo[i].pu = articulo[i+1].pu;
    strncpy(articulo[i].desc, articulo[i+1].desc, maxdes);
    articulo[i].cant = articulo[i+1].cant;
  }
  (*reng)--;

  wclear(vent);
  if (numarts>vent->_maxy)
    for (i=*reng, j=vent->_maxy; j>=0; j--, i-- ) {
      wmove(vent, j,0);
      wclrtoeol(vent);
      show_item_list(vent, i, FALSE);
    }
  else
    for (i=0; i<*reng && i<vent->_maxy; i++)
      show_item_list(vent, i, TRUE);

  mvprintw(getmaxy(stdscr)-3,0,"C�digo de barras, descripci�n o cantidad:\n");
  wrefresh(vent);
  return(num_item);
}


void show_item_list(WINDOW *vent, int i, short cr) {
  double iva_articulo;

  if (iva_incluido)
    iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
  else
    iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);

  mvwprintw(vent, vent->_cury,  0, "%2.2f %-15s", articulo[i].cant, articulo[i].codigo);
  mvwprintw(vent, vent->_cury, 22, "%-39s", articulo[i].desc);

  if (listar_neto && !iva_incluido)
    mvwprintw(vent, vent->_cury, 62, "$%8.2f", articulo[i].pu + iva_articulo);
  else
    mvwprintw(vent, vent->_cury, 62, "$%8.2f", articulo[i].pu);

  if (!articulo[i].iva_porc)
        wprintw(vent, "E");
  if (cr)
    wprintw(vent, "\n");
  wrefresh(vent);
}

void show_subtotal(double subtotal) {

  attrset(COLOR_PAIR(verde_sobre_negro));
  mvprintw(getmaxy(stdscr)-5,0,"Subtotal acumulado: $%8.2f",subtotal);
  attrset(COLOR_PAIR(blanco_sobre_negro));
  clrtoeol();
}

void switch_pu(WINDOW *v_arts,
               struct articulos *art,
               int which_pu,
               double *subtotal,
               double tax[maxtax],
               int i) 
{
  double iva_articulo;
  double tax_item[maxtax];
  double new_pu;
  int j;

  switch (which_pu) {
  case 2:
    new_pu = art->pu2;
    break;
  case 3:
    new_pu = art->pu3;
    break;
  case 4:
    new_pu = art->pu4;
    break;
  case 5:
  default:
    new_pu = art->pu5;
  }

  if (iva_incluido)
    iva_articulo = new_pu - new_pu / (art->iva_porc/100 + 1);
  else
    iva_articulo = new_pu * (art->iva_porc/100);
  iva += (iva_articulo * art->cant);

  tax_item[0] =  new_pu - new_pu / (art->tax_0/100 + 1);
  tax_item[1] =  new_pu - new_pu / (art->tax_1/100 + 1);
  tax_item[2] =  new_pu - new_pu / (art->tax_2/100 + 1);
  tax_item[3] =  new_pu - new_pu / (art->tax_3/100 + 1);
  tax_item[4] =  new_pu - new_pu / (art->tax_4/100 + 1);
  tax_item[5] =  new_pu - new_pu / (art->tax_5/100 + 1);
  for (j=0; j<maxtax; j++)
    tax[j] += (tax_item[j] * art->cant);

  *subtotal += (art->cant * (new_pu - art->pu));
  art->pu = new_pu;
  wmove(v_arts, v_arts->_cury-1, 0);
  show_item_list(v_arts, i-1, TRUE);
  show_subtotal(*subtotal);


}

/***************************************************************************/

double item_capture(PGconn *con, int *numart, double *util, 
                    double tax[maxtax],
                    struct tm fecha)
{
  double  subtotal = 0.0;
  double  iva_articulo;
  double  tax_item[maxtax];
  int     i=0,
          j,k;
  char    *buff, *buff2, *last_sale_fname;
  int     chbuff = -1;
  WINDOW  *v_arts;
  FILE    *f_last_sale;
  int     exist_journal=0;
  FILE    *f_last_items;
  FILE    *p_impr;

  *util = 0;
  iva = 0.0;

  v_arts = newwin(getmaxy(stdscr)-8, getmaxx(stdscr)-1, 4, 0);
  scrollok(v_arts, TRUE);
  buff = calloc(1,mxbuff);
  buff2 = calloc(1, mxbuff);
  last_sale_fname = calloc(1, strlen(home_directory) + strlen("/ultima_venta.tmp")+1);
  if (buff == NULL  || buff2 == NULL || last_sale_fname == NULL) {
    fprintf(stderr, "remision. FATAL: no puedo apartar %d bytes de memoria\n", mxbuff);
    /* can't allocate %d memory bytes */
    return(MEMORY_ERROR);
  }
  
  sprintf(last_sale_fname, "%s/ultima_venta.tmp", home_directory);
  f_last_sale = fopen(last_sale_fname, "w");
  if (f_last_sale == NULL)
    fprintf(stderr, "remision. No puedo escribir al archivo %s\n", last_sale_fname);
  else {
    fprintf(f_last_sale, "  %2d/%2d/%2d,  %2d:%2d:%2d hrs.\n",
            fecha.tm_mday, (fecha.tm_mon)+1, fecha.tm_year, fecha.tm_hour,
            fecha.tm_min, fecha.tm_sec);
    fclose(f_last_sale);
  }

  if (lector_serial)
    init_serial(&saio, &oldtio, &newtio);

  exist_journal = check_for_journal(home_directory);
  if (exist_journal) {
    sprintf(nm_journal, "%s/.last_items.%d", home_directory, exist_journal);
    f_last_items = fopen(nm_journal, "r");
  }
  else
    strcpy(nm_journal, nm_orig_journal);

  mvprintw(3,0,
           "Cant.       Clave                Descripci�n                 P. Unitario");
            /* qt.       code                description                 unit cost */
  articulo[0].cant = 1;

  do {
    memset(buff, 0, mxbuff);
    buff2[0] = 0;
    iva_articulo = 0.0;
    for (j=0; j<maxtax; j++)
      tax_item[j] = 0.0;
    articulo[i].pu = 0;
    articulo[i].p_costo = 0;
    mvprintw(getmaxy(stdscr)-3,0,"C�digo de barras, descripci�n o cantidad:\n");
    /* barcode, description or qty. */
    if (exist_journal) {
      if (feof(f_last_items)) {
        articulo[i].cant = 0;
        continue;
      }
      fgets(buff, mxbuff, f_last_items);
      buff[strlen(buff)-1] = 0;
    }
    else {
      do {
        switch(chbuff = wgetkeystrokes(stdscr, buff, mxbuff)) {
          //        case KEY_F(1):
        case 2: /* F2 */
          strcpy(buff, "descuento");
          strcpy(articulo[i].desc, "descuento");
          chbuff = 0;
          break;
        case 3: /* F3 */
          journal_marked_items(nm_journal, "cancela", 0);
          cancela_articulo(v_arts, &i, &subtotal, tax, &iva, last_sale_fname);
          show_subtotal(subtotal);
          mvprintw(getmaxy(stdscr)-3,0,"C�digo de barras, descripci�n o cantidad:\n");
          continue;
          break;
        case 5:
        case 6:
        case 7:
        case 8:
          if (i) {
            articulo[i].codigo[0] = 0;
            articulo[i].desc[0] = 0;
            switch_pu(v_arts, &articulo[i-1], chbuff-3, &subtotal, tax, i);
            move(getmaxy(stdscr)-2,0);
          }
          break;
        case 12:
          if (puede_hacer(con, log_name, "caja_cajon_manual"))
            AbreCajon(tipo_disp_ticket);
            sprintf(buff2, "lpr -P %s %s", lp_disp_ticket, nm_disp_ticket);
            p_impr = popen(buff2, "w");
            pclose(p_impr);
          break;
        }
      }
      while (chbuff  && !(lector_serial && wait_flag == FALSE));
      if (lector_serial && wait_flag == FALSE) {
        strcpy(buff, s_buf);
        //        printw("%s", buff);
        wait_flag = TRUE;
      }

      //    getstr(buff);
    }
    chbuff = -1; /* Limpiamos para usar despues */

    if (strstr(articulo[i].desc,"ancela") == NULL)
      journal_marked_items(nm_journal, buff, exist_journal);

    move(getmaxy(stdscr)-2,0);
    deleteln();

    /* Secci�n de multiplicador */
    if ( ((toupper(buff[0])=='X') || (buff[0]=='*')) && i) {
      if (!isdigit(buff[1])) {
        mensaje("Error en multiplicador. Intente de nuevo");
        continue;
      }
      for (j=0; !isdigit(buff[j]); j++);
      for (k=0; isdigit(buff[j]) && j<strlen(buff); j++,k++)
        articulo[i].desc[k] = buff[j];
      articulo[i].desc[k]=0;
      articulo[i].cant = atof(articulo[i].desc);

      if (!articulo[i].cant) {
        f_last_sale = fopen(last_sale_fname, "a");
        if (f_last_sale != NULL) {
          fprintf(f_last_sale, "Cancelando renglones previos de %s\n", articulo[i-1].codigo);
          fclose(f_last_sale);
        }
         /* Cancela el �ltimo art�culo */
        wmove(v_arts, v_arts->_cury-1,0);
        wclrtoeol(v_arts);
        wrefresh(v_arts);
        --i;
        subtotal -= (articulo[i].pu * articulo[i].cant);
        if (iva_incluido)
          iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
        else
          iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
        iva  -= (iva_articulo * articulo[i].cant);

        tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
        tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
        tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
        tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
        tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
        tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
        for (j=0; j<maxtax; j++)
          tax[j] -= (tax_item[j] * articulo[i].cant);
      }
      else {
        f_last_sale = fopen(last_sale_fname, "a");
        if (f_last_sale != NULL) {
          fprintf(f_last_sale, "Nueva cantidad: %.2f\n", articulo[i].cant);
          fclose(f_last_sale);
        }
        if (iva_incluido)
          iva_articulo = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].iva_porc/100 + 1);
        else
          iva_articulo = articulo[i-1].pu - articulo[i-1].iva_porc/100;
        tax_item[0] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_0/100 + 1);
        tax_item[1] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_1/100 + 1);
        tax_item[2] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_2/100 + 1);
        tax_item[3] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_3/100 + 1);
        tax_item[4] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_4/100 + 1);
        tax_item[5] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_5/100 + 1);

        subtotal += ((articulo[i].cant-articulo[i-1].cant) * articulo[i-1].pu);
        iva  += (iva_articulo * (articulo[i].cant-articulo[i-1].cant));
        for (j=0; j<maxtax; j++)
          tax[j]  += (tax_item[j] * (articulo[i].cant-articulo[i-1].cant));
        articulo[i-1].cant = articulo[i].cant;
      }
      if (i) {
        wmove(v_arts, v_arts->_cury-1, v_arts->_curx);
        show_item_list(v_arts, i-1, TRUE);
      }
      show_subtotal(subtotal);

      articulo[i].desc[0] = 0;
      articulo[i].cant = 1;
      continue;
    }

    if (strlen(buff) > 1) {
    /* Si la clave es mayor de 1 caracteres, busca un c�digo o repetici�n */

        /* Repetici�n de art�culo */
        /* � No deber�a de ser && en lugar de || ? */
      if (i && (!strcmp(buff,articulo[i-1].desc) || !strcmp(buff,articulo[i-1].codigo)) ) {
        f_last_sale = fopen(last_sale_fname, "a");
        if (f_last_sale != NULL) {
          fprintf(f_last_sale, "Repitiendo c�digo: %s\n", articulo[i-1].codigo);
          fclose(f_last_sale);
        }
        (articulo[i-1].cant)++;
        subtotal += articulo[i-1].pu;
        if (iva_incluido)
          iva_articulo = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].iva_porc/100 + 1);
        else
          iva_articulo = articulo[i-1].pu * (articulo[i-1].iva_porc/100);
        iva += iva_articulo;

        tax_item[0] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_0/100 + 1);
        tax_item[1] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_1/100 + 1);
        tax_item[2] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_2/100 + 1);
        tax_item[3] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_3/100 + 1);
        tax_item[4] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_4/100 + 1);
        tax_item[5] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_5/100 + 1);
        for (j=0; j<maxtax; j++)
          tax[j] += tax_item[j];

        wmove(v_arts, v_arts->_cury-1, v_arts->_curx);
        show_item_list(v_arts, i-1, TRUE);
        show_subtotal(subtotal);
        continue;
      }

      chbuff = find_code(buff, &articulo[i], search_2nd); /* Reusamos chbuff */
      if (chbuff < 0) {
        strncpy(articulo[i].desc, buff, maxdes);
        articulo[i].iva_porc = TAX_PERC_DEF;
      }
    }

    /* El art�culo no tiene c�digo, puede ser un art. no registrado */
    /* o un descuento */
    if (chbuff<0) {
      if (strlen(buff) <= 1) {
        articulo[i].cant=0;
        continue;
      }
      if (strstr(articulo[i].desc,"ancela") && i) {
        cancela_articulo(v_arts, &i, &subtotal, &iva, tax, last_sale_fname);
        show_subtotal(subtotal);
        continue;
      }
      do {
        wmove(stdscr, getmaxy(stdscr)-3,0);
        if (strlen(buff2) > 6)
          printw("Cantidad muy grande... ");   /* Quantity very big */
        printw("Introduce precio unitario: "); /* Introduce unitary cost */
        clrtoeol();
        if (exist_journal) {
          fgets(buff2, mxbuff, f_last_items);
          buff2[strlen(buff2)-1] = 0;
        }
        else
          getnstr(buff2, mxbuff);
        if (strlen(buff2) > 6)
          continue;
        //        scanw("%f",&articulo[i].pu);
        deleteln();
      }
      while (strlen(buff2) > 6);

      articulo[i].pu = atof(buff2);


      if ( strstr(articulo[i].desc,"escuento") && i && !manual_discount) {
        journal_last_sale(i, last_sale_fname);
        journal_marked_items(nm_journal, buff2, exist_journal);

        articulo[i-1].pu += articulo[i].pu;
        articulo[i].codigo[0] = 0;
        articulo[i].desc[0] = 0;

        if (articulo[i-1].iva_porc) {
          if (iva_incluido)
            iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
          else
            iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
        }
        else
          iva_articulo = 0;

        if (articulo[i-1].tax_0)
          tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
        else
          tax_item[0] = 0;
        if (articulo[i-1].tax_1)
          tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
        else
          tax_item[1] = 0;
        if (articulo[i-1].tax_2)
          tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
        else
          tax_item[2] = 0;
        if (articulo[i-1].tax_3)
          tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
        else
          tax_item[3] = 0;
        if (articulo[i-1].tax_4)
          tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
        else
          tax_item[4] = 0;
        if (articulo[i-1].tax_5)
          tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
        else
          tax_item[5] = 0;

        subtotal += (articulo[i-1].cant * articulo[i].pu);
        iva += (iva_articulo * articulo[i-1].cant);
        for (j=0; j<maxtax; j++)
          tax[j] += (tax_item[j] * articulo[i-1].cant);
        wmove(v_arts, v_arts->_cury-1, 0);
        show_item_list(v_arts, i-1, TRUE);
        show_subtotal(subtotal);
        continue;
      }
      /* Art�culo no registrado */
      strncpy(articulo[i].desc, buff, maxdes);
      strcpy(articulo[i].codigo,"Sin codigo"); /* No code */
      buff[0] = 0;

      if (!strstr(articulo[i].desc, "escuento")) {
        journal_marked_items(nm_journal, buff2, exist_journal);
        buff2[0] = 0;
        do {
          wmove(stdscr, getmaxy(stdscr)-3,0);
          if (strlen(buff2) > 6)
            printw("Cantidad muy grande... ");   /* Quantity very big */
          mvprintw(getmaxy(stdscr)-3,0,"Introduce porcentaje de I.V.A.: "); /* Introd. tax % */
          clrtoeol();
          if (exist_journal) {
            fgets(buff2, mxbuff, f_last_items);
            buff2[strlen(buff2)-1] = 0;
          }
          else
            getnstr(buff2, mxbuff);
          if (strlen(buff2) > 6)
            continue;
          articulo[i].iva_porc = atof(buff2);
          //        scanw("%f",&articulo[i].pu);
          deleteln();
        }
        while (strlen(buff2) > 6);

        clrtoeol();
        deleteln();
      }

      journal_last_sale(i, last_sale_fname);
      journal_marked_items(nm_journal, buff2, exist_journal);

      show_item_list(v_arts, i, TRUE);

      if (iva_incluido)
        iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
      else
        iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
      iva += (iva_articulo * articulo[i].cant);

      tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
      tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
      tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
      tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
      tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
      tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
      for (j=0; j<maxtax; j++)
        tax[j] += (tax_item[j] * articulo[i].cant);

      subtotal += (articulo[i].pu * articulo[i].cant);
      show_subtotal(subtotal);
      articulo[++i].cant = 1;
    }
    else {  /* Articulo registrado */
      journal_last_sale(i, last_sale_fname);
      show_item_list(v_arts, i, TRUE);
      subtotal += (articulo[i].pu * articulo[i].cant);

      if (iva_incluido)
        iva_articulo =  articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
      else
        iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
      iva += (iva_articulo * articulo[i].cant);

      tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
      tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
      tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
      tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
      tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
      tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
      for (j=0; j<maxtax; j++)
        tax[j] += (tax_item[j] * articulo[i].cant);

      show_subtotal(subtotal);
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
    if (iva_incluido)
      iva_articulo = articulo[j].pu - articulo[j].pu / (articulo[j].iva_porc/100 + 1);
    else
      iva_articulo = articulo[j].pu * (articulo[j].iva_porc/100);
    tax_item[0] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_0/100 + 1);
    tax_item[1] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_1/100 + 1);
    tax_item[2] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_2/100 + 1);
    tax_item[3] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_3/100 + 1);
    tax_item[4] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_4/100 + 1);
    tax_item[5] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_5/100 + 1);
  }

  if (i) {
    attrset(COLOR_PAIR(amarillo_sobre_negro));
    attron(A_BOLD);
    mvprintw(getmaxy(stdscr)-5,0, "Total a pagar: %.2f", subtotal); /* Sale total */
    attroff(A_BOLD);
    attrset(COLOR_PAIR(blanco_sobre_negro));
    clrtoeol();
  }

  free(buff);
  free(buff2);
  if (lector_serial)
    close_serial(fd, &oldtio);
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
    mvprintw(getmaxy(stdscr)-2, 0, "�Forma de pago (Efectivo/Tarjeta/Credito/cHeque)? E");
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

/*  PGresult *registra_venta(PGconn *base, */
/*                           char   *tabla, */
/*                           int    num_venta, */
/*                           double total, */
/*                           double utilidad, */
/*                           int    tipo_pago, */
/*                           int    tipo_fact, */
/*                           bool   corte_parcial) */
/*  { */
/*    char     *comando_sql; */
/*    PGresult *resultado; */
/*    char     exp_boleana = 'F'; */

/*    if (corte_parcial) */
/*      exp_boleana = 'T'; */
/*    comando_sql = calloc(1,mxbuff); */
/*    sprintf(comando_sql, "INSERT INTO %s VALUES(%d, %.2f, %d, %d, '%c', %.2f)", */
/*                          tabla, num_venta, total, */
/*                          tipo_pago, tipo_fact, exp_boleana, utilidad); */
/*    resultado = PQexec(base, comando_sql); */
/*    if (PQresultStatus(resultado) != PGRES_COMMAND_OK) { */
/*      fprintf(stderr,"Comando INSERT INTO fall�\n"); */
/*      fprintf(stderr,"Error: %s\n",PQerrorMessage(base)); */
/*    } */
/*    free(comando_sql); */
/*    return(resultado); */
/*  } */


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

void print_ticket_arts() {
  FILE *impr;
  static int i;

  impr = fopen(nm_disp_ticket,"w");
  if (impr == NULL)
    ErrorArchivo(nm_disp_ticket);

  for (i=0; i<numarts; i++) {
    fprintf(impr," -> %s\n",articulo[i].desc);
    fprintf(impr," %3.2f x %10.2f = %10.2f",
       articulo[i].cant,articulo[i].pu,articulo[i].pu*articulo[i].cant);
        if (!articulo[i].iva_porc)
          fputs(" E", impr);
        fputs("\n", impr);
  }
  if (strstr(tipo_disp_ticket, "EPSON")) {
    fprintf(impr, "\n%cr%c      Total: %10.2f\n", ESC, 1, a_pagar);
    fprintf(impr, "%cr%c", ESC, 0);
  }
  else
    fprintf(impr, "\n     Total: %10.2f\n", a_pagar);
  fprintf(impr, "     I.V.A.: %10.2f\n", iva);
  fprintf(impr, "E = Exento\n\n");
  fclose(impr);
}

void print_ticket_footer(struct tm fecha, unsigned numventa) {
  FILE *impr,*fpie;
  static char *s;

  impr = fopen(nm_disp_ticket,"w");
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
    fprintf(impr,"e-mail: ventas@elpuntodeventa.com\n");
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
  imprime_razon_social(impr, tipo_disp_ticket,
                       "   La Botanita", "   Fernando Garcia Torres    " );
  free(s);
  fclose(impr);
}

//void ImpTicketEncabezado() {
void print_ticket_header(char *nm_ticket_header) {
  FILE *impr,*fenc;
  char *s;

  impr = fopen(nm_disp_ticket,"w");
  if (impr == NULL)
    ErrorArchivo(nm_disp_ticket);

  s = calloc(1, mxbuff);
  fenc = fopen(nm_ticket_header,"r");
  if (!fenc) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede leer el encabezado de ticket\n\r");
    fprintf(stderr,"del archivo %s\n", nm_ticket_header);
    fprintf(impr,
     "Sistema OsoPOS, programa Remision %s en\n",vers);
    fprintf(impr,"un sistema Linux ventas@elpuntodeventa.com\n");
  }
  else {
    do {
      fgets(s, sizeof(s), fenc);
      if (!feof(fenc)) {
        fprintf(impr, "%s", s);
      }
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

  impr = fopen(nm_disp_ticket,"w");
  if (!impr) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede activar el caj�n de dinero\n\r");
    fprintf(stderr,"operado por %s\n",nm_disp_ticket);
    return(0);
  }
  if (strstr(tipo_miniimp, "STAR"))
    fprintf(impr, "%c", abre_cajon_star);
  else
    fprintf(impr,"%c%c%c%c%c", ESC,'p', 0, 255, 255);
  fclose(impr);
  return(1);
}

void aviso_existencia(struct articulos art)
{
  FILE *arch;
  char aviso[255];


  sprintf(aviso, "Baja existencia en producto de proveedor num. %u\n%s %s quedan %f\n",
                 art.id_prov, art.codigo, art.desc, art.exist-art.cant);

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
    mensaje("No puedo escribir al programa de env�o de mensajes");
    getch();
  }
  else {
        fprintf(arch, "To: %s\n", dir_avisos);
        fprintf(arch, "From: %s\n", "scaja");
        fprintf(arch, "Subject: %s\n", asunto_avisos);
        if (cc_avisos != NULL)
          fprintf(arch, "Cc: %s\n", cc_avisos);
        fprintf(arch, "\n\n%s\n", aviso);
        fclose(arch);
  }
}

/* Actualiza las existencias de los art�culos que se vendieron en una sola operaci�n */
int actualiza_existencia(PGconn *base_inv, struct tm *fecha) {
  int      i,resurtir = 0;
  PGresult *res;
  char     tabla[255];
  float    pu;

  sprintf(tabla, "almacen_%d", almacen);
  for(i=0; i<numarts; i++) {
    articulo[i].exist = -1;
    pu = articulo[i].pu;
    if (!search_product(base_inv, tabla, "codigo", articulo[i].codigo, &articulo[i]))
      continue;
    if (articulo[i].exist > articulo[i].exist_min  &&
                articulo[i].exist-articulo[i].cant <= articulo[i].exist_min) {
      aviso_existencia(articulo[i]);
      resurtir++;
    }
    articulo[i].exist -= articulo[i].cant;
    articulo[i].pu = pu;
    res = salida_almacen(base_inv, almacen, articulo[i], log_name, fecha);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      fprintf(stderr, "Error al actualizar art�culo %s %s\n",
                       articulo[i].codigo, articulo[i].desc);
    PQclear(res);
  }
  return(resurtir);
}

void Termina(PGconn *con, PGconn *con_s, int error)
{
  fprintf(stderr, "Error n�mero %d encontrado, abortando...",errno);
  PQfinish(con);
  PQfinish(con_s);
  exit(errno);
}


void mensaje(char *texto)
{
  attrset(COLOR_PAIR(azul_sobre_blanco));
  mvprintw(getmaxy(stdscr)-1,0, texto);
  attrset(COLOR_PAIR(blanco_sobre_negro));
  clrtoeol();
}

long obten_num_venta(PGconn *base)
{
  char query[1024];
  PGresult *res;
  long numero;

  sprintf(query, "SELECT max(numero) FROM ventas");
  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR: no puedo obtener el n�mero de la venta");
    PQclear(res);
    return(0);
  }
  numero = atoi(PQgetvalue(res, 0, 0));
  PQclear(res);
  return(numero);
}

/********************************************************/

/* Rutinas de recuperaci�n de ventas */
int journal_last_sale(int i, char *last_sale_fname) {

  FILE *f_last_sale;

  f_last_sale = fopen(last_sale_fname, "a");
  if (f_last_sale != NULL) {
    fprintf(f_last_sale, "%3.0f ", articulo[i].cant);
    if (strlen(articulo[i].codigo) >= maxcod)
      articulo[i].codigo[maxcod-1] = 0;
    fprintf(f_last_sale, "%s ", articulo[i].codigo);
    Espacios(f_last_sale, maxcod - strlen(articulo[i].codigo));
    if (strlen(articulo[i].desc) >= maxdes)
      articulo[i].desc[maxdes-1] = 0;
    fprintf(f_last_sale, "%s ", articulo[i].desc);
    Espacios(f_last_sale, maxdes - strlen(articulo[i].desc));
    fprintf(f_last_sale, "%.2f\n", articulo[i].pu);
    fclose(f_last_sale);
    return(OK);
  }
  else
    return(ERROR_ARCHIVO_1);
}

int journal_marked_items(char *marked_items_fname, char *code, int exist_previous) {
  FILE *f_last_items;

  if (exist_previous)
    return(OK);
  f_last_items = fopen(marked_items_fname, "a");
  if (f_last_items != NULL) {
    fprintf(f_last_items, "%s\n", code);
    fclose(f_last_items);
    return(OK);
  }
  else
    return(FILE_1_ERROR);
}

int clean_journal(char *marked_items_fname) {
  /*  FILE *f_last_items;
  f_last_items = fopen(marked_items_fname, "r");
  if (f_last_items == NULL)
    return(FILE_1_ERROR);
  else
    fclose(f_last_items);
  return(OK);
  */
  if (unlink(marked_items_fname))
    return(FILE_1_ERROR);
  else
    return(OK);
}

int  check_for_journal(char *dirname) {
  DIR *dir;
  struct dirent *dir_ent;
  FILE *p_cmd;
  char *pid[255]; /* IGM We need to make it a double pointer */
  char buf[mxbuff];
  char *buff2;
  int i=0, orphan=0;

  buff2 = calloc(1, mxbuff);
  dir = opendir(dirname);
  if (dir == NULL)
    return(FILE_1_ERROR);

  /* IGM We need to change de hardcode in the near future */
  sprintf(buf, "/sbin/pidof %s", program_name);
  p_cmd = popen(buf, "r");

  if (!feof(p_cmd))
    fgets(buf, mxbuff, p_cmd);
  else {
    closedir(dir);
    pclose(p_cmd);
    return(FILE_2_ERROR);
  }

  pclose(p_cmd);
  buf[strlen(buf)-1] = 0;
  pid[0] = strtok(buf, " ");

  /* Fetch the pids */
  while ((pid[++i] = strtok(NULL, " ")) != NULL) {
  }

  /* We could use scandir() for this */
  while ((dir_ent = readdir(dir)) != NULL  &&  !orphan) {
    if (strstr(dir_ent->d_name, ".last_items.")!=NULL) {
      strcpy(buff2, dir_ent->d_name+12);
      i = 0;
      orphan = atoi(buff2);
      while (pid[i] != NULL && orphan) {
        if (strcmp(buff2, pid[i++]) == 0)
          orphan = 0;
      }
      /* If we get here, there was a process lost and an orphan sale */
    }
  }
  closedir(dir);
  free(buff2);
  return(orphan);
}

int aborta_remision(PGconn *con, PGconn *con_s, char *mens, char tecla, int senial) {

  do
    mensaje(mens);
  while (toupper(getch()) != tecla);

  PQfinish(con);
  PQfinish(con_s);
  endwin();

  free(nmfpie);
  nmfpie = NULL;
  free(nmfenc);
  nmfenc = NULL;
  free(nmtickets);
  nmtickets = NULL;
  free(nm_factur);
  nm_factur = NULL;
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

  exit(senial);
}

/********************************************************/
/********************************************************/
/*** rutinas de comunicaci�n serial. Basado en el Serial-Programming-HOWTO ***/
void init_serial(struct sigaction *saio, struct termios *oldtio, struct termios *newtio)
{
  /* open the device to be non-blocking (read will return immediatly) */
  fd = open(disp_lector_serie, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd <0) {perror(disp_lector_serie); exit(-1); }

  /* install the signal handler before making the device asynchronous */
  saio->sa_handler = signal_handler_IO;
  saio->sa_mask.__val[0] =  0;
  saio->sa_flags = 0;
  saio->sa_restorer = NULL;
  sigaction(SIGIO,saio,NULL);
  
  /* allow the process to receive SIGIO */
  fcntl(fd, F_SETOWN, getpid());
  /* Make the file descriptor asynchronous (the manual page says only 
     O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
  fcntl(fd, F_SETFL, FASYNC);

  tcgetattr(fd,oldtio); /* save current port settings */
  /* set new port settings for canonical input processing */
  newtio->c_cflag = serial_bps | CRTSCTS | CS8 | CLOCAL | CREAD;
  //  newtio->c_iflag = IGNPAR | ICRNL;
  newtio->c_iflag = IGNPAR | ICRNL;
  newtio->c_oflag = 0;
  newtio->c_lflag = ICANON;
  newtio->c_cc[VMIN]=1;
  newtio->c_cc[VTIME]=0;
  tcflush(fd, TCIFLUSH);
  tcsetattr(fd,TCSANOW,newtio);
}

void close_serial(int fd, struct termios *tio)
{
  /* restore old port settings */
  tcsetattr(fd,TCSANOW,tio);
}

void signal_handler_IO (int status)
{
  int res;

  res = read(fd,s_buf,255);
  if (s_buf[res-1]=='\n')
    s_buf[res-1]=0;
  else
    s_buf[res]=0;

  wait_flag = FALSE;
}


/********************************************************/   

int main(int argc, char *argv[]) {
  static char buffer, buf[255];
  static char encabezado1[mxbuff],
      encabezado2[mxbuff] = "E. Israel Osorio H., 1999-2003 soporte@elpuntodeventa.com";
  FILE *impr_cmd;
  time_t tiempo;        
  static int dgar;
  PGconn *con, *con_s;
  long num_venta = 0;
  unsigned formadepago;
  double utilidad;
  double tax[maxtax];
  struct tm *fecha;     /* Hora y fecha local   */
  int i;

  program_name = argv[0];

  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fecha->tm_year += 1900;
  pid = getpid();

  init_config();
  read_general_config();
  read_global_config();
  read_config();
  initscr();
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

  con_s = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd);
  if (con_s == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }
  //  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, "osopos", log_name, "");
  if (db.passwd == NULL)
    obten_passwd(db.user, db.passwd);
  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.user, db.passwd); 
  if (con == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }
  //  num_venta = obten_num_venta(nm_num_venta);
  num_venta = obten_num_venta(con);

  numbarras = lee_articulos(con, con_s);
  if (!numbarras) {
    mensaje("No hay art�culos en la base de datos. Use el programa de inventarios primero");
    getch();
  }
  else if (numbarras == SQL_ERROR) {
    mensaje("Error al leer base de datos, b�squeda de art�culos deshabilitada.");
    getch();
    numbarras = 0;
  }

  numdivisas = lee_divisas(con);
  if (!numdivisas) {
    mensaje("No hay divisas en la base de datos. Use el programa de inventarios primero");
    getch();
  }
  else if (numdivisas == SQL_ERROR) {
    mensaje("Error al leer base de datos, b�squeda de divisas deshabilitada.");
    getch();
    numbarras = 0;
  }

  sprintf(encabezado1, "Sistema OsoPOS - Programa Remision %s R.%s", vers, release);
  do {
    tiempo = time(NULL);
    fecha = localtime(&tiempo);
    fecha->tm_year += 1900;
    for (i=0; i<maxtax; i++)
      tax[i] = 0.0;


    clear();
    mvprintw(0,0,"%u/%u/%u",
               fecha->tm_mday, (fecha->tm_mon)+1, fecha->tm_year);
    attrset(COLOR_PAIR(cyan_sobre_negro));
    mvprintw(0,(getmaxx(stdscr)-strlen(encabezado1))/2,
             "%s",encabezado1);
    mvprintw(1,(getmaxx(stdscr)-strlen(encabezado2))/2,
             "%s",encabezado2);
    attrset(COLOR_PAIR(blanco_sobre_negro));
    a_pagar = item_capture(con, &numarts, &utilidad, tax, *fecha);


    if (numarts && a_pagar) {
      attron(A_BOLD);
      mvprintw(getmaxy(stdscr)-2,0,"�Imprimir Remision, Factura o Ticket (R,F,T)? T\b");
      attroff(A_BOLD);
      buffer = toupper(getch());
      formadepago = forma_de_pago();

      switch (buffer) {
        case 'R':
          attron(A_BOLD);
          mvprintw(getmaxy(stdscr)-1,0,"�D�as de garant�a? : ");
          attroff(A_BOLD);
          clrtoeol();
          scanw("%d",&dgar);
          num_venta = sale_register(con, con_s, "ventas", a_pagar, iva, tax, utilidad, formadepago,
                         _NOTA_MOSTRADOR, FALSE, *fecha, id_teller, 0, articulo, numarts, almacen);
          if (num_venta<0)
            aborta_remision(con, con_s, "ERROR AL REGISTRAR VENTA, PRESIONE \'A\' PARA ABORTAR...", 'A', (int)num_venta);
            
          if (num_venta>0)
            clean_journal(nm_journal);
          sprintf(buf,"%s %ld %d",nmimprrem, num_venta, dgar);
          impr_cmd = popen(buf, "w");
          if (pclose(impr_cmd) != 0)
            fprintf(stderr, "Error al ejecutar %s\n", buf);
          if (formadepago >= 20) {
            AbreCajon(tipo_disp_ticket);
            sprintf(buf, "lpr -P %s %s", lp_disp_ticket, nm_disp_ticket);
            impr_cmd = popen(buf, "w");
            pclose(impr_cmd);
          }
          mensaje("Aprieta una tecla para continuar (t para terminar)...");
          buffer = toupper(getch());
        break;
      case 'F':
        if (formadepago >= 20) {
          AbreCajon(tipo_disp_ticket);
          sprintf(buf, "lpr -P %s %s", lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
        }
        num_venta = sale_register(con, con_s, "ventas", a_pagar, iva, tax, utilidad, formadepago,
                       _FACTURA, FALSE, *fecha, id_teller, 0, articulo, numarts, almacen);
        if (num_venta<0)
          aborta_remision(con, con_s, "ERROR AL REGISTRAR VENTA, PRESIONE \'A\' PARA ABORTAR...", 'A', (int)num_venta);
            
        if (num_venta>0)
          clean_journal(nm_journal);
        sprintf(buf, "Venta %ld. Aprieta una tecla para capturar, c para cancelar...", num_venta);
        mensaje(buf);
        buffer = toupper(getch());
        if (buffer != 'C') {
          if (strstr(nm_factur, "http") != NULL)
            sprintf(buf, "%s?id_venta=%ld &", nm_factur, num_venta);
          else
            sprintf(buf, "%s -r %ld", nm_factur, num_venta);
          system(buf);
        }
        clear();
        mensaje("Aprieta una tecla para continuar (t para terminar)...");
        buffer = toupper(getch());
        break;
      case 'T':
      default:
        print_ticket_arts();
        sprintf(buf, "lpr -Fb -P %s %s", lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        unlink(nm_disp_ticket);
        if (formadepago >= 20) {
          AbreCajon(tipo_disp_ticket);
          sprintf(buf, "lpr -Fb -P %s %s", lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
          unlink(nm_disp_ticket);
        }
        num_venta = sale_register(con, con_s, "ventas", a_pagar, iva, tax, utilidad,
                                   formadepago, _TEMPORAL, FALSE, *fecha,
                                   id_teller, 0, articulo, numarts, almacen);
        if (num_venta<0)
          aborta_remision(con, con_s, "ERROR AL REGISTRAR VENTA, PRESIONE \'A\' PARA ABORTAR...", 'A', (int)num_venta);

        print_ticket_footer(*fecha, num_venta);
        if (num_venta>0)
          clean_journal(nm_journal);
        sprintf(buf, "lpr -Fb -P %s %s", lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        unlink(nm_disp_ticket);
        mensaje("Corta el papel y aprieta una tecla para continuar (t para terminar)...");
        buffer = toupper(getch());
        print_ticket_header(nmfenc);
        sprintf(buf, "lpr -Fb -P %s %s", lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        break;
      }
      if (actualiza_existencia(con, fecha)) {
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

  PQfinish(con);
  PQfinish(con_s);
  unlink(nm_journal);
  endwin();

  free(nmfpie);
  nmfpie = NULL;
  free(nmfenc);
  nmfenc = NULL;
  free(nmtickets);
  nmtickets = NULL;
  free(nm_factur);
  nm_factur = NULL;
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

/* Pendientes:

- Optimizar la funcion busca_precio para que solo devuelva el indice del c�digo buscado y
  posteriormente se haga una copia de registro a registro

  */
