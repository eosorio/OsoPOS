#if	!defined(OK) || ((OK) != 0)
#define OK      (0)
#endif

#define ERROR_ARCHIVO_1   15
#define ERROR_ARCHIVO_2   16

#define ERROR_SQL      -1
#define ERROR_MEMORIA  -2

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
#define maxart  30	/* Máximo de artículos */
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
 /* Códigos de impresora */
#define ESC 27
#define FF 12

struct datoscliente {
  char rfc[maxrfc];
  char curp[maxcurp];
  char nombre[maxspc];
  /* Se dejan para compatibilidad a domicilio, ciudad y edo, pero se deben quitar eventualmente */
  char domicilio[maxspc];
  char ciudad[maxspc-17];
  char estado[maxspc-20];
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
  char     codigo[maxcod];
  double   pu,                 /* Precio Unitario      */
           p_costo;            /* Precio de cosro      */
  double   disc;               /* Discuento   .=)      */
  unsigned id_prov;            /* Id del proveedor */
  unsigned id_depto;
  int      exist_min, exist_max;
  double   iva_porc;          /*  Gravamen de IVA del artículo */
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

