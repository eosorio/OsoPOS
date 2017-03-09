#include "print-func.h"

#ifdef _MOD_FACTUR
#include "factur.h"
#endif

#ifndef maxdes
#define maxdes 39
#endif

#ifndef maxobs
#define maxobs 5
#endif

#ifndef maximport
#define maximport 250
#endif

#ifndef maxart
#define maxart 40
#endif

unsigned Espacios(FILE* arch, unsigned nespacios);

void Crea_Factura( struct datoscliente cliente,
		    struct   fech fecha,
		    struct   articulos art[maxart],
		    unsigned num_articulos,
		    double   subtotal,
		    double   iva,
		    double   total,
		    char     *garantia,
		    char     *obs[maxobs],
		    char     *nmfact,
		    char     *tipoimp);

		
void Crea_Factura( struct datoscliente cliente,
		    struct   fech fecha,
		    struct   articulos art[maxart],
		    unsigned num_articulos,
		    double   subtotal,
		    double   iva,
		    double   total,
		    char     *garantia,
		    char     *obs[maxobs], /* maxobs renglones de maxdes longitud */
		    char     *nmfact,
		    char     *tipoimp) {
  int i,j,buff;
  FILE *ffact;
  char importe_letra[maximport];
  unsigned margen=2; /*igm*/

  ffact = fopen(nmfact,"w");
  if (!ffact)
    ErrorArchivo(nmfact);

  //  fprintf(ffact,"%c",18);
  /* Cancela modo condensado */

  /*  if (!strcmp(tipoimp,"EPSON"))
    fprintf(ffact,"%c%c",ESC,77);
  else
  fprintf(ffact,"%c%c",ESC,58);*/
  /* 12cpi en modo Epson: ESC,77, en modo IBM: ESC,58 */


  fprintf(ffact,"\n\n\r");

  /*  for (i=0; i<54; ++i)
      fprintf(ffact," ");*/
  Espacios(ffact, 52);
  Espacios(ffact, margen);
  fprintf(ffact,"%u     %u   ",fecha.dia,fecha.mes);
  fprintf(ffact,"%u\n\r",fecha.anio);
  fprintf(ffact,"%cJ%c",ESC,30);
  Espacios(ffact, margen);
  fprintf(ffact,"     %s\n\r",cliente.nombre);
  fprintf(ffact,"%cJ%c",ESC,15);
  Espacios(ffact, margen);
  fprintf(ffact,"     %s, %s", cliente.dom_calle, cliente.dom_numero);
  if (strlen(cliente.dom_inter))
    fprintf(ffact, "-%s.", cliente.dom_inter);
  if (strlen(cliente.dom_col))
    fprintf(ffact, " %s", cliente.dom_col);
  fprintf(ffact, "\n\r");
  fprintf(ffact,"%cJ%c",ESC,15);
  Espacios(ffact, margen);
  fprintf(ffact,"     %s, %s",cliente.dom_ciudad, cliente.dom_edo);
  buff = strlen(cliente.dom_ciudad) + strlen(cliente.dom_edo) + 2 + margen;
  if (buff<40)
    Espacios(ffact, 40-buff);
  fprintf(ffact,"%s\n\n\n\n\r",cliente.rfc);
  for (i=0; i<num_articulos; ++i) {
    Espacios(ffact, margen);
    fprintf(ffact,"%3.0f",art[i].cant);
    fprintf(ffact,"   %s",art[i].desc);
    buff = strlen(art[i].desc);
    Espacios(ffact, 40-buff);
    fprintf(ffact,"%8.2f   %8.2f\n\r",art[i].pu,art[i].pu*art[i].cant);
  }
  for (j=0; strlen(obs[j]); j++) {
    Espacios(ffact, margen);
    Espacios(ffact, 6);
    fprintf(ffact,"%s\n\r", obs[j]);
  /* imprime las observaciones, si las hubiere */
  }

  for (i=0; i<(maxrenf-num_articulos-j); ++i)
    fprintf(ffact,"\n"); /* imprime renglones no usados en articulos */
  if (strlen(garantia) > 3) {
    Espacios(ffact, margen);
    Espacios(ffact, 13);
    fprintf(ffact,"%s de garantia",garantia);
  }
  fprintf(ffact,"\n\n\r");
  Espacios(ffact, margen);
  for (i=0; i<55; ++i)
    fprintf(ffact," ");
  fprintf(ffact," %9.2f\n\n\r",subtotal);
  Espacios(ffact, margen);
  for (i=0; i<55; ++i)
    fprintf(ffact," ");
  fprintf(ffact," %9.2f\n\n\r",iva);

  Espacios(ffact, margen);
  Espacios(ffact, 55);
  fprintf(ffact," %9.2f\n\r",total);
  fprintf(ffact,"\n\n");

  Espacios(ffact, margen);
  Espacios(ffact, 13);

  strncpy(importe_letra,str_cant(total,&buff), maximport);

  if (buff>9)
    fprintf(ffact,"%spesos %2d/100 M.N.",importe_letra,buff);
  else
    fprintf(ffact,"%spesos 0%1d/100 M.N.",importe_letra,buff);

  fprintf(ffact,"%c",FF);

  fclose(ffact);
}

