typedef _Bool bool;
typedef   signed char schar;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned long long ulonglong;
typedef void *vptr;
typedef int *iptr;

#define DEFINE_CAST(from, to)			\
	static to from##2##to(from x) {		\
		return (to)x;			\
	}

#define DEFINE_CASTS(from)			\
	DEFINE_CAST(from, bool)			\
	DEFINE_CAST(from, char)			\
	DEFINE_CAST(from, schar)		\
	DEFINE_CAST(from, uchar)		\
	DEFINE_CAST(from, short)		\
	DEFINE_CAST(from, ushort)		\
	DEFINE_CAST(from, int)			\
	DEFINE_CAST(from, uint)			\
	DEFINE_CAST(from, long)			\
	DEFINE_CAST(from, ulong)		\
	DEFINE_CAST(from, longlong)		\
	DEFINE_CAST(from, ulonglong)		\
	DEFINE_CAST(from, float)		\
	DEFINE_CAST(from, double)

DEFINE_CASTS(bool)
DEFINE_CASTS(char)
DEFINE_CASTS(schar)
DEFINE_CASTS(uchar)
DEFINE_CASTS(short)
DEFINE_CASTS(ushort)
DEFINE_CASTS(int)
DEFINE_CASTS(uint)
DEFINE_CASTS(long)
DEFINE_CASTS(ulong)
DEFINE_CASTS(longlong)
DEFINE_CASTS(ulonglong)
DEFINE_CASTS(float)
DEFINE_CASTS(double)

DEFINE_CAST(long, vptr)
DEFINE_CAST(long, iptr)
DEFINE_CAST(vptr, long)
DEFINE_CAST(iptr, long)
DEFINE_CAST(int, vptr)
DEFINE_CAST(vptr, int)

/*
 * check-name: Cast code generation
 * check-command: sparsec -c $file -o tmp.o
 */
