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

#include <openssl/sha.h>

int verif_passwd(PGconn *con, gchar *login, gchar *passwd) {
  gchar *query, *us_passwd;
  unsigned char hash[20];
  char hashed_passwd[41];
  char hex_char[3];
  PGresult *db_r;
  SHA_CTX ctx; 
  int i=0;

  query = g_strdup_printf("SELECT passwd FROM users WHERE \"user\"='%s'", login);
  db_r = PQexec(con, query);
  if (PQresultStatus(db_r) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR: El usuario %s no puede consultar usuarios.\n", PQuser(con));
    PQclear(db_r);
    return(ERROR_SQL);
  }
  if (PQntuples(db_r) == 0)
	return(FALSE);

  SHA1_Init(&ctx);
  SHA1_Update(&ctx, passwd, strlen(passwd));
  SHA1_Final(hash, &ctx);

  hashed_passwd[0] = 0;
  for (i=0; i<20; i++) {
    sprintf(hex_char, "%02x", hash[i]);
    hashed_passwd[i*2] = hex_char[0];
    hashed_passwd[i*2+1] = hex_char[1];
  }
  hashed_passwd[40] = 0;

  us_passwd = g_strdup(PQgetvalue(db_r, 0, 0));
  //  enc_passwd = crypt(passwd, "$1$ch8wq$");

  if (strcmp(hashed_passwd, us_passwd))
	return(FALSE);
  else
	return(TRUE);

}
