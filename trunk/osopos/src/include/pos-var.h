/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-

   OsoPOS Sistema auxiliar en punto de venta para pequeños negocios
   Programa OsoPOS (C) 1999-2003 E. Israel Osorio H.
   desarrollo@elpuntodeventa.com
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
#define maxpreciolong 10 /* Máxima longitud de precio */
#endif

#ifndef maxexistlong
#define maxexistlong 4
#endif

#ifndef maxarts
#define maxarts 80       /* Máximo de artículos en remisiones */
#endif

#ifndef maxart
#define maxart  30  /* Máximo de artículos */
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

#ifndef maxtax /* Número máximo de impuestos por PLU*/
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

 /* Códigos de impresora */
#define ESC 27
#define FF 12

struct db_data {
  char *name;      /* Nombre de la base de datos */
  char *user;      /* Nombre del usuario que se conecta a la base de datos */
  char *passwd;    /* Conraseña del usuario */
  char *sup_user;  /* Nombre del usuario supervisor */
  char *sup_passwd;/* Contraseña del supervisor */
  char *hostname;  /* Nombre host con base de datos */
  char *hostport;  /* Puerto en el que acepta conexiones */
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
  char     divisa[3];          /* Divisa de los precios */
  double   disc;               /* Discuento   .=)      */
  unsigned id_prov, id_prov2;  /* Id de los dos principales proveedores */
  unsigned id_depto;
  double   exist_min, exist_max;
  double   iva_porc;          /*  Gravamen de IVA del artículo */
  double   tax_0;             /* I.E.P.S. (impuesto suntuario) */
  double   tax_1, tax_2, tax_3, tax_4, tax_5;
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

struct divisas {
  char id[MX_LON_DIVISA];
  char descripcion[MX_LON_DIVISA_DES];
  double tc;
};
