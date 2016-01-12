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


#ifndef _SEQ_NAMEVALUES
#define _SEQ_NAMEVALUES
typedef struct _SeqNameValues {
    char *name;
    char *value;
    struct _SeqNameValues *nextPtr;
} SeqNameValues;

typedef SeqNameValues *SeqNameValuesPtr;

void SeqNameValues_insertItem(SeqNameValuesPtr *listPtrPtr, char *name, char* value);
void SeqNameValues_deleteItem(SeqNameValuesPtr *listPtrPtr, char* name);
void SeqNameValues_printList(SeqNameValuesPtr listPtr);
char* SeqNameValues_getValue( SeqNameValuesPtr ptr, char* attr_name );
void SeqNameValues_setValue( SeqNameValuesPtr* ptr, char* attr_name, char* attr_value );
SeqNameValuesPtr SeqNameValues_clone(SeqNameValuesPtr listPtr);
void SeqNameValues_deleteWholeList(SeqNameValuesPtr *listPtrPtr);

/*

int SeqNameValues_isListEmpty(SeqNameValuesPtr listPtr);
*/
#endif
