#include <stdio.h>
#include <string.h>

#ifndef _printfunc
#include "print-func.h"
#define _printfunc
#endif

#ifndef ESC
#define ESC 27
#endif

void imprime_razon_social(FILE *disp, char *miniprinter_type, char *s1, char *s2)
{
  if (strstr(miniprinter_type, "EPSON")) {
    fprintf(disp, "%ca%c%c!%c%s\n%c!%c", ESC, 1, ESC, 48, s1, ESC, 0);
    fprintf(disp, "%s\n%c!%c%ca%c", s2, ESC, 1, ESC, 0);
  }
  else {
    fprintf(disp,"%c%c%c%s\n", ESC,'P',14, s1);
    fprintf(disp,"%c%c%c%s\n", 15,ESC,'M', s2);
  }

}

#ifdef _IMPRREM

int imprime(char *nmimpre, struct articulos art[maxart], int numart) {
  extern unsigned maxitem;
  extern unsigned ajuste_microv, ajuste_v, ajuste_h;
  extern unsigned desglose_iva;
  extern char *lp_printer;
  extern short unsigned price_pos[3];
  extern unsigned total_price_pos, desc_pos, desc_len, qt_pos, code_pos;
  extern short unsigned date_pos;
  extern short article_pitch;
  extern short date_pitch;
  extern char *prt_type;

  FILE *impresora;
  static int i=0, j, renglon;
  static int ib;
  time_t tiempo;
  struct tm *fecha;
  char str_buff[mxbuff], buff1[mxbuff];
  unsigned mode;

  if (prt_type == "IBM")
	mode = 1;
  else
	mode = 0;

  impresora = fopen(nmimpre,"w");
  if (!impresora) {
    fprintf(stderr,
	"ERROR: No se puede escribir al archivo %s\n",nmimpre);
    fprintf(stderr,"Quiza no se cuenta con los permisos necesarios\n");
    exit(10);
  }
  select_pitch(impresora, date_pitch, mode);

  tiempo = time(NULL);
  fecha = localtime(&tiempo);
  fecha->tm_year += 1900;

  feed_paper(impresora, ajuste_v, mode);
  avance_vert(impresora, ajuste_microv, mode);

   /* Imprime encabezado */
  Espacios(impresora, ajuste_h);
  Espacios(impresora, date_pos);
  fprintf(impresora,"%2d  %3d  %4d",fecha->tm_mday,
          fecha->tm_mon +1 ,fecha->tm_year);
  fprintf(impresora,"\r\n\n");

  select_pitch(impresora, article_pitch, mode);
  fprintf(impresora,"\n\n\n\n\n");
  avance_vert(impresora, 890, mode);
  /* 890/216" de avance vertical */

  /* Imprime articulos */
  for (i=0,renglon=0; i<numart; i++,renglon++) {
	/*	for (j=0; j<mxbuff-1; j++)
		buff1[j] = ' '; */
	memset(buff1, ' ', mxbuff-1);

	Espacios(impresora, ajuste_h);

	sprintf(str_buff, "%2d ", art[i].cant);
	memcpy(&buff1[qt_pos], str_buff, strlen(str_buff));

	if (code_pos)
	  memcpy(&buff1[code_pos], art[i].codigo, strlen(art[i].codigo));

	j = strlen(art[i].desc) > mxbuff ? desc_len : strlen(art[i].desc);
	memcpy(&buff1[desc_pos], art[i].desc, j);

	if (price_pos[0]) {
	  sprintf(str_buff, "%8.2f", art[i].pu);
	  memcpy(&buff1[price_pos[0]], str_buff, strlen(str_buff));
	}

	if (price_pos[1]) {
	  sprintf(str_buff, "%8.2f", art[i].pu*art[i].cant);
	  memcpy(&buff1[price_pos[1]], str_buff, strlen(str_buff));
	  buff1[price_pos[1]+strlen(str_buff)] = 0;
	}

	fprintf(impresora, "%s\n", buff1);
  }

  /* Imprime renglones vacios faltantes */
     /* desglose_iva*2  a causa del Subtotal y el IVA, */
  feed_paper(impresora, maxitem+(desglose_iva*2)-renglon, mode);

  /*  for (; renglon<maxitem+(desglose_iva*2); renglon++) {
    fprintf(impresora,"\n\r");
	}*/

  /* Importe con letra */
  strncpy(buff1,str_cant(total, &ib),sizeof(buff1));

  Espacios(impresora, ajuste_h);
  Espacios(impresora, 15);
  fprintf(impresora,"%spesos\r\n", buff1);
  Espacios(impresora, ajuste_h);
  Espacios(impresora, 15);
  if (ib>9)
    fprintf(impresora,"%2d/100 M.N.\r\n",ib);
  else	/* Asegura que los centavos se impriman a dos digitos */
    fprintf(impresora,"0%1d/100 M.N.\r\n",ib);


     /* Imprime subtotal, IVA y total */
  avance_vert(impresora, 3, mode); /* 3/216" de avance */

  Espacios(impresora, ajuste_h);
  Espacios(impresora, total_price_pos);
  if (desglose_iva) {
    fprintf(impresora, "%6.2f\n\r", subtotal);
    Espacios(impresora, ajuste_h);
	Espacios(impresora, total_price_pos);
    fprintf(impresora, " %6.2f\n\r", iva);
    Espacios(impresora, total_price_pos);
  }

  fprintf(impresora,"%6.2f\r\n", total);
  avance_vert(impresora, 15, mode);


  fprintf(impresora,"%c",FF);
  fclose(impresora);
  sprintf(str_buff, "lpr -Fb -P %s %s", lp_printer, nmimpre);
  impresora = popen(str_buff, "r");
  i = pclose(impresora);
  sprintf(str_buff, "rm -rf %s", nmimpre);
  impresora = popen (str_buff, "r");
  pclose(impresora);
  return(i);
}

#endif



