/*
   OsoPOS - Sistema Auxiliar para gestión de negocios 
   (C) 1999,2003,2017 Eduardo Osorio eduardo.osorio.ti@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

int strToUpper(char *strOrigin) {
  int i;

  
  for (i=0; i<strlen(strOrigin); i++)
    strOrigin[i] = toupper(strOrigin[i]);
  return 0;
}
