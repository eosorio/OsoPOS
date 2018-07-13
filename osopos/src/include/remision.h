#ifndef _POSVAR
#include "pos-var.h"
#define _POSVAR
#endif

typedef struct {
  int size;
  int availSlots;
  struct articulos *items;
} itemVector;

void itemsInit(itemVector *item) {
  // initialize size and capacity
  item->size = 0;
  item->availSlots = maxart;

  // allocate memory 
  item->items = malloc(sizeof(struct articulos) * item->availSlots);
}


void itemsAppend(itemVector *item, struct articulos art) {
  // make sure there's room to expand into
  itemsExpandIfFull(item);

  // append the value and increment item->size
  //item->items[item->size++] = art;
  memcpy(&item->items[item->size++], &art, sizeof(struct articulos));
}

struct articulos itemsGet(itemVector *item, int index) {
  if (index >= item->size || index < 0) {
    printf("Index %d out of bounds for vector of size %d\n", index, item->size);
    exit(1);
  }
  return item->items[index];
}

void itemsSet(itemVector *item, int index, struct articulos art) {
  // zero fill the vector up to the desired index
  //while (index >= item->size) {
    //itemsAppend(item, 0);
  //}

  // set the value at the desired index
  item->items[index] = art;
}

void itemsExpandIfFull(itemVector *item) {
  if (item->size >= item->availSlots) {
    // add item->capacity and resize the allocated memory accordingly
    item->availSlots += maxart;
    item->items = realloc(item->items, sizeof(struct articulos) * item->availSlots);
  }
}

void itemsFree(itemVector *item) {
  free(item->items);
}

double item_capture(PGconn *con, int *numart, double *util, 
                    double tax[maxtax],
                    struct tm fecha, char *program_name)
{
  double  subtotal = 0.0;
  double  iva_articulo;
  double  tax_item[maxtax];
  int     i=0,
          j,k;
  char    *buff, *buff2, *last_sale_fname;
  gchar   *g_buff = NULL;
  int     chbuff = -1;
  WINDOW  *v_arts;
  PANEL   *p_arts;
  FILE    *f_last_sale;
  int     exist_journal=0;
  FILE    *f_last_items;
  FILE    *p_impr;
  double  b_double = 0.0;
  short   es_granel = 0;

  *util = 0;
  iva = 0.0;

  p_arts = mkpanel(COLOR_BLACK, getmaxy(stdscr)-8, getmaxx(stdscr)-1, 4, 0);
  v_arts = panel_window(p_arts);
  //  v_arts = newwin(getmaxy(stdscr)-8, getmaxx(stdscr)-1, 4, 0);
  scrollok(v_arts, TRUE);
  buff = calloc(1,mxbuff);
  buff2 = calloc(1, mxbuff);
  last_sale_fname = calloc(1, strlen(home_directory) + strlen("/ultima_venta.tmp")+1);
  if (buff == NULL  || buff2 == NULL || last_sale_fname == NULL) {
    fprintf(stderr, "remision. FATAL: no puedo apartar %d bytes de memoria\n", mxbuff);
    /* can't allocate %d memory bytes */
    return(MEMORY_ERROR);
  }
  
  sprintf(last_sale_fname, "%s/ultima_venta.tmp", home_directory);
  f_last_sale = fopen(last_sale_fname, "w");
  if (f_last_sale == NULL)
    fprintf(stderr, "remision. No puedo escribir al archivo %s\n", last_sale_fname);
  else {
    fprintf(f_last_sale, "  %2d/%2d/%2d,  %2d:%2d:%2d hrs.\n",
            fecha.tm_mday, (fecha.tm_mon)+1, fecha.tm_year, fecha.tm_hour,
            fecha.tm_min, fecha.tm_sec);
    fclose(f_last_sale);
  }

#ifdef _SERIAL_COMM
  if (lector_serial)
    init_serial(&saio, &oldtio, &newtio);
#endif

  exist_journal = check_for_journal(home_directory, program_name);
  if (exist_journal) {
    sprintf(nm_journal, "%s/.last_items.%d", home_directory, exist_journal);
    f_last_items = fopen(nm_journal, "r");
  }
  else
    strcpy(nm_journal, nm_orig_journal);

  mvprintw(3,0,
           "Cant.       Clave                Descripción                 P. Unitario");
            /* qt.       code                description                 unit cost */
  articulo[0].cant = 1;

  do {
    if (g_buff == NULL)
      g_buff = g_malloc0(mxbuff);
    else
      memset(g_buff, 0, mxbuff);
    buff2[0] = 0;
    iva_articulo = 0.0;
    for (j=0; j<maxtax; j++)
      tax_item[j] = 0.0;
    articulo[i].pu = 0;
    articulo[i].p_costo = 0;
    mvprintw(getmaxy(stdscr)-3,0,"Código de barras, descripción o cantidad:\n");
    /* barcode, description or qty. */
    if (exist_journal) {
      if (feof(f_last_items)) {
        articulo[i].cant = 0;
        continue;
      }
      fgets(g_buff, mxbuff, f_last_items);
      g_buff[strlen(buff)-1] = 0;
    }
    else {
      do {
        switch(chbuff = wgetkeystrokes(stdscr, g_buff, mxbuff)) {
          //        case KEY_F(1):
        case 2: /* F2 */
          if (!puede_hacer(con, log_name, "caja_descuento_manual")) {
            mensaje_v("No esta autorizado para descuentos manuales. <Esc>", 
                      azul_sobre_blanco, ESC);
            continue;   
          }
          g_buff = g_strdup_printf("descuento");
          strcpy(articulo[i].desc, "descuento");
          chbuff = 0;
          break;
        case 3: /* F3 */
          journal_marked_items(nm_journal, "cancela", 0);
          cancela_articulo(v_arts, &i, &subtotal, &iva, tax, last_sale_fname);
          show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));
          mvprintw(getmaxy(stdscr)-3,0,"Código de barras, descripción o cantidad:\n");
          continue;
          break;
        case 4:
          sprintf(buff2, "%s %s", cmd_lp, lp_disp_ticket);
          p_impr = popen(buff2, "w");
          sprintf(buff2, "Eduardo Israel Osorio Hernandez");
          imprime_razon_social(p_impr, tipo_disp_ticket,
                               "elpuntodeventa", buff2);
          pclose(p_impr);

          print_ticket_header(nmfenc);
          sprintf(buff2, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
          p_impr = popen(buff2, "w");
          pclose(p_impr);

          g_buff = g_malloc0(mxbuff);
          //memset(buff, 0, mxbuff);

          break;
        case 5:
        case 6:
        case 7:
        case 8:
          if (!puede_hacer(con, log_name, "caja_descuento")) {
            mensaje_v("No autorizado para hacer descuentos. <Esc>",
                      azul_sobre_blanco, ESC);
            continue;   
          }
          if (i) {
            articulo[i].codigo[0] = 0;
            articulo[i].desc[0] = 0;
            switch_pu(v_arts, &articulo[i-1], chbuff-3, &subtotal, tax, i);
            move(getmaxy(stdscr)-2,0);
          }
          break;
        case 11:
          if (busqueda_articulo(g_buff) > 0)
            chbuff = 0;
          update_panels();
          doupdate();
          wrefresh(stdscr);
          break;
        case 12:
          if (puede_hacer(con, log_name, "caja_cajon_manual"))
            AbreCajon(tipo_disp_ticket);
            sprintf(buff2, "%s %s %s", cmd_lp, lp_disp_ticket, nm_disp_ticket);
            p_impr = popen(buff2, "w");
            pclose(p_impr);
          break;
        }
      }
      while (chbuff  && !(lector_serial && wait_flag == FALSE));
      if (lector_serial && wait_flag == FALSE) {
        //strcpy(buff, s_buf);
        g_buff = g_strdup_printf("%s", s_buf);
        
        wait_flag = TRUE;
      }

      //    getstr(buff);
    }
    chbuff = -1; /* Limpiamos para usar despues */

    if (!strcmp(g_buff,"descuento") && !puede_hacer(con, log_name, "descuento")) {
      mensaje_v("No esta autorizado para hacer descuentos manuales. <Esc>",
                azul_sobre_blanco, ESC);
      g_free(g_buff);
      g_buff = NULL;
      continue;   
    }
    if (strstr(articulo[i].desc,"ancela") == NULL)
      journal_marked_items(nm_journal, g_buff, exist_journal);

    move(getmaxy(stdscr)-2,0);
    deleteln();

    /* Sección de multiplicador */
    if ( ((toupper(g_buff[0])=='X') || (g_buff[0]=='*')) && i) {
      if (!isdigit(g_buff[1])) {
        mensaje("Error en multiplicador. Intente de nuevo");
        continue;
      }
      for (j=0; !isdigit(g_buff[j]) && g_buff[j]!='.'; j++);
      for (k=0; (isdigit(g_buff[j]) || g_buff[j]=='.') && j<strlen(g_buff); j++,k++)
        articulo[i].desc[k] = g_buff[j];
      articulo[i].desc[k]=0;
      articulo[i].cant = atof(articulo[i].desc);

      if (!articulo[i].cant) {
        f_last_sale = fopen(last_sale_fname, "a");
        if (f_last_sale != NULL) {
          fprintf(f_last_sale, "Cancelando renglones previos de %s\n", articulo[i-1].codigo);
          fclose(f_last_sale);
        }
         /* Cancela el último artículo */
        wmove(v_arts, v_arts->_cury-1,0);
        wclrtoeol(v_arts);
        wrefresh(v_arts);
        --i;
        subtotal -= (articulo[i].pu * articulo[i].cant);
        if (iva_incluido)
          iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
        else
          iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
        iva  -= (iva_articulo * articulo[i].cant);

        tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
        tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
        tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
        tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
        tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
        tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
        for (j=0; j<maxtax; j++)
          tax[j] -= (tax_item[j] * articulo[i].cant);
      }
      else {
        f_last_sale = fopen(last_sale_fname, "a");
        if (f_last_sale != NULL) {
          fprintf(f_last_sale, "Nueva cantidad: %.2f\n", articulo[i].cant);
          fclose(f_last_sale);
        }
        if (iva_incluido)
          iva_articulo = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].iva_porc/100 + 1);
        else
          iva_articulo = articulo[i-1].pu * articulo[i-1].iva_porc/100;
        tax_item[0] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_0/100 + 1);
        tax_item[1] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_1/100 + 1);
        tax_item[2] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_2/100 + 1);
        tax_item[3] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_3/100 + 1);
        tax_item[4] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_4/100 + 1);
        tax_item[5] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_5/100 + 1);

        subtotal += ((articulo[i].cant-articulo[i-1].cant) * articulo[i-1].pu);
        iva  += (iva_articulo * (articulo[i].cant-articulo[i-1].cant));
        for (j=0; j<maxtax; j++)
          tax[j]  += (tax_item[j] * (articulo[i].cant-articulo[i-1].cant));
        articulo[i-1].cant = articulo[i].cant;
      }
      if (i) {
        wmove(v_arts, v_arts->_cury-1, v_arts->_curx);
        show_item_list(v_arts, i-1, TRUE);
      }
      show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));

      articulo[i].desc[0] = 0;
      articulo[i].cant = 1;
      continue;
    }

    if (strlen(g_buff) > 1) {
    /* Si la clave es mayor de 1 caracteres, busca un código o repetición */

        /* Repetición de artículo */
        /* ¿ No debería de ser && en lugar de || ? */
      if (i && (!strcmp(g_buff,articulo[i-1].desc) || !strcmp(g_buff,articulo[i-1].codigo) || !strcmp(g_buff,articulo[i-1].codigo2)) ) {
        f_last_sale = fopen(last_sale_fname, "a");
        if (f_last_sale != NULL) {
          fprintf(f_last_sale, "Repitiendo código: %s\n", articulo[i-1].codigo);
          fclose(f_last_sale);
        }
        (articulo[i-1].cant)++;
        subtotal += articulo[i-1].pu;
        if (iva_incluido)
          iva_articulo = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].iva_porc/100 + 1);
        else
          iva_articulo = articulo[i-1].pu * (articulo[i-1].iva_porc/100);
        iva += iva_articulo;

        tax_item[0] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_0/100 + 1);
        tax_item[1] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_1/100 + 1);
        tax_item[2] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_2/100 + 1);
        tax_item[3] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_3/100 + 1);
        tax_item[4] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_4/100 + 1);
        tax_item[5] = articulo[i-1].pu - articulo[i-1].pu / (articulo[i-1].tax_5/100 + 1);
        for (j=0; j<maxtax; j++)
          tax[j] += tax_item[j];

        wmove(v_arts, v_arts->_cury-1, v_arts->_curx);
        show_item_list(v_arts, i-1, TRUE);
        show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));
        continue;
      }

       /* Reusamos chbuff */
      if (catalog_search || serial_num_enable) {
        /***********OJO********* 
         Colocar aqui una función para revisar que
         el artículo no se encuentre con status de rentado */
        /*if (lease_mode && serial_num_enable && esta_rentado(con, buff)) {
          // El artículo está rentado, mostrar aviso y regresar a captura de codigo
          }*/
        if (serial_num_enable && busca_art_marcado(g_buff, articulo, 1, 1)>=0) {
          mensaje_v("El articulo ya fué marcado anteriormente <Esc>",
                    azul_sobre_blanco, ESC);
          g_buff[0] = 0;
          continue;
        }
        chbuff = find_db_code(con, g_buff, &articulo[i], fecha.tm_wday, &es_granel);
      }
      else
        chbuff = find_mem_code(g_buff, &articulo[i], search_2nd, &es_granel);

      if (chbuff < 0) {
        strncpy(articulo[i].desc, g_buff, maxdes);
        articulo[i].iva_porc = TAX_PERC_DEF;
      }
    }

    /* El artículo no tiene código, puede ser un art. no registrado */
    /* o un descuento */
    if (chbuff<0) {
      if (strlen(g_buff) <= 1) {
        articulo[i].cant=0;
        continue;
      }
      if (strstr(articulo[i].desc,"ancela") && i) {
        cancela_articulo(v_arts, &i, &subtotal, &iva, tax, last_sale_fname);
        show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));
        continue;
      }
      do {
        wmove(stdscr, getmaxy(stdscr)-3,0);
        if (strlen(buff2) > 6)
          printw("Cantidad muy grande... ");   /* Quantity very big */
        /******************* OJO ***************************/
        /* Revisar esta parte, porque cuando se hace un descuento de mas de 6 digitos,
           tambien entra en esta condición */
        printw("Introduce precio unitario: "); /* Introduce unitary cost */
        clrtoeol();
        if (exist_journal) {
          fgets(buff2, mxbuff, f_last_items);
          buff2[strlen(buff2)-1] = 0;
        }
        else
          getnstr(buff2, mxbuff);
        if (strlen(buff2) > 6)
          continue;
        //        scanw("%f",&articulo[i].pu);
        deleteln();
      }
      while (strlen(buff2) > 6);

      articulo[i].pu = atof(buff2);


      if ( strstr(articulo[i].desc,"escuento") && i && !manual_discount) {
        journal_last_sale(i, last_sale_fname);
        journal_marked_items(nm_journal, buff2, exist_journal);

        b_double = 0.0; /* En esta variable vamos a calcular el descuento total en impuestos */
        if (articulo[i-1].iva_porc) {
          if (iva_incluido>0 || listar_neto>0)
            iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i-1].iva_porc/100 + 1);
          else
            iva_articulo = articulo[i].pu * (articulo[i-1].iva_porc/100);
        }
        else
          iva_articulo = 0;

        b_double += iva_articulo;

        if (articulo[i-1].tax_0)
          tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
        else
          tax_item[0] = 0;
        b_double += tax_item[0];

        if (articulo[i-1].tax_1)
          tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
        else
          tax_item[1] = 0;
        b_double += tax_item[1];

        if (articulo[i-1].tax_2)
          tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
        else
          tax_item[2] = 0;
        b_double += tax_item[2];

        if (articulo[i-1].tax_3)
          tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
        else
          tax_item[3] = 0;
        b_double += tax_item[3];

        if (articulo[i-1].tax_4)
          tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
        else
          tax_item[4] = 0;
        b_double += tax_item[4];

        if (articulo[i-1].tax_5)
          tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
        else
          tax_item[5] = 0;
        b_double += tax_item[5];

        if (listar_neto>0 && iva_incluido==0)
          articulo[i].pu = articulo[i].pu-b_double;

        articulo[i-1].pu += articulo[i].pu;
        articulo[i].codigo[0] = 0;
        articulo[i].desc[0] = 0;

        subtotal += (articulo[i-1].cant * articulo[i].pu);
        iva += (iva_articulo * articulo[i-1].cant);
        for (j=0; j<maxtax; j++)
          tax[j] += (tax_item[j] * articulo[i-1].cant);
        wmove(v_arts, v_arts->_cury-1, 0);
        show_item_list(v_arts, i-1, TRUE);
        show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));
        continue;
      }
      /* Artículo no registrado */
      strncpy(articulo[i].desc, g_buff, maxdes);
      strcpy(articulo[i].codigo,"Sin codigo"); /* No code */
      g_buff[0] = 0;

      if (!strstr(articulo[i].desc, "escuento")) {
        journal_marked_items(nm_journal, buff2, exist_journal);
        buff2[0] = 0;
        do {
          wmove(stdscr, getmaxy(stdscr)-3,0);
          if (strlen(buff2) > 6)
            printw("Cantidad muy grande... ");   /* Quantity very big */
          mvprintw(getmaxy(stdscr)-3,0,"Introduce porcentaje de I.V.A.: "); /* Introd. tax % */
          clrtoeol();
          if (exist_journal) {
            fgets(buff2, mxbuff, f_last_items);
            buff2[strlen(buff2)-1] = 0;
          }
          else
            getnstr(buff2, mxbuff);
          if (strlen(buff2) > 6)
            continue;
          articulo[i].iva_porc = atof(buff2);
          //        scanw("%f",&articulo[i].pu);
          deleteln();
        }
        while (strlen(buff2) > 6);

        clrtoeol();
        deleteln();
      }

      journal_last_sale(i, last_sale_fname);
      journal_marked_items(nm_journal, buff2, exist_journal);

      show_item_list(v_arts, i, TRUE);

      if (iva_incluido)
        iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
      else
        iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
      iva += (iva_articulo * articulo[i].cant);

      tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
      tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
      tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
      tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
      tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
      tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
      for (j=0; j<maxtax; j++)
        tax[j] += (tax_item[j] * articulo[i].cant);

      subtotal += (articulo[i].pu * articulo[i].cant);
      show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));
      articulo[++i].cant = 1;
    }
    else {  /* Articulo registrado */
      journal_last_sale(i, last_sale_fname);
      show_item_list(v_arts, i, TRUE);
      subtotal += (articulo[i].pu * articulo[i].cant);
      if (iva_incluido)
        iva_articulo =  articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
      else
        iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);
      iva += (iva_articulo * articulo[i].cant);

      tax_item[0] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_0/100 + 1);
      tax_item[1] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_1/100 + 1);
      tax_item[2] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_2/100 + 1);
      tax_item[3] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_3/100 + 1);
      tax_item[4] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_4/100 + 1);
      tax_item[5] = articulo[i].pu - articulo[i].pu / (articulo[i].tax_5/100 + 1);
      for (j=0; j<maxtax; j++)
        tax[j] += (tax_item[j] * articulo[i].cant);

      show_subtotal(subtotal, iva, subtotal+(!iva_incluido*iva));
      articulo[++i].cant = 1;
    }
  }
  while (i<maxitemr &&  articulo[i].cant);
  move(getmaxy(stdscr)-2,0);
  deleteln();
  if (i && !articulo[i-1].cant)
    i--;
  *numart = i;
  for (j=0; j<(*numart); j++) {
    articulo[j].utilidad = ((articulo[j].pu - articulo[j].p_costo) * articulo[j].cant);
    *util += (articulo[j].utilidad);
    if (iva_incluido)
      iva_articulo = articulo[j].pu - articulo[j].pu / (articulo[j].iva_porc/100 + 1);
    else
      iva_articulo = articulo[j].pu * (articulo[j].iva_porc/100);
    tax_item[0] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_0/100 + 1);
    tax_item[1] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_1/100 + 1);
    tax_item[2] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_2/100 + 1);
    tax_item[3] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_3/100 + 1);
    tax_item[4] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_4/100 + 1);
    tax_item[5] = articulo[j].pu - articulo[j].pu / (articulo[j].tax_5/100 + 1);
  }

  if (i) {
    attrset(COLOR_PAIR(amarillo_sobre_negro));
    attron(A_BOLD);
    mvprintw(getmaxy(stdscr)-5,0, "Total a pagar: %.2f", subtotal+(!iva_incluido*iva)); /* Sale total */
    attroff(A_BOLD);
    attrset(COLOR_PAIR(blanco_sobre_negro));
    clrtoeol();
  }

  g_free(g_buff);
  g_buff = NULL;
  free(buff2);
#ifdef _SERIAL_COMM
  if (lector_serial)
    close_serial(fd, &oldtio);
#endif
  return(subtotal+(!iva_incluido*iva));
}

