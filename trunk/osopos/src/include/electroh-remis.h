#include <stdio.h>
#include <string.h>

#ifndef ESC
#define ESC 27
#endif

void imprime_razon_social(FILE *disp, char *miniprinter_type, char *s1, char *s2)
{
  if (strstr(miniprinter_type, "EPSON")) {
    fprintf(disp, "%ca%c%c!%c%s\n%c!%c", ESC, 1, ESC, 48, s1, ESC, 0);
    fprintf(disp, "%s\n%c!%c%ca%c", s2, ESC, 1, ESC, 0);
  }
  else {
    fprintf(disp,"%c%c%c%s\n", ESC,'P',14, s1);
    fprintf(disp,"%c%c%c%s\n", 15,ESC,'M', s2);
  }

}

