/* imprem.c version 0.12 */
/* gcc imprrem.c -lcurses -o ~/bin/imprrem */

/* IMPORTANTE: Revisar compatibilidad con el 2000 en hardware viejo */
/* NO se encontró problemas con el 2000 en sistemas Linux Red Hat 5.1
   porque el sistema devuelve el año como el número de años transcurridos
   desde 1900 a la fecha y esto NO REPRESENTA los últimos dos digitos del
   año */ 

#include <stdio.h>
#include <time.h> 
#include "include/pos-func.h" 

/*#define maxitem 5*/	/* Número máximo de artículos diferentes en */
			/* remision */
#define vers "0.14-2"

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
	tipoimp[maxbuf];	/* Tipo de impresora */
int numarts, dgaran=-1;
short unsigned maxitem;
short iva_incluido;

int LeeConfig();
int CalculaIVA();
void ImprimeRemision(int numart);


int LeeConfig(char *nmdatos) {
  char       nmconfig[] = "imprem.config";
  FILE       *config;
  char       buff[maxbuf],buf[maxbuf];
  char       *b;
  static int i;

  iva_incluido = 1;
  strcpy(nmimpre,"/dev/lp1");
  strcpy(nmdatos,"/home/OsoPOS/caja/venta.ultima");
  strcpy(tipoimp,"EPSON");
  maxitem = 20;

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

	/* Busca parámetros de impresora y archivo de última venta */
      if (!strcmp(b,"impresora")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(nmimpre,buf);
      }
      else if (!strcmp(b,"impresora.tipo")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(tipoimp,buf);
        for (i=0; i<strlen(tipoimp); i++)
          tipoimp[i] = toupper(tipoimp[i]);
      }
      else if (!strcmp(b,"datos")) {
        strcpy(buf, strtok(NULL,"="));
        free(nmdatos);
        nmdatos = NULL;
        nmdatos = calloc(1, strlen(buf)+1);
        strcpy(nmdatos,buf);
      }
      else if (!strcmp(b,"iva.incluido")) {
        strcpy(buf, strtok(NULL,"="));
        iva_incluido = (buf[0] == 'S');
      }
      else if (!strcmp(b,"maxitem")) {
	strcpy(buf, strtok(NULL,"="));
	maxitem = (unsigned) atoi(buf);
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
    if (!iva_incluido)
      art[i].pu =  art[i].pu / (iva_porcentaje+1);
    sumatoria += (art[i].pu * art[i].cant);
  }
  subtotal = sumatoria;
  iva = subtotal * iva_porcentaje * iva_incluido;
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

  impresora = fopen(nmimpre,"a");
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
  for (; renglon<maxrenglon-2; renglon++) {
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

  if (iva_incluido) {
    fprintf(impresora, "%8.2f\n\r", subtotal);
    fprintf(impresora, "%47s %8.2f\n\r", "", iva);
  }
  else
    fprintf(impresora, "\n\n\r");

  avance_vert(impresora,15); /* 15/216" entre renglones */
  fprintf(impresora,"      %45s%8.2f\r\n","",total);
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


int main(int argc, char *argv[]) {

  char *nmdatos;

  nmdatos = calloc(1, strlen("/home/OsoPOS/caja/venta.ultima")+1);
  LeeConfig(nmdatos);
  numarts = LeeVenta(nmdatos,art);
  if (argc>1)
    dgaran = atoi(argv[1]);

  if (numarts) {
    CalculaIVA();
    ImprimeRemision(numarts);
  }
  free(nmdatos);
  return (OK);
}
