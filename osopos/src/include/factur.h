/*  -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-
factur.h vers. 0.10, biblioteca para el uso del módulo de facturación de OsoPOS
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

#ifndef mxbuff
#define mxbuff 128
#endif


FILE *baseclien;
FILE *basefact;
struct datoscliente cliente;
struct fech fecha;
struct articulos art[maxart];
char buffer;
struct tm *f;
/*time_t tiempo;*/

char /*nmimpre[mxbuff], */
      nmfact[mxbuff],
      tipoimp[mxbuff],
      lprimp[mxbuff],
      puerto_imp[mxbuff];
char *home_directory;

unsigned maxrenf;
unsigned numarticulos;
short    impresion_directa;

int  read_config();
int  RegistraFactura(unsigned numfact, struct articulos art[maxarts], PGconn *conexion);
int  CalculaIVA();
int  BuscaCliente(char *rfc, struct datoscliente *clien, PGconn *con);

/************************************************************************/

int read_config() {

  char *nmconfig;
  FILE *config;
  char *buf;   /* Cadena modificable para busqueda de tokens */
  char *valor;
  char *linea;
  int i;

  home_directory = calloc(1, 255);
  nmconfig = calloc(1, 255);
  config = popen("printenv HOME", "r");
  fgets(home_directory, 255, config);
  home_directory[strlen(home_directory)-1] = 0;
  pclose(config);

  strcpy(nmconfig, home_directory);
  strcat(nmconfig, "/.osopos/factur.config");

  strcpy(nmfact,"/tmp/factura");
  strcpy(tipoimp,"EPSON");
  strcpy(lprimp, "facturas");
  iva_porcentaje = 10;
  impresion_directa = 0;
  strcpy(puerto_imp, "/dev/lp");
  maxrenf = 20;

  config = fopen(nmconfig,"r");
  if (config != NULL) {         /* Si existe archivo de configuración */
    buf = calloc(1, mxbuff);
    valor = calloc(1, mxbuff);
    linea = calloc(1, mxbuff);
    //    *b = calloc(1, mxbuff);
    //    buf = *b;

    fgets(linea,mxbuff,config);
    while (!feof(config)) {
      linea [ strlen(linea) - 1 ] = 0;

      if (!strlen(linea) || linea[0] == '#') { /* Linea vacía o coment. */
        fgets(linea,mxbuff,config);
        continue;
      }

      strcpy(buf, linea);  /* Variable modificable */
      strcpy(valor, strtok(buf,"="));

        /* Busca parámetros de impresora y archivo de última venta */
      if (!strcmp(valor, "factura")) {
        strcpy(nmfact, strtok(NULL,"="));
      }
      /*      else if (!strcmp(valor,"registro")) {
        strcpy(nmlog, strtok(NULL,"="));
        }*/
      else if (!strcmp(valor,"renglones.articulos")) {
        maxrenf = atoi(strtok(NULL,"="));
      }
      else if (!strcmp(valor,"iva.porcentaje")) {
        iva_porcentaje = atof(strtok(NULL,"="));
      }
      else if (!strcmp(valor,"impresora.tipo")) {
        strcpy(tipoimp, strtok(NULL,"="));
        for (i=0; i<strlen(tipoimp); i++)
          tipoimp[i] = toupper(tipoimp[i]);
      }
      else if (!strcmp(valor,"impresion.directa")) {
        strcpy(linea, strtok(NULL, "="));
        impresion_directa = toupper(linea[0])=='S';
      }
      else if (!strcmp(valor,"impresora.lpr")) {
        strcpy(lprimp, strtok(NULL,"="));
      }
      else if (!strcmp(valor,"impresora.puerto")) {
        strcpy(puerto_imp, strtok(NULL,"="));
      }

      fgets(linea,mxbuff,config);
    }
    fclose(config);
    free(valor);
    free(buf);
    free(linea);
    free(nmconfig);
    return(0);
  }
  free(nmconfig);
  return(1);
}

/******************************************************************/

int BuscaCliente(char *rfc, struct datoscliente *cliente, PGconn *con) {
  PGresult *resultado;
  char     *query;

  query = calloc(1,mxbuff);
  if (query == NULL) {
    free(query);
    return(-1);
  }

  strcpy(query, "SELECT dom_calle,dom_numero,dom_inter,dom_col,dom_ciudad,dom_edo,dom_cp");
  strcat(query, " FROM facturas_ingresos WHERE rfc=");
  sprintf(query, "%s'%s' ORDER BY id DESC LIMIT 1", query, rfc);
  resultado = PQexec(con, query);

  if (resultado == NULL  || PQresultStatus(resultado) != PGRES_TUPLES_OK) {
    free(query);
    return(ERROR_SQL);
  }
  if (!PQntuples(resultado)) {
    strcpy(cliente->dom_calle, "Sin facturas emitidas");
  }
  else {
    cliente->nombre[0] = 0;
    strncpy(cliente->dom_calle, PQgetvalue(resultado,0,0), maxspcalle);
    strncpy(cliente->dom_numero,PQgetvalue(resultado,0,1), maxspext);
    strncpy(cliente->dom_inter, PQgetvalue(resultado,0,2), maxspint);
    strncpy(cliente->dom_col,   PQgetvalue(resultado,0,3), maxspcol);
    strncpy(cliente->dom_ciudad,PQgetvalue(resultado,0,4), maxspcd);
    strncpy(cliente->dom_edo,   PQgetvalue(resultado,0,5), maxspedo);
    cliente->cp = (unsigned) atoi(PQgetvalue(resultado,0,6));
    PQclear(resultado);
  }

  sprintf(query, "SELECT curp, nombre FROM clientes_fiscales WHERE rfc='%s'",
          rfc);
  resultado = PQexec(con, query);

  if (resultado == NULL  || PQresultStatus(resultado) != PGRES_TUPLES_OK) {
    free(query);
    return(ERROR_SQL);
  }
  if (!PQntuples(resultado))
    strcpy(cliente->nombre, "Registro nuevo");
  else {
    strncpy(cliente->curp,  PQgetvalue(resultado,0,0), maxcurp);
    strncpy(cliente->nombre, PQgetvalue(resultado,0,1), maxspc);
  }

  free(query);
  return(0);
}

/***********************************************************************/

int RegistraFactura(unsigned numfact, struct articulos art[maxarts],
                    PGconn *base) {
  PGresult *res;
  char     buff[mxbuff], query[mxbuff], fecha_iso[11];
  /* Tengo que hacerlos apuntadores en lugar de arreglos  .=P  */
  unsigned num_max;
  int i;

  strcpy(query, "SELECT max(id) FROM facturas_ingresos");
  res = PQexec(base, query);
  if (!res  ||  PQresultStatus(res) != PGRES_TUPLES_OK) {
    return(ERROR_SQL);
  }
  if (PQntuples(res)) {
    num_max = atoi(PQgetvalue(res, 0, 0));
  }
  else {
    num_max = 0;
  }
  PQclear(res);

  if (numfact  &&  numfact <= num_max) {
    fprintf(stderr, 
            "(OsoPOS) ADVERTENCIA: El folio %u puede estarse duplicando)",
            numfact);
  }
  numfact = numfact ? numfact : num_max+1;

  sprintf(fecha_iso, "%d-%2d-%2d", fecha.anio, fecha.mes, fecha.dia);
  if (fecha.mes < 10)
    fecha_iso[5] = '0';
  if (fecha.dia < 10)
    fecha_iso[8] = '0';
  strcpy(buff, "INSERT INTO facturas_ingresos (id, fecha,rfc,dom_calle,dom_numero,");
  strcat(buff, "dom_inter,dom_col,dom_ciudad,dom_edo,dom_cp,subtotal,iva) VALUES");
  sprintf(query, "%s (%d, '%s','%s','%s','%s','%s','%s','%s','%s',%u,%.2f,%.2f)",
          buff, numfact, fecha_iso, cliente.rfc, cliente.dom_calle, cliente.dom_numero,
          cliente.dom_inter, cliente.dom_col, cliente.dom_ciudad, cliente.dom_edo,
          cliente.cp,subtotal, iva);
  res = PQexec(base, query);
  if (!res  ||  PQresultStatus(res) != PGRES_COMMAND_OK) {
    return(ERROR_SQL);
  }
  PQclear(res);

  sprintf(query, "SELECT rfc from clientes_fiscales WHERE rfc='%s'",
          cliente.rfc);
  res = PQexec(base, query);
  if (!res  ||  PQresultStatus(res) != PGRES_TUPLES_OK) {
    return(ERROR_SQL);
  }
  if (!PQntuples(res)) {
    PQclear(res);
    sprintf(query, "INSERT INTO clientes_fiscales VALUES ('%s','%s','%s')",
            cliente.rfc, cliente.curp, cliente.nombre);
    res = PQexec(base, query);
    if (res == NULL  ||  PQresultStatus(res) != PGRES_COMMAND_OK)
      return(ERROR_SQL);
    PQclear(res);
  }

  i = 0;
  while (strlen(art[i].desc)) {
    strcpy(query, "INSERT INTO fact_ingresos_detalle (id_factura, codigo, concepto, cant, precio)");
    sprintf(buff, " VALUES (%u, '%s', '%s', %f, %f)",
            numfact, art[i].codigo, art[i].desc, art[i].cant, art[i].pu);
    strcat(query, buff);
    res = PQexec(base, query);
    if (res == NULL  ||  PQresultStatus(res) != PGRES_COMMAND_OK)
      return(ERROR_SQL);
    PQclear(res);
    i++;
  }
  return(0);
}

int CalculaIVA() {
  int i;
  double sumatoria=0.0;
  double mult_iva;

  iva = 0;
  for (i=0; i<numarticulos; i++) {
    mult_iva = art[i].iva_porc / 100;
    art[i].pu =  art[i].pu / (mult_iva+1);
    sumatoria += (art[i].pu * art[i].cant);
    iva += art[i].pu * mult_iva * art[i].cant;
  }
  subtotal = sumatoria;
  total = subtotal + iva;
  return(i);
}

