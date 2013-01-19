

#include <stdio.h>
#include <math.h>

#include "sgscript.h"
#include "sgs_proc.h"
#include "sgs_ctx.h"


#define CHKARGS( cnt ) \
	if( sgs_StackSize( C ) != cnt ) { \
		sgs_Printf( C, SGS_ERROR, -1, "Incorrect number of arguments, need " #cnt "." ); \
		return 0; }

/* Containers */


/*
	ARRAY
*/

typedef struct sgsstd_array_header_s
{
	uint32_t size;
	uint32_t mem;
}
sgsstd_array_header_t;

#define SGSARR_UNIT sizeof( sgs_Variable )
#define SGSARR_HDRBASE sgsstd_array_header_t* hdr = (sgsstd_array_header_t*) data
#define SGSARR_HDR sgsstd_array_header_t* hdr = (sgsstd_array_header_t*) data->data
#define SGSARR_HDRUPDATE hdr = (sgsstd_array_header_t*) data->data
#define SGSARR_HDRSIZE sizeof(sgsstd_array_header_t)
#define SGSARR_ALLOCSIZE( cnt ) ((cnt)*sizeof(sgs_Variable)+SGSARR_HDRSIZE)
#define SGSARR_PTR( base ) ((sgs_Variable*)(((char*)base)+SGSARR_HDRSIZE))

static void sgsstd_array_reserve( SGS_CTX, sgs_VarObj* data, uint32_t size )
{
	void* nd;
	SGSARR_HDR;
	if( size <= hdr->mem )
		return;

	UNUSED( C );

	nd = sgs_Malloc( SGSARR_ALLOCSIZE( size ) );
	memcpy( nd, data->data, SGSARR_ALLOCSIZE( hdr->mem ) );
	sgs_Free( data->data );
	data->data = nd;
	SGSARR_HDRUPDATE;
	hdr->mem = size;
}

static void sgsstd_array_clear( SGS_CTX, sgs_VarObj* data )
{
	sgs_Variable *var, *vend;
	SGSARR_HDR;
	var = SGSARR_PTR( data->data );
	vend = var + hdr->size;
	while( var < vend )
	{
		sgs_Release( C, var );
		var++;
	}
	hdr->size = 0;
}

static void sgsstd_array_insert( SGS_CTX, sgs_VarObj* data, uint32_t pos )
{
	int i;
	uint32_t cnt = sgs_StackSize( C ) - 1;
	SGSARR_HDR;
	uint32_t nsz = hdr->size + cnt;
	sgs_Variable* ptr = SGSARR_PTR( data->data );

	if( !cnt ) return;

	if( nsz > hdr->mem )
	{
		sgsstd_array_reserve( C, data, MAX( nsz, hdr->mem * 2 ) );
		SGSARR_HDRUPDATE;
		ptr = SGSARR_PTR( data->data );
	}
	if( pos < hdr->size )
		memmove( ptr + pos + cnt, ptr + pos, ( hdr->size - pos ) * SGSARR_UNIT );
	for( i = 1; i < sgs_StackSize( C ); ++i )
	{
		sgs_Variable* var = sgs_StackItem( C, i );
		sgs_Acquire( C, var );
		ptr[ pos + i - 1 ] = *var;
	}
	hdr->size = nsz;
}

static void sgsstd_array_erase( SGS_CTX, sgs_VarObj* data, uint32_t from, uint32_t to )
{
	uint32_t i;
	uint32_t cnt = to - from + 1, to1 = to + 1;
	SGSARR_HDR;
	sgs_Variable* ptr = SGSARR_PTR( data->data );

	sgs_BreakIf( from >= hdr->size || to >= hdr->size || from > to );

	for( i = from; i <= to; ++i )
		sgs_Release( C, ptr + i );
	if( to1 < hdr->size )
		memmove( ptr + from, ptr + to1, ( hdr->size - to1 ) * SGSARR_UNIT );
	hdr->size -= cnt;
}

static int sgsstd_array_destruct( SGS_CTX, sgs_VarObj* data )
{
	sgsstd_array_clear( C, data );
	sgs_Free( data->data );
	return 0;
}

#define SGSARR_IHDR \
	sgs_VarObj* data; \
	sgsstd_array_header_t* hdr; \
	if( !sgs_Method( C ) ){ sgs_Printf( C, SGS_ERROR, -1, "Function isn't called on an array" ); return 0; } \
	data = sgs_GetObjectData( C, 0 ); \
	hdr = (sgsstd_array_header_t*) data->data; \
	UNUSED( hdr );


static int sgsstd_arrayI_clear( SGS_CTX )
{
	SGSARR_IHDR;
	sgsstd_array_clear( C, data );
	return 0;
}
static int sgsstd_arrayI_push( SGS_CTX )
{
	SGSARR_IHDR;
	sgsstd_array_insert( C, data, hdr->size );
	return 0;
}
static int sgsstd_arrayI_pop( SGS_CTX )
{
	SGSARR_IHDR;
	sgs_Variable* ptr = SGSARR_PTR( data->data );
	if( !hdr->size )
	{
		sgs_Printf( C, SGS_ERROR, -1, "Array is empty, cannot pop." );
		return 0;
	}
	sgs_PushVariable( C, ptr + hdr->size - 1 );
	sgsstd_array_erase( C, data, hdr->size - 1, hdr->size - 1 );
	return 1;
}
static int sgsstd_arrayI_shift( SGS_CTX )
{
	SGSARR_IHDR;
	sgs_Variable* ptr = SGSARR_PTR( data->data );
	if( !hdr->size )
	{
		sgs_Printf( C, SGS_ERROR, -1, "Array is empty, cannot shift." );
		return 0;
	}
	sgs_PushVariable( C, ptr );
	sgsstd_array_erase( C, data, 0, 0 );
	return 1;
}
static int sgsstd_arrayI_unshift( SGS_CTX )
{
	SGSARR_IHDR;
	sgsstd_array_insert( C, data, 0 );
	return 0;
}
static int sgsstd_array_getprop( SGS_CTX, sgs_VarObj* data )
{
	const char* name = sgs_ToString( C, -1 );
	sgs_CFunc func;
	if( 0 == strcmp( name, "size" ) )
	{
		SGSARR_HDR;
		sgs_PushInt( C, hdr->size );
		return SGS_SUCCESS;
	}
	else if( 0 == strcmp( name, "capacity" ) )
	{
		SGSARR_HDR;
		sgs_PushInt( C, hdr->mem );
		return SGS_SUCCESS;
	}
	else if( 0 == strcmp( name, "push" ) )		func = sgsstd_arrayI_push;
	else if( 0 == strcmp( name, "pop" ) )		func = sgsstd_arrayI_pop;
	else if( 0 == strcmp( name, "shift" ) )		func = sgsstd_arrayI_shift;
	else if( 0 == strcmp( name, "unshift" ) )	func = sgsstd_arrayI_unshift;
	else if( 0 == strcmp( name, "clear" ) )		func = sgsstd_arrayI_clear;
	else return SGS_ENOTFND;

	sgs_PushCFunction( C, func );
	return SGS_SUCCESS;
}

static int sgsstd_array_getindex( SGS_CTX, sgs_VarObj* data )
{
	SGSARR_HDR;
	sgs_Variable* ptr = SGSARR_PTR( data->data );
	sgs_Integer i = sgs_ToInt( C, -1 );
	if( i < 0 || i >= hdr->size )
	{
		sgs_Printf( C, SGS_ERROR, -1, "Array index out of bounds" );
		return SGS_EBOUNDS;
	}
	sgs_PushVariable( C, ptr + i );
	return SGS_SUCCESS;
}

static int sgsstd_array_setindex( SGS_CTX, sgs_VarObj* data )
{
	SGSARR_HDR;
	sgs_Variable* ptr = SGSARR_PTR( data->data );
	sgs_Integer i = sgs_ToInt( C, -1 );
	if( i < 0 || i >= hdr->size )
	{
		sgs_Printf( C, SGS_ERROR, -1, "Array index out of bounds" );
		return SGS_EBOUNDS;
	}
	sgs_Release( C, ptr + i );
	ptr[ i ] = *sgs_StackItem( C, -3 );
	sgs_Acquire( C, ptr + i );
	return SGS_SUCCESS;
}

static int sgsstd_array_gettype( SGS_CTX, sgs_VarObj* data )
{
	UNUSED( data );
	sgs_PushString( C, "array" );
	return SGS_SUCCESS;
}

static int sgsstd_array_tobool( SGS_CTX, sgs_VarObj* data )
{
	SGSARR_HDR;
	sgs_PushBool( C, !!hdr->size );
	return SGS_SUCCESS;
}

static int sgsstd_array_tostring( SGS_CTX, sgs_VarObj* data )
{
	int cnt;
	SGSARR_HDR;
	sgs_Variable* var = SGSARR_PTR( data->data );
	sgs_Variable* vend = var + hdr->size;
	cnt = vend - var;

	sgs_PushString( C, "[" );
	while( var < vend )
	{
		sgs_PushVariable( C, var );
		var++;
		if( var < vend )
			sgs_PushString( C, "," );
	}
	sgs_PushString( C, "]" );
	return sgs_StringMultiConcat( C, cnt * 2 + 1 );
}

static int sgsstd_array_tostring_UNUSED( SGS_CTX, sgs_VarObj* data )
{
	char buf[ 32 ];
	sprintf( buf, "Array(0x%8p)", data );
	sgs_PushString( C, buf );
	return SGS_SUCCESS;
}

static void sgsstd_array_resize( SGS_CTX, sgs_VarObj* data, uint32_t size )
{
	SGSARR_HDR;
	if( size > hdr->mem )
		sgsstd_array_reserve( C, data, ( size < hdr->mem * 2 ) ? hdr->mem * 2 : size );
}

static int sgsstd_array_gcmark( SGS_CTX, sgs_VarObj* data )
{
	SGSARR_HDR;
	sgs_Variable* var = SGSARR_PTR( data->data );
	sgs_Variable* vend = var + hdr->size;
	while( var < vend )
	{
		int ret = sgs_GCMark( C, var++ );
		if( ret != SGS_SUCCESS )
			return ret;
	}
	return SGS_SUCCESS;
}

void* sgsstd_array_functable[] =
{
	SOP_DESTRUCT, sgsstd_array_destruct,
	SOP_GETPROP, sgsstd_array_getprop,
	SOP_GETINDEX, sgsstd_array_getindex,
	SOP_SETINDEX, sgsstd_array_setindex,
	SOP_GETTYPE, sgsstd_array_gettype,
	SOP_TOBOOL, sgsstd_array_tobool,
	SOP_TOSTRING, sgsstd_array_tostring,
	SOP_GCMARK, sgsstd_array_gcmark,
	SOP_END,
};

int sgsstd_array( SGS_CTX )
{
	int i, objcnt = sgs_StackSize( C );
	void* data = sgs_Malloc( SGSARR_ALLOCSIZE( objcnt ) );
	SGSARR_HDRBASE;
	hdr->size = objcnt;
	hdr->mem = objcnt;
	for( i = 0; i < objcnt; ++i )
	{
		sgs_Variable* var = sgs_StackItem( C, i );
		sgs_Acquire( C, var );
		SGSARR_PTR( data )[ i ] = *var;
	}
	sgs_PushObject( C, data, sgsstd_array_functable );
	return 1;
}


/*
	DICT
*/

#define HTHDR HashTable* ht = (HashTable*) data->data

static void sgsstd_dict_clearvals( SGS_CTX, sgs_VarObj* data )
{
	HTHDR;
	HTPair* pair = ht->pairs, *pend = ht->pairs + ht->size;
	while( pair < pend )
	{
		if( pair->str )
		{
			sgs_Release( C, (sgs_Variable*) pair->ptr );
			sgs_Free( pair->ptr );
		}
		pair++;
	}
}

static int sgsstd_dict_destruct( SGS_CTX, sgs_VarObj* data )
{
	HTHDR;
	sgsstd_dict_clearvals( C, data );
	ht_destroy( ht );
	return SGS_SUCCESS;
}

static int sgsstd_dict_tostring( SGS_CTX, sgs_VarObj* data )
{
	HTHDR;
	HTPair *pair = ht->pairs, *pend = ht->pairs + ht->size;
	int cnt = 0;
	sgs_PushString( C, "{" );
	while( pair < pend )
	{
		if( pair->str )
		{
			if( cnt )
				sgs_PushString( C, "," );
			sgs_PushStringBuf( C, pair->str, pair->size );
			sgs_PushString( C, "=" );
			sgs_PushVariable( C, (sgs_Variable*) pair->ptr );
			cnt++;
		}
		pair++;
	}
	sgs_PushString( C, "}" );
	return sgs_StringMultiConcat( C, cnt * 4 + 1 );
}

static int sgsstd_dict_gettype( SGS_CTX, sgs_VarObj* data )
{
	UNUSED( data );
	sgs_PushString( C, "dict" );
	return SGS_SUCCESS;
}

static int sgsstd_dict_gcmark( SGS_CTX, sgs_VarObj* data )
{
	HTHDR;
	HTPair* pair = ht->pairs, *pend = ht->pairs + ht->size;
	while( pair < pend )
	{
		if( pair->str )
		{
			int ret = sgs_GCMark( C, (sgs_Variable*) pair->ptr );
			if( ret != SGS_SUCCESS )
				return ret;
		}
		pair++;
	}
	return SGS_SUCCESS;
}

static int sgsstd_dict_getindex( SGS_CTX, sgs_VarObj* data )
{
	HTHDR;
	HTPair* pair;
	sgs_ToString( C, -1 );
	if( sgs_StackItem( C, -1 )->type != SVT_STRING )
		return SGS_EINVAL;
	pair = ht_find( ht, sgs_GetStringPtr( C, -1 ), sgs_GetStringSize( C, -1 ) );
	if( !pair )
		return SGS_ENOTFND;
	return sgs_PushVariable( C, (sgs_Variable*) pair->ptr );
}

static int sgsstd_dict_setindex( SGS_CTX, sgs_VarObj* data )
{
	HTHDR;
	HTPair* pair;
	sgs_Variable* var, *key;
	sgs_ToString( C, -2 );
	key = sgs_StackItem( C, -2 );
	if( key->type != SVT_STRING )
		return SGS_EINVAL;
	pair = ht_find( ht, var_cstr( key ), key->data.S->size );
	if( pair )
	{
		sgs_Release( C, (sgs_Variable*) pair->ptr );
		sgs_Free( pair->ptr );
	}
	var = sgs_Alloc( sgs_Variable );
	*var = *sgs_StackItem( C, -1 );
	sgs_Acquire( C, var );
	ht_set( ht, var_cstr( key ), key->data.S->size, var );
	return SGS_SUCCESS;
}

#define sgsstd_dict_getprop sgsstd_dict_getindex
#define sgsstd_dict_setprop sgsstd_dict_setindex

void* sgsstd_dict_functable[] =
{
	SOP_DESTRUCT, sgsstd_dict_destruct,
	SOP_GETPROP, sgsstd_dict_getprop,
	SOP_SETPROP, sgsstd_dict_setprop,
	SOP_GETINDEX, sgsstd_dict_getindex,
	SOP_SETINDEX, sgsstd_dict_setindex,
	SOP_TOSTRING, sgsstd_dict_tostring,
	SOP_GETTYPE, sgsstd_dict_gettype,
	SOP_GCMARK, sgsstd_dict_gcmark,
	SOP_END,
};

int sgsstd_dict( SGS_CTX )
{
	int i, objcnt = sgs_StackSize( C );
	HashTable* ht = ht_create();
	for( i = 0; i < objcnt; i += 2 )
	{
		sgs_Variable* vkey = sgs_StackItem( C, i );
		sgs_Variable* var = sgs_Alloc( sgs_Variable );
		*var = *sgs_StackItem( C, i + 1 );
		sgs_Acquire( C, var );
		ht_set( ht, var_cstr( vkey ), vkey->data.S->size, var );
	}
	sgs_PushObject( C, ht, sgsstd_dict_functable );
	return 1;
}

int sgsstd_isset( SGS_CTX )
{
	sgs_Variable* var;
	HashTable* ht;
	HTPair* pair;
	if( sgs_StackSize( C ) != 2 )
		goto argerr;

	var = sgs_StackItem( C, -2 );
	if( var->type != SVT_OBJECT || var->data.O->iface != sgsstd_dict_functable )
		goto argerr;

	sgs_ToString( C, -1 );
	if( sgs_StackItem( C, -1 )->type != SVT_STRING )
		goto argerr;
	ht = (HashTable*) var->data.O->data;

	pair = ht_find( ht, sgs_GetStringPtr( C, -1 ), sgs_GetStringSize( C, -1 ) );
	return sgs_PushBool( C, pair != NULL ) == SGS_SUCCESS;

argerr:
	sgs_Printf( C, SGS_ERROR, -1, "'isset' requires 2 arguments: object (dict), property name (string)" );
	return 0;
}

int sgsstd_unset( SGS_CTX )
{
	sgs_Variable* var;
	HashTable* ht;
	HTPair* val;
	if( sgs_StackSize( C ) != 2 )
		goto argerr;

	var = sgs_StackItem( C, -2 );
	if( var->type != SVT_OBJECT || var->data.O->iface != sgsstd_dict_functable )
		goto argerr;

	sgs_ToString( C, -1 );
	if( sgs_StackItem( C, -1 )->type != SVT_STRING )
		goto argerr;
	ht = (HashTable*) var->data.O->data;

	val = ht_find( ht, sgs_GetStringPtr( C, -1 ), sgs_GetStringSize( C, -1 ) );
	if( val )
	{
		sgs_Release( C, (sgs_VarPtr) val->ptr );
		sgs_Free( val->ptr );
		ht_unset( ht, sgs_GetStringPtr( C, -1 ), sgs_GetStringSize( C, -1 ) );
	}
	return 0;

argerr:
	sgs_Printf( C, SGS_ERROR, -1, "'unset' requires 2 arguments: object (dict), property name (string)" );
	return 0;
}



/* Math */

#define MATHFUNC( name ) \
static int sgsstd_##name( SGS_CTX ) { \
	CHKARGS( 1 ); \
	sgs_PushReal( C, name( sgs_ToReal( C, -1 ) ) ); \
	return 1; }

#define MATHFUNC2( name ) \
static int sgsstd_##name( SGS_CTX ) { \
	CHKARGS( 2 ); \
	sgs_PushReal( C, name( sgs_ToReal( C, -2 ), sgs_ToReal( C, -1 ) ) ); \
	return 1; }

MATHFUNC( abs )
MATHFUNC( sqrt )
MATHFUNC( log )
MATHFUNC( log10 )
MATHFUNC( exp )
MATHFUNC( floor )
MATHFUNC( ceil )
MATHFUNC( sin )
MATHFUNC( cos )
MATHFUNC( tan )
MATHFUNC( asin )
MATHFUNC( acos )
MATHFUNC( atan )

MATHFUNC2( pow )
MATHFUNC2( atan2 )
MATHFUNC2( fmod )


/* I/O */

static int sgsstd_print( SGS_CTX )
{
	int i, ssz;
	ssz = sgs_StackSize( C );
	for( i = 0; i < ssz; ++i )
	{
		printf( "%s", sgs_ToString( C, i ) );
	}
	return 0;
}


/*
	String
*/

static SGS_INLINE int32_t idx2off( int32_t size, int32_t i )
{
	if( -i > size || i >= size ) return -1;
	return i < 0 ? size + i : i;
}

static int sgsstd_string_cut( SGS_CTX )
{
	int32_t size;
	const char* str;
	sgs_Integer i1, i2;
	if( !sgs_CheckArgs( C, "string,int,int" ) )
		return 0;
	str = sgs_ToString( C, 0 );
	size = sgs_GetStringSize( C, 0 );
	i1 = idx2off( size, sgs_ToInt( C, 1 ) );
	i2 = idx2off( size, sgs_ToInt( C, 2 ) );
	if( i1 < 0 || i2 < 0 || i1 > i2 )
	{
		sgs_Printf( C, SGS_ERROR, -1, "string_cut - invalid indices" );
		return 0;
	}
	sgs_PushStringBuf( C, str + i1, i2 - i1 + 1 );
	return 1;
}


/* OS */

static int sgsstd_ftime( SGS_CTX )
{
	sgs_PushReal( C, sgs_GetTime() );
	return 1;
}


/* utils */

static int sgsstd_typeof( SGS_CTX )
{
	CHKARGS( 1 );
	sgs_TypeOf( C );
	return 1;
}

static int sgsstd_exec( SGS_CTX )
{
	UNUSED( C );
	return 0;
}

static int sgsstd_gc_collect( SGS_CTX )
{
	int32_t orvc = sgs_Stat( C, SGS_STAT_VARCOUNT );
	int ret = sgs_GCExecute( C );
	if( ret == SGS_SUCCESS )
		sgs_PushInt( C, orvc - sgs_Stat( C, SGS_STAT_VARCOUNT ) );
	else
		sgs_PushBool( C, FALSE );
	return 1;
}


/* register all */
#define FN( name ) #name, sgsstd_##name
void* regfuncs[] =
{
	/* containers */
	FN( array ), FN( isset ), FN( unset ),
	/* math */
	FN( abs ), FN( sqrt ), FN( log ), FN( log10 ), FN( exp ), FN( floor ), FN( ceil ),
	FN( sin ), FN( cos ), FN( tan ), FN( asin ), FN( acos ), FN( atan ),
	FN( pow ), FN( atan2 ), FN( fmod ),
	/* I/O */
	FN( print ),
	/* string */
	FN( string_cut ),
	/* OS */
	FN( ftime ),
	/* utils */
	FN( typeof ),
/*	FN( exec ),	*/
	FN( gc_collect ),
	NULL
};

int sgsVM_RegStdLibs( SGS_CTX )
{
	void** fn = regfuncs;
	while( *fn )
	{
		sgs_PushCFunction( C, (sgs_CFunc) fn[ 1 ] );
		sgs_SetGlobal( C, (const char*) fn[ 0 ] );
		fn += 2;
	}

	C->array_func = &sgsstd_array;
	C->dict_func = &sgsstd_dict;

	return SGS_SUCCESS;
}
