/* Part of the Maestro sequencer software package.
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


#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>

#ifndef _XMLUTILS
#define _XMLUTILS
xmlDocPtr XmlUtils_getdoc (char *_docname);

xmlXPathObjectPtr
XmlUtils_getnodeset (xmlChar *_xpathQuery, xmlXPathContextPtr _context);

void XmlUtils_resolve (char *_docname, xmlXPathContextPtr _context, char *_deffile, const char* _seq_exp_home);

#endif
