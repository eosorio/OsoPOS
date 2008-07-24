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
unsigned almacen;
struct db_data db;
struct tm *f;
/*time_t tiempo;*/

char  *lprimp,
  *tipoimp,
  *nmfact,
  *nmfact,
  *disp_lector_serie;  /* Ruta al scanner de c. de barras serial */
char   s_divisa[3];       /* Designación de la divisa que se usa para cobrar en la base de datos */
double TAX_PERC_DEF; /* Porcentaje de IVA por omisión */
char *home_directory;
char *log_name;

int lector_serial; /* Cierto si se usa un scanner serial */
int serial_crlf=1; /* 1 si el scanner envia CRLF, 0 si solamente LF */
tcflag_t serial_bps=B38400; /* bps por omisión */
int wait_flag=TRUE;                    /* TRUE while no signal received */
char s_buf[255];    /* Buffer de puerto serie */
struct termios oldtio,newtio;  /* Parametros anteriores y nuevos de terminal serie */
struct sigaction saio;              /* definition of signal action */
int fd;            /* Descriptor de archivo de puerto serie */

unsigned maxrenf;
unsigned numarticulos;
short unsigned maxitemr;
int iva_incluido;

int  read_config();
int  RegistraFactura(unsigned numfact, struct articulos art[maxarts], PGconn *conexion);
int  CalculaIVA();
int  BuscaCliente(char *rfc, struct datoscliente *clien, PGconn *con);

/************************************************************************/

int init_config()
{
  FILE *env_process;

  home_directory = calloc(1, 255);
  log_name = calloc(1, 255);

  if (!(env_process = popen("printenv HOME", "r"))) {
    free(log_name);
    free(home_directory);
    return(PROCESS_ERROR);
  }
  fgets(home_directory, 255, env_process);
  home_directory[strlen(home_directory)-1] = 0;
  pclose(env_process);

  if (!(env_process = popen("printenv LOGNAME", "r"))) {
    free(log_name);
    free(home_directory);
    return(PROCESS_ERROR);
  }
  fgets(log_name, 255, env_process);
  log_name[strlen(log_name)-1] = 0;
  pclose(env_process);

  lprimp = calloc(1, strlen("facturas")+20);
  strcpy(lprimp, "facturas");

  disp_lector_serie = calloc(1, strlen("/dev/barcode")+1);
  strcpy(disp_lector_serie, "/dev/barcode");
  lector_serial = 0;

  tipoimp = calloc(1, strlen("EPSON")+20);
  strcpy(tipoimp, "EPSON");
 
  db.name= NULL;
  db.user = NULL;
  db.passwd = NULL;
  db.sup_user = NULL;
  db.sup_passwd = NULL;
  db.hostport = NULL;
  db.hostname = NULL;

  db.hostname = calloc(1, strlen("255.255.255.255"));
  db.name = calloc(1, strlen("elpuntodeventa.com"));
  db.user = calloc(1, strlen(log_name)+1);
  strcpy(db.user, log_name);
  db.sup_user = calloc(1, strlen("scaja")+1);
  db.hostport = calloc(1, strlen("54320"));

  maxitemr = 6;

  iva_incluido = 0;
  TAX_PERC_DEF = 15;
  strcpy(s_divisa, "MXP");

  almacen = 1;

  nmfact = calloc(1, strlen("/tmp/factura")+1);
  strcpy(nmfact,"/tmp/factura");
  iva_porcentaje = 10;
  maxrenf = 20;

  return(OK);
}

/************************************************************************/

int read_config() {

  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;
  //  struct kbd_struct teclado;

  nmconfig = calloc(1, 255);
  strncpy(nmconfig, home_directory, 255);
  strcat(nmconfig, "/.osopos/factur.config");

  config = fopen(nmconfig,"r");
  if (config != NULL) {         /* Si existe archivo de configuración */
    b = buff;
    fgets(buff,mxbuff,config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;
      if (!strlen(buff) || buff[0] == '#') { /* Linea vacía o coment. */
        if (!feof(config))
          fgets(buff,mxbuff,config);
        continue;
      }
      strncpy(buf, strtok(buff,"="), mxbuff);

        /* Busca parámetros de impresora y archivo de última venta */
      if (!strcmp(b, "factura")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(nmfact, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(nmfact, buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuracion %s\n", b);
      }
      /*      else if (!strcmp(b,"registro")) {
        strcpy(nmlog, strtok(NULL,"="));
        }*/
      else if (!strcmp(b,"renglones.articulos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        maxrenf = atoi(buf);
      }
      else if (!strcmp(b,"iva.porcentaje")) {
        iva_porcentaje = atof(strtok(NULL,"="));
      }
      else if (!strcmp(b,"impresora.tipo")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(tipoimp, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(tipoimp, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"impresora.lpr")) {
        strcpy(lprimp, strtok(NULL,"="));
      }
      if (!feof(config))
        fgets(buff,mxbuff,config);
    }
    fclose(config);
    if (aux != NULL)
      aux = NULL;
    free(nmconfig);
    return(0);
  }
  free(nmconfig);
  b = NULL;
  return(1);
}

/******************************************************************/

int read_global_config()
{
  char *nmconfig;
  FILE *config;
  char buff[mxbuff],buf[mxbuff];
  char *b;
  char *aux = NULL;
  int i_buf;
  
  nmconfig = calloc(1, 255);
  strncpy(nmconfig, "/etc/osopos/factur.config", 255);

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuración */
    b = buff;
    fgets(buff,mxbuff,config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;
      if (!strlen(buff) || buff[0] == '#') {
        if (!feof(config))
          fgets(buff,mxbuff,config);
        continue;
      }
      strncpy(buf, strtok(buff,"="), mxbuff);
        /* La función strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

      if (!strcmp(b, "impresora.lpr")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(lprimp, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(lprimp, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
       }
      else if (!strcmp(b,"impresora.tipo")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(tipoimp, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(tipoimp, buf);
          aux = NULL;
        }
        else
          fprintf(stderr,
                  "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.host")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostname, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostname,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.port")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.hostport, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.hostport,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"db.nombre")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.name, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.name,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      /*      else if (!strcmp(b,"db.usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
                  }*/
      else if (!strcmp(b,"db.sup_usuario")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        aux = realloc(db.sup_user, strlen(buf)+1);
        if (aux != NULL) {
          strcpy(db.sup_user,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuración %s\n",
                  b);
      }
      else if (!strcmp(b,"db.sup_passwd")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        db.sup_passwd = calloc(1, strlen(buf)+1);
        if (db.sup_passwd  != NULL) {
          strcpy(db.sup_passwd,buf);
          aux = NULL;
        }
        else
          fprintf(stderr, "factur. Error de memoria en argumento de configuracion %s\n",
                  b);
      }
      else if (!strcmp(b,"renglones.articulos")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        maxitemr = atoi(buf);
      }
      else if (!strcmp(b,"porcentaje_iva")) {
        strncpy(buf, strtok(NULL,"="), mxbuff);
        TAX_PERC_DEF = atoi(buf);
      }
      else if(!strcmp(b, "divisa")) {
        strncpy(buf, strtok(NULL,"="), MX_LON_DIVISA);
        strcpy(s_divisa, buf);
      }
     else if (!strcmp(b,"iva_incluido")) {
       strncpy(buf, strtok(NULL,"="), mxbuff);
       iva_incluido = atoi(buf);
     }
     if (!feof(config))
       fgets(buff,mxbuff,config);
    }
    fclose(config);
    if (aux != NULL)
      aux = NULL;
    free(nmconfig);
    b = NULL;
    return(0);
  }
  if (aux != NULL)
    aux = NULL;
  free(nmconfig);
  b = NULL;
  return(1);
}

/***************************************************************************/


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
    if (art[i].iva_porc > 0)
      mult_iva = art[i].iva_porc / 100;
    else
      mult_iva = 0;
    art[i].pu =  art[i].pu / (mult_iva+1);
    sumatoria += (art[i].pu * art[i].cant);
    iva += art[i].pu * mult_iva * art[i].cant;
  }
  subtotal = sumatoria;
  total = subtotal + iva;
  return(i);
}

