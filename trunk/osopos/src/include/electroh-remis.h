#include <stdio.h>

#define ESC 27

void imprime_razon_social(FILE *disp)
{
  fprintf(disp,"%c%c%c   Electro Hogar\n",
              ESC,'P',14);
  fprintf(disp,"%c%c%c    Francisco Javier Mondrag%cn Villa\n",
                15,ESC,'M', 186);
}