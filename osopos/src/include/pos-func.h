/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 pos-func.h 0.25-1 Biblioteca de funciones de OsoPOS.
        Copyright (C) 1999-2002 Eduardo Israel Osorio Hernández

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
#include <libpq-fe.h>
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

int lee_venta(PGconn *base,
              int num_venta,
              struct articulos art[maxarts]);

/* Registra la venta en base de datos y devuelve el número de venta */
/* antes: registra_venta */
int sale_register(PGconn *base, PGconn *base_sup,
                  char *tabla,
                  double monto,
                  double iva,
                  double tax[maxtax],
                  double utilidad,
                  int tipo_pago,
                  int tipo_factur,
                  int serie,
                  long *folio,
                  struct tm fecha,
                  int id_teller,
                  int id_seller,
                  struct articulos art[maxart],
                  unsigned num_arts,
                  unsigned almacen);

long int registra_renta(PGconn *base, PGconn *base_sup,
                   char *tabla,
                   long id_cliente,
                   time_t f_pedido,
                   time_t f_entrega,
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


PGresult *search_product(PGconn *base, 
			      char *tabla,
			      char *campo,
			      char *llave,
                              int  exacto,
			      struct articulos *art);

short imprime_doc(char *ruta_doc, char *nm_disp);
/* Copia el archivo ruta_doc hacia nm_disp */

short busca_proveedor(PGconn *base, char *proveedor);
/* Busca el id del proveedor */

short busca_depto(PGconn *base, char *depto);
/* Busca el id del departamento */

short busca_id(PGconn *base, char *aguja, char *pajar, char *campo);
  /* Busca el id de aguja en el catálogo pajar, contenida en campo */

PGresult *salida_almacen(PGconn *base, unsigned almacen, struct articulos art, char *usuario, struct tm *fecha);

int puede_hacer(PGconn *base, char *usuario, char *accion);
/* Revisa si el usuario puede hacer accion */

int checa_descuento(PGconn *base, int num_venta, int almacen);
/* Revisa si se cobró el precio unitario original o si hubo descuento */

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

  buffer[ strcspn(buffer, ".") ] = 0; /* Dejamos la parte entera sin decimal */
  num1 = atoi(buffer);               /* Le damos la vuelta al bug de punto flotante */
  //  num1 = (int) total;
  buffer[0] = 0;


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
	/* Ignoro porque se mete basura a unidades cuando se manda como parámetro de función
	   total=1  */
	strncpy(unidades,interp_cant(unidad,decena,centena), mxchcant);
  else
    unidades[0]=0;
  strcat(buffer,unidades);
  cantletra = buffer;
  cantletra[0] = toupper(buffer[0]);
/*  strcpy(scant,buffer);*/
  return(cantletra);
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

  sprintf(query, "SELECT codigo, descrip, cantidad, pu, iva_porc, tax_0, tax_1, tax_2, tax_3, tax_4, tax_5 FROM ventas_detalle WHERE id_venta=%d",
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
    art[i].cant = atof(PQgetvalue(res, i, 2));
    art[i].pu = atof(PQgetvalue(res, i, 3));
    art[i].iva_porc = atof(PQgetvalue(res, i, 4));
    art[i].tax_0 =  atof(PQgetvalue(res, i, 5));
    art[i].tax_1 =  atof(PQgetvalue(res, i, 6));
    art[i].tax_2 =  atof(PQgetvalue(res, i, 7));
    art[i].tax_3 =  atof(PQgetvalue(res, i, 8));
    art[i].tax_4 =  atof(PQgetvalue(res, i, 9));
    art[i].tax_5 =  atof(PQgetvalue(res, i,10));
  }

  free(query);
  PQclear(res);
  return(i);
}

/*********************************************************************/

int sale_register(PGconn *base, PGconn *base_sup,
                  char *tabla,
                  double monto,
                  double iva,
                  double tax[maxtax],
                  double utilidad,
                  int tipo_pago,
                  int tipo_factur,
                  int serie,
                  long *folio,
                  struct tm fecha,
                  int id_teller,
                  int id_seller,
                  struct articulos art[maxart], /* convertir en *art */
                  unsigned num_arts,
                  unsigned almacen)
{

  char *query;
  char *ch_aux;
  char *corte;
  int num_venta, i;
  int descto_aplicado=0;
  PGresult *res;

  query = calloc(1, 1024);
  if (query == NULL)
    return (ERROR_MEMORIA);

  corte = "00000000";

  *folio = -1;

  sprintf(query, "INSERT INTO ventas (monto, tipo_pago, tipo_factur, utilidad, id_vendedor, id_cajero, fecha, hora, iva, tax_0, tax_1, tax_2, tax_3, tax_4, tax_5) VALUES (%.2f, %d, %d, %.2f, %d, %d, '%d-%d-%d', '%d:%d:%d', %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f)",
          monto, tipo_pago, tipo_factur, utilidad, id_seller, id_teller,
          fecha.tm_year, (fecha.tm_mon)+1, fecha.tm_mday,
          fecha.tm_hour, fecha.tm_min, fecha.tm_sec, iva,
          tax[0], tax[1], tax[2], tax[3], tax[4], tax[5]);
  res = PQexec(base_sup, query);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error al registrar venta\n");
    free(query);
    PQclear(res);
    return(ERROR_SQL);
  }      
  sprintf(query, "SELECT numero FROM ventas WHERE id_cajero=%d AND fecha='%d-%d-%d' AND hora='%d:%d:%d'",
	  id_teller, fecha.tm_year, (fecha.tm_mon)+1, fecha.tm_mday,
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

  switch (tipo_factur) {
  case _NOTA_MOSTRADOR:
    break;
  case _FACTURA:
    break;
  case _TEMPORAL:
    sprintf(query, "INSERT INTO folios_tickets_%d (venta) VALUES (%d) ",
            serie, num_venta);
    res = PQexec(base, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr,"ERROR: no puedo registrar folio de ticket de la venta %d\n", num_venta);
      free(query);
      PQclear(res);
      return(ERROR_SQL);
    }
    PQclear(res);
    sprintf(query, "SELECT id FROM folios_tickets_1 WHERE venta=%d", num_venta);
    res = PQexec(base, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      fprintf(stderr,"ERROR: no encuentro registro de folio de ticket de la venta %d\n", num_venta);
      free(query);
      PQclear(res);
      return(ERROR_SQL);
    }
    *folio = atoi(PQgetvalue(res, 0, 0));
    break;
  }

  for (i=0; i<num_arts; i++) {
    /* Le damos la vuelta al problema de los apostrofos */
    /* en realidad deberia de reemplazarlos por la secuencia de escape y no
       por un espacio, pero eso me quitaria unos minutos de mi tiempo .=) */
    while ((ch_aux = index(art[i].desc, '\'')) != NULL)
      art[i].desc[ ch_aux - art[i].desc ] = ' ';
      
    strncpy(query, "INSERT INTO ventas_detalle (\"id_venta\", \"codigo\", \"cantidad\", \"descrip\", \"pu\", \"iva_porc\", \"tax_0\", \"tax_1\", \"tax_2\", \"tax_3\", \"tax_4\", \"tax_5\", \"util\")", 1024);
    sprintf(query, "%s VALUES (%d, '%s', %f, '%s', %f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f)",
            query, num_venta,
            art[i].codigo, art[i].cant, art[i].desc, art[i].pu, art[i].iva_porc,
            art[i].tax_0, art[i].tax_1, art[i].tax_2, art[i].tax_3, art[i].tax_4,
            art[i].tax_5, art[i].utilidad);
    res = PQexec(base_sup, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "Error al registrar detalle de ventas.\n%s\n", query);
      free(query);
      PQclear(res);
      return(ERROR_SQL);
    }
  }
  descto_aplicado = checa_descuento(base, num_venta, almacen);
  if (descto_aplicado)
    corte = "00010000";
  else
    corte = "00000000";

  sprintf(query, "INSERT INTO corte (numero,bandera) VALUES (%d, B'%s')", num_venta, corte);
  res = PQexec(base_sup, query);
  /* Comparamos compatibilidad con versiones anteriores */
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "Error al registrar ventas. Actualize su versión.\n");
    /* Tratamos de hacerlo al estilo antiguo */
    sprintf(query, "UPDATE ventas SET corte=B'%s' WHERE numero=%d", corte, num_venta);
    res = PQexec(base_sup, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "Error al registrar ventas.\n");
      free(query);
      PQclear(res);
      return(ERROR_SQL);
    }
  }

  free(query);

  return(num_venta);
}
 
/*********************************************************************/
long int registra_renta(PGconn *base, PGconn *base_sup,
                   char *tabla,
                   long id_cliente,
                   time_t f_pedido,
                   time_t f_entrega,
                   struct articulos art[maxart],
                   unsigned num_arts)
{
  char *query;
  char *s_pedido, *s_entrega;
  int i;
  long int id_renta;
  struct tm *tm_entrega;

  PGresult *db_res;

  query = calloc(1, mxbuff);
  s_pedido = calloc(1, mxbuff*2);
  s_entrega = calloc(1, mxbuff*2);

  strcpy(s_pedido, ctime(&f_pedido));
  s_pedido[strlen(s_pedido)-1] = 0;

  sprintf(query, "INSERT INTO %s (pedido, cliente, status) ", tabla);
  sprintf(query, "%s VALUES ('%s', %ld, B'00000000') ", query, s_pedido, id_cliente);

  db_res = PQexec(base, query);
  if (PQresultStatus(db_res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Error al insertar datos de rentas\n");
    PQclear(db_res);
    free(query);
    free(s_pedido);
    free(s_entrega);
    return(ERROR_SQL);
  }
  PQclear(db_res);

  sprintf(query, "SELECT id FROM %s WHERE pedido='%s' AND ", tabla, s_pedido);
  sprintf(query, "%s cliente=%ld AND status=B'00000000' ", query, id_cliente);

  db_res = PQexec(base, query);
  if (PQresultStatus(db_res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"ERROR: No encuentro registro de la renta\n");
    PQclear(db_res);
    free(query);
    free(s_pedido);
    free(s_entrega);
    return(ERROR_SQL);
  }
  id_renta = atol(PQgetvalue(db_res, 0, 0));
  PQclear(db_res);


  sprintf(query, "UPDATE cliente SET status = status|B'10000000' WHERE id=%ld ", id_cliente);

  db_res = PQexec(base, query);
  if (PQresultStatus(db_res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Error al actualizar datos de cliente\n");
    PQclear(db_res);
    free(query);
    free(s_pedido);
    free(s_entrega);
    return(ERROR_SQL);
  }
  PQclear(db_res);

  for (i=0; i<num_arts; i++) {

    if (f_entrega == 0) {
      tm_entrega = localtime(&f_pedido);

      switch (art[i].unidad_t) {
      case 0: 
        tm_entrega->tm_min += (art[i].t_renta);
        break;
      case 1:
        tm_entrega->tm_hour+= (art[i].t_renta);
        break;
      case 2:
        tm_entrega->tm_mday+= (art[i].t_renta);
        break;
      case 3:
        tm_entrega->tm_mday += (art[i].t_renta * 7);
        break;
      case 4:
        tm_entrega->tm_mon+= (art[i].t_renta);
        break;
      case 5:
        tm_entrega->tm_year+= (art[i].t_renta);
        break;
      }
      f_entrega = mktime(tm_entrega);
    }

    strcpy(s_entrega, ctime(&f_entrega));
    s_entrega[strlen(s_entrega)-1] = 0;

    sprintf(query, "INSERT INTO rentas_detalle (id, serie, f_entrega, costo, status) VALUES ");
    sprintf(query, "%s (%ld, '%s', '%s', %f, B'00000000') ", query,
            id_renta, art[i].serie, s_entrega, art[i].pu);

    db_res = PQexec(base, query);
    if (PQresultStatus(db_res) != PGRES_COMMAND_OK) {
      fprintf(stderr,"ERROR: Al agregar articulos rentados.\n");
      PQclear(db_res);
      free(query);
      free(s_pedido);
      free(s_entrega);
      return(ERROR_SQL);
    }
  }


  PQclear(db_res);
  free(query);
  free(s_pedido);
  free(s_entrega);
  return (id_renta);
}

/*********************************************************************/

PGresult *Agrega_en_Inventario(PGconn *base, char *tabla, struct articulos art)
{
  char *query;
  PGresult *resultado;

  query = calloc(1,mxbuff);
  sprintf(query,
          "INSERT INTO %s VALUES ('%s', '%s', %u, %u, %.2f, '%s', %.2f, '%3s', '%s')", 
          tabla, art.codigo, art.desc, 
          art.id_prov, art.id_depto,
          art.p_costo, art.prov_clave, art.iva_porc, art.divisa, art.codigo2);
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
  char aux_descr[maxdes], *tok_descr;
  PGresult *res;

  strncpy(aux_descr, art.desc, maxdes);
  strncpy(art.desc, strtok(aux_descr, "'"), maxdes);
  while ((tok_descr = strtok(NULL, "'"))  != NULL) {
    strncat(art.desc, "\\'", maxdes-strlen(art.desc)-2);
    strncat(art.desc, tok_descr, maxdes-strlen(art.desc));
  }
  tok_descr = NULL;
  comando_sql = calloc(1,mxbuff*2);
  sprintf(comando_sql,
         "UPDATE %s SET descripcion='%s', pu=%.2f, descuento=%.2f, cant=%f, min=%f, max=%f, id_prov1=%u, id_depto=%u, p_costo=%.2f, prov_clave='%s', iva_porc=%.2f, divisa='%3s', codigo2='%s', pu2=%.2f, pu3=%.2f, pu4=%.2f, pu5=%.2f WHERE codigo='%s'",
		  tabla, art.desc, art.pu, art.disc, art.exist,
		  art.exist_min, art.exist_max, art.id_prov,
		  art.id_depto, art.p_costo, art.prov_clave,
		  art.iva_porc, art.divisa, art.codigo2, art.pu2, art.pu3, art.pu4, art.pu5,
          art.codigo);
  res = PQexec(base, comando_sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
    fprintf(stderr, "Error: %s\n", PQerrorMessage(base));
  free(comando_sql);
  return(res);
}

/*********************************************************************/

PGresult *salida_almacen(PGconn *base, unsigned almacen, struct articulos art, char *usuario, struct tm *fecha)
{
  char *comando_sql;
  PGresult *res;
  int i;
  char dia[9];
  char hora[9];

  sprintf(dia, "20%d%2d%2d", fecha->tm_year, fecha->tm_mon, fecha->tm_mday);
  for (i=2; i<9; i+=2)
    if (dia[i] == ' ')
      dia[i] = '0';
  sprintf(hora, "%2d:%2d:%2d", fecha->tm_hour, fecha->tm_min, fecha->tm_sec);
  for (i=0; i<9; i+=2)
    if (dia[i] == ' ')
      dia[i] = '0';

  comando_sql = calloc(1,mxbuff*2);
  sprintf(comando_sql,
         "UPDATE almacen_1 SET cant=%f WHERE id_alm=%d AND codigo='%s'",
		  art.exist, almacen, art.codigo);
  res = PQexec(base, comando_sql);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
    fprintf(stderr, "Error: %s\n", PQerrorMessage(base));

  sprintf(comando_sql,
          "INSERT INTO mov_inv (tipo, codigo, ct, descr, pu, usuario, alm_orig, tiempo) VALUES (%d, %s, %.2f, %s, %.2f, %s, %u, '%s %s')",
          1, art.codigo, art.cant, art.desc, art.pu, usuario, almacen, dia, hora);
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

PGresult *search_product(PGconn *base, 
			      char *tabla,
			      char *campo,
			      char *llave,
                              int exacto,
			      struct articulos *art)
{
  char      *query;
  PGresult* res;

  res = PQexec(base,"BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
   fprintf(stderr,"Falló comando BEGIN al buscar en inventario\n");
   return(res);
  }
  PQclear(res);

  query = calloc(1,mxbuff*2);
  sprintf(query, "DECLARE cursor_arts CURSOR FOR SELECT ar.codigo, ar.descripcion, al.pu, al.pu2, al.pu3, al.pu4, al.pu5, al.cant, al.c_min, al.c_max, ar.id_prov1, ar.id_depto, ar.p_costo, ar.prov_clave, ar.iva_porc, ar.divisa, ar.codigo2 ");
  sprintf(query, "%s FROM %s al, articulos ar ",
          query, tabla);
  if (exacto)
    sprintf(query, "%s WHERE al.%s='%s' AND ar.%s='%s'", query, campo, llave, campo, llave);
  else
    sprintf(query, "%s WHERE al.%s~*'%s' AND ar.%s~*'%s'", query, campo, llave, campo, llave);
  res = PQexec(base, query);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr,"Fallo comando DECLARE CURSOR al buscar un artículo\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
    free(query);
    return(res);
  }
  PQclear(res);

  strcpy(query, "FETCH ALL in cursor_arts");
  res = PQexec(base, query);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"comando FETCH ALL no regresó registros apropiadamente\n");
    free(query);
    return(res);
  }


  if (PQntuples(res)) {
    strcpy(query, PQgetvalue(res,0,0));
    strncpy(art->codigo, query, maxcod);

    strcpy(query, PQgetvalue(res,0,1));
    strncpy(art->desc, query, maxdes);

    art->pu = atof(PQgetvalue(res,0,2));
    art->pu2 = atof(PQgetvalue(res,0, 3));
    art->pu3 = atof(PQgetvalue(res,0, 4));
    art->pu4 = atof(PQgetvalue(res,0, 5));
    art->pu5 = atof(PQgetvalue(res,0, 6));
    art->exist = atof(PQgetvalue(res,0,7));
    art->exist_min = atof(PQgetvalue(res,0,8));
    art->exist_max = atof(PQgetvalue(res,0, 9));
    art->id_prov = atoi(PQgetvalue(res,0,10));
    art->id_depto = atoi(PQgetvalue(res,0,11));
    art->p_costo = atof(PQgetvalue(res,0,12));
    strncpy(art->prov_clave, PQgetvalue(res, 0, 13), maxcod);
    art->iva_porc = atof(PQgetvalue(res, 0, 14));
    strncpy(art->divisa, PQgetvalue(res, 0, 15), 3);
    strncpy(art->codigo2, PQgetvalue(res, 0, 16), maxcod);
  }
  PQclear(res);

  /* close the portal */
  res = PQexec(base, "CLOSE cursor_arts");
  PQclear(res);

  /* end the transaction */
  res = PQexec(base, "END");
  PQclear(res);
  free(query);
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

/*********************************************************************/

short busca_proveedor(PGconn *base, char *proveedor) {
  char query[255];
  PGresult* res;


  sprintf(query, "SELECT id from proveedores WHERE nick~'%s'", proveedor);
  
  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr,"Falló comando %s\n", query);
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
	PQclear(res);
    return(ERROR_SQL);
  }

  if (PQntuples(res))
	return(atoi(PQgetvalue(res, 0, 0)));
  else
	return(0);
}

/*********************************************************************/

short busca_depto(PGconn *base, char *depto) {
  char query[255];
  PGresult* res;


  sprintf(query, "SELECT id from departamento WHERE nombre~'%s'", depto);
  
  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr,"Falló comando %s\n", query);
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
	PQclear(res);
    return(ERROR_SQL);
  }

  if (PQntuples(res))
	return(atoi(PQgetvalue(res, 0, 0)));
  else
	return(0);
}

/*********************************************************************/

short busca_movinvent(PGconn *base, char *movimiento) {
  char query[255];
  PGresult* res;


  sprintf(query, "SELECT id from tipo_mov_inv WHERE nombre~'%s'", movimiento);
  
  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr,"Falló comando %s\n", query);
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
	PQclear(res);
    return(ERROR_SQL);
  }

  if (PQntuples(res))
	return(atoi(PQgetvalue(res, 0, 0)));
  else
	return(0);
}

/*********************************************************************/

short busca_id(PGconn *base, char *aguja, char *pajar, char *campo) {
  char query[255];
  PGresult* res;


  sprintf(query, "SELECT id FROM %s WHERE %s='%s'", pajar, campo, aguja);
  
  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr,"Falló comando %s\n", query);
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
	PQclear(res);
    return(ERROR_SQL);
  }

  if (PQntuples(res))
	return(atoi(PQgetvalue(res, 0, 0)));
  else
	return(0);
}

/*********************************************************************/

int puede_hacer(PGconn *base, char *usuario, char *accion) {

  char query[255];
  PGresult* res;

  sprintf(query, "SELECT mod_usr.usuario FROM modulo, modulo_usuarios mod_usr ");
  sprintf(query, "%s WHERE modulo.id=mod_usr.id AND modulo.nombre='%s' AND usuario='%s' ", 
          query, accion, usuario);

  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "Error al consultar permisos del usuario\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
	PQclear(res);
    return(ERROR_SQL);
  }
  return(PQntuples(res));
}

int checa_descuento(PGconn *base, int num_venta, int almacen) {
  PGresult *res;
  char query[255];

  sprintf(query, "SELECT d.id_venta AS id, d.codigo, d.pu AS venta, a.pu AS precio FROM ventas_detalle d, almacen_1 a WHERE d.id_venta=%d AND d.codigo=a.codigo AND d.pu!=a.pu AND id_alm=%d", num_venta, almacen);

  res = PQexec(base, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "Error al consultar ventas\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base));
	PQclear(res);
    return(ERROR_SQL);
  }
  return(PQntuples(res));
}
