/*   -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 print-func.h 0.1-1 Biblioteca de funciones de impresi�n para OsoPOS.
        Copyright (C) 1999,2000 Eduardo Israel Osorio Hern�ndez

        Este programa es un software libre; puede usted redistribuirlo y/o
modificarlo de acuerdo con los t�rminos de la Licencia P�blica General GNU
publicada por la Free Software Foundation: ya sea en la versi�n 2 de la
Licencia, o (a su elecci�n) en una versi�n posterior.

        Este programa es distribuido con la esperanza de que sea �til, pero 
SIN GARANTIA ALGUNA; incluso sin la garant�a impl�cita de COMERCIABILIDAD o
DE ADECUACION A UN PROPOSITO PARTICULAR. V�ase la Licencia P�blica General
GNU para mayores detalles.

        Deber�a usted haber recibido una copia de la Licencia P�blica General
GNU junto con este programa; de no ser as�, escriba a Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA02139, USA.
 */

#include<stdio.h>


/*********************************************************************/
/* Imprime espacios en al archivo ar */
unsigned Espacios (FILE* ar, unsigned n) {

  unsigned i;

  for (i=0; i<n; ++i)
    fprintf(ar, " ");
  return(i);
}

/*********************************************************************/

void espacio(char *linea, unsigned num_espacios)
{
  int i;

  for (i=0; i<num_espacios; i++)
    strcat(linea, " ");
}

/*********************************************************************/

/* Avanza n/216" en modo Epson */
void avance_vert(FILE *disp, unsigned avance, unsigned mode)
{
  if (mode==0 || mode==1)
    fprintf(disp, "%c%c%c", ESC, 'J', avance);
}

/*********************************************************************/

/* Escoge tama�o de letra */
void select_pitch(FILE *disp, short unsigned pitch, unsigned mode)
{

  fprintf(disp, "%c", 18);

  if (pitch>12)
    fprintf(disp, "%c", 15);

  if (mode == 0) { /* Epson */
    switch(pitch) {
      case 10:
      case 17:
        fprintf(disp, "%cP", ESC);
      break;
      default:
        fprintf(disp ,"%cM",ESC);
    }
  }
  else if(mode==1) { /* IBM */
    switch(pitch) {
      case 10:
      case 17:
        fprintf(disp, "%c", 18);
        break;
        default:
      fprintf(disp, "%c%c", ESC, 58);
    }
  }
}

/*********************************************************************/

/* Escoge tipo de caracteres internacionales */
void select_charset(FILE *disp, short unsigned chset, unsigned mode)
{
  if (mode == 0) {
    fprintf(disp, "%cR%c", ESC, chset);
  }
}

/*********************************************************************/

/* Avanza n l�neas de papel, donde n[0..127] */
void feed_paper(FILE *disp, short lines, unsigned mode)
{
  int i;

  if (mode ==0)
    fprintf(disp, "%c%c%c%c", 27,102,1,lines);
  else
    for (i=0; i<lines; i++, fprintf(disp, "\r"));
}
