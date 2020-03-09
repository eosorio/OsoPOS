int lee_articulos(PGconn *base_inventario, PGconn *con_s, short almacen)
{
  char comando[1024];
/*  char *datos[7]; */
  int  i;
  int   nCampos;
  PGresult* res;

  sprintf(comando, "SELECT ");
  sprintf(comando, "%s al.codigo, ar.descripcion, al.codigo2, al.pu, ", comando);
  sprintf(comando, "%s al.pu2, al.pu3, al.pu4, al.pu5, al.cant,  al.c_min, ", comando);
  sprintf(comando, "%s al.tax_0, al.tax_1, al.tax_2, al.tax_3, al.tax_4, al.tax_5, ", comando);
  sprintf(comando, "%s ar.iva_porc, ar.p_costo, al.divisa, ar.granel ", comando);
  sprintf(comando, "%s FROM almacen_1 al, articulos ar WHERE al.codigo=ar.codigo AND id_alm=%d ", comando, almacen);
  res = PQexec(base_inventario, comando);
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"Error al consultar articulos para cargar en memoria\n");
    fprintf(stderr,"Error: %s\n",PQerrorMessage(base_inventario));
    PQclear(res);
    return(ERROR_SQL);
  }

  nCampos = PQnfields(res);

  for (i=0; i < PQntuples(res) && i<mxmembarra; i++) {
    strncpy(barra[i].codigo , PQgetvalue(res,i,0), maxcod-1);
    strncpy(barra[i].codigo2 , PQgetvalue(res,i,2), maxcod-1);
    strncpy(barra[i].desc, PQgetvalue(res,i,1), maxdes-1);

    barra[i].pu  = atof(PQgetvalue(res,i, 3));
    barra[i].pu2 = atof(PQgetvalue(res,i, 4));
    barra[i].pu3 = atof(PQgetvalue(res,i, 5));
    barra[i].pu4 = atof(PQgetvalue(res,i, 6));
    barra[i].pu5 = atof(PQgetvalue(res,i, 7));
    //    barra[i].disc = atof(PQgetvalue(res,i,3));
    barra[i].disc = 0;
    barra[i].exist = atof(PQgetvalue(res,i,8));
    barra[i].exist_min = atof(PQgetvalue(res,i,9));
    barra[i].p_costo = atof(PQgetvalue(res,i,17));
    barra[i].iva_porc = atof(PQgetvalue(res,i,16));
    barra[i].tax_0 = atof(PQgetvalue(res,i,10));
    barra[i].tax_1 = atof(PQgetvalue(res,i,11));
    barra[i].tax_2 = atof(PQgetvalue(res,i,12));
    barra[i].tax_3 = atof(PQgetvalue(res,i,13));
    barra[i].tax_4 = atof(PQgetvalue(res,i,14));
    barra[i].tax_5 = atof(PQgetvalue(res,i,15));
    strncpy(comando, PQgetvalue(res,i,19), 1);
    barra[i].granel = comando[0];
    strncpy(barra[i].divisa, PQgetvalue(res,i,18), MX_LON_DIVISA);
  }

  if (PQntuples(res) >= mxmembarra)  {
    fprintf(stderr, "ADVERTENCIA: Se ha excedido el m�ximo de art�culos en memoria, no fueron\n");
    fprintf(stderr, "             cargados todos los que existen en la base");
    sprintf(comando,
            "Se ha excedido el m�ximo de art�culos en memoria, se leyeron los primeros %d",
            mxmembarra);
    mensaje(comando);
  }
  PQclear(res);

  return(i);
}

/***************************************************************************/

int find_db_code(PGconn *con, char *cod, struct articulos *art, int wday, short *es_granel) {
  char query[1024];
  PGresult *res;

  /* colocar aqu� c�digo para buscar si el art�culo est� rentado */
  /* igm if (lease_mode && esta_rentado(con, cod)) {
    printw(msg, "El producto ya se encuentra rentado");
    return(ERR_ITEM);
  }
  */
  sprintf(query, "SELECT al.codigo, ar.descripcion, al.codigo2, ");
  if (!lease_mode)
    sprintf(query, "%s al.pu, al.pu2, al.pu3, al.pu4, al.pu5, al.cant,  al.c_min, ", query);
  else {
    sprintf(query, "%s r.p%d_1 as pu, r.p%d_2 as pu2, r.p%d_3 as pu3, r.p%d_4 as pu4, r.p%d_5 as pu5, al.cant,  al.c_min, ",
            query, wday, wday, wday, wday, wday);
  }
  sprintf(query, "%sal.tax_0, al.tax_1, al.tax_2, al.tax_3, al.tax_4, al.tax_5, ", query);
  sprintf(query, "%sar.iva_porc, ar.p_costo, al.divisa, s.id ", query);
  sprintf(query, "%sFROM almacen_1 al, articulos ar, articulos_series s ", query);
  if (lease_mode)
    sprintf(query, "%s, articulos_rentas r ", query );
  sprintf(query, "%sWHERE al.codigo=ar.codigo AND id_alm=%d ", query, almacen);

  if (serial_num_enable) {
    sprintf(query, "%s AND ar.codigo=s.codigo AND s.id='%s' " , query, cod);
    if (lease_mode)
      sprintf(query, "%sAND r.codigo=ar.codigo ", query);
  }
  else {
    sprintf(query, "%s AND al.codigo='%s' ", query, cod);
  }

  res = PQexec(con, query);
  if (PQresultStatus(res) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR: no puedo obtener datos del producto.\n");
    PQclear(res);
    return(ERROR_SQL);
  }

  if (!PQntuples(res)) {
    PQclear(res);
    // igm printw(msg, "El producto no est� registrado.");
    return(ERR_ITEM);
  }

  strncpy(art->codigo , PQgetvalue(res,0,0), maxcod-1);
  strncpy(art->codigo2 , PQgetvalue(res,0,2), maxcod-1);
  strncpy(art->desc, PQgetvalue(res,0,1), maxdes-1);

  art->pu  = atof(PQgetvalue(res,0, 3));
  art->pu2 = atof(PQgetvalue(res,0, 4));
  art->pu3 = atof(PQgetvalue(res,0, 5));
  art->pu4 = atof(PQgetvalue(res,0, 6));
  art->pu5 = atof(PQgetvalue(res,0, 7));
  //    barra[i].disc = atof(PQgetvalue(res,0,3));
  art->disc = 0;
  art->exist = atof(PQgetvalue(res,0,8));
  art->exist_min = atof(PQgetvalue(res,0,9));
  art->p_costo = atof(PQgetvalue(res,0,17));
  art->iva_porc = atof(PQgetvalue(res,0,16));
  art->tax_0 = atof(PQgetvalue(res,0,10));
  art->tax_1 = atof(PQgetvalue(res,0,11));
  art->tax_2 = atof(PQgetvalue(res,0,12));
  art->tax_3 = atof(PQgetvalue(res,0,13));
  art->tax_4 = atof(PQgetvalue(res,0,14));
  art->tax_5 = atof(PQgetvalue(res,0,15));
  strncpy(art->divisa, PQgetvalue(res,0,18), MX_LON_DIVISA);
  strncpy(art->serie, PQgetvalue(res, 0, 19), MX_LON_NUMSERIE);

  PQclear(res);
  return(TRUE);
}

/***************************************************************************/

int lee_divisas(PGconn *base_invent)
{
  char comando[256];

  int i;
  int nCampos;
  PGresult* res;
  int div_mxbuff=255;
  char buf[255];

  res = PQexec(base_invent, "SELECT * FROM divisas ");
  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    fprintf(stderr,"Fall� comando al tratar de leer divisas para colocar en memoria\n");
    PQclear(res);
    return(ERROR_SQL);
  }

  nCampos = PQnfields(res);

  for (i=0; i < PQntuples(res) && i<mxmemdivisa; i++) {
    strncpy(divisa[i].id, PQgetvalue(res,i,0), MX_LON_DIVISA);
    strncpy(buf, PQgetvalue(res,i,2), div_mxbuff);
    divisa[i].tc = atof(buf);
  }

  if (PQntuples(res) >= mxmemdivisa)  {
    fprintf(stderr, "ADVERTENCIA: Se ha excedido el m�ximo de divisas en memoria, no fueron\n");
    fprintf(stderr, "             cargadas todas las que existen en la base");
    sprintf(comando,
            "Se ha excedido el m�ximo de divisas en memoria, se leyeron las primeras %d",
            mxmembarra);
    mensaje(comando);
  }
  PQclear(res);
  return(i);
}

/***************************************************************************/
/* Actualiza las existencias de los art�culos que se vendieron en una sola operaci�n */
int actualiza_existencia(PGconn *base_inv, struct tm *fecha) {
  int      i,resurtir = 0;
  PGresult *res;
  char     tabla[255];
  float    pu;

  sprintf(tabla, "almacen_1");
  for(i=0; i<numarts; i++) {
    articulo[i].exist = -1;
    pu = articulo[i].pu;
    if (search_product(base_inv, tabla, "codigo", articulo[i].codigo, TRUE, &articulo[i]) != OK)
      continue;
    if (articulo[i].exist > articulo[i].exist_min  &&
                articulo[i].exist-articulo[i].cant <= articulo[i].exist_min) {
      aviso_existencia(articulo[i]);
      resurtir++;
    }
    articulo[i].exist -= articulo[i].cant;
    articulo[i].pu = pu;
    res = salida_almacen(base_inv, almacen, articulo[i], log_name, fecha);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      fprintf(stderr, "Error al actualizar art�culo %s %s\n",
                       articulo[i].codigo, articulo[i].desc);
    PQclear(res);
  }
  return(resurtir);
}


/***********************************************************/

int actualiza_carrito(PGconn *base_inv, struct tm *fecha) {
  int      i,resurtir = 0;
  PGresult *res;
  char     tabla[255];
  char     buf[255];
  float    pu;

  sprintf(tabla, "almacen_1");
  for(i=0; i<numarts; i++) {
    articulo[i].exist = -1;
    pu = articulo[i].pu;
    if (search_product(base_inv, tabla, "codigo", articulo[i].codigo, TRUE, &articulo[i]) != OK) {
      sprintf(buf, "Art�culo %s no existente en almacen", articulo[i].codigo);
      mensaje_v(buf, amarillo_sobre_azul, ESC);
      continue;
    }

    res = Agrega_en_Carrito(base_inv, articulo[i], log_name);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      sprintf(buf, "Error al actualizar art�culo %s %s en carrito\n",
              articulo[i].codigo, articulo[i].desc);
      fprintf(stderr, "%s", buf);
      mensaje_v(buf, azul_sobre_blanco, ESC);
    }
    PQclear(res);
  }
  return(resurtir);
}
/************************************************/

