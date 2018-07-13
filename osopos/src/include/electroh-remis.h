#include <stdio.h>
#include <string.h>

#include "print-func.h"

#ifndef ESC
#define ESC 27
#endif

#ifndef maxbuf
#define maxbuf 254
#endif

unsigned Espacios(FILE* arch, unsigned nespacios);
void avance_vert(FILE *disp, unsigned avance, unsigned mode);

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
int imprime(char *nmimpre, struct articulos art[maxart], int numart);

int imprime(char *nmimpre, struct articulos art[maxart], int numart) {
  extern unsigned maxitem;
  extern unsigned ajuste_microv, ajuste_v, ajuste_h;
  extern unsigned desglose_iva;
  extern char *lp_printer;
  extern short unsigned price_pos[3];
  extern unsigned total_price_pos, desc_pos, desc_len, code_len, qt_pos, code_pos;
  extern short unsigned date_posv, date_posh;
  extern short article_pitch;
  extern short date_pitch;
  extern char *prt_type;
  extern int dgaran;
  extern char *cmd_lp;

  FILE *impresora;
  static int i=0, j, k, renglon;
  static int ib;
  time_t tiempo;
  struct tm *fecha;
  char str_buff[maxbuf];
  char buff1[mxbuff],buff2[mxbuff];
  int modo;

  if (strcmp(prt_type, "EPSON"))
    modo = 1;
  else modo = 0;

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

  avance_vert(impresora, ajuste_microv, modo);

   /* Imprime encabezado */
  for (i=0; i<date_posv; i++)
    fprintf(impresora, "\n");

  Espacios(impresora, ajuste_h);
  fprintf(impresora,"%2d  %3d  %4d",fecha->tm_mday,
          fecha->tm_mon +1 ,fecha->tm_year);
  fprintf(impresora,"\n\n");
  fprintf(impresora,"%c%c%cM",ESC,15,ESC); /* 17cpi */
  fprintf(impresora,"\n\n\n\n%c%c%c",ESC,'J',890);
  /* 890/216" de avance vertical */

  /* Imprime articulos */
  for (i=0,renglon=0; i<numart; i++,renglon++) {
    Espacios(impresora, ajuste_h);
    str_buff[0] = 0;
    espacio(str_buff, qt_pos);
    sprintf(str_buff,"%s%2.0f", str_buff, art[i].cant);

    if (strlen(str_buff) < desc_pos)
      espacio(str_buff, desc_pos-strlen(str_buff));

	/* Descripcion */
	if (strlen(art[i].desc) > desc_len)
	  strncpy(buff1, art[i].desc, desc_len-1);
	else
	  strcpy(buff1, art[i].desc);
    strcat(str_buff, buff1);

    k = price_pos[1] - strlen(str_buff);
    if (k)
      espacio(str_buff, k);

    fprintf(impresora,"%s%8.2f\n\r", str_buff, art[i].cant*art[i].pu);
 
    if (renglon<maxitem)
      avance_vert(impresora,15, modo); /* 15/216" entre renglones */
  }

  /* Imprime renglones vacios faltantes */
  for (; renglon<maxitem+(desglose_iva*2); renglon++) {
     /* 2 renglones por el Subtotal y el IVA, */
    fprintf(impresora,"\n\r");
    if (renglon<maxitem)
      avance_vert(impresora,15, modo); /* 15/216" entre renglones */
  }

     /* Imprime subtotal, IVA y total */
  avance_vert(impresora,3, modo); /* 3/216" de avance */

  if (dgaran < 0)
    sprintf(buff1,"(     dias de garantia)");
  else
    sprintf(buff1,"( %3d dias de garantia)",dgaran);

  Espacios(impresora, ajuste_h);
//  fprintf(impresora, "      %-46s",buff1);
  sprintf(str_buff, "   %-30s",buff1);


  k = price_pos[1] - strlen(str_buff);

  if (desglose_iva) {
    for (j=0; j<k; j++)
      strcat(str_buff, " ");
    fprintf(impresora,"%s%8.2f\n\r", str_buff, subtotal);
//    fprintf(impresora, "%6.2f\n\r", subtotal);
    Espacios(impresora, ajuste_h);
//    fprintf(impresora, "%46s %6.2f\n\r", "", iva);
    Espacios(impresora, total_price_pos);
    fprintf(impresora, "%8.2f\n\r", iva);
    avance_vert(impresora, 20, modo); /* 20/216" entre renglones */
    Espacios(impresora, total_price_pos);
  }
  else {
    Espacios(impresora, ajuste_h);
    fprintf(impresora, "%s", str_buff);
    Espacios(impresora, k);
  }

  fprintf(impresora,"%8.2f\r\n", total);
  avance_vert(impresora,15, modo);

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
  //  sprintf(str_buff, "lpr -l -P %s %s", lp_printer, nmimpre);
  sprintf(str_buff, "%s %s %s", cmd_lp, lp_printer, nmimpre);
  impresora = popen(str_buff, "w");
  i = pclose(impresora);
  sprintf(str_buff, "rm -rf %s", nmimpre);
  impresora = popen (str_buff, "r");
  pclose(impresora);
  return(i);
}

#endif
