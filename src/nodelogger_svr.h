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
|  Projet  :  Unificaion de ocm runcontrol                                         |
|  Fichier :  process.h                                                            |
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

#include "LogServerUtil.h"

#ifndef NODELOGGER_SVR_H
#define NODELOGGER_SVR_H 

/*-----------------------------------------------------------------------
 *		Les routines utiles
 *-----------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  cade_logfiles    ()                                              |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (28 jul 2004)                           |
|                                                                                  |
|  Description :  our SIGHUP catcher routine                                       |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void cascade_logfiles();

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  termine          ()                                              |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  our SIGINT catcher routine                                       |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void termine  (); 

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  reapchild          ()                                            |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (28 jul 2004)                           |
|                                                                                  |
|  Description :  our SIGCLD catcher routine                                       |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void reapchild () ; 

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  process_cmds            ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (07 jul 2004)                           |
|                                                                                  |
|  Description :  Execute la commande de l'usager                                  |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void process_cmds ();

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  update_pidfile          ()                                       |
|                                                                                  |
|  Auteur      :  Francois , Patrick Andre (08 jul 2004)                           |
|                                                                                  |
|  Description :  Mise-a-jour le fichier pid                                       |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
extern void update_svr_file (int port);

#endif
