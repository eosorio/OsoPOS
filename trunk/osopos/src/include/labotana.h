#include <stdio.h>

#ifndef ESC
#define ESC 27
#endif

void imprime_razon_social(FILE *disp)
{
  fprintf(disp,"%c%c%c      La Botana\n",
              ESC,'P',14);
  fprintf(disp,"%c%c%c     Fanny Nuricumbo Melchor\n",
                15,ESC,'M');
}