/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa Remision 1.47 (C) 1999-2006 E. Israel Osorio H.
   eosorioh en gmail punto com
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


#define MALLOC_CHECK_ 2

//#include <linux/kbd_kern.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <glib.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */

//#define _SERIAL_COMM   /* Se usarán rutinas de comunicación serial */

#include "include/pos-curses.h"
#define _pos_curses

#ifndef CTRL
#define CTRL(x)         ((x) & 0x1f)
#endif

#define QUIT            CTRL('Q')
#define ESCAPE          CTRL('[')
#define ENTER		10

#include "include/print-func.h"
#define _printfunc

#define vers "1.47"
#define release "ElPunto"

#ifndef maxdes
#define maxdes 39
#endif

#ifndef maxcod
#define maxcod 20
#endif

#ifndef mxmembarra
#define mxmembarra 100
#endif

#ifndef maxitem_busqueda
#define maxitem_busqueda 100
#endif

#define blanco_sobre_negro       1
#define amarillo_sobre_negro     2
//#define verde_sobre_negro        3
#define azul_sobre_blanco        4
#define cyan_sobre_negro         5
//#define normal                   6
//#define amarillo_sobre_azul      7
#define inverso                  8


/* Printer codes */
#define abre_cajon_star 7
#ifndef ESC
#define ESC 27
#endif

#define _PAGO_TARJETA    1
#define _PAGO_CREDITO    2
#define _PAGO_EFECTIVO  20
#define _PAGO_CHEQUE    21

#include "include/minegocio-remis.h"

int forma_de_pago(double *, double *);
void Termina(PGconn *con, PGconn *con_s, int error);
//int LeeConfig(char*, char*);
void print_ticket_arts(double importe, double cambio);
void print_ticket_header(char *nm_ticket_header);
void print_ticket_footer(struct tm fecha, unsigned numvents, long folio, int serie);
int AbreCajon(char *tipo_miniimp);
int corta_papel(char *tipo_miniimp);
/* Función que reemplaza a ActualizaEx */
int actualiza_existencia(PGconn *con, struct tm *fecha);
/* Esta función reemplaza a leebarras */
int lee_articulos(PGconn *, PGconn *, short almacen);
void show_item_list(WINDOW *vent, int i, short cr);
void show_subtotal(double, double, double);
void mensaje(char *texto);
void aviso_existencia(struct articulos art);
void switch_pu(WINDOW *, struct articulos *, int, double *, double tax[maxtax], int);
int journal_last_sale(int i, char *last_sale_fname);
int journal_marked_items(char *marked_items_fname, char *code, int exist_previous); 
int clean_journal(char *marked_items_fname);
int check_for_journal(char *dir_name, char *program_name);
int cancela_articulo(WINDOW *vent, int *reng, double *subtotal, double *iva,
                     double tax[maxtax], char *last_sale_fname);
int aborta_remision(PGconn *con, PGconn *con_s, char *mensaje, char tecla, int senial);
#ifdef _SERIAL_COMM
//void captura_serial(char *);
void init_serial(struct sigaction *saio, struct termios *oldtio, struct termios *newtio);
void signal_handler_IO (int status);
void close_serial(int fd, struct termios *tio);
#endif
int busqueda_articulo(char *);
void muestra_renglon(WINDOW *, int y, int x, short borra_ven, unsigned renglon, unsigned num_items);
int datos_renta(int *num_cliente, time_t *f_pedido, time_t *f_entrega);
double tipo_cambio(char *);
void asign_mem_code(struct articulos *art, int item);
int find_mem_code(char *cod, struct articulos *art, int search_2nd, short *es_granel);
int find_db_code(PGconn *con, char *cod, struct articulos *art, int wday, short *es_granel);
int form_virtualize(WINDOW *w, int readchar, int c);
int busca_art_marcado(char *cod, struct articulos art[maxarts], int campo, int ren);
int imprime_fechas_renta(PGconn *con, PGconn *con_s, time_t *f_pedido, time_t *f_entrega);
int print_customer_data(PGconn *con, PGconn *con_s, long int id_cliente);
int captura_vendedor();
int selecciona_caja_virtual(PGconn *con, int num_cajas);
void read_pos_config(PGconn *con, unsigned num_caja);
int read_db_config(PGconn *con);

volatile int STOP=FALSE; 

FILE *impresora;
double a_pagar; 
int numbarras=0;
int numdivisas=0;
/* struct barras barra[mxmembarra];  */
struct articulos articulo[maxarts], barra[mxmembarra]; /* barra se queda siempre en memoria */
struct divisas divisa[mxmemdivisa];
int numarts=0;          /* Número de items capturados   */
short unsigned maxitemr;
pid_t pid;
short es_remision=1;
unsigned almacen;
struct db_data db;
char   *item[maxitem_busqueda]; /* Líneas de caracteres en ventana de búsqueda */

  /* Variables de configuración */
char *home_directory;
char *log_name;
char *nm_disp_ticket,           /* Nombre de impresora de ticket */
  *cmd_lp,              /* Comando usado para imprimir */
  *disp_lector_serie,  /* Ruta al scanner de c. de barras serial */
  *nmfpie,              /* Pie de página de ticket */
  *nmfenc,              /* Archivo de encabezado de ticket */
  *nmtickets,   /* Registro de tickets impresos */
  *tipo_disp_ticket, /* Tipo de miniprinter */
  *nmimprrem,           /* Ubicación de imprrem */
  *nm_factur,    /* Nombre del programa de facturación */
  *nm_avisos,    /* Nombre del archivo de avisos */
  *nm_journal,   /* Nombre actual del archivo del diario de marcaje */
  *nm_orig_journal, /* Nombre del archivo del diario original */
  *nm_sendmail,  /* Ruta completa de sendmail */
  *dir_avisos,   /* email de notificación */
  *asunto_avisos, /* Asunto del correo de avisos */
  *cc_avisos;     /* con copia para */

gchar  *lp_disp_ticket;      /* Definición de miniprinter en /etc/printcap */

char   s_divisa[MX_LON_DIVISA];       /* Designación de la divisa que se usa para cobrar en la base de datos */
short unsigned search_2nd;  /* ¿Buscar código alternativo al mismo tiempo que el primario ? */ 
int id_teller = 0;              /* Número de cajero */
int id_vendedor = 0;            /* Número de agente de ventas */
short unsigned manual_discount; /* Aplicar el descuento manual de precio en la misma línea */
int iva_incluido;
int listar_neto;
short unsigned autocutter = 0;

int lector_serial; /* Cierto si se usa un scanner serial */
int serial_crlf=1; /* 1 si el scanner envia CRLF, 0 si solamente LF */
tcflag_t serial_bps=B38400; /* bps por omisión */
int wait_flag=TRUE;                    /* TRUE while no signal received */
char s_buf[255];    /* Buffer de puerto serie */
struct termios oldtio,newtio;  /* Parametros anteriores y nuevos de terminal serie */
struct sigaction saio;              /* definition of signal action */
int fd;            /* Descriptor de archivo de puerto serie */

int serial_num_enable = 0;
int catalog_search = 0;
int lease_mode = 0;
int cnf_impresion_garantia = 1;
int serie;        /* Serie de folios de comprobantes */
int cnf_registrar_vendedor = 0;
unsigned caja_virtual = 0; /* Número de caja virtual que se está operando */

#include "include/remision.h"
#include "include/articulos-bd-func.h"
#include "include/pos-bd-func.h"

int init_config()
{
  home_directory = calloc(1, 255);
  log_name = calloc(1, 255);

  strcpy(home_directory, getenv("HOME"));
  strcpy(log_name, getenv("USER"));

  nm_journal = calloc(1, mxbuff);
  nm_orig_journal = calloc(1, strlen(home_directory) + strlen("/.last_items.") + 10);
  /* Dejamos 10 caracteres extras para el PID */
  sprintf(nm_orig_journal, "%s/.last_items.%d", home_directory, pid);
  search_2nd = 1;

  nm_disp_ticket = calloc(1, strlen("/tmp/ticket_")+strlen(log_name)+1);
  strcpy(nm_disp_ticket, "/tmp/ticket_");
  strcat(nm_disp_ticket, log_name);

  lp_disp_ticket = g_strdup_printf("ticket");

  disp_lector_serie = calloc(1, strlen("/dev/barcode")+1);
  strcpy(disp_lector_serie, "/dev/barcode");
  lector_serial = 0;

  tipo_disp_ticket = calloc(1, strlen("STAR")+20);
  strcpy(tipo_disp_ticket, "STAR");

  cmd_lp = calloc(1, strlen("lpr -P ")+1);
  strcpy(cmd_lp, "lpr -P ");

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

  db.hostname = g_strdup("255.255.255.255");
  db.hostport = g_strdup("5432");
  db.name = g_strdup("elpuntodeventa.com");
  db.user = g_ascii_strdown(g_strdup_printf("%s", log_name), -1);
  db.sup_user = g_strdup_printf("scaja");

  maxitemr = 6;

  iva_incluido = 0;
  TAX_PERC_DEF = 15;
  strcpy(s_divisa, "MXP");

  manual_discount = 0;

  almacen = 1;

  serie = 1;

  return(OK);
}

int read_config()
{
  gchar *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;
  //  struct kbd_struct teclado;

  //  setledstate(teclado, VC_NUMLOCK); 

  nmconfig = g_strdup_printf("%s/.osopos/remision.config", home_directory);

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

        /* OBSOLETO por config. caja virtual. Definición de impresora de ticket */
/*       if (!strcmp(b,"ticket")) { */
/*         strncpy(buf, strtok(NULL,"="), mxbuff); */
/*         aux = realloc(nm_disp_ticket, strlen(buf)+1); */
/*         if (aux != NULL) { */
/*           strcpy(nm_disp_ticket, buf); */
/*           aux = NULL; */
/*         } */
/*         else */
/*           fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n", b); */
/*       } */
      if (!strcmp(b,"ticket.pie")) {
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
        g_free(db.hostname);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.hostname =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.port")) {
        g_free(db.hostport);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.hostport =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.passwd")) {
        g_free(db.passwd);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.passwd =  g_strdup_printf("%s", buf);
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
      /*     else if (!strcmp(b,"almacen.numero")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       almacen = atoi(buf);
       }*/
#ifdef _SERIAL_COMM
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
       int i_buf;
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
#endif
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

int read_global_config()
{
  gchar *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;
  
  nmconfig = g_strdup_printf("/etc/osopos/remision.config");

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

        /* OBSOLETO por config. caja virtual. Definición de impresora de ticket */
/*       if (!strcmp(b,"ticket")) { */
/*         strncpy(buf, strtok(NULL,"="), mxbuff); */
/*         aux = realloc(nm_disp_ticket, strlen(buf)+1); */
/*         if (aux != NULL) { */
/*           strcpy(nm_disp_ticket, buf); */
/*           aux = NULL; */
/*         } */
/*         else */
/*           fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n", b); */
/*       } */
      /* REMPLAZADO POR CATALOGO DE CONFIGURACION EN BASE DE DATOS */
      /*      else if (!strcmp(b, "lp_ticket")) {
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
                  }*/
      if (!strcmp(b,"ticket.pie")) {
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
      /* REMPLAZADO POR CATALOGO DE CONFIGURACION EN BASE DE DATOS */
      /*      else if (!strcmp(b,"miniimpresora.tipo") || !strcmp(b,"impresion.tipo_miniprinter")) {
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
      else if (!strcmp(b,"impresion.comando")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(cmd_lp, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(cmd_lp, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "remision. Error de memoria en argumento de configuracion %s\n",
                  b);
                  }*/
      else if (!strcmp(b,"impresion.autocutter")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        if (atoi(buf) == 1)
          autocutter = 1;
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
        g_free(db.hostname);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.hostname =  g_strdup_printf("%s", buf);
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
        g_free(db.sup_user);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_user =  g_strdup_printf("%s", buf);
      }
      else if (!strcmp(b,"db.sup_passwd")) {
        g_free(db.sup_passwd);
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_passwd =  g_strdup_printf("%s", buf);
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
#ifdef _SERIAL_COMM
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
        int i_buf;
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
#endif
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
     else if (!strcmp(b, "desglosar_descuento")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       manual_discount = atoi(buf);
     }
     else if (!strcmp(b, "capturar_serie")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       serial_num_enable = atoi(buf);
     }
     else if (!strcmp(b, "consulta_catalogo")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       catalog_search = atoi(buf);
     }
     else if (!strcmp(b,"modo_renta")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       lease_mode = atoi(buf);
     }
     else if (!strcmp(b, "registrar_vendedor")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       if (atoi(buf) > 0)
         cnf_registrar_vendedor = TRUE;
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

        /* Obsoleto por config. caja virtual. Definición de impresora de ticket */
/*       if (!strcmp(b,"ticket")) { */
/*         strncpy(buf, strtok(NULL,"="), mxbuff); */
/*         aux = realloc(nm_disp_ticket, strlen(buf)+1); */
/*         if (aux != NULL) { */
/*           strcpy(nm_disp_ticket, buf); */
/*           aux = NULL; */
/*         } */
/*         else */
/*           fprintf(stderr, "remision. Error de memoria en argumento de configuracion %s\n", b); */
/*       } */
      /* REMPLAZADO POR CATALOGO DE CONFIGURACION EN BASE DE DATOS */
      /*      else if (!strcmp(b, "lp_ticket")) {
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
                  }*/
      /* REMPLAZADO POR CATALOGO DE CONFIGURACION EN BASE DE DATOS */
      /*      else if (!strcmp(b,"miniimpresora.tipo")) {
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
                  }*/
      if (!strcmp(b,"programa.factur")) {
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
        int i_buf;
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

void asign_mem_code(struct articulos *art, int item) {

double tc=1;

  strncpy(art->codigo, barra[item].codigo, maxcod);
  strncpy(art->codigo2, barra[item].codigo2, maxcod);
  strncpy(art->desc, barra[item].desc, maxdes);

  tc = tipo_cambio(barra[item].divisa);

  art->p_costo = barra[item].p_costo;
  art->pu = barra[item].pu * tc;
  art->pu2= barra[item].pu2 * tc;
  art->pu3= barra[item].pu3 * tc;
  art->pu4= barra[item].pu4 * tc;
  art->pu5= barra[item].pu5 * tc;
  art->id_prov = barra[item].id_prov;
  art->id_depto = barra[item].id_depto;
  art->iva_porc = barra[item].iva_porc;
  strcpy(art->divisa, barra[item].divisa);
  art->tax_0 = barra[item].tax_0;
  art->tax_1 = barra[item].tax_1;
  art->tax_2 = barra[item].tax_2;
  art->tax_3 = barra[item].tax_3;
  art->tax_4 = barra[item].tax_4;
  art->tax_5 = barra[item].tax_5;

}

/***************************************************************************/

int find_mem_code(char *cod, struct articulos *art, int search_2nd, short *es_granel) {
//int busca_precio(char *cod, struct articulos *art) {
/* Busca el código de barras en la memoria y devuelve la posicion */
/* del producto en la matriz de productos en memoria. */
int i;
int exit=0; /* falso si hay que salirse */
int granel=-1; /* Coincidencia con PLU de producto de granel */
gchar *plu_granel;
gchar *s_peso;

  plu_granel = NULL;
  if (strlen(cod) == 13) /* longitud de EAN13 */
    /* Se asigna un max. de 6 digitos en el código de barras para el PLU */
    plu_granel = g_strndup(cod+1, 6);

  for (i=0; (i<numbarras); ++i) {
    exit = strcmp(cod, barra[i].codigo);
    if (exit && search_2nd)
      exit = strcmp(cod, barra[i].codigo2);

    if (!exit) {
      asign_mem_code(art, i);
      *es_granel = FALSE;
      return(i);
    }
    else
      if (plu_granel && barra[i].granel == 't' && strcmp(plu_granel, barra[i].codigo) == 0) {
        granel = i;
      }
  }
  /* No se localizo el código como tal, ¿coincide como plu de granel ? */
  if (granel>=0) {
    asign_mem_code(art, granel);
    s_peso = g_strndup(cod+8, 4);
    art->cant = atof(s_peso)/1000;
    *es_granel = TRUE;
    return(granel);
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

  mvprintw(getmaxy(stdscr)-3,0,"Código del(los) artículo(s) a cancelar: ");
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
  /* Desplaza los artículos a una posición inferior */
  for (; i<*reng; i++) {
    memcpy(&articulo[i], &articulo[i+1], sizeof(articulo[i+1]));
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

  mvprintw(getmaxy(stdscr)-3,0,"Código de barras, descripción o cantidad:\n");
  wrefresh(vent);
  return(num_item);
}

#include "include/articulos-curses-func.h"

void switch_pu(WINDOW *v_arts,
               struct articulos *art,
               int which_pu,
               double *subtotal,
               double tax[maxtax],
               int i) 
{
  double iva_articulo, iva_articulo_o;
  double tax_item[maxtax], tax_item_o[maxtax];
  double new_pu, old_pu;
  int j;

  old_pu = art->pu;
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

  if (iva_incluido) {
    iva_articulo = new_pu - new_pu / (art->iva_porc/100 + 1);
    iva_articulo_o = old_pu - old_pu / (art->iva_porc/100 + 1);
  }
  else {
    iva_articulo = new_pu * (art->iva_porc/100);
    iva_articulo_o = old_pu * (art->iva_porc/100);
  }
  iva += ((iva_articulo-iva_articulo_o) * art->cant);

  tax_item[0] =  new_pu - new_pu / (art->tax_0/100 + 1);
  tax_item_o[0] =  old_pu - old_pu / (art->tax_0/100 + 1);
  tax_item[1] =  new_pu - new_pu / (art->tax_1/100 + 1);
  tax_item_o[1] =  old_pu - old_pu / (art->tax_1/100 + 1);
  tax_item[2] =  new_pu - new_pu / (art->tax_2/100 + 1);
  tax_item_o[2] =  old_pu - old_pu / (art->tax_2/100 + 1);
  tax_item[3] =  new_pu - new_pu / (art->tax_3/100 + 1);
  tax_item_o[3] =  old_pu - old_pu / (art->tax_3/100 + 1);
  tax_item[4] =  new_pu - new_pu / (art->tax_4/100 + 1);
  tax_item_o[4] =  old_pu - old_pu / (art->tax_4/100 + 1);
  tax_item[5] =  new_pu - new_pu / (art->tax_5/100 + 1);
  tax_item_o[5] =  old_pu - old_pu / (art->tax_5/100 + 1);
  for (j=0; j<maxtax; j++)
    tax[j] += ((tax_item[j]-tax_item_o[j]) * art->cant);

  *subtotal += (art->cant * (new_pu - art->pu));
  art->pu = new_pu;
  wmove(v_arts, v_arts->_cury-1, 0);
  show_item_list(v_arts, i-1, TRUE);
  show_subtotal(*subtotal, iva, *subtotal+(!iva_incluido*iva));


}

/***************************************************************************/

int forma_de_pago(double *recibo, double *dar)
{
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
    if (*recibo != 0.0) {
      *recibo = 0.0;
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
  while (*recibo);

  if (tipo >= 20) {
    mvprintw(getmaxy(stdscr)-2,0,"Se recibe: ");
    clrtoeol();
    scanw("%lf",recibo);
    if (*recibo >= a_pagar)
      *dar = *recibo - a_pagar;
    else
      *dar = 0;
    mvprintw(getmaxy(stdscr)-2, getmaxx(stdscr)/2, "Cambio de: %.2f", *dar);
  }
  return(tipo);
}

/***************************************************************************/

int captura_vendedor()
{
  char id[mxbuff];

  move(getmaxy(stdscr)-3, 0);
  clrtoeol();

  mvprintw(getmaxy(stdscr)-2, 0, "¿Número de vendedor? ");
  clrtoeol();
  wgetstr(stdscr, id);
  return(atoi(id));
}

/***************************************************************************/

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

void print_ticket_arts(double importe, double cambio) {
  FILE *impr;
  int i;

  impr = fopen(nm_disp_ticket,"w");
  if (impr == NULL)
    ErrorArchivo(nm_disp_ticket);

  //  fprintf(impr,"%c%c", ESC, 'M');
  //  fprintf(impr, "%c%c", 27, 69);
  //  fprintf(impr, "%c%c%c%c%c", 27, 38, 107, 50, 83);
  for (i=0; i<numarts; i++) {
    fprintf(impr," -> %s\n",articulo[i].desc);
    fprintf(impr," %3.2f x %10.2f = %10.2f",
            articulo[i].cant,articulo[i].pu,articulo[i].pu*articulo[i].cant);
    if (!articulo[i].iva_porc)
      fputs(" E", impr);
    fputs("\n", impr);
    if (cnf_impresion_garantia && strcmp(articulo[i].codigo, "Sin codigo")) {
      fprintf(impr, "%d ", articulo[i].garan_t);
      switch (articulo[i].garan_unidad) {
        case 0:
       default:
          fputs("días ", impr);
          break;
        case 1:
          fputs("meses ", impr);
          break;
        case 2:
          fputs("año(s) ", impr);
          break;
      }
      fputs("de garantía\n", impr);
    }
  }

  if (strstr(tipo_disp_ticket, "EPSON")) {
    fprintf(impr, "\n%cr%c      Total: %10.2f\n", ESC, 1, a_pagar);
    fprintf(impr, "%cr%c", ESC, 0);
  }
  else
    fprintf(impr, "\n     Total: %10.2f\n", a_pagar);
  fprintf(impr, "     I.V.A.: %10.2f\n", iva);
  fprintf(impr, "Recibido: %.2f  Cambio: %.2f\n", importe, cambio);
  fprintf(impr, "E = Exento\n\n");
  fclose(impr);
}

void print_ticket_footer(struct tm fecha, unsigned numventa, long folio, int serie) {
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
  if (caja_virtual > 0)
    fprintf(impr, "Caja: %d. ", caja_virtual);
  else
    fprintf(impr, "Caja única. ");
  fprintf(impr, "Cajero %d", id_teller);
  fprintf(impr, "Nota num. %ld, serie %c. Venta %u\n", folio, 'A'+serie-1, numventa);
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

  sprintf(s, "Eduardo Israel Osorio Hernandez");
  imprime_razon_social(impr, tipo_disp_ticket,
                       "elpuntodeventa", s );
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
    fprintf(impr,"un sistema Linux. elpuntodeventa.com\n");
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
  fprintf(impr,"---------------------------------------\n");
  fclose(impr);
  free(s);
}

int print_customer_data(PGconn *con, PGconn *con_s, long int id_cliente) {
  FILE *impr;
  char query[mxbuff];
  PGresult *db_res;
  struct datoscliente clt;

  sprintf(query, "SELECT ap_paterno, ap_materno, nombres FROM cliente WHERE id=%ld ",
          id_cliente);

  db_res = PQexec(con, query);
  if (PQresultStatus(db_res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR: no puedo obtener datos del cliente.\n");
    PQclear(db_res);
    return(ERROR_SQL);
  }
  sprintf(clt.nombre, "%s %s", PQgetvalue(db_res, 0, 2), PQgetvalue(db_res, 0, 0));
  
  if (!(impr = popen(cmd_lp, "w"))) {
    fprintf(stderr, "Error: No puedo imprimir. %s\n", cmd_lp);
    return(PROCESS_ERROR);
  }
  fprintf(impr, "Cliente: %ld\n", id_cliente);
  pclose(impr);
  PQclear(db_res);
  return(OK);
}

/********************************************************/   

int AbreCajon(char *tipo_miniimp) {
  FILE *impr;

  impr = fopen(nm_disp_ticket,"w");
  if (!impr) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede activar el cajón de dinero\n\r");
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

/********************************************************/   

int corta_papel(char *tipo_miniimp) {
  FILE *impr;

  impr = fopen(nm_disp_ticket,"w");
  if (!impr) {
    fprintf(stderr,
      "OsoPOS(Remision): No se puede cortar el papel\n\r");
    fprintf(stderr,"operado por %s\n",nm_disp_ticket);
    return(0);
  }
  if (strstr(tipo_miniimp, "STAR"))
    fprintf(impr, "%c", abre_cajon_star);
  else if (strstr(tipo_miniimp, "EPSONTM"))
    fprintf(impr, "\n%ci", ESC);
  else if (strstr(tipo_miniimp, "EPSON"))
    fprintf(impr,"\n%cV%c", 29, 1);
  fclose(impr);
  return(1);


}

/********************************************************/   

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
    mensaje("No puedo escribir al programa de envío de mensajes");
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

/************************************************/
void Termina(PGconn *con, PGconn *con_s, int error)
{
  fprintf(stderr, "Error número %d encontrado, abortando...",error);
  if (con != NULL)
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
    fprintf(stderr, "ERROR: no puedo obtener el número de la venta");
    PQclear(res);
    return(0);
  }
  numero = atoi(PQgetvalue(res, 0, 0));
  PQclear(res);
  return(numero);
}

/********************************************************/

/* Rutinas de recuperación de ventas */
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

int  check_for_journal(char *dirname, char *program_name) {
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

  g_free(db.name);
  g_free(db.user);
  g_free(db.passwd);
  g_free(db.sup_user);
  g_free(db.sup_passwd);
  g_free(db.hostport);
  g_free(db.hostname);

  /*if (cc_avisos != NULL)
    free(cc_avisos);*/
  free(asunto_avisos);
  asunto_avisos = NULL;
  free(tipo_disp_ticket);
  tipo_disp_ticket = NULL;
  g_free(lp_disp_ticket);
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
#ifdef _SERIAL_COMM
/********************************************************/
/*** rutinas de comunicación serial. Basado en el Serial-Programming-HOWTO ***/
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
#endif

int busqueda_articulo(char *codigo)
{
  WINDOW *v_busq;
  PANEL  *pan = 0;

  int vx, vy; /* Dimensiones horizontal y vertical de la ventana */
  int x, y;   /* Coordenadas del primer elemento del menú */
  int max_v_desc;
  int i,j,k;
  int num_items = 0;
  int ch, finished = 0;
  char descr[mxbuff];
  char *aux;
  gchar *b;
  char *cod[maxitem_busqueda];

  vx = 70;
  vy = 15;
  x = 1;
  y = 1;
  max_v_desc = vx - maxcod - 15;
  b = g_malloc(255);
  pan = mkpanel(COLOR_BLACK, vy, vx, (getmaxy(stdscr)-vy)/2, (getmaxx(stdscr)-vx)/2);
  set_panel_userptr(pan, "pan");

  v_busq = panel_window(pan);


  scrollok(v_busq, TRUE);
  box(v_busq, 0, 0);

  aux = calloc(1, mxbuff);

  show_panel(pan);
  mvwprintw(v_busq, 1, 1, "Indique parte de la descripción: ");
  wgetstr(v_busq,descr);
  for (i=0; i<strlen(descr); i++)
    descr[i] = toupper(descr[i]);

  /* Desde aqui comienza búsqueda en memoria */
  for (i=0, j=0; i<numbarras && j<maxitem_busqueda; i++) {
    strncpy(aux, barra[i].desc, maxdes-1);
    aux[mxbuff-1] = 0;
    for (k=0; k<strlen(aux); k++)
      aux[k] = toupper(aux[k]);
    limpiacad(aux, TRUE);
    if (strstr(aux, descr) != NULL) {
      item[j] = calloc(1, getmaxx(v_busq));
      cod[j] = calloc(1, maxcod+1);
      strcpy(cod[j], barra[i].codigo);

      g_snprintf(b, max_v_desc, "%s", barra[i].desc);
      if (strlen(barra[i].desc) >= max_v_desc) {
        b[max_v_desc] = 0;
      }
      else {
        while (strlen(b) < max_v_desc)
          b = g_strdup_printf("%s ", b);
      }

      sprintf(item[j], "%-15s %s %9.2f %.2f", cod[j], b, barra[i].pu, barra[i].exist);
      //      mvwprintw(v_busq, v_busq->j, 1, "%s", item[j]);
      j++;
    }
  }

  /* aqui termina código de búsqueda */

  num_items = j;
  if (num_items) {
    i = 0;
    noecho();
    keypad(v_busq, TRUE);
    muestra_renglon(v_busq, y, x, FALSE, i, num_items);
    while (!finished)
      {
        switch(ch = wgetch(v_busq)) {
        case KEY_UP:
          if (i <= 0) {
            beep();
            continue;
          }
          wattrset(v_busq, COLOR_PAIR(normal));
          mvwprintw(v_busq, v_busq->_cury, x, "%s", item[i--]);
          if (v_busq->_cury-y == 0) {
            wscrl(v_busq, -1);
            wattrset(v_busq, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_busq, y, x, "%s", item[i]);
          }
          else {
            wattrset(v_busq, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_busq, v_busq->_cury-1, x, "%s", item[i]);
          }
          wrefresh(v_busq);
    
          break;
        case KEY_DOWN:
          if (i+1 >= num_items) {
            beep();
            continue;
          }
          wattrset(v_busq, COLOR_PAIR(normal));
          mvwprintw(v_busq, v_busq->_cury, x, "%s", item[i++]);
          if (v_busq->_cury == v_busq->_maxy) {
            wscrl(v_busq, +1);
            wattrset(v_busq, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_busq, v_busq->_maxy, x, "%s", item[i]);
          }
          else {
            wattrset(v_busq, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_busq, v_busq->_cury+1, x, "%s", item[i]);
        }        
          wrefresh(v_busq);
        break;
        case ENTER:
          finished = TRUE;
        }
      }

    strcpy(codigo, cod[i]);
    for (j=0; j<num_items; j++) {
      free(item[j]);
      free(cod[j]);
    }
  }
  else {
    wattrset(v_busq, COLOR_PAIR(inverso));
    mvwprintw(v_busq, v_busq->_maxy-1, 1, 
             "No se encontraron coincidencias. Presione una tecla...");
    wrefresh(v_busq);
    wgetch(v_busq);
  }
  wrefresh(v_busq);
  wclear(v_busq);
  wrefresh(v_busq);
  rmpanel(pan);
  free(aux);
  g_free(b);
  echo();
  return(num_items);
}

void muestra_renglon(WINDOW *v_arts, int y, int x, short borra_ven, unsigned renglon, unsigned num_items)
{
  int      i;

  wattrset(v_arts, COLOR_PAIR(normal));
  if (borra_ven)
    wclear(v_arts);
  for (i=0; i<num_items-1-renglon && i<v_arts->_maxy; i++)
    mvwprintw(v_arts, i+1+y, x, "%s", item[renglon+i+1]);

  wattrset(v_arts, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
  mvwprintw(v_arts, y, x, "%s", item[renglon]);
  refresh();
  wrefresh(v_arts);
}

int datos_renta(int *num_cliente, time_t *f_pedido, time_t *f_entrega) {

  WINDOW *v, *v_form;
  FORM   *forma;
  FIELD *c[15];
  PANEL  *pan = 0;

  int campo_cliente=2;
  int campo_e_dia=4;
  int campo_e_mes=5;
  int campo_e_anio=6;
  int campo_e_hora=8;
  int campo_e_min=9;

  int vx, vy; /* Dimensiones horizontal y vertical de la ventana */
  int max_v_desc;

  int ch, finished = 0;
  char nm_cliente[mxbuff];
  char b[255];
  struct tm *fecha;
  time_t tiempo;

  tiempo = time(NULL);

  vx = 60;
  vy = 15;
  max_v_desc = vx - maxcod - 3;
  pan = mkpanel(COLOR_BLACK, vy, vx, (getmaxy(stdscr)-vy)/2, (getmaxx(stdscr)-vx)/2);
  set_panel_userptr(pan, "pan");

  v = panel_window(pan);
  box(v, 0, 0);

  /* Forma */
  strcpy(b, "Introduzca los siguientes datos");
  c[0] = CreaEtiqueta(2, 30, b);
  c[1] = CreaEtiqueta(4, 0, "Num. cliente");
  c[campo_cliente] = CreaCampo(4, 13, 1, 8, amarillo_sobre_azul);

  c[3] = CreaEtiqueta(5, 0, "F. entrega");
  c[campo_e_dia] = CreaCampo(5, 13, 1, 2, amarillo_sobre_azul);
  c[10] = CreaEtiqueta(5, 15, "/");
  c[campo_e_mes] = CreaCampo(5, 16, 1, 2, amarillo_sobre_azul);
  c[11] = CreaEtiqueta(5, 18, "/");
  c[campo_e_anio] = CreaCampo(5, 19, 1, 2, amarillo_sobre_azul);


  c[7] = CreaEtiqueta(6, 0, "H. entrega");
  c[campo_e_hora] = CreaCampo(6, 13, 1, 2, amarillo_sobre_azul);
  c[12] = CreaEtiqueta(5, 15, ":");
  c[campo_e_min] = CreaCampo(6, 17, 1, 2, amarillo_sobre_azul);
  c[13] = (FIELD *)0;

  //  set_field_type(c[campo_e_dia], TYPE_NUMERIC, 2, 1, 31);
  //  set_field_type(c[campo_e_mes], TYPE_NUMERIC, 0, 1, 12);
  //  set_field_type(c[campo_e_hora], TYPE_NUMERIC, 0, 1, 23);
  //  set_field_type(c[campo_e_min], TYPE_NUMERIC, 0, 1, 59);

  forma = new_form(c);

  /* Calcula y coloca la etiqueta a la mitad de la forma */
  scale_form(forma, &vy, &vx);
  c[0]->fcol = (unsigned) ((vx - strlen(b)) / 2);

  //igm  MuestraForma(forma, vy, vx);
  //igm  v_form = form_win(forma);

    /*igm*/ finished = TRUE;
  while (!finished)
  {
    switch(form_driver(forma, ch = form_virtualize(v_form, TRUE, 0))) {
    case E_OK:
      break;
    case E_UNKNOWN_COMMAND:
      /*      if (ch == CTRL('B')) {
        strcpy(cliente.rfc,campo[1]->buf);
        if (!BuscaCliente(cliente.rfc, &cliente, con)) {
          set_field_buffer(campo[CAMPO_NOMBRE], 0, cliente.nombre);
          set_field_buffer(campo[4], 0, cliente.dom_calle);
          set_field_buffer(campo[6], 0, cliente.dom_numero);
          set_field_buffer(campo[8], 0, cliente.dom_inter);
          set_field_buffer(campo[10], 0, cliente.dom_col);
          set_field_buffer(campo[12], 0, cliente.dom_ciudad);
          set_field_buffer(campo[14], 0, cliente.dom_edo);
          sprintf(scp, "%5u", cliente.cp);
          set_field_buffer(campo[16], 0, scp);

          set_field_buffer(campo[18], 0, cliente.curp);
        }
        else
          set_field_buffer(campo[CAMPO_NOMBRE], 0, "Nuevo registro");
      } 
      else */
        finished = my_form_driver(forma, ch);
      break;
    default:

        beep();
      break;
    }
  }

  // igm  BorraForma(forma);

  fecha = localtime(&tiempo);

  show_panel(pan);
  mvwprintw(v, 1, 1, "Número de cliente: ");
  wgetstr(v, b);
  *num_cliente = atoi(b);

  // busca_cliente(db_con, *num_cliente, nm_cliente);
  // mvwprintw(v, 1, 10, "%s", nm_cliente);

  mvwprintw(v, 2, 1, "Fecha de entrega");

  mvwprintw(v, 3, 1, "Día: ");
  wgetstr(v, b);
  fecha->tm_mday = atoi(b);

  mvwprintw(v, 4, 1, "Mes: ");
  wgetstr(v, b);
  fecha->tm_mon = atoi(b)-1;

  mvwprintw(v, 5, 1, "Año: ");
  wgetstr(v, b);
  fecha->tm_year = atoi(b);
  if (fecha->tm_year<2000)
    (fecha->tm_year)+=100;

  mvwprintw(v, 6, 1, "Horas: ");
  wgetstr(v, b);
  fecha->tm_hour = atoi(b);

  mvwprintw(v, 7, 1, "Minutos: ");
  wgetstr(v, b);
  fecha->tm_min = atoi(b);

  *f_entrega = mktime(fecha);
  *f_pedido = time(NULL);

  wrefresh(v);
  wclear(v);
  wrefresh(v);
  rmpanel(pan);
  echo();
  return(OK);
}

/********************************************************/   

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

int busca_art_marcado(char *cod, struct articulos art[maxarts], int campo, int ren) {
  /* Valores posibles de campo:
     0: Búsqueda en art.codigo
     1: Búsqueda en art.serie
  */

  int i;
  int no_salir=1;

  for (i=0; no_salir && i<ren; i++)
    no_salir = campo==0 ? strcmp(cod, articulo[i].codigo) : strcmp(cod, articulo[i].serie);

  if (!no_salir)
    return(i);
  else 
    return(-1);

}

/********************************************************/   

int imprime_fechas_renta(PGconn *con, PGconn *con_s, time_t *f_pedido, time_t *f_entrega) {
  FILE *imp;
  struct tm *fecha_p, *fecha_e;


  if (!(imp = popen(cmd_lp, "w"))) {
    fprintf(stderr, "Error: No puedo imprimir. %s\n", cmd_lp);
    return(PROCESS_ERROR);
  }

  fecha_p = localtime(f_pedido);
  fecha_e = localtime(f_entrega);
  fprintf(imp, "Fecha de entrega: %d/%d/%d. Hora: %d:%d.\n", 
          fecha_e->tm_mday, fecha_e->tm_mon, fecha_e->tm_year+1900,
          fecha_e->tm_hour, fecha_e->tm_min);
  fprintf(imp, "Fecha de pedido: %d/%d/%d.\n",
          fecha_p->tm_mday, fecha_p->tm_mon, fecha_p->tm_year+1900);

  pclose(imp);
  return(OK);
}

/********************************************************/   

int selecciona_caja_virtual(PGconn *con, int num_cajas)
{
  WINDOW *v_selecc;
  PANEL  *pan = 0;

  int vx, vy; /* Dimensiones horizontal y vertical de la ventana */
  int i,j;
  int x, y;
  int ch, finished = 0;
  char *aux;

  vx = 70;
  vy = 15;
  y = 2;  /* Coordenadas de la primera línea de menu de selección */
  x = 1;

  pan = mkpanel(COLOR_BLACK, vy, vx, (getmaxy(stdscr)-vy)/2, (getmaxx(stdscr)-vx)/2);
  set_panel_userptr(pan, "pan");

  v_selecc = panel_window(pan);


  scrollok(v_selecc, TRUE);
  //  box(v_selecc, 0, 0);

  aux = calloc(1, mxbuff);

  show_panel(pan);
  mvwprintw(v_selecc, 1, 1, "Seleccione la caja virtual para operar:");
  wrefresh(v_selecc);
 
  for (j=0; j<num_cajas; j++) {
    item[j] = calloc(1, getmaxx(v_selecc));
    /* Aqui debe ir la consulta al catálogo de nombres de cajas virtuales */
    sprintf(item[j], "Caja %d", j+1);
    mvwprintw(v_selecc, v_selecc->_cury+1, 1, "%s", item[j]);
  }

  box(v_selecc, 0, 0);
  wrefresh(v_selecc);

  if (num_cajas) {
    i = 0;
    noecho();
    keypad(v_selecc, TRUE);
    muestra_renglon(v_selecc, y, x, FALSE, i, num_cajas);
    while (!finished)
      {
        switch(ch = wgetch(v_selecc)) {
        case KEY_UP:
          if (i <= 0) {
            beep();
            continue;
          }
          wattrset(v_selecc, COLOR_PAIR(normal));
          mvwprintw(v_selecc, v_selecc->_cury, x, "%s", item[i--]);
          if (v_selecc->_cury-y == 0) {
            wscrl(v_selecc, -1);
            wattrset(v_selecc, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_selecc, y, x, "%s", item[i]);
          }
          else {
            wattrset(v_selecc, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_selecc, v_selecc->_cury-1, x, "%s", item[i]);
          }
          wrefresh(v_selecc);
    
          break;
        case KEY_DOWN:
          if (i+1 >= num_cajas) {
            beep();
            continue;
          }
          wattrset(v_selecc, COLOR_PAIR(normal));
          mvwprintw(v_selecc, v_selecc->_cury, x, "%s", item[i++]);
          if (v_selecc->_cury == v_selecc->_maxy) {
            wscrl(v_selecc, +1);
            wattrset(v_selecc, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_selecc, v_selecc->_maxy, x, "%s", item[i]);
          }
          else {
            wattrset(v_selecc, COLOR_PAIR(amarillo_sobre_azul) | A_BOLD);
            mvwprintw(v_selecc, v_selecc->_cury+1, x, "%s", item[i]);
        }        
          wrefresh(v_selecc);
        break;
        case ENTER:
          finished = TRUE;
        }
      }


    for (j=0; j<num_cajas; j++) {
      free(item[j]);
    }
  }
  else {
  }
  wrefresh(v_selecc);
  wclear(v_selecc);
  wrefresh(v_selecc);
  rmpanel(pan);
  free(aux);
  echo();
  return(i+1);
}
/********************************************************/   

int read_db_config(PGconn *con)
{
  char *buf;


  buf = lee_config(con, "ALMACEN");
  if (buf != NULL)
    almacen = atoi(buf);




}

/********************************************************/   

void read_pos_config(PGconn *con, unsigned num_caja)
{
  /* Valores aceptados de num_caja
     0:  No existen cajas virtuales se lee configuración general
     >0: Representa el número de caja virtual
  */
  lp_disp_ticket = g_strdup_printf(lee_config_pos(con, num_caja, "COLA_TICKET"));
  strcpy(tipo_disp_ticket, lee_config_pos(con, num_caja, "MINIIMPRESORA_TIPO"));
  strcpy(cmd_lp, lee_config_pos(con, num_caja, "IMPRESION_COMANDO"));
  almacen = atoi(lee_config_pos(con, num_caja, "ALMACEN"));
  /*lee_config_pos(con, num_caja, "MINIIMPRESORA_AUTOCUTTER");
  lee_config_pos(con, num_caja, "ALMACEN");
  lee_config_pos(con, num_caja, "SCANNER_PUERTOSERIE");
  lee_config_pos(con, num_caja, "DIVISA");
  lee_config_pos(con, num_caja, "LISTAR_PRECIO_NETO");
  lee_config_pos(con, num_caja, "DESGLOSAR_DESCUENTO");
  lee_config_pos(con, num_caja, "CAPTURAR_SERIE");
  lee_config_pos(con, num_caja, "CONSULTA_CATALOGO");
  lee_config_pos(con, num_caja, "MODO_RENTA");
  lee_config_pos(con, num_caja, "REGISTRAR_VENDEDOR");*/

}

/********************************************************/   

int main(int argc, char *argv[]) {
  static char buffer, buf[255];
  static char encabezado1[mxbuff],
      encabezado2[mxbuff] = "Eduardo I. Osorio H., 1999-2006 soporte en elpuntodeventa.com";
  FILE *impr_cmd;
  time_t tiempo;        
  static int dgar;
  PGconn *con, *con_s;
  long num_venta = 0;
  long folio = 0;
  unsigned formadepago;
  double utilidad;
  double tax[maxtax];
  double importe_recibido, cambio;
  struct tm *fecha;     /* Hora y fecha local   */
  int i = 0;
  time_t f_entrega, f_pedido; /* Para control de rentas */
  int id_cliente = 0;
  int num_cajas_virtuales = 0;
  struct usuario usr;
  char *login, *pass;

  login = calloc(1,mxbuff);
  pass = calloc(1,mxbuff);

  es_remision = strstr(argv[0], "remision") == NULL ? FALSE : TRUE;
  buf[0] = 0;

  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fecha->tm_year += 1900;
  pid = getpid();

  init_config();
  read_general_config();
  read_global_config();
  read_config();

  for (i=1; i<argc; i++) {
    if (!strcmp(argv[i], "-h")) {
      db.hostname =  g_strdup_printf("%s", argv[i+1]);
      i++;
    }
    else if (!strcmp(argv[i], "-p")) {
      db.hostport =  g_strdup_printf("%s", argv[i+1]);
      i++;
    }
    else if (!strcmp(argv[i], "-m")) {
      if(!strcmp(argv[i+1], "carrito"))
        es_remision = FALSE;
      i++;
    }
    else
      db.name = g_strdup_printf("%s", argv[i]);
  }    
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

  if (i == ERROR_SQL) {
    aborta("Error al consultar usuarios\n", ERROR_DIVERSO);
  }
  else if (i == FALSE) {
    aborta("Nombre de usuario/contraseña incorrecto\n", ERROR_DIVERSO);
  }

  /* Proximamente desapareceremos esta llamada */
  con = Abre_Base(db.hostname, db.hostport, NULL, NULL, db.name, db.sup_user, db.sup_passwd); 
  if (con == NULL) {
    aborta("FATAL: Problemas al accesar la base de datos. Pulse una tecla para abortar...",
            ERROR_SQL);
  }

  read_db_config(con_s);

  if ((num_cajas_virtuales = atoi(lee_config(con_s, "CAJAS_VIRTUALES"))) > 0)
    caja_virtual = selecciona_caja_virtual(con_s, num_cajas_virtuales);
  else
    caja_virtual = 0;
  read_pos_config(con_s, caja_virtual);

  //  num_venta = obten_num_venta(nm_num_venta);
  num_venta = obten_num_venta(con_s);

  if (!serial_num_enable && !catalog_search) {
    numbarras = lee_articulos(con_s, con_s, almacen);
    if (!numbarras) {
      mensaje("No hay artículos en la base de datos. Use el programa de inventarios primero");
      getch();
    }
    else if (numbarras == SQL_ERROR) {
      mensaje("Error al leer base de datos, búsqueda de artículos deshabilitada.");
      getch();
      numbarras = 0;
    }
  }

  numdivisas = lee_divisas(con_s);
  if (!numdivisas) {
    mensaje("No hay divisas en la base de datos. Use el programa de inventarios primero");
    getch();
  }
  else if (numdivisas == SQL_ERROR) {
    mensaje("Error al leer base de datos, búsqueda de divisas deshabilitada.");
    getch();
    numbarras = 0;
  }

  sprintf(encabezado1, "OsoPOS - Módulo POS %s R.%s. Caja ", vers, release);
  if (num_cajas_virtuales==0)
    sprintf(encabezado1, "%s ÚNICA", encabezado1);
  else
    sprintf(encabezado1, "%s %u", encabezado1, caja_virtual);

  //  /*igm*/ sprintf(encabezado1, "Host: %s | D. datos: %s", db.hostname, db.name);
  do {
    tiempo = time(NULL);
    fecha = localtime(&tiempo);
    fecha->tm_year += 1900;
    for (i=0; i<maxtax; i++)
      tax[i] = 0.0;
    importe_recibido = 0.0;
    cambio = 0.0;

    clear();
    mvprintw(0,0,"%u/%u/%u",
               fecha->tm_mday, (fecha->tm_mon)+1, fecha->tm_year);
    attrset(COLOR_PAIR(cyan_sobre_negro));
    mvprintw(0,(getmaxx(stdscr)-strlen(encabezado1))/2,
             "%s",encabezado1);
    mvprintw(1,(getmaxx(stdscr)-strlen(encabezado2))/2,
             "%s",encabezado2);
    attrset(COLOR_PAIR(blanco_sobre_negro));
    a_pagar = item_capture(con_s, &numarts, &utilidad, tax, *fecha, argv[0]);


    if (numarts && a_pagar) {
      attron(A_BOLD);
      if (es_remision) {
        if (lease_mode)
          mvprintw(getmaxy(stdscr)-2,0,"¿Expedir Nota de venta, Factura, Ticket, nota de Renta (N,F,T,R)? T\b");
        else
          mvprintw(getmaxy(stdscr)-2,0,"¿Expedir Nota de venta, Factura, Ticket (N,F,T)? T\b");
      } 
      else
        mvprintw(getmaxy(stdscr)-2,0,"¿Carro virtual ó inventario Físico (C,F)? C\b");

      attroff(A_BOLD);
      buffer = toupper(getch());
      if (cnf_registrar_vendedor)
        id_vendedor = captura_vendedor();
      if (es_remision)
        formadepago = forma_de_pago(&importe_recibido, &cambio);

      switch (buffer) {
        case 'N':
          attron(A_BOLD);
          if (cnf_impresion_garantia) {
            mvprintw(getmaxy(stdscr)-1,0,"¿Días de garantía? : ");
            attroff(A_BOLD);
            clrtoeol();
            scanw("%d",&dgar);
          }
          num_venta = sale_register(con_s, con_s, "ventas", a_pagar, iva, tax, utilidad, formadepago,
                         _NOTA_MOSTRADOR, serie, &folio, *fecha, id_teller, 0, articulo, numarts, almacen);
          if (num_venta<0)
            aborta_remision(con_s, con_s, "ERROR AL REGISTRAR VENTA, PRESIONE \'A\' PARA ABORTAR...", 'A',
                            (int)num_venta);
            
          if (num_venta>0)
            clean_journal(nm_journal);
          sprintf(buf,"%s %ld %d",nmimprrem, num_venta, dgar);
          impr_cmd = popen(buf, "w");
          if (pclose(impr_cmd) != 0)
            fprintf(stderr, "Error al ejecutar %s\n", buf);
          if (formadepago >= 20) {
            AbreCajon(tipo_disp_ticket);
            sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
            impr_cmd = popen(buf, "w");
            pclose(impr_cmd);
          }
          mensaje("Aprieta una tecla para continuar (t para terminar)...");
          buffer = toupper(getch());
        break;
      case 'F':
        if (formadepago >= 20) {
          AbreCajon(tipo_disp_ticket);
          sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
        }
        num_venta = sale_register(con_s, con_s, "ventas", a_pagar, iva, tax, utilidad, formadepago,
                       _FACTURA, serie, &folio, *fecha, id_teller, 0, articulo, numarts, almacen);
        if (num_venta<0)
          aborta_remision(con_s, con_s, "ERROR AL REGISTRAR VENTA, PRESIONE \'A\' PARA ABORTAR...", 'A',
                          (int)num_venta);
            
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

      case 'R':
        if (lease_mode) {
          datos_renta(&id_cliente, &f_pedido, &f_entrega);
          registra_renta(con_s, con_s, "rentas", id_cliente, f_pedido, f_entrega, 
                         articulo, numarts);

          imprime_fechas_renta(con_s, con_s, &f_pedido, &f_entrega);
          print_customer_data(con_s, con_s, id_cliente);
          print_ticket_arts(importe_recibido, cambio);
          sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
          unlink(nm_disp_ticket);
          if (formadepago >= 20) {
            AbreCajon(tipo_disp_ticket);
            sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
            impr_cmd = popen(buf, "w");
            pclose(impr_cmd);
            unlink(nm_disp_ticket);
          }
          print_ticket_footer(*fecha, num_venta, folio, serie);

          if (num_venta>0)
            clean_journal(nm_journal);
          sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
          unlink(nm_disp_ticket);

          corta_papel(tipo_disp_ticket);
          sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
          unlink(nm_disp_ticket);

          mensaje("Corta el papel y aprieta una tecla para continuar (t para terminar)...");
          buffer = toupper(getch());
          print_ticket_header(nmfenc);
          sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
        }
        break;
      case 'T':
      default:
        // lee_garantia(con, articulo, numarts);
        print_ticket_arts(importe_recibido, cambio);
        sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        unlink(nm_disp_ticket);
        if (formadepago >= 20) {
          AbreCajon(tipo_disp_ticket);
          sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          impr_cmd = popen(buf, "w");
          pclose(impr_cmd);
          unlink(nm_disp_ticket);
        }
        num_venta = sale_register(con_s, con_s, "ventas", a_pagar, iva, tax, utilidad,
                                   formadepago, _TEMPORAL, serie, &folio, *fecha,
                                   id_teller, id_vendedor, articulo, numarts, almacen);
        if (num_venta<0)
          aborta_remision(con_s, con_s,
                          "ERROR AL REGISTRAR VENTA, PRESIONE \'A\' PARA ABORTAR...", 'A',
                          (int)num_venta);

        print_ticket_footer(*fecha, num_venta, folio, serie);
        if (num_venta>0)
          clean_journal(nm_journal);
        sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        unlink(nm_disp_ticket);

        corta_papel(tipo_disp_ticket);
        sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        unlink(nm_disp_ticket);

        mensaje("Corta el papel y aprieta una tecla para continuar (t para terminar)...");
        buffer = toupper(getch());
        print_ticket_header(nmfenc);
        sprintf(buf, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
        impr_cmd = popen(buf, "w");
        pclose(impr_cmd);
        break;
      }
      if (es_remision && actualiza_existencia(con_s, fecha)) {
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

  g_free(db.name);
  g_free(db.user);
  g_free(db.passwd);
  g_free(db.sup_user);
  g_free(db.sup_passwd);
  g_free(db.hostport);
  g_free(db.hostname);

  /*if (cc_avisos != NULL)
    free(cc_avisos);*/
  free(asunto_avisos);
  asunto_avisos = NULL;
  free(tipo_disp_ticket);
  tipo_disp_ticket = NULL;
  g_free(lp_disp_ticket);
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

- Optimizar la funcion busca_precio para que solo devuelva el indice del código buscado y
  posteriormente se haga una copia de registro a registro

  */
