/*------------------------------------------------------------------------------\
 |                      E N V I O N N E M E N T   C A N A D A                   |
 |                      -------------------------------------                   |
 | Centre Meteorologique Canadien                                               |
 | 2121 route transcanadienne                                                   |
 | Dorval , Quebec                                                              |
 | H9P 1J3                                                                      |
 |                                                                              |
 |                                                                              |
 | Projet   :  Affichage des images sur le web                                  |
 | Fichier  :  utile.c                                                          |
 | Auteur   :  Francois Patrick Andre                                           |
 | Date     :  4 oct 2002                                                       |
 | Version  :  1.0                                                              |
 |                                                                              |
 |------------------------------------------------------------------------------|
 | Description : Ce module est un utilitaire pour gerer les informations        |
 |                                                                              |
 | LANGAGE     : C                                                              |
 |                                                                              |
 | Constantes  : Aucune                                                         |
 |                                                                              |
 |------------------------------------------------------------------------------|
 | Modification   :                                                             |
 |                                                                              |
 |    Nom         :                                                             |
 |    Date        :                                                             |
 |    Description :                                                             |
 \------------------------------------------------------------------------------*/

 /*-------------------------------------------------------------------------------\
 |                      Inclusion des librairies                                  |
 \-------------------------------------------------------------------------------*/
#include  "LogServerUtil.h"
#include  "SeqUtil.h"

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    : Enlever_Espace    ()                                              |
|                                                                                  |
|  Creation    : Francois , Patrick Andre (15 nov 2001)                            |
|                                                                                  |
|  Description : Permet de stocker les donnees dans un fichier                     |
|                                                                                  |
|  Parametre(s): chaine : La chaine a retourner                                    |
|                Name   : La chaine a enlever les espaces                          |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
char *Enlever_Espace (char *Name ) {
   char  chaine[256],*CHAINE = NULL;
   int   indice , j = 0;

   for (indice = 0 ; indice < strlen (Name) ; indice++) {
     if (!isspace (Name[indice])) {
       chaine[j]  = Name[indice] ;
       j++;
     }
  }
  chaine[j]  = '\0' ;
  CHAINE     = strdup (chaine);
  return     CHAINE ;
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  match  ()                                                        |
|                                                                                  |
|  Creation    :  Francois , Patrick  Andre   (10 dec 2002)                        |
|                                                                                  |
|  Description :  Comparaison de deux chaines de caractere                         |
|                                                                                  |
|  Parametre   :  string  : La chaine constante                                    |
|                 pattern : La chaine a compare                                    |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
int match(const char *string, char *pattern) {
  int      status;
  regex_t  regx ;

  if (regcomp(&regx,pattern,REG_EXTENDED) != 0)
    return (0);
  status = regexec(&regx, string,(size_t) 0, NULL, 0);
  regfree (&regx);
  if (status != 0)
   return(0);

 return(1);
}
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  ToLower          ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (22 nov 2001)                           |
|                                                                                  |
|  Description :  convertir une chaine en minuscule                                |
|                                                                                  |
|  Parametre(s):  chaine : La chaine a mettre en minuscule                         |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
char *ToLower (char *chaine) {
  char Min[MAX_CAR],*min ;
  int i;

  for (i  = 0 ; i < strlen(chaine);i++)
    Min[i] = tolower(chaine[i]);
  Min[i] = '\0';
  min    = strdup (Min);
  return min ;
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  ToUpper          ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (22 nov 2001)                           |
|                                                                                  |
|  Description :  Convertir  une chaine en majuscule                               |
|                                                                                  |
|  Parametre(s):  chaine : La chaine a mettre en majuscule                         |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
char *ToUpper (char *chaine) {
  char Maj[MAX_CAR],*maj ;
  int i;

  for (i  = 0 ; i < strlen(chaine);i++)
    Maj[i] = toupper(chaine[i]);
  Maj[i] = '\0';
  maj    = strdup (Maj);
  return maj ;
}
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Create_buf_msg    ()                                             |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  Formater le message du buffer                                    |
|                                                                                  |
|  Parametre(s):  *split  : La chaine a coupe                                      |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void Create_buf_msg (char *split,int flag,char *buf) {

  memset(buf,'\0', sizeof buf);
  split = strtok (NULL,"\n");
  while (split != NULL) {
    if (!flag)
      strcpy(buf,split);
    else
      strcat (buf,split);
    strcat (buf,"\n");
    flag = 1 ;
    split = strtok (NULL,"\n");
  }
  buf[strlen(buf)] = '\0';
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  ToUpper          ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (22 nov 2001)                           |
|                                                                                  |
|  Description :  Convertir  une chaine en majuscule                               |
|                                                                                  |
|  Parametre(s):  chaine : La chaine a mettre en majuscule                         |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
/* example of buf
afsisul:/users/dor/afsi/sul/.ocm/hello_pat                                                                                                                    :01/2015:09:43:BOP_EX00       :ABORT: afsisul test msg center new 6                                                  
*/
void Set_buf_message (char *buf,char *firsin) {
  char *split,*usager = NULL,rep[MAX_CAR],*tmplog=NULL, *logdir=NULL;
  char *log_message   = NULL,*path = NULL;

  log_message = strdup (buf);
  printf( "Set_buf_message buf:%s\n", log_message );
  split  = strtok (log_message,":");
  usager = strdup (split);
  split  = strtok (NULL,":");
  tmplog = strdup (split);
  Create_buf_msg  (split,0,buf);
  tmplog = strdup (Enlever_Espace(tmplog));
  printf( "tmplog:%s\n", tmplog );
  logdir = SeqUtil_getPathBase(tmplog); 

  if (directoryExists (logdir)) {
    sprintf (firsin,"%s",tmplog);
    printf( "dir:%s exists firsin=%s!\n", logdir, firsin );
  } else {
    printf( "Experiment Log Directory=%s does not exists!\n", logdir );
  }
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Create_rep       ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  Creation de repertoire si celui-ci n'existe pas                  |
|                                                                                  |
|  Parametre(s):  rep : Le repertoire a creer                                      |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void Create_rep (char *rep) {
  struct stat   st ;

  if (lstat (rep,&st) < 0)
    mkdir (rep,00775);
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Retourn_Exp      ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  Retourne l'experience de l'usager                                |
|                                                                                  |
|  Parametre(s):  expr : Le nom de l'experience                                    |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
char *Retourn_Exp (char *expr) {
  char *split,*chaine = NULL,*chreturn =NULL;
 
  chaine = strdup (Enlever_Espace(expr));
  split  = strtok (chaine,"/");
  while (split != NULL) {
    chreturn = strdup (split);
    split = strtok (NULL,"/");
  }
  return chreturn;
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Exist_Exp        ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  Verifie si l'experience de l'usager existe                       |
|                                                                                  |
|  Parametre(s):  exp : L'experience de l'usager                                   |
|                 rep : Repertoire de recherche                                    |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
int Exist_Exp (char *exp,char *rep) {
  struct dirent *d;
  int    found = 0;
  DIR   *dp ;

  if ((dp = opendir (rep)) != NULL) {
    while ((d = readdir (dp)) != NULL && !found) {
      if (isalnum (d->d_name[0])) {
        if (match (exp,d->d_name))
          found = 1;
      }
    }
    closedir (dp) ;
  }
  return found ;
}
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  open_logfile     ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  Verifie si l'experience de l'usager existe                       |
|                                                                                  |
|  Parametre(s):  exp : L'experience de l'usager                                   |
|                 rep : Repertoire de recherche                                    |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void open_logfile(char *firsin,int *logfile) {

  if ((*logfile = open(firsin, O_WRONLY|O_APPEND|O_CREAT, 00666)) < 1 ){
    perror("server: can't open logfile");
    exit(1);
  }
}


int directoryExists( const char* path_name )
{
   DIR *pDir;
   int bExists = 0;

   if ( path_name == NULL) return 0;
   pDir = opendir (path_name);

   if (pDir != NULL)
   {
      bExists = 1;    
      (void) closedir (pDir);
   }

   return bExists;
}
