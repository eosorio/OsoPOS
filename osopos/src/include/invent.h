#ifndef maxbuf
#define maxbuf 255
#endif

unsigned Espacios(FILE* arch, unsigned nespacios);

void imprime_renglon(FILE *disp, char *datos[numdat])
{
  fprintf(disp, "%s", datos[0]);
  Espacios(disp, maxcod-strlen(datos[0]));
  fprintf(disp, "%s", datos[1]);
  Espacios(disp, maxdes-strlen(datos[1]));
  fprintf(disp, "%s", datos[2]);
  Espacios(disp, maxexistlong-strlen(datos[2]));
  fprintf(disp, "%s", datos[3]);
  Espacios(disp, maxexistlong-strlen(datos[3]));
  fprintf(disp, "%s", datos[4]);
  Espacios(disp, maxexistlong-strlen(datos[4]));
  fprintf(disp, "%s", datos[5]);
  Espacios(disp, maxexistlong-strlen(datos[5]));
  fprintf(disp, "%s", datos[6]);
  Espacios(disp, maxexistlong-strlen(datos[6]));
  fprintf(disp, "%s", datos[7]);
  Espacios(disp, maxcod-strlen(datos[7]));
  fprintf(disp, "\n");
}
