#include <stdio.h>

#define ESC 27

void imprime_razon_social(FILE *disp)
{
  fprintf(disp,"%c%c%c     La Reina      \n",
              ESC,'P',14);
  fprintf(disp,"%c%c%c       Ricardo A. Chapa C%crdenas   \n",
                15,ESC,'M', 206);
}
