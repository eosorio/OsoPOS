#include <stdio.h>
#include <string.h>

#ifndef ESC
#define ESC 27
#endif

void imprime_razon_social(FILE *disp, char *miniprinter_type)
{
  if (strstr(miniprinter_type, "EPSON")) {
    //    fprintf(disp, "%ca%c%c!%cElectro Hogar\n", ESC, 1, ESC, 48);
    fprintf(disp, "%ca%c%c!%cElectro Hogar\n%c!%c", ESC, 1, ESC, 48, ESC, 0);
    fprintf(disp, "Francisco Javier Mondragón Villa\n%c!%c%ca%c", ESC, 1, ESC, 0);
  }
  else {
    fprintf(disp,"%c%c%c    Electro Hogar\n",
	    ESC,'P',14);
    fprintf(disp,"%c%c%c  Francisco Javier Mondrag%cn Villa\n",
	    15,ESC,'M', 186);
  }

}
