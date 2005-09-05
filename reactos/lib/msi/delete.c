/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002-2005 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"

#include "query.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


/*
 * Code to delete rows from a table.
 *
 * We delete rows by blanking them out rather than trying to remove the row.
 * This appears to be what the native MSI does (or tries to do). For the query:
 *
 * delete from Property
 *
 * some non-zero entries are left in the table by native MSI.  I'm not sure if
 * that's a bug in the way I'm running the query, or a just a bug.
 */

typedef struct tagMSIDELETEVIEW
{
    MSIVIEW        view;
    MSIDATABASE   *db;
    MSIVIEW       *table;
} MSIDELETEVIEW;

static UINT DELETE_fetch_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT *val )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %d %d %p\n", dv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT DELETE_fetch_stream( struct tagMSIVIEW *view, UINT row, UINT col, IStream **stm)
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %d %d %p\n", dv, row, col, stm );

    return ERROR_FUNCTION_FAILED;
}

static UINT DELETE_set_int( struct tagMSIVIEW *view, UINT row, UINT col, UINT val )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %d %d %04x\n", dv, row, col, val );

    return ERROR_FUNCTION_FAILED;
}

static UINT DELETE_insert_row( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %p\n", dv, record );

    return ERROR_FUNCTION_FAILED;
}

static UINT DELETE_execute( struct tagMSIVIEW *view, MSIRECORD *record )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;
    UINT r, i, j, rows = 0, cols = 0;

    TRACE("%p %p\n", dv, record);

    if( !dv->table )
         return ERROR_FUNCTION_FAILED;

    r = dv->table->ops->execute( dv->table, record );
    if( r != ERROR_SUCCESS )
        return r;

    r = dv->table->ops->get_dimensions( dv->table, &rows, &cols );
    if( r != ERROR_SUCCESS )
        return r;

    TRACE("blanking %d rows\n", rows); 

    /* blank out all the rows that match */
    for( i=0; i<rows; i++ )
        for( j=1; j<=cols; j++ )
            dv->table->ops->set_int( dv->table, i, j, 0 );

    return ERROR_SUCCESS;
}

static UINT DELETE_close( struct tagMSIVIEW *view )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p\n", dv );

    if( !dv->table )
         return ERROR_FUNCTION_FAILED;

    return dv->table->ops->close( dv->table );
}

static UINT DELETE_get_dimensions( struct tagMSIVIEW *view, UINT *rows, UINT *cols )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %p %p\n", dv, rows, cols );

    if( !dv->table )
         return ERROR_FUNCTION_FAILED;

    *rows = 0;

    return dv->table->ops->get_dimensions( dv->table, NULL, cols );
}

static UINT DELETE_get_column_info( struct tagMSIVIEW *view,
                UINT n, LPWSTR *name, UINT *type )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %d %p %p\n", dv, n, name, type );

    if( !dv->table )
         return ERROR_FUNCTION_FAILED;

    return dv->table->ops->get_column_info( dv->table, n, name, type );
}

static UINT DELETE_modify( struct tagMSIVIEW *view, MSIMODIFY eModifyMode,
                MSIRECORD *rec )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p %d %p\n", dv, eModifyMode, rec );

    return ERROR_FUNCTION_FAILED;
}

static UINT DELETE_delete( struct tagMSIVIEW *view )
{
    MSIDELETEVIEW *dv = (MSIDELETEVIEW*)view;

    TRACE("%p\n", dv );

    if( dv->table )
        dv->table->ops->delete( dv->table );

    HeapFree( GetProcessHeap(), 0, dv );

    return ERROR_SUCCESS;
}


MSIVIEWOPS delete_ops =
{
    DELETE_fetch_int,
    DELETE_fetch_stream,
    DELETE_set_int,
    DELETE_insert_row,
    DELETE_execute,
    DELETE_close,
    DELETE_get_dimensions,
    DELETE_get_column_info,
    DELETE_modify,
    DELETE_delete
};

UINT DELETE_CreateView( MSIDATABASE *db, MSIVIEW **view, MSIVIEW *table )
{
    MSIDELETEVIEW *dv = NULL;

    TRACE("%p\n", dv );

    dv = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *dv );
    if( !dv )
        return ERROR_FUNCTION_FAILED;
    
    /* fill the structure */
    dv->view.ops = &delete_ops;
    dv->db = db;
    dv->table = table;

    *view = &dv->view;

    return ERROR_SUCCESS;
}
