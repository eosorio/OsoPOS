#include <stdio.h>
#include "pos-var.h"

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


void CreaFactura( struct datoscliente cliente,
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

		
void CreaFactura( struct datoscliente cliente,
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

  ffact = fopen(nmfact,"w");
  if (!ffact)
    ErrorArchivo(nmfact);

  fprintf(ffact,"%c",18);
  /* Cancela modo condensado */

  if (!strcmp(tipoimp,"EPSON"))
    fprintf(ffact,"%c%c",ESC,77);
  else
    fprintf(ffact,"%c%c",ESC,58);
  /* 12cpi en modo Epson: ESC,77, en modo IBM: ESC,58 */

  fprintf(ffact,"%cJ%c",ESC,2);
  /* 2/n-avos de pulgada */

  fprintf(ffact,"\n\n\r");

  for (i=0; i<54; ++i)
    fprintf(ffact," ");
  fprintf(ffact,"%u     %u   ",fecha.dia,fecha.mes);
  fprintf(ffact,"%u\n\r",fecha.anio);
  fprintf(ffact,"%cJ%c",ESC,30);
  fprintf(ffact,"       %s\n\r",cliente.nombre);
  fprintf(ffact,"%cJ%c",ESC,15);
  fprintf(ffact,"       %s\n\r",cliente.domicilio);
  fprintf(ffact,"%cJ%c",ESC,15);
  fprintf(ffact,"       %s",cliente.ciudad);
  /* fprintf(ffact, "%s", cliente.edo); */
  buff = strlen(cliente.ciudad);
  fprintf(ffact,"%s\n\n\n\n\r",cliente.rfc);
  for (i=0; i<num_articulos; ++i) {
    fprintf(ffact,"%5.0u",art[i].cant);
    fprintf(ffact,"   %s",art[i].desc);
    buff = strlen(art[i].desc);
    for (j=0; j<(40-buff); ++j)
      fprintf(ffact," ");
    fprintf(ffact,"%8.2f   %8.2f\n\r",art[i].pu,art[i].pu*art[i].cant);
  }
  for (j=0; strlen(obs[j]); j++)
    fprintf(ffact,"%8s%s\n\r"," ",obs[j]);
  /* imprime las observaciones, si las hubiere */

  for (i=0; i<(maxrenf-num_articulos-j); ++i)
    fprintf(ffact,"\n"); /* imprime renglones no usados en articulos */
  if (strlen(garantia) > 3) {
    for (i=0; i<15; ++i)
      fprintf(ffact," ");
    fprintf(ffact,"%s de garantia",garantia);
  }
  fprintf(ffact,"\n\n\r");
  for (i=0; i<57; ++i)
    fprintf(ffact," ");
  fprintf(ffact," %9.2f\n\n\r",subtotal);
  for (i=0; i<57; ++i)
    fprintf(ffact," ");
  fprintf(ffact," %9.2f\n\n\r",iva);
  for (i=0; i<57; ++i)
    fprintf(ffact," ");
  fprintf(ffact," %9.2f\n\r",total);
  fprintf(ffact,"\n\n");
  for (i=0; i<15; ++i)
    fprintf(ffact," ");

  strncpy(importe_letra,str_cant(total,&buff),sizeof(importe_letra));

  if (buff>9)
    fprintf(ffact,"%spesos %2d/100 M.N.",importe_letra,buff);
  else
    fprintf(ffact,"%spesos 0%1d/100 M.N.",importe_letra,buff);

  fprintf(ffact,"%c",FF);

  fclose(ffact);
}

/*void imprime_razon_social(FILE *disp)
{
  fprintf(impr,"%c%c%c    Electro Hogar\n",
              ESC,'P',14);
  fprintf(impr,"%c%c%c  Francisco Javier Mondrag%cn Villa\n", 15,ESC,'M', 186);

}*/
