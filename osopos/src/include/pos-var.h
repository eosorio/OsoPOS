/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para peque�os negocios
   Programa OsoPOS (C) 1999-2001 E. Israel Osorio H.
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

#if !defined(OK) || ((OK) != 0)
#define OK      (0)
#endif

#define ERROR_ARCHIVO_1   15
#define ERROR_ARCHIVO_2   16

#define ERROR_SQL      -1
#define ERROR_MEMORIA  -2
#define ERROR_DIVERSO  -5

#ifndef mxchcant
#define mxchcant 50
#endif

#ifndef mxbuff
#define mxbuff 250
#endif

#ifndef maxcod
#define maxcod 20
#endif

#ifndef maxdes
#define maxdes 39
#endif

#ifndef maxdisclong
#define maxdisclong 4
#endif

#ifndef maxpreciolong
#define maxpreciolong 10 /* M�xima longitud de precio */
#endif

#ifndef maxexistlong
#define maxexistlong 4
#endif

#ifndef maxarts
#define maxarts 80       /* M�ximo de art�culos en remisiones */
#endif

#ifndef maxart
#define maxart  30  /* M�ximo de art�culos */
#endif

#ifndef maxrfc
#define maxrfc  14
#endif

#ifndef maxcurp
#define maxcurp 19
#endif

#ifndef maxspc
#define maxspc 71
#endif

#ifndef maxspcalle
#define maxspcalle 55
#endif

#ifndef maxspext
#define maxspext   16
#endif

#ifndef maxspint
#define maxspint    4
#endif

#ifndef maxspcol
#define maxspcol   21
#endif

#ifndef maxspcd 
#define maxspcd    31
#endif

#ifndef maxspedo 
#define maxspedo   30
#endif

#ifndef maxdepto
#define maxdepto   100
#endif

#ifndef maxdeptolen
#define maxdeptolen  20
#endif

#ifndef maxprov
#define maxprov    100
#endif

#ifndef maxnickprov
#define maxnickprov  20
#endif

#ifndef maxnmdepto
#define maxnmdepto   25
#endif
 /* C�digos de impresora */
#define ESC 27
#define FF 12

struct datoscliente {
  char rfc[maxrfc];
  char curp[maxcurp];
  char nombre[maxspc];
  char dom_calle[maxspcalle];
  char dom_numero[maxspext];
  char dom_inter[maxspint];
  char dom_col[maxspcol];
  char dom_ciudad[maxspcd];
  char dom_edo[maxspedo];
  unsigned cp;
};

struct articulos {
  int      cant, exist;
  char     desc[maxdes];
  char     codigo[maxcod], codigo2[maxcod];
  double   pu,                 /* Precios unitarios */
           pu2,
           pu3,
           pu4,
           pu5,
           p_costo;            /* Precio de costo      */
  char     divisa[3];          /* Divisa de los precios */
  double   disc;               /* Discuento   .=)      */
  unsigned id_prov;            /* Id del proveedor */
  unsigned id_depto;
  int      exist_min, exist_max;
  double   iva_porc;          /*  Gravamen de IVA del art�culo */
  char     prov_clave[maxcod]; /* Clave del articulo con proveedor */
};

struct proveedor {
  char     nick[15];
  char     razon_soc[30],
           calle[30],
           colonia[25],
           ciudad[30],
           estado[30],
           contacto[40];
};

struct departamento {
  unsigned id;
  char     nombre[maxnmdepto];
};

struct fech {
  short unsigned dia;
  short unsigned mes;
  short unsigned anio;
};

