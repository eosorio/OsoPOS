#ifdef _MOD_FACTUR
#include "factur.h"
#endif

#ifndef maxdes
#define maxdes 60
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

#ifndef maxrenf
#define maxrenf 22
#endif

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
		    char     *obs[maxobs],
		    char     *nmfact,
		    char     *tipoimp)

{
int i,buff;
FILE *ffact;
char *importe_letra;

  ffact = fopen(nmfact,"w");

#ifdef _pos_curses
  if (!ffact)
    ErrorArchivo(nmfact);
#endif

  fprintf(ffact, "%ck",ESC);

  fprintf(ffact,"%c",18);
  /* Cancela modo condensado */

  if (!strcmp(tipoimp,"EPSON"))
    fprintf(ffact,"%c%c",ESC,77);
  else
    fprintf(ffact,"%c%c",ESC,58);
  /* 12cpi en modo Epson: ESC,77, en modo IBM: ESC,58 */

  fprintf(ffact,"%cJ%c",ESC,800);
  /* 2/n-avos de pulgada */

  for (i=0; i<11; i++)
    fprintf(ffact, "\n");

  fprintf(ffact,"    %s\n",cliente.nombre);
  fprintf(ffact,"    %s, %s-%s\n",
		  cliente.dom_calle, cliente.dom_numero, cliente.dom_inter);
  fprintf(ffact,"    Col. %s ", cliente.dom_col);
  fprintf(ffact, "\n");
  fprintf(ffact,"    %s, %s", cliente.dom_ciudad, cliente.dom_edo);
  if (strlen(obs[0])) {
	if (strlen(cliente.dom_ciudad)+strlen(cliente.dom_edo) <= 46)
	  Espacios(ffact,  46 - strlen(cliente.dom_ciudad) - strlen(cliente.dom_edo) - 2 );
	fprintf(ffact, "%s", obs[0]); /* Observaciones */
  }
  fprintf(ffact, "\n");
  fprintf(ffact,"    C.P. %5d", cliente.cp);
  if (strlen(obs[1])) {
	Espacios(ffact, 36);
	fprintf(ffact, "%s", obs[1]); /* Observaciones */
  }
  fprintf(ffact, "\n");
  fprintf(ffact,"%cJ%c",ESC,14); /* 14-avos de pulgada de avance vert */
  fprintf(ffact,"            %s",cliente.rfc);
  if (strlen(obs[2])) {
    Espacios(ffact, 38-strlen(cliente.rfc));
    if (strlen(obs[2]) > 28)
      obs[2][18] = 0;
    fprintf(ffact, "%s", obs[2]); /* Observaciones */
    Espacios(ffact, 29-strlen(obs[2])); /* Espacio entre obs y fecha */
  }
  else
    Espacios(ffact, 65-strlen(cliente.rfc)-strlen(obs[2]));
  fprintf(ffact,"%u / %u / %u\n\n",fecha.dia,fecha.mes,fecha.anio);
  fprintf(ffact,"%cJ%c",ESC,19);

  fprintf(ffact,"%cJ%c",ESC,90);

  for (i=0; i<num_articulos; ++i) {
	fprintf(ffact,"  %15s", art[i].codigo);
    fprintf(ffact," %5.0u",art[i].cant);
    fprintf(ffact,"    %-40s ",art[i].desc);
    buff = strlen(art[i].desc);
    fprintf(ffact,"%10.2f %11.2f\n\r",art[i].pu,art[i].pu*art[i].cant);
  }

  for (i=0; i<((maxrenf)-num_articulos); ++i)
    fprintf(ffact,"\n"); /* imprime renglones no usados en articulos */

  importe_letra = calloc(1, maximport);
  strncpy(importe_letra,str_cant(total,&buff),maximport);
  Espacios(ffact, 20);
  fprintf(ffact,"%s---",importe_letra);
  fprintf(ffact, "\n");
  free(importe_letra);

  Espacios(ffact, 20);
  if (buff>9)
    fprintf(ffact,"pesos %2d/100 M.N.\n",buff);
  else
    fprintf(ffact,"pesos 0%1d/100 M.N.\n",buff);
  fprintf(ffact, "\n");

  fprintf(ffact,"%cJ%c",ESC,12);
  if (strlen(garantia) > 3) {
    Espacios(ffact, 5);
    fprintf(ffact,"%s de garantia",garantia);
  }
  fprintf(ffact,"\n\n");
  Espacios(ffact, 80);
  fprintf(ffact,"   %9.2f\n\n",subtotal);
  fprintf(ffact,"%cJ%c",ESC,15);
  Espacios(ffact, 80);
  fprintf(ffact,"   %9.2f\n\n",iva);
  fprintf(ffact,"%cJ%c",ESC,15);
  Espacios(ffact,80);
  fprintf(ffact,"   %9.2f\n\n",total);

  fprintf(ffact, "\n\n\n\n");
  fprintf(ffact,"%cJ%c",ESC,12);
  Espacios(ffact,25);
  fprintf(ffact,"Elaborado con OsoPOS. http://punto-deventa.com\n");

  fprintf(ffact,"%c",FF);

  fclose(ffact);
}
