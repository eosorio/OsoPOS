/*
 pos-func.h 0.19-1 Biblioteca de funciones de OsoPOS.
        Copyright (C) 1999,2000 Eduardo Israel Osorio Hernández

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <time.h>
#include <crypt.h>
#include <pgsql/libpq-fe.h>
#include <err.h>
#include <errno.h>

#ifndef _ctype
#define _ctype
#include <ctype.h>
#endif

#include "pos-var.h"

float iva_porcentaje=0.1; /* <-------  ******* PORCENTAJE DE IVA ******** */
double iva,subtotal,total;

short limpiacad(char *, short alfinal);
/* Quita los caracteres de espacio al final o inicio de la cadena */

char *interp_cant(int unidades, int decenas, int centenas);
/* Interpreta una cifra de tres digitos y la devuelve una cadena */

char *str_cant(double f,int *cent);
/* Interpreta f y devuelve su valor con letra, devuelve centavos como *int */

unsigned Espacios(FILE* arch, unsigned nespacios);
/* Imprime espacios en al archivo ar */

int BuscaBarraArch(char *nmarch, char *cod, char *descr, float *precio,
					unsigned *exist);
/* Busca un codigo cod en el archivo de nombre nmarch (base de datos de
   precios) y devuelve descripción, precio unitario y existencia */

int LeeVenta(char *nombre, struct articulos art[maxarts]);
/* Interpreta el archivo generado por la última venta efectuada */

int lee_venta(PGconn *base,
              int num_venta,
              struct articulos art[maxarts]);

/* Registra la venta en base de datos y devuelve el número de venta */
int registra_venta(PGconn *base,
		   char *tabla,
		   double monto,
		   double utilidad,
		   int tipo_pago,
		   int tipo_factur,
		   int corte_parcial,
		   struct tm fecha,
		   int id_cajero,
		   int id_vendedor,
		   struct articulos art[maxart],
		   unsigned num_arts);


PGresult *Agrega_en_Inventario(PGconn *base, char *tabla, struct articulos art);

PGresult *Modifica_en_Inventario(PGconn *base, char *tabla, struct articulos art);

PGconn *Abre_Base( char *host_pg,     /* nombre de host en servidor back end */
                   char *pg_puerto,   /* puerto del host en servidor back end */
                   char *pg_opciones, /* opciones para iniciar el servidor */
                   char *pg_tty,      /* tty para debugear el servidor back end */
                   char *bd_nombre,   /* nombre de las base de postgresql */
                   char *login,
                   char *passwd   );

PGresult *Busca_en_Inventario(PGconn *base, 
			      char *tabla,
			      char *campo,
			      char *llave,
			      struct articulos *art);

short imprime_doc(char *ruta_doc, char *nm_disp);
/* Copia el archivo ruta_doc hacia nm_disp */

/*********************************************************************/

short limpiacad(char *cad, short alfinal) {
  int i, j;

  if (alfinal) {
    i = strlen(cad)-1;
    j = i;
    while (isspace(cad[i])) {
      cad[i] = 0;
      i--;
    }
   return (abs(i-j));
  }
  else {
    for (i=0; cad[i]==' '; i++);
    memmove(&cad[0], &cad[i], strlen(cad)-i-1);
    return(i);
  }
}

/*********************************************************************/

char *interp_cant(int u, int d, int c) {
  char buffer[mxbuff];
  char *cantidad;

  buffer[0] = 0;
  cantidad = buffer;
  switch (c) {
    case 1: if (d || u)
         strcat(buffer,"ciento ");
       else
	 strcat(buffer,"cien ");
       break;
    case 2: strcat(buffer,"doscientos ");
        break;
    case 3: strcat(buffer,"trescientos ");
        break;
    case 4: strcat(buffer,"cuatrocientos ");
        break;
    case 5: strcat(buffer,"quinientos ");
        break;
    case 6: strcat(buffer,"seiscientos ");
        break;
    case 7: strcat(buffer,"setecientos ");
        break;
    case 8: strcat(buffer,"ochocientos ");
        break;
    case 9: strcat(buffer,"novecientos ");
        break;
    default:
    case 0:
  }

	/* Sección de decenas */
  switch (d) {
    case 1: 
      switch (u) { 
          case 1: strcat(buffer,"on");
             break;
          case 2: strcat(buffer,"do");
             break;
          case 3: strcat(buffer,"tre");
             break;
          case 4: strcat(buffer,"cator");
             break;
          case 5: strcat(buffer,"quin");
             break;
          default: strcat(buffer,"diez ");
      }
      break;
    case 2: strcat(buffer,"veinte ");
        break;
    case 3: strcat(buffer,"treinta ");
        break;
    case 4: strcat(buffer,"cuarenta ");
        break;
    case 5: strcat(buffer,"cincuenta ");
        break;
    case 6: strcat(buffer,"sesenta ");
        break;
    case 7: strcat(buffer,"setenta ");
        break;
    case 8: strcat(buffer,"ochenta ");
        break;
    case 9: strcat(buffer,"noventa ");
        break;
    default:
    case 0:
  }
	/* Seccion de unidades */
  if (d==1 && u && u<=5)
    strcat(buffer,"ce "); /* onCE, doCE, treCE... */
  else {
    if ((u) && (d))
      strcat(buffer,"y ");
    switch (u) {
      case 1: strcat(buffer,"un ");
          break;
      case 2: strcat(buffer,"dos ");
          break;
      case 3: strcat(buffer,"tres ");
          break;
      case 4: strcat(buffer,"cuatro ");
          break;
      case 5: strcat(buffer,"cinco ");
          break;
      case 6: strcat(buffer,"seis ");
          break;
      case 7: strcat(buffer,"siete ");
          break;
      case 8: strcat(buffer,"ocho ");
          break;
      case 9: strcat(buffer,"nueve ");
          break;
      case 0:
      default:
    }
  }
/*  strncpy(cantidad,buffer,mxchcant);*/
  return(cantidad);
}

/*********************************************************************/

char *str_cant(double total,int *centavos) {
  int unidad,decena,centena,munidad,mdecena,mcentena,millon;
  static char buffer[mxbuff], *s;
  char unidades[mxchcant],miles[mxchcant],millones[mxchcant];
  static double tot;
  unsigned num1;
  div_t divis;
  char *cantletra;

  tot = total;
  unidades[0] = 0;
  miles[0] = 0;
  millones[0] = 0;

  sprintf(buffer,"%0.2f",total);
  s = strchr(buffer,'.')+1;
  *centavos = atoi(s);

  buffer[0] = 0;

  num1 = (int) total;

  /* Obtención de millones */
  divis = div(num1,1000000);
  millon = divis.quot;
  num1 = num1 - millon*1000000;

  /* Obtención de centenas de miles */
  divis = div(num1,100000);
  mcentena = divis.quot;
  num1 = num1 - mcentena*100000;

  /* Obtencion de decenas de miles */
  divis = div(num1,10000);
  mdecena = divis.quot;
  num1 = num1 - mdecena*10000;

  /* Obtencion de miles */
  divis = div(num1,1000);
  munidad = divis.quot;
  num1 = num1 - munidad*1000;

  /* Obtencion de centenas */
  divis = div(num1,100);
  centena = divis.quot;
  num1 = num1 - centena*100;

  /* Obtencion de decenas */
  divis = div(num1,10);
  decena = divis.quot;
  num1 = num1 - decena*10;

  unidad = num1;

  if (millon) {
    strcpy(millones,interp_cant(millon,0,0));
    strcat(millones,"millones ");
    strcat(buffer,millones);
  }
  else
    millones[0] = 0;
  if (munidad || mdecena || mcentena) {
    strcpy(miles,interp_cant(munidad,mdecena,mcentena));
    strcat(miles,"mil ");
    strcat(buffer,miles);
  }
  else
    miles[0]=0;
  if (unidad || decena || centena)
    strcpy(unidades,interp_cant(unidad,decena,centena));
  else
    unidades[0]=0;
  strcat(buffer,unidades);
  cantletra = buffer;
  cantletra[0] = toupper(buffer[0]);
/*  strcpy(scant,buffer);*/
  return(cantletra);
}

/*********************************************************************/

unsigned Espacios (FILE* ar, unsigned n) {

  unsigned i;

  for (i=0; i<n; ++i)
    fprintf(ar, " ");
  return(i);
}

/*********************************************************************/

int LeeVenta(char *nombre, struct articulos art[maxarts]) {
  FILE *ar;
  static char buff[mxbuff];
  static int i=0;
  static int salir;

  ar = fopen(nombre,"r");
  if (!ar)
    return(0);
  do {
    fgets(buff,sizeof(buff),ar);
    if (feof(ar)) {
      salir = 1;
      fclose(ar);
      continue;
    }
    else {
      salir = 0;
      strncpy(art[i].codigo, buff, maxcod);
    }
    art[i].codigo[ strlen(art[i].codigo)-1 ] = 0;
    fgets(buff,sizeof(buff),ar);
    if (feof(ar)) {
      salir = 1;
      fclose(ar);
      continue;
    }
    else {
      salir = 0;
      strncpy(art[i].desc, buff, maxdes);
    }
    art[i].desc[ strlen(art[i].desc)-1 ] = 0;
    fscanf(ar, "%i", &(art[i].cant));
    fscanf(ar, "%lf", &(art[i].pu));
	fscanf(ar, "%lf", &(art[i].iva_porc));
    i++;
    fgetc(ar);
  }
  while (!salir);
  return(i);
}

/*********************************************************************/
int lee_venta(PGconn *base,
              int num_venta,
              struct articulos art[maxarts])
{
  char *query;
  PGresult *res;
  int i;

  query = calloc(1, 1024);
  if (query == NULL)
    return (ERROR_MEMORIA);

  sprintf(query, "SELECT codigo, descrip, cantidad, pu, iva_porc FROM ventas_detalle WHERE id_venta=%d",
          num_venta);
  res = PQexec(base, query);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"ERROR: no encuentro registro de la venta %d.",
	    num_venta);
    free(query);
    PQclear(res);
    return(ERROR_SQL);
  }

  for (i=0; i<PQntuples(res); i++) {
    strncpy(art[i].codigo, PQgetvalue(res, i, 0), maxcod);
    strncpy(art[i].desc, PQgetvalue(res, i, 1), maxdes);
    art[i].cant = atoi(PQgetvalue(res, i, 2));
    art[i].pu = atof(PQgetvalue(res, i, 3));
    art[i].iva_porc = atof(PQgetvalue(res, i, 4));
  }

  PQclear(res);
  return(i);
}

/*********************************************************************/

int registra_venta(PGconn *base,
		   char *tabla,
		   double monto,
		   double utilidad,
		   int tipo_pago,
		   int tipo_factur,
		   int corte_parcial,
		   struct tm fecha,
		   int id_cajero,
		   int id_vendedor,
		   struct articulos art[maxart], /* convertir en *art */
		   unsigned num_arts)
{

  char *query;
  char corte;
  int num_venta, i;
  PGresult *res;

  query = calloc(1, 1024);
  if (query == NULL)
    return (ERROR_MEMORIA);

  if (corte_parcial)
    corte = 't';
  else
    corte = 'f';
  sprintf(query, "INSERT INTO ventas (monto, tipo_pago, tipo_factur, corte_parcial, utilidad, id_vendedor, id_cajero, fecha, hora) VALUES (%.2f, %d, %d, '%c', %.2f, %d, %d, '%d-%d-%d', '%d:%d:%d')",
	  monto, tipo_pago, tipo_factur, corte, utilidad, id_vendedor, id_cajero,
	  fecha.tm_year, (fecha.tm_mon)+1, fecha.tm_mday,
	  fecha.tm_hour, fecha.tm_min, fecha.tm_sec);
  res = PQexec(base, query);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error al registrar venta\n %s\n", query);
    free(query);
    PQclear(res);
    return(ERROR_SQL);
  }      
  sprintf(query, "SELECT numero FROM ventas WHERE id_cajero=%d AND fecha='%d-%d-%d' AND hora='%d:%d:%d'",
	  id_cajero, fecha.tm_year, (fecha.tm_mon)+1, fecha.tm_mday,
	  fecha.tm_hour, fecha.tm_min, fecha.tm_sec);
  res = PQexec(base, query);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"ERROR: no encuentro registro de la venta a las %d:%d:%d horas del día %d-%d-%d",
	    fecha.tm_hour, fecha.tm_min, fecha.tm_sec, fecha.tm_year, (fecha.tm_mon)+1, fecha.tm_mday);
    free(query);
    PQclear(res);
    return(ERROR_SQL);
  }
  num_venta = atoi(PQgetvalue(res, 0, 0));

  for (i=0; i<num_arts; i++) {
    strncpy(query, "INSERT INTO ventas_detalle (\"id_venta\", \"codigo\", \"cantidad\", \"descrip\", \"pu\", \"iva_porc\")", 1024);
    sprintf(query, "%s VALUES (%d, '%s', %d, '%s', %.2f, %.2f)", query, num_venta,
	    art[i].codigo, art[i].cant, art[i].desc, art[i].pu, art[i].iva_porc);
    res = PQexec(base, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "Error al registrar detalle de ventas.\n%s\n", query);
      free(query);
      PQclear(res);
      return(ERROR_SQL);
    }
  }

  
  free(query);
  return(num_venta);
}
 
/*********************************************************************/

PGresult *Agrega_en_Inventario(PGconn *base, char *tabla, struct articulos art)
{
  char *query;
  PGresult *resultado;

  query = calloc(1,mxbuff);
  sprintf(query,
          "INSERT INTO %s VALUES ('%s', '%s', %.2f, %.2f, %u, %u, %u, %u, %u, %.2f, '%s', %.2f)",
	  tabla, art.codigo, art.desc, art.pu, art.disc, art.exist,
	  art.exist_min, art.exist_max, art.id_prov, art.id_depto,
	  art.p_costo, art.prov_clave, art.iva_porc);
  resultado = PQexec(base, query);
  if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error al registrar artículos.\n%s\n", query);
    free(query);
    return(resultado);
  }  
  free(query);
  return(resultado);
}

/*********************************************************************/

PGresult *Modifica_en_Inventario(PGconn *base, char *tabla, struct articulos art)
{
  char *comando_sql;
  PGresult *res;

  comando_sql = calloc(1,mxbuff);
  sprintf(comando_sql,
         "UPDATE %s SET descripcion='%s', pu=%.2f, descuento=%.2f, cant=%u, min=%u, max=%u, id_prov='%u', id_depto='%u', p_costo=%.2f, prov_clave='%s', iva_porc=%.2f WHERE codigo='%s'",
		  tabla, art.desc, art.pu, art.disc, art.exist,
		  art.exist_min, art.exist_max, art.id_prov,
		  art.id_depto, art.p_costo, art.prov_clave,
		  art.iva_porc, art.codigo);
  res = PQexec(base, comando_sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
    fprintf(stderr, "Error: %s\n", PQerrorMessage(base));
  free(comando_sql);
  return(res);
}

/*********************************************************************/

PGresult *Quita_de_Inventario(PGconn *base, char *tabla, char *codigo)
{
  char *comando_sql;
  PGresult *res;

  comando_sql = "BEGIN";
  res = PQexec(base, comando_sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error, no se pudo comenzar la transacción para borrar\n");
    fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
    return(res);
  }
  PQclear(res);

  comando_sql = calloc(1,mxbuff);
  sprintf(comando_sql, "DELETE FROM %s WHERE codigo='%s'", tabla, codigo);
  res = PQexec(base, comando_sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
    fprintf(stderr, "Error: %s\n", PQerrorMessage(base));
  free(comando_sql);
  PQclear(res);

  res = PQexec(base, "END");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error, no se pudo terminar la transacción para borrar\n");
    fprintf(stderr, "Mensaje de error: %s\n", PQerrorMessage(base));
  }
  return(res);
}

/*************************************************************/

PGresult *Busca_en_Inventario(PGconn *base, 
			      char *tabla,
			      char *campo,
			      char *llave,
			      struct articulos *art)
{
  char      *comando;
  PGresult* res;

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr,"Falló comando BEGIN al buscar en inventario\n");
   return(res);
  }
  PQclear(res);

  comando = calloc(1,mxbuff);
  sprintf(comando,
      "DECLARE cursor_arts CURSOR FOR SELECT * FROM articulos WHERE \"%s\"~*'%s'",
	  campo, llave);
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Fallo comando DECLARE CURSOR al buscar un artículo\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    free(comando);
    return(res);
  }
  PQclear(res);

  strcpy(comando, "FETCH ALL in cursor_arts");
  res = PQexec(base, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
    free(comando);
    return(res);
  }

  /*nCampos = PQnfields(res); */

/*  strcpy(art.codigo,PQgetvalue(res,registro,campo));  */

  if (PQntuples(res)) {
    strcpy(comando, PQgetvalue(res,0,0));
    strncpy(art->codigo, comando, maxcod);

    strcpy(comando, PQgetvalue(res,0,1));
    strncpy(art->desc, comando, maxdes);

    art->pu = atof(PQgetvalue(res,0,2));
    art->disc = atof(PQgetvalue(res,0,3));
    art->exist = atof(PQgetvalue(res,0,4));
    art->exist_min = atof(PQgetvalue(res,0,5));
    art->exist_max = atof(PQgetvalue(res,0,6));
    art->id_prov = atoi(PQgetvalue(res,0,7));
    art->id_depto = atoi(PQgetvalue(res,0,8));
    art->p_costo = atof(PQgetvalue(res,0,9));
	strncpy(art->prov_clave, PQgetvalue(res, 0, 10), maxcod);
    art->iva_porc = atof(PQgetvalue(res, 0, 11));

  }
  PQclear(res);

  /* close the portal */
  res = PQexec(base, "CLOSE cursor_arts");
  PQclear(res);

  /* end the transaction */
  res = PQexec(base, "END");
  PQclear(res);
  free(comando);
  return(OK);
}


/*************************************************************/

PGconn *Abre_Base( char *host_pg,
                   char *puerto_pg,
                   char *opciones_pg,
                   char *tty_pg,
                   char *nombre_bd,
                   char *login,
                   char *passwd )
{
  PGconn *con;
  char *msg;

  con = PQsetdbLogin(host_pg, puerto_pg, opciones_pg, tty_pg, nombre_bd, login, passwd);
/*  con = PQsetdb("", "", "", "", "osopos"); */
  if (PQstatus(con) == CONNECTION_BAD) {
    msg = calloc(1,256);
    sprintf(msg, "Falló la conexión a la base '%s' .\n\r", nombre_bd);
    fprintf(stderr, msg);
    sprintf(msg,"Error: %s\n\r",PQerrorMessage(con));
    fprintf(stderr,msg);
    free(msg);
    msg = NULL;
    return(NULL);
  }
  return(con);
}

/*********************************************************************/

short imprime_doc(char *ruta_doc, char *nm_disp)
{

FILE *disp,
     *arch;
char *buff;

  disp = fopen(nm_disp, "a");
  if (!disp)
    return(ERROR_ARCHIVO_1);

  arch = fopen(ruta_doc, "r");
  if (!arch) {
    fclose(disp);
    return(ERROR_ARCHIVO_2);
  }

  buff = calloc(1,mxbuff);
  fgets(buff, mxbuff, arch);

  while (!feof(arch)) {
    fprintf(disp, buff);
    fgets(buff, mxbuff, arch);
  }
  free(buff);
  buff = NULL;
  fclose(disp);
  fclose(arch);
  return(OK);
}

