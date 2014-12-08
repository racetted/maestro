/*--------------------------------------------------------------------------------*\
|                                                                                  |
|                     E N V I R O N N E M E N T   C A N A D A                      |
|                     ---------------------------------------                      |
|                                                                                  |
|  Centre Meteorologique Canadien                                                  |
|  2121 route Transcanadienne                                                      |
|  Dorval, Québec                                                                  |
|  H9P 1J3                                                                         |
|                                                                                  |
|                                                                                  |
|  Projet  :  Affichage des images web                                             |
|  Fichier :  utile.h                                                              |
|  Auteur  :  Francois , Patrick Andre                                             |
|  Date    :  7 juillet 2004                                                       |
|                                                                                  |
|----------------------------------------------------------------------------------|
|  Description : Fichier d'entete contenant des informations utiles                |
|                                                                                  |
|  Note       :                                                                    |
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

#ifndef LOGSERVERUTIL_H
#define LOGSERVERUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <pwd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/param.h>
#include <dirent.h>
#include <libgen.h>
#include <regex.h>
#include "initvar.h"

#ifdef sun
#include <strings.h>
#else
#ifdef sgi
#include <strings.h>
#else
#include <string.h>
#endif
#endif


/*--------------------------------------------------------------------------
 *		Definition des variables globales
 *-------------------------------------------------------------------------*/

#define  MAX_LINE       1024 
#define  MAX_CH         512 
#define  MAX_CAR        256 
#define  MAX_CONN       3000

  
/*-----------------------------------------------------------------------
 *		Les routines utiles
 *-----------------------------------------------------------------------*/

extern char *__loc1;             /* needed for regex */

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  match            ()                                              |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (22 nov 2001)                           |
|                                                                                  |
|  Description :  Comapre 2 chaines de caractere                                   |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern int match                  (const char *, char *);

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  ToLower          ()                                              |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (22 nov 2001)                           |
|                                                                                  |
|  Description :  Convertion d'une chaine en minuscule                             |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern char *ToLower        (char *) ;

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  ToUpper          ()                                              |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (22 nov 2001)                           |
|                                                                                  |
|  Description :  Convertion d'une chaine en majuscule                             |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern char *ToUpper (char *) ;

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Enlever_Espace          ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (07 july 2004)                          |
|                                                                                  |
|  Description :  Enleve des espaces dans une chaine                               |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern char *Enlever_Espace (char *);
/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Exist_Exp               ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (07 july 2004)                          |
|                                                                                  |
|  Description :  Verifie si l'experience de l'usager existe                       |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern int Exist_Exp (char *,char *);

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Create_rep              ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (07 july 2004)                          |
|                                                                                  |
|  Description :  Creation du repertoire si celui ci n'existe pas                  |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void Create_rep (char *) ;

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Retourn_Exp             ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (07 july 2004)                          |
|                                                                                  |
|  Description :  Retourne le nom de l'experience de l'usager                      |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern char *Retourn_Exp (char *) ;

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Create_buf_msg          ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (07 july 2004)                          |
|                                                                                  |
|  Description :  Creation du message de l'usager                                  |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void Create_buf_msg (char *,int,char * );

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  open_logfile            ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (08 july 2004)                          |
|                                                                                  |
|  Description :  Ouvre  un fichier                                                |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void open_logfile    (char *,int *) ;

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  Set_buf_message         ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (08 july 2004)                          |
|                                                                                  |
|  Description :  Modifier le buffer                                               |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void Set_buf_message (char *,char *);

extern int directoryExists (const char *);
#endif
