/* OsoPOS - Programa de inventario 1999 E. Israel Osorio */
/* Para compilar:							*/
/* gcc ~/fuente/invent.c -lcurses -lform -o ~/bin/inventario		*/

#include <stdio.h>
#ifndef _pos
#include "include/pos-curses.h"
#define _pos
#endif

#include <form.h>

#ifndef CTRL
#define CTRL(x)         ((x) & 0x1f)
#endif

#define QUIT            CTRL('Q')
#define ESCAPE          CTRL('[')
#define ENTER		10
#define BLANK           ' '        /* caracter de fondo */

#define amarillo_azul 3
#define normal 1
#define verde_negro 2

#define version "0.04-3"

int LeeConfig();
void AjustaModoTerminal(void);
void AjustaVentanaForma(void);
void ArticulosForma();
FIELD *CreaEtiqueta(int frow, int fcol, NCURSES_CONST char *label);
FIELD *CreaCampo(int pren, int pcol, int ren, int cols);
void MuestraForma(FORM *f, unsigned ren, unsigned col);
void BorraForma(FORM *f);
int form_virtualize(WINDOW *w);
int my_form_driver(FORM *form, int c);

void muestra();

WINDOW *warts, *wforma, *wmensaje;
struct articulos art;
char nminvent[mxbuff],
     nmlog[mxbuff];

int LeeConfig() {
  char nmconfig[] = "inventario.config";
  FILE *config;
  char buff[mxbuff], buf[mxbuff];
  char *b;
  int i;

  strcpy(nminvent,"articulos.osopos");
  strcpy(nmlog,"/home/OsoPOS/scaja/log");

  config = fopen(nmconfig,"r");
  if (config) {         /* Si existe archivo de configuración */
    b = buff;
    fgets(buff,sizeof(buff),config);
    while (!feof(config)) {
      buff [ strlen(buff) - 1 ] = 0;

      if (!strlen(buff) || buff[0] == '#') { /* Linea vacía o coment. */
        fgets(buff,sizeof(buff),config);
        continue;
      }

      strcpy(buf, strtok(buff,"="));
        /* La función strtok modifica el contenido de la cadena buff    */
        /* remplazando con NULL el argumento divisor (en este caso "=") */
        /* por lo que b queda apuntando al primer token                 */

        /* Busca parámetros de impresora y archivo de última venta */
      if (!strcmp(b,"articulos")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(nminvent,buf);
      }
      else if (!strcmp(b,"registro")) {
        strcpy(buf, strtok(NULL,"="));
        strcpy(nmlog,buf);
      }
      fgets(buff,sizeof(buff),config);
    }
    fclose(config);
    return(0);
  }
  return(1);
}

/* ************************************************************ */

void ErrorVentana(unsigned ren, unsigned col) {

  endwin();
  printf("ERROR: El tamaño de la pantalla de terminal es muy chico,\n");
  printf("se necesita al menos %u columnas y %u renglones", col, ren);
  exit(2);

}

/* ************************************************************ */

void MuestraAyudaForma(int ren, int col) {
  mvaddstr(ren,col,
   "Las teclas de flecha mueven el cursor a traves del campo\n");
  addstr("<Ctrl-Q> Terminar");
  addstr("<Ctrl-A> Agrega articulo");
  addstr("<Inicio> Primer campo (nombre)             ");
  addstr("<Fin>    Ultimo campo (RFC)\n");
  addstr("<Intro>  Siguiente campo            \n");
  addstr("<Ctrl-X> Borra el campo                    ");
  addstr("<Insert> Cambia modo sobreescribir/insertar\n");
}

/* ************************************************************ */

int form_virtualize(WINDOW *w)
{
  int  mode = REQ_INS_MODE;
  int         c = wgetch(w);

  switch(c) {
    case QUIT:
    case ESCAPE:
        return(MAX_FORM_COMMAND + 1);

    case CTRL('A'):
    case CTRL('T'):
    case CTRL('B'):
    case CTRL('D'):
        return(c);
    case KEY_NEXT:
    case CTRL('I'):
    case CTRL('N'):
/*    case CTRL('M'):   */
    case KEY_ENTER:
    case ENTER:
        return(REQ_NEXT_FIELD);
    case KEY_PREVIOUS:
    case CTRL('P'):
        return(REQ_PREV_FIELD);

    case KEY_HOME:
        return(REQ_FIRST_FIELD);
    case KEY_END:
    case KEY_LL:
        return(REQ_LAST_FIELD);

    case CTRL('L'):
        return(REQ_LEFT_FIELD);
    case CTRL('R'):
        return(REQ_RIGHT_FIELD);
    case CTRL('U'):
        return(REQ_UP_FIELD);
/*    case CTRL('D'):
        return(REQ_DOWN_FIELD);   */

   case CTRL('W'):
        return(REQ_NEXT_WORD);
/*    case CTRL('B'):
        return(REQ_PREV_WORD);*/
      break;
    case CTRL('S'):
        return(REQ_BEG_FIELD);
    case CTRL('E'):
        return(REQ_END_FIELD);

    case KEY_LEFT:
        return(REQ_LEFT_CHAR);
    case KEY_RIGHT:
        return(REQ_RIGHT_CHAR);
    case KEY_UP:
        return(REQ_UP_CHAR);
    case KEY_DOWN:
        return(REQ_DOWN_CHAR);

/*    case CTRL('M'):
        return(REQ_NEW_LINE);*/
    /* case CTRL('I'):
        return(REQ_INS_CHAR); */
    case CTRL('O'):
        return(REQ_INS_LINE);
    case CTRL('V'):
        return(REQ_DEL_CHAR);

    case CTRL('H'):
    case KEY_BACKSPACE:
        return(REQ_DEL_PREV);
    case CTRL('Y'):
        return(REQ_DEL_LINE);
    case CTRL('G'):
        return(REQ_DEL_WORD);

    case CTRL('C'):
        return(REQ_CLR_EOL);
    case CTRL('K'):
        return(REQ_CLR_EOF);
    case CTRL('X'):
        return(REQ_CLR_FIELD);
/*    case CTRL('A'):
        return(REQ_NEXT_CHOICE);         */
    case CTRL('Z'):
        return(REQ_PREV_CHOICE);

    case 331: /* Insert en teclado para PC */
    case CTRL(']'):
        if (mode == REQ_INS_MODE)
            return(mode = REQ_OVL_MODE);
        else
            return(mode = REQ_INS_MODE);
    default:
        return(c);
    }
}

/* ************************************************************ */

void ArticulosForma() {
  WINDOW *ven;
  FORM *forma;
  FIELD *campo[10];
  char etiqueta[mxbuff];
  int  finished = 0, c, i;
  int tam_ren, tam_col, pos_ren, pos_col;

  pos_ren = 0;
  pos_col = 0;
  strcpy(etiqueta,"Detalles de articulo");

  /* describe la forma */
  campo[0] = CreaEtiqueta(1, 0, "Codigo");
  campo[1] = CreaCampo(2, 0, 1, maxcod-2);
  /* Se acorta en dos posiciones el tamaño del campo de còdigo para que
     la forma quepa en una terminal de 80 columnas de ancho */
  campo[2] = CreaEtiqueta(1, maxcod-1, "Descripcion");
  campo[3] = CreaCampo(2, maxcod-1, 1, maxdes);
  campo[4] = CreaEtiqueta(1, maxcod+maxdes, "P.U.");
  campo[5] = CreaCampo(2, maxcod+maxdes, 1, maxpreciolong);
  campo[6] = CreaEtiqueta(1, maxcod+maxdes+maxpreciolong+1, "Existencia");
  campo[7] = CreaCampo(2, maxcod+maxdes+maxpreciolong+1, 1, maxexistlong);
  campo[8] = CreaEtiqueta(0,0,etiqueta);
  campo[9] = (FIELD *)0;

  forma = new_form(campo);

  /* Calcula y coloca la etiqueta a la mitad de la forma */
  scale_form(forma, &tam_ren, &tam_col);
  campo[8]->fcol = (unsigned) ((tam_col - strlen(etiqueta)) / 2);

/*  MuestraAyudaForma(tam_ren+pos_ren+3,0); */
/*  refresh(); */

  /* --------->  Aqui se hace la ventana */
  MuestraForma(forma, pos_ren, pos_col);
/*  ven = form_win(forma);    */
  raw();

  while (!finished)  {
    switch(form_driver(forma, c = form_virtualize(wforma))) {
      case E_OK:
        break;
      case E_UNKNOWN_COMMAND:
        for (i=1; i<=7; i+=2)
          campo[i]->buf[strlen(campo[i]->buf)-1] = 0;
          for (i=0; i<sizeof(art.codigo); art.codigo[i++]=' ');
          for (i=0; i<sizeof(art.desc); art.desc[i++]=' ');

          strcpy(art.codigo, campo[1]->buf);
          art.codigo [sizeof(art.codigo)-3] = ' ';
    /* Se corrije el recorte de tres bytes en el campo de còdigo de producto */
          
          strcpy(art.desc,campo[3]->buf);
          art.pu = atof(campo[5]->buf);
          art.exist = atoi(campo[7]->buf);
          finished = my_form_driver(forma, c);
/*          waddch(wforma, c); */
        break;
      default:
          beep();
        break;
    }
  }

  BorraForma(forma);

/*  for (i=2; i<=8; i+=2)
    if (campo[i]->buf[ strlen(campo[i]->buf)-1 ] == ' ')
      campo[i]->buf[ strlen(campo[i]->buf)-1 ] = 0;*/

  free_form(forma);
  for (c = 0; campo[c] != 0; c++)
      free_field(campo[c]);
  noraw();
  echo();
  /*  noraw(); Afecta el comportamiento de getch() y hace que se comporte
  como getchar(); */

  attrset(COLOR_PAIR(normal));
  clrtobot();
  raw();
}

/* ************************************************************ */

void AjustaModoTerminal(void)
{
  noraw();
  cbreak();
  noecho();
/*  scrollok(stdscr, TRUE);
  idlok(stdscr, TRUE);
  keypad(stdscr, TRUE);    */
}

/* ************************************************************ */

void AjustaVentanaForma(void)
{
  scrollok(wforma, TRUE);
  idlok(wforma, TRUE);
  keypad(wforma, TRUE);
}

/* ************************************************************ */


FIELD *CreaEtiqueta(int pren, int pcol, NCURSES_CONST char *etiqueta)
{
    FIELD *f = new_field(1, strlen(etiqueta), pren, pcol, 0, 0);

    if (f)
    {
        set_field_buffer(f, 0, etiqueta);
        set_field_opts(f, field_opts(f) & ~O_ACTIVE);
    }
    return(f);
}

 FIELD *CreaCampo(int frow, int fcol, int ren, int cols)
{
    FIELD *f = new_field(ren, cols, frow, fcol, 0, 0);

    if (f)
        set_field_back(f,COLOR_PAIR(amarillo_azul) | A_BOLD);
    return(f);
}

/* ************************************************************ */

void MuestraForma(FORM *f, unsigned pos_ren, unsigned pos_col)
{
    int ren, col;

    scale_form(f, &ren, &col);

    if ((wforma =newwin(ren, col, pos_ren, pos_col)) != (WINDOW *)0)
    {
        set_form_win(f, wforma);
        set_form_sub(f, derwin(wforma, ren, col, 1, 1));
/*      box(wforma, 0, 0); */
        keypad(wforma, TRUE);
    }
    else
      ErrorVentana(ren, col);

    if (post_form(f) != E_OK)
        wrefresh(wforma);
}

/* ************************************************************ */

void BorraForma(FORM *f)
{
    WINDOW      *s = form_sub(f);

    unpost_form(f);
    werase(wforma);
    wrefresh(wforma);
    delwin(s);
    delwin(wforma);
}

/* ************************************************************ */

int my_form_driver(FORM *form, int c)
{
    if (c == (MAX_FORM_COMMAND + 1)
                && form_driver(form, REQ_VALIDATION) == E_OK)
        return(TRUE);
    else
    {
      switch(c) {
        case CTRL('A'):
          AgregaArt(nmlog,art);
          break;
        case CTRL('T'):
          muestra();
          break;
        case CTRL('D'):
          ModificaArt(nmlog,art);
          break;
        default:
           beep();
      }
      return(FALSE);
    }
}

/*********************************************************************/

int LeeParaBorrar(char *codigo[10], char buffer[20][mxbuff]) {
  int i=0, j;
  int borrar;
  char codart[20][maxcod];
  char buff[20][mxbuff];
  FILE *arch;

  fgets(&codart[i][0], maxcod, arch);
  while(!feof(arch) && i<20) {
    borrar = 0;
    fgets(&buff[i][0], maxdes+maxpreciolong+maxexistlong+1, arch);
    if(feof(arch))
      continue;
    for (j=0; j<10; j++) {
      if (borrar  =  strstr(codart[i], codigo[j]) != NULL)
        continue;
    }
    strcpy(buffer[i], codart[i]);
    strcat(buffer[i], buff[i]);
    if (!borrar)
      i++;
    fgets(&codart[i][0], maxcod, arch);
  }
  return(i);
}

/* ************************************************************ */

int BorraArts(char *cod[10]) {

  int i;
  char buff[20][mxbuff];

  do {
    i = LeeParaBorrar(cod, buff);
/*    EscribeBuffer(buff);    */





  }
  while(i<20);

}


void muestra() {
/* Muestra todos los artìculos */

char buff[mxbuff];
FILE *finvent;
int i=0;

/* Se puede calcular el tamaño de la ventana con _maxy-_begy */
  
  finvent = fopen(nmlog,"r");
  fgets(buff,sizeof(buff),finvent);
  wmove(warts,0,0);
  while (!feof(finvent)) {
    if (buff[0] == '#') {
      fgets(buff,sizeof(buff),finvent);
      continue;
    }
    buff[strlen(buff)-1] = 0;
    if (warts->_maxy  <=  warts->_cury+1)
    {
      mvwprintw(wmensaje,0,0,"Presione barra espaciadora para mas artículos...");
      wrefresh(wmensaje);
      wgetch(wmensaje);
      wmove(wmensaje, 0,0);
      wclrtoeol(wmensaje);
      wrefresh(wmensaje);
      
      wclear(warts);
      wmove(warts, 0, 0);
      wrefresh(warts);
    }
      wprintw(warts,"%s\n",buff);
      wrefresh(warts);

    fgets(buff,sizeof(buff),finvent);
  }
  fclose(finvent);
}


main() {
char codigo[maxcod];
int existencia;

  LeeConfig();
  strcat(nmlog, "/");
  strcat(nmlog,nminvent);
   
  initscr();
  start_color();
  init_pair(amarillo_azul, COLOR_YELLOW, COLOR_BLUE);
  init_pair(verde_negro, COLOR_GREEN, COLOR_BLACK);
  init_pair(normal, COLOR_WHITE, COLOR_BLACK);
  AjustaModoTerminal();

  warts =  newwin(LINES-7,COLS-1,5,0);
  scrollok(warts, TRUE);
  
  wmensaje = newwin(2, COLS-1, LINES-2, 0);
  scrollok(wmensaje, FALSE);
  mvwprintw(wmensaje,1,0, "<Ctrl-A>Agrega   <Ctrl-T>muestra Todos   ");
  wprintw(wmensaje, "<Ctrl-D>moDifica");
  wrefresh(wmensaje);

  AjustaVentanaForma();
  ArticulosForma();
  endwin();

}

/* Pendientes: Hacer que despliegue un mensaje de error al no poder abrir un
   archivo */


/*
int scroll(win)
 This function will scroll up the window (and the lines in the data structure) one 
 line.

int scrl(n)
 int wscrl(win, n)
 These functions will scroll the window or win up or down depending on the value of the
 integer n. If n is positive the window will be scrolled up n lines, otherwise if n is
 negative the window will be scrolled down n lines.

int setscrreg(t, b)
 int wsetscrreg(win, t, b)
 Set a software scrolling region.

*/
