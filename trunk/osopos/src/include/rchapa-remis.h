#include <stdio.h>
#include <string.h>

#ifndef ESC
#define ESC 27
#endif

void imprime_razon_social(FILE *disp, char *miniprinter_type)
{
  if (strstr(miniprinter_type, "EPSON")) {
    fprintf(disp, "%ca%c%c!%cLa Reina\n%c!%c", ESC, 1, ESC, 48, ESC, 0);
    fprintf(disp, "Ricardo Alejandro Chapa Ramos\n%c!%c%ca%c", ESC, 1, ESC, 0);
  }
  else {
    fprintf(disp,"%c%c%c         La Reina \n",
	    ESC,'P',14);
    fprintf(disp,"%c%c%c   Ricardo Alejandro Chapa Ramos\n",
	    15,ESC,'M');
  }

}
