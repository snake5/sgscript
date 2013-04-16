
#ifndef SGS_UTIL_H_INCLUDED
#define SGS_UTIL_H_INCLUDED

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sgscript.h"


/* useful shortcut definitions */
#define TRUE 1
#define FALSE 0
#define MAX( a, b ) ((a)>(b)?(a):(b))
#define MIN( a, b ) ((a)<(b)?(a):(b))
#define ARRAY_SIZE( a ) (sizeof(a)/sizeof(a[0]))

#define isoneof( chr, str ) (!!strchr( str, chr ))


typedef int16_t LineNum;

void sgs_BreakIfFunc( const char* code, const char* file, int line );
#if SGS_DEBUG && SGS_DEBUG_VALIDATE
#  define sgs_BreakIf( expr ) { if( expr ){ sgs_BreakIfFunc( #expr, __FILE__, __LINE__ ); } }
#else
#  define sgs_BreakIf( expr )
#endif


/* conversions */
static SGS_INLINE int hexchar( char c ){ return ( (c) >= '0' && (c) <= '9' ) || ( (c) >= 'a' && (c) <= 'f' ) || ( (c) >= 'A' && (c) <= 'F' ); }
static SGS_INLINE int gethex( char c ){ return ( (c) >= '0' && (c) <= '9' ) ? ( (c) - '0' ) : ( ( (c) >= 'a' && (c) <= 'f' ) ? ( (c) - 'a' + 10 ) : ( (c) - 'A' + 10 ) ); }
static SGS_INLINE int decchar( char c ){ return c >= '0' && c <= '9'; }
static SGS_INLINE int getdec( char c ){ return c - '0'; }
static SGS_INLINE int octchar( char c ){ return c >= '0' && c <= '7'; }
static SGS_INLINE int getoct( char c ){ return c - '0'; }
static SGS_INLINE int binchar( char c ){ return c == '0' || c == '1'; }
static SGS_INLINE int getbin( char c ){ return c - '0'; }


#define AS_( ptr, wat ) (*(wat*)(ptr))
#define AS_INT8( ptr ) AS_( ptr, int8_t )
#define AS_UINT8( ptr ) AS_( ptr, uint8_t )
#define AS_INT16( ptr ) AS_( ptr, int16_t )
#define AS_UINT16( ptr ) AS_( ptr, uint16_t )
#define AS_INT32( ptr ) AS_( ptr, int32_t )
#define AS_UINT32( ptr ) AS_( ptr, uint32_t )
#define AS_INT64( ptr ) AS_( ptr, int64_t )
#define AS_UINT64( ptr ) AS_( ptr, uint64_t )
#define AS_FLOAT( ptr ) AS_( ptr, float )
#define AS_DOUBLE( ptr ) AS_( ptr, double )

#define AS_INTEGER( ptr ) AS_( ptr, sgs_Integer )
#define AS_REAL( ptr ) AS_( ptr, sgs_Real )


/* flow/data debugging */
#if SGS_DEBUG && SGS_DEBUG_FLOW
#  define FUNC_HIT( what ) printf( "Hit \"%s\" line %d in function \"%s\"\n", what, __LINE__, __FUNCTION__ );
#  ifdef _MSC_VER
#    define FUNC_INFO( ... ) printf( __VA_ARGS__ );
#  else
#    define FUNC_INFO( what... ) printf( what );
#  endif
#  define FUNC_ENTER printf( "Entering a function from \"%s\" at line %d\n", __FUNCTION__, __LINE__ );
#  define FUNC_BEGIN printf( "Inside \"%s\"\n", __FUNCTION__ );
#  define FUNC_END printf( "Out of \"%s\" at line %d\n", __FUNCTION__, __LINE__ );
#else
#  define FUNC_HIT( what )
#  ifdef _MSC_VER
#    define FUNC_INFO( ... )
#  else
#    define FUNC_INFO( what... )
#  endif
#  define FUNC_ENTER
#  define FUNC_BEGIN
#  define FUNC_END
#endif

void print_safe( FILE* fp, const char* buf, int32_t size );


/* string buffer */
typedef
struct _StrBuf
{
	char*   ptr;
	int32_t size;
	int32_t mem;
}
StrBuf;

StrBuf strbuf_create( void );
void strbuf_destroy( StrBuf* sb, SGS_CTX );
StrBuf strbuf_partial( char* ch, int32_t size );
void strbuf_reserve( StrBuf* sb, SGS_CTX, int32_t size );
void strbuf_resize( StrBuf* sb, SGS_CTX, int32_t size );
void strbuf_inschr( StrBuf* sb, SGS_CTX, int32_t pos, char ch );
void strbuf_insbuf( StrBuf* sb, SGS_CTX, int32_t pos, const void* buf, int32_t size );
void strbuf_insstr( StrBuf* sb, SGS_CTX, int32_t pos, const char* str );
void strbuf_appchr( StrBuf* sb, SGS_CTX, char ch );
void strbuf_appbuf( StrBuf* sb, SGS_CTX, const void* buf, int32_t size );
void strbuf_appstr( StrBuf* sb, SGS_CTX, const char* str );


/* data buffer */
#define MemBuf StrBuf
#define membuf_create strbuf_create
#define membuf_destroy strbuf_destroy
#define membuf_partial strbuf_partial
void membuf_reserve( MemBuf* mb, SGS_CTX, int32_t size );
void membuf_resize( MemBuf* mb, SGS_CTX, int32_t size );
void membuf_resize_opt( MemBuf* mb, SGS_CTX, int32_t size );
void membuf_insbuf( MemBuf* mb, SGS_CTX, int32_t pos, const void* buf, int32_t size );
void membuf_erase( MemBuf* mb, int32_t from, int32_t to );
void membuf_appbuf( MemBuf* mb, SGS_CTX, const void* buf, int32_t size );
static SGS_INLINE void membuf_appchr( MemBuf* mb, SGS_CTX, char chr ){ membuf_appbuf( mb, C, &chr, 1 ); }


/* hash table */
typedef uint32_t Hash;

typedef
struct _HTPair
{
	char* str;
	int   size;
	Hash  hash;
	void* ptr;
}
HTPair;

typedef
struct _HashTable
{
	HTPair* pairs;
	int32_t size;
	int32_t load;
}
HashTable;

void ht_init( HashTable* T, SGS_CTX, int size );
void ht_free( HashTable* T, SGS_CTX );
void ht_dump( HashTable* T );
void ht_rehash( HashTable* T, SGS_CTX, int size );
void ht_check( HashTable* T, SGS_CTX, int inc );
HTPair* ht_find( HashTable* T, const char* str, int size );
void* ht_get( HashTable* T, const char* str, int size );
void ht_setpair( HTPair* P, SGS_CTX, const char* str, int size, Hash h, void* ptr );
HTPair* ht_set( HashTable* T, SGS_CTX, const char* str, int size, void* ptr );
void ht_unset_pair( HashTable* T, SGS_CTX, HTPair* p );
void ht_unset( HashTable* T, SGS_CTX, const char* str, int size );


double sgs_GetTime();


/* returns 0 on failure, 1/2 on integer/real */
int util_strtonum( const char** at, const char* end, sgs_Integer* outi, sgs_Real* outf );
sgs_Integer util_atoi( const char* str, int len );
sgs_Real util_atof( const char* str, int len );



void quicksort( void *array, size_t length, size_t size,
	int(*compare)(const void *, const void *, void*), void* userdata);


#endif /* SGS_UTIL_H_INCLUDED */
