/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa ImprRem 0.15 (C) 1999-2001 E. Israel Osorio H.
   desarrollo@punto-deventa.com
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

/*#define maxitem 5*/	/* Número máximo de artículos diferentes en */
			/* remision */
#define vers "0.15-1"

#define maxrenglon 8
#define maxdes 39
#define maxbuf 130
#define ESC 27
#define FF 12
#define SI 15

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

char buff1[maxbuf],buff2[maxbuf];
struct articulos art[maxart];
char nmimpre[maxbuf],
  tipoimp[maxbuf],	/* Tipo de impresora */
  lp_printer[maxbuf];
int numarts, dgaran=-1;
short unsigned maxitem;
short iva_incluido;
short desglose_iva;
short unsigned ajuste_microv, ajuste_v, ajuste_h;
char *home_directory;

int read_config();
int CalculaIVA();
void ImprimeRemision(int numart);
int imprime(int numart);

int read_config() {
  char       *nmconfig;
  FILE       *config;
  char       buff[maxbuf],buf[maxbuf];
  char       *b;
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
  strcpy(tipoimp,"EPSON");
  strncpy(lp_printer, "facturas", maxbuf);
  maxitem = 20;

  ajuste_v = 0;
  ajuste_microv = 0;
  ajuste_h = 0;

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
        strncpy(buf, strtok(NULL,"="), maxbuf);
        strncpy(lp_printer,buf, maxbuf);
      }
      else if (!strcmp(b,"impresora.tipo")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(tipoimp,buf);
        for (i=0; i<strlen(tipoimp); i++)
          tipoimp[i] = toupper(tipoimp[i]);
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

void avance_vert(FILE *disp, unsigned avance)
{
  fprintf(disp, "%c%c%c", ESC, 'J', avance);

}

void ImprimeRemision(int numart) {
  FILE *impresora;
  static int i=0, renglon;
  static int ib;
  time_t tiempo;
  struct tm *fecha;

  impresora = fopen(nmimpre,"w");
  if (!impresora) {
    fprintf(stderr,
	"ERROR: No se puede escribir al archivo %s\n",nmimpre);
    fprintf(stderr,"Quiza no se cuenta con los permisos necesarios\n");
    exit(10);
  }
  fprintf(impresora,"%c",18);
  /* Cancela modo condensado */
  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fecha->tm_year += 1900;

       /* Imprime encabezado */
  fprintf(impresora,"\n\n\n\n\n\n");
  fprintf(impresora,"%3d   %3d  %5d",fecha->tm_mday,
          fecha->tm_mon +1 ,fecha->tm_year);
  fprintf(impresora,"\r\n\n");
  fprintf(impresora,"%c%c%cM",ESC,15,ESC); /* 17cpi */
  fprintf(impresora,"\n\n\n\n\n%c%c%c",ESC,'J',890);
  /* 890/216" de avance vertical */

  /* Imprime articulos */
  for (i=0,renglon=0; i<numart; i++,renglon++) {
    fprintf(impresora,"%3d     %-39s    %8.2f\n\r",
	art[i].cant,art[i].desc,art[i].cant*art[i].pu);
    if (renglon<maxitem)
      avance_vert(impresora,15); /* 15/216" entre renglones */
  }

  /* Imprime renglones vacios faltantes */
  for (; renglon<maxitem; renglon++) {
     /* 2 renglones por el Subtotal y el IVA, */
    fprintf(impresora,"\n\r");
    if (renglon<maxitem)
      avance_vert(impresora,15); /* 15/216" entre renglones */
  }

     /* Imprime subtotal, IVA y total */
  avance_vert(impresora,3); /* 3/216" de avance */

  if (dgaran < 0)
    sprintf(buff1,"(     dias de garantia)");
  else
    sprintf(buff1,"( %3d dias de garantia)",dgaran);

  fprintf(impresora, "        %-42s",buff1);

  if (desglose_iva) {
    fprintf(impresora, "%8.2f\n\r", subtotal);
    fprintf(impresora, "%49s %8.2f\n\r", "", iva);
  }
  else
    fprintf(impresora, "\n\n\r");

  avance_vert(impresora,15); /* 15/216" entre renglones */
  fprintf(impresora,"     %45s%8.2f\r\n","",total);
  avance_vert(impresora,15);

  strncpy(buff1,str_cant(total, &ib),sizeof(buff1));

  /* Importe con letra */
  fprintf(impresora,"%16s%spesos\r\n%16s"," ",buff1," ");
  if (ib>9)
    fprintf(impresora,"%2d/100 M.N.\r\n",ib);
  else	/* Asegura que los centavos se impriman a dos digitos */
    fprintf(impresora,"0%1d/100 M.N.\r\n",ib);

  fprintf(impresora,"%c",FF);
  fclose(impresora);
}


int imprime(int numart) {
  FILE *impresora;
  static int i=0, renglon;
  static int ib;
  time_t tiempo;
  struct tm *fecha;
  char str_buff[maxbuf];

  impresora = fopen(nmimpre,"w");
  if (!impresora) {
    fprintf(stderr,
	"ERROR: No se puede escribir al archivo %s\n",nmimpre);
    fprintf(stderr,"Quiza no se cuenta con los permisos necesarios\n");
    exit(10);
  }
  fprintf(impresora,"%c",18);
  /* Cancela modo condensado */
  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fecha->tm_year += 1900;

  for (i=0; i<ajuste_v; i++)
    fprintf(impresora, "\n");

  avance_vert(impresora, ajuste_microv);

   /* Imprime encabezado */
  fprintf(impresora,"\n\n");
  Espacios(impresora, ajuste_h);
  fprintf(impresora,"%2d  %3d  %4d",fecha->tm_mday,
          fecha->tm_mon +1 ,fecha->tm_year);
  fprintf(impresora,"\r\n\n");
  fprintf(impresora,"%c%c%cM",ESC,15,ESC); /* 17cpi */
  fprintf(impresora,"\n\n\n\n\n%c%c%c",ESC,'J',890);
  /* 890/216" de avance vertical */

  /* Imprime articulos */
  for (i=0,renglon=0; i<numart; i++,renglon++) {
    Espacios(impresora, ajuste_h);
    fprintf(impresora,"%2d     %-39s    %8.2f\n\r",
	art[i].cant,art[i].desc,art[i].cant*art[i].pu);
    if (renglon<maxitem)
      avance_vert(impresora,15); /* 15/216" entre renglones */
  }

  /* Imprime renglones vacios faltantes */
  for (; renglon<maxitem+(desglose_iva*2); renglon++) {
     /* 2 renglones por el Subtotal y el IVA, */
    fprintf(impresora,"\n\r");
    if (renglon<maxitem)
      avance_vert(impresora,15); /* 15/216" entre renglones */
  }

     /* Imprime subtotal, IVA y total */
  avance_vert(impresora,3); /* 3/216" de avance */

  if (dgaran < 0)
    sprintf(buff1,"(     dias de garantia)");
  else
    sprintf(buff1,"( %3d dias de garantia)",dgaran);

  Espacios(impresora, ajuste_h);
  fprintf(impresora, "      %-46s",buff1);

  if (desglose_iva) {
    fprintf(impresora, "%6.2f\n\r", subtotal);
    Espacios(impresora, ajuste_h);
    fprintf(impresora, "%46s %6.2f\n\r", "", iva);
    avance_vert(impresora, 20); /* 20/216" entre renglones */
    Espacios(impresora, 51);
  }

  fprintf(impresora,"%6.2f\r\n", total);
  avance_vert(impresora,15);

  strncpy(buff1,str_cant(total, &ib),sizeof(buff1));

  /* Importe con letra */
  Espacios(impresora, ajuste_h);
  Espacios(impresora, 15);
  fprintf(impresora,"%spesos\r\n", buff1);
  Espacios(impresora, ajuste_h);
  Espacios(impresora, 15);
  if (ib>9)
    fprintf(impresora,"%2d/100 M.N.\r\n",ib);
  else	/* Asegura que los centavos se impriman a dos digitos */
    fprintf(impresora,"0%1d/100 M.N.\r\n",ib);

  fprintf(impresora,"%c",FF);
  fclose(impresora);
  sprintf(str_buff, "lpr -Fb -P %s %s", lp_printer, nmimpre);
  impresora = popen(str_buff, "w");
  i = pclose(impresora);
  sprintf(str_buff, "rm -rf %s", nmimpre);
  impresora = popen (str_buff, "r");
  pclose(impresora);
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
    //    ImprimeRemision(numarts);
    imprime(numarts);
    PQfinish(conn);
    return (OK);
  }
  else {
    PQfinish(conn);
    return (numarts);
  }
}
