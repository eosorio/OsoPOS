/* OsoPOS - Programa auxiliar para conversion de archivos */
/* Este programa no serà ditribuido a usuario final       */
/* Para compilar:							*/
/* gcc ~/fuente/convierte.c -lcurses -o ~/bin/convierte	-lpq */

#include <stdio.h>
#ifndef _pos
#include "include/pos-curses.h"
#define _pos
#endif

#include <libpq-fe.h>


main() {
  int  i;
  char nmviejo[] = "/home/OsoPOS/caja/precios.osopos",
       buff[maxcod+maxdes+maxpreciolong+maxexistlong+1],
       sprecio[maxpreciolong];
  
  struct articulos art;
  FILE *fant;
  double pu;
  PGconn *base_inv;

  fant = fopen(nmviejo, "r");
  base_inv = Abre_Base_Inventario();
  
  fgets(art.codigo, sizeof(art.codigo), fant);
  art.codigo[ strlen(art.codigo)-1 ] = 0;

  while (!feof(fant)) {
    fgets(art.desc, maxdes, fant);
    art.desc[ strlen(art.desc)-1 ] = 0;
    fgets(sprecio, maxpreciolong, fant);
    sprecio[ strlen(sprecio)-1 ] = 0;
    art.pu = atof(sprecio);

    art.exist = 1000;
    art.exist_min = 0;
    art.exist_max = 0;
    strcpy(art.cod_prov, "ACTUALIZAR");
    art.disc = 0.0;

    Agrega_en_Inventario(base_inv, "articulos", art);
    fgets(art.codigo, maxcod, fant);
    art.codigo[ strlen(art.codigo)-1 ] = 0;
  }
  fclose(fant);
  PQfinish(base_inv);
}
