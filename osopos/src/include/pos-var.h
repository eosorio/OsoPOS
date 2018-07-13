/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para peque�os negocios
   Sistema OsoPOS (C) 1999-2006 E. Israel Osorio H.
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


#include <sys/types.h>
#include <glib.h>

#if !defined(OK) || ((OK) != 0)
#define OK      (0)
#endif

#define ERROR_ARCHIVO_1   15
#define FILE_1_ERROR      15
#define ERROR_ARCHIVO_2   16
#define FILE_2_ERROR      16

#define ERROR_SQL      -1
#define SQL_ERROR      -1
#define ERROR_MEMORIA  -2
#define MEMORY_ERROR   -2
#define PROCESS_ERROR  -3
#define ERROR_DIVERSO  -5
#define OTHER_ERROR    -5
#define ERR_ITEM       -6  /* El producto no est� disponible */

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

#ifndef maxtax /* N�mero m�ximo de impuestos por PLU*/
#define maxtax 6
#endif

#ifndef maxmovinvent
#define maxmovinvent    100
#endif

#ifndef maxstr_movinvent
#define maxstr_movinvent    30
#endif

#ifndef MX_LON_DIVISA
#define MX_LON_DIVISA       3
#endif

#ifndef MX_LON_DIVISA_DES
#define MX_LON_DIVISA_DES   20
#endif

#ifndef MX_LON_NUMSERIE
#define MX_LON_NUMSERIE 50
#endif

#define _NOTA_MOSTRADOR  1
#define _FACTURA         2
#define _TEMPORAL        5

double TAX_PERC_DEF; /* Porcentaje de IVA por omisi�n */

 /* C�digos de impresora */
#define ESC 27
#define FF 12

struct db_data {
  gchar *name;       /* Nombre de la base de datos */
  gchar *user;       /* Nombre del usuario que se conecta a la base de datos */
  gchar *passwd;     /* Contraseña del usuario */
  gchar *sup_user;   /* Nombre del usuario supervisor */
  gchar *sup_passwd; /* Contraseña del supervisor */
  gchar *hostname;   /* Nombre host con base de datos */
  gchar *hostport;   /* Puerto en el que acepta conexiones */
};

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
  double   cant, exist;
  char     desc[maxdes];
  char     codigo[maxcod], codigo2[maxcod];
  double   pu,                 /* Precios unitarios */
           pu2,
           pu3,
           pu4,
           pu5,
           p_costo;            /* Precio de costo      */
  double   utilidad;           /* Utilidad unitaria en la divisa dada */
  char     divisa[3];          /* Divisa de los precios */
  double   disc;               /* Discuento   .=)      */
  unsigned id_prov, id_prov2;  /* Id de los dos principales proveedores */
  unsigned id_depto;
  double   exist_min, exist_max;
  double   iva_porc;          /*  Gravamen de IVA del art�culo */
  double   tax_0;             /* I.E.P.S. (impuesto suntuario) */
  double   tax_1, tax_2, tax_3, tax_4, tax_5;
  char     prov_clave[maxcod]; /* Clave del articulo con proveedor */
  char     serie[MX_LON_NUMSERIE]; /* N�mero de serie */
  char     tiene_serie;        /* El art�culo tiene control de num. serie */
  unsigned almacen;            /* Almacen de origen */
  unsigned t_renta;            /* Tiempo proporcionado en renta */
  short    unidad_t;           /* Unidad del tiempo de renta (0: minutos, ... 3: semanas ... 5:a�os*/
  short    garan_t;            /* Tiempo de garant�a */
  short    garan_unidad;       /* Unidad de tiempo de garant�a */
  char     granel;             /* Se vende a granel y se usa b�scula */
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

struct divisas {
  char id[MX_LON_DIVISA];
  char descripcion[MX_LON_DIVISA_DES];
  double tc;
};

struct usuario {
  unsigned id;
  gchar *login;
  gchar *passwd;
  gchar *nombre;
  unsigned nivel;
};
