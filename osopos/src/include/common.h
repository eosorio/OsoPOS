


int strToUpper(char *strOrigin) {
  int i;

  
  for (i=0; i<strlen(strOrigin); i++)
    strOrigin[i] = toupper(strOrigin[i]);
  return 0;
}
