int verif_passwd(PGconn *con, gchar *login, gchar *passwd);

int verif_passwd(PGconn *con, gchar *login, gchar *passwd) {
  gchar *query, *us_passwd;
  char *enc_passwd;
  PGresult *db_r;

  query = g_strdup_printf("SELECT passwd FROM users WHERE \"user\"='%s'", login);
  db_r = PQexec(con, query);
  if (PQresultStatus(db_r) !=  PGRES_TUPLES_OK) {
    fprintf(stderr, "ERROR: no puedo consultar contraseñas.\n");
    PQclear(db_r);
    return(ERROR_SQL);
  }
  if (PQntuples(db_r) == 0)
	return(FALSE);

  us_passwd = g_strdup(PQgetvalue(db_r, 0, 0));
  enc_passwd = crypt(passwd, us_passwd);

  if (strcmp(enc_passwd, us_passwd))
	return(FALSE);
  else
	return(TRUE);

}
