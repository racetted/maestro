/* expcatchup.c - "catchup" mechanism and functions used for flow control in the Maestro sequencer software package.
 * Copyright (C) 2011-2015  Operations division of the Canadian Meteorological Centre
 *                          Environment Canada
 *
 * Maestro is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * Maestro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "expcatchup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include "XmlUtils.h"
#include "SeqUtil.h"



/*****************************************************************************
* catchup:
* set or retrieve the experiement's catchup value
*
*
******************************************************************************/
const char* CATCHUP_XML_FILE = "/resources/catchup.xml";
const char* XML_NODE_NAME = "CATCHUP";
const char* CATCHUP_QUERY = "/CATCHUP/@value";

/*
 * retrieves the catchup value from the experiment catchup file $SEQ_EXP_HOME/resources/catchup.xml
 * returns default value 8 if xml file not found
 */
int catchup_get( char* _expHome) {
   
   char *catchupXmlFile = NULL;
   xmlDocPtr docPtr = NULL;
   xmlXPathObjectPtr result = NULL;
   xmlXPathContextPtr context = NULL;
   xmlNodeSetPtr nodeset = NULL;
   const xmlChar *nodeName = NULL;
   xmlNodePtr nodePtr = NULL;
   int catchupValue = CatchupNormal;

   catchupXmlFile = malloc( strlen( _expHome ) + strlen( CATCHUP_XML_FILE ) + 1 );
   sprintf( catchupXmlFile, "%s%s", _expHome, CATCHUP_XML_FILE );
   if ( access(catchupXmlFile, R_OK) == 0 ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "catchup_get(): loading xml file:%s\n", catchupXmlFile );
      docPtr = (xmlDocPtr)XmlUtils_getdoc( catchupXmlFile );
       /* parse the xml file */
      if ((docPtr = XmlUtils_getdoc(catchupXmlFile)) == NULL) {
         raiseError("Unable to parse file: %s\n", catchupXmlFile);
      }
      context = xmlXPathNewContext(docPtr);
      SeqUtil_TRACE(TL_FULL_TRACE, "catchup_get(): xml query:%s\n", CATCHUP_QUERY ); 
      if( (result = (xmlXPathObjectPtr) XmlUtils_getnodeset (CATCHUP_QUERY, context)) == NULL ) {
         raiseError("CATCHUP value not found in XML master file!\n");
      }
      
      nodeset = result->nodesetval;
      nodePtr = nodeset->nodeTab[0];
      nodeName = nodePtr->name;
      SeqUtil_TRACE(TL_FULL_TRACE, "catchup_get(): xml nodeName=%s\n", nodeName ); 
      if ( nodePtr->children != NULL ) {
         catchupValue = atoi( nodePtr->children->content );
         if ( catchupValue == 0 && strcmp( nodePtr->children->content, "0" ) != 0 ) {
            fprintf(stderr, "ERROR: invalid catchup value=%s found in xml file.\n", nodePtr->children->content );
            exit(1);
         }
      }

      xmlXPathFreeObject (result);
      xmlFreeDoc(docPtr);
   } else {
      SeqUtil_TRACE(TL_ERROR, "catchup_get(): xml file not found:%s\n", catchupXmlFile );
   }
   free( catchupXmlFile );  
   
   return catchupValue;
}

/* writes catchup value if $SEQ_EXP_HOME/resources/catchup.xml file
 * It writes the int value.
 */
void catchup_set( char* _expHome, int _catchupValue ) {
   
   char *catchupXmlFile = NULL;
   xmlTextWriterPtr writer;
   int rc;  
   
   SeqUtil_TRACE(TL_FULL_TRACE, "catchup_set(): experiment catchup set to %d\n", _catchupValue );
   if ( _catchupValue > CatchupMax ) {
      fprintf(stderr, "ERROR: invalid catchup value=%d, must be between [0-%d]\n", _catchupValue, CatchupMax );
      exit(1);
   }
   
   catchupXmlFile = malloc( strlen( _expHome ) + strlen( CATCHUP_XML_FILE ) + 1 );
   sprintf( catchupXmlFile, "%s%s", _expHome, CATCHUP_XML_FILE );

   SeqUtil_TRACE(TL_FULL_TRACE, "catchup_set(): catchupXmlFile=%s\n", catchupXmlFile );
   if( access( catchupXmlFile, F_OK ) == 1 && access( catchupXmlFile, W_OK ) == -1 ) {
      fprintf( stderr, "ERROR: writing xml file: %s.\n", catchupXmlFile );
      exit(1);
   }

   /* Create a new XmlWriter with no compression. */
   if ( (writer = xmlNewTextWriterFilename(catchupXmlFile, 0)) == NULL ) {
      fprintf( stderr, "ERROR: creating xml writer.\n" );
      exit(1);
   }
   
   /* start the document with default values */
   rc = xmlTextWriterStartDocument( writer, NULL, NULL, NULL );
   /* create the CATCHUP root element */
   rc = xmlTextWriterStartElement(writer, BAD_CAST XML_NODE_NAME);
   /* add the catchup value to the value attribute */
   rc = xmlTextWriterWriteFormatAttribute(writer, "value", "%d", _catchupValue);
    /* close the document, elements are automcatically closed */
    rc = xmlTextWriterEndDocument(writer);
    
   xmlFreeTextWriter(writer);
}

