void show_item_list(WINDOW *vent, int i, short cr) {
  double iva_articulo;

  if (iva_incluido)
    iva_articulo = articulo[i].pu - articulo[i].pu / (articulo[i].iva_porc/100 + 1);
  else
    iva_articulo = articulo[i].pu * (articulo[i].iva_porc/100);

  mvwprintw(vent, vent->_cury,  0, "%3.2f %-15s", articulo[i].cant, articulo[i].codigo);
  mvwprintw(vent, vent->_cury, 22, "%-39s", articulo[i].desc);

  if (listar_neto && !iva_incluido)
    mvwprintw(vent, vent->_cury, 62, "$%8.2f", articulo[i].pu + iva_articulo);
  else
    mvwprintw(vent, vent->_cury, 62, "$%8.2f", articulo[i].pu);

  if (!articulo[i].iva_porc)
        wprintw(vent, "E");
  if (cr)
    wprintw(vent, "\n");
  wrefresh(vent);
}

void show_subtotal(double subtotal, double iva, double total) {
  int pos_y;

  pos_y = getmaxy(stdscr)-5;
  attrset(COLOR_PAIR(verde_sobre_negro));
  mvprintw(pos_y,0,"Subt. acumulado: $%8.2f",subtotal);
  mvprintw(pos_y, 35, "I.V.A: $%7.2f",iva);
  mvprintw(pos_y, 60, "Total: $%8.2f", total);
  attrset(COLOR_PAIR(blanco_sobre_negro));
  clrtoeol();
}
