#pragma once
#include <Windows.h>

struct RGSS1Runtime {

#define RUBYCALL __cdecl
#ifdef __cplusplus
#define ANYARGS ...
#else
#define ANYARGS
#endif

	typedef unsigned long VALUE;
	typedef VALUE(RUBYCALL* RubyFunc)(...);

	typedef void(*RGSSInitialize)(HMODULE hRgssDll);
	typedef int(*RGSSEval)(const char* pScripts);

	typedef VALUE(*pfn_rb_define_module)(const char*);
	typedef	void(*pfn_rb_define_module_function)(VALUE, const char*, RubyFunc, int);
	typedef char *(*pfn_rb_string_value_ptr)(volatile VALUE*);
	typedef VALUE(*pfn_rb_iv_get)(VALUE, const char*);

	typedef void (*pfn_rgss_get_filefullname)(char*, char*);

	static const int addr_rb_define_module = 0x20400;
	static const int addr_rb_define_module_function = 0x20B50;
	static const int addr_rb_string_value_ptr = 0x733A0;
	static const int addr_rb_iv_get = 0x7E050;

	static const int addr_rgss_get_filefullname = 0x9200;

	static inline long FIX2LONG(VALUE x) { return (long)(((VALUE)x) >> 1); }

	static const VALUE Qfalse = 0;
	static const VALUE Qtrue = 2;
	static const VALUE Qnil = 4;
	static const VALUE Qundef = 6;

	typedef long SIGNED_VALUE;
	static inline bool RTEST(VALUE v) { return ((v & ~Qnil) != 0); }

	static const VALUE FL_USHIFT = 12;
	static const VALUE FL_USER0 = (((VALUE)1) << (FL_USHIFT + 0));
	static const VALUE FL_USER1 = (((VALUE)1) << (FL_USHIFT + 1));
	static const VALUE FL_USER2 = (((VALUE)1) << (FL_USHIFT + 2));
	static const VALUE FL_USER3 = (((VALUE)1) << (FL_USHIFT + 3));
	static const VALUE FL_USER4 = (((VALUE)1) << (FL_USHIFT + 4));
	static const VALUE FL_USER5 = (((VALUE)1) << (FL_USHIFT + 5));
	static const VALUE FL_USER6 = (((VALUE)1) << (FL_USHIFT + 6));
	static const VALUE FL_USER7 = (((VALUE)1) << (FL_USHIFT + 7));
	static const VALUE FL_USER8 = (((VALUE)1) << (FL_USHIFT + 8));
	static const VALUE FL_USER9 = (((VALUE)1) << (FL_USHIFT + 9));
	static const VALUE FL_USER10 = (((VALUE)1) << (FL_USHIFT + 10));
	static const VALUE FL_USER11 = (((VALUE)1) << (FL_USHIFT + 11));
	static const VALUE FL_USER12 = (((VALUE)1) << (FL_USHIFT + 12));
	static const VALUE FL_USER13 = (((VALUE)1) << (FL_USHIFT + 13));
	static const VALUE FL_USER14 = (((VALUE)1) << (FL_USHIFT + 14));
	static const VALUE FL_USER15 = (((VALUE)1) << (FL_USHIFT + 15));
	static const VALUE FL_USER16 = (((VALUE)1) << (FL_USHIFT + 16));
	static const VALUE FL_USER17 = (((VALUE)1) << (FL_USHIFT + 17));
	static const VALUE FL_USER18 = (((VALUE)1) << (FL_USHIFT + 18));
	static const VALUE FL_USER19 = (((VALUE)1) << (FL_USHIFT + 19));

	static const VALUE RUBY_T_NONE = 0x00;
	static const VALUE RUBY_T_OBJECT = 0x01;
	static const VALUE RUBY_T_CLASS = 0x02;
	static const VALUE RUBY_T_MODULE = 0x03;
	static const VALUE RUBY_T_FLOAT = 0x04;
	static const VALUE RUBY_T_STRING = 0x05;
	static const VALUE RUBY_T_REGEXP = 0x06;
	static const VALUE RUBY_T_ARRAY = 0x07;
	static const VALUE RUBY_T_HASH = 0x08;
	static const VALUE RUBY_T_STRUCT = 0x09;
	static const VALUE RUBY_T_BIGNUM = 0x0a;
	static const VALUE RUBY_T_FILE = 0x0b;
	static const VALUE RUBY_T_DATA = 0x0c;
	static const VALUE RUBY_T_MATCH = 0x0d;
	static const VALUE RUBY_T_COMPLEX = 0x0e;
	static const VALUE RUBY_T_RATIONAL = 0x0f;

	static const VALUE RUBY_T_NIL = 0x11;
	static const VALUE RUBY_T_TRUE = 0x12;
	static const VALUE RUBY_T_FALSE = 0x13;
	static const VALUE RUBY_T_SYMBOL = 0x14;
	static const VALUE RUBY_T_FIXNUM = 0x15;

	static const VALUE RUBY_T_UNDEF = 0x1b;
	static const VALUE RUBY_T_NODE = 0x1c;
	static const VALUE RUBY_T_ICLASS = 0x1d;
	static const VALUE RUBY_T_ZOMBIE = 0x1e;

	static const VALUE RUBY_T_MASK = 0x1f;

	static const VALUE T_NONE = RUBY_T_NONE;
	static const VALUE T_NIL = RUBY_T_NIL;
	static const VALUE T_OBJECT = RUBY_T_OBJECT;
	static const VALUE T_CLASS = RUBY_T_CLASS;
	static const VALUE T_ICLASS = RUBY_T_ICLASS;
	static const VALUE T_MODULE = RUBY_T_MODULE;
	static const VALUE T_FLOAT = RUBY_T_FLOAT;
	static const VALUE T_STRING = RUBY_T_STRING;
	static const VALUE T_REGEXP = RUBY_T_REGEXP;
	static const VALUE T_ARRAY = RUBY_T_ARRAY;
	static const VALUE T_HASH = RUBY_T_HASH;
	static const VALUE T_STRUCT = RUBY_T_STRUCT;
	static const VALUE T_BIGNUM = RUBY_T_BIGNUM;
	static const VALUE T_FILE = RUBY_T_FILE;
	static const VALUE T_FIXNUM = RUBY_T_FIXNUM;
	static const VALUE T_TRUE = RUBY_T_TRUE;
	static const VALUE T_FALSE = RUBY_T_FALSE;
	static const VALUE T_DATA = RUBY_T_DATA;
	static const VALUE T_MATCH = RUBY_T_MATCH;
	static const VALUE T_SYMBOL = RUBY_T_SYMBOL;
	static const VALUE T_RATIONAL = RUBY_T_RATIONAL;
	static const VALUE T_COMPLEX = RUBY_T_COMPLEX;
	static const VALUE T_UNDEF = RUBY_T_UNDEF;
	static const VALUE T_NODE = RUBY_T_NODE;
	static const VALUE T_ZOMBIE = RUBY_T_ZOMBIE;
	static const VALUE T_MASK = RUBY_T_MASK;

	static const VALUE RUBY_FIXNUM_FLAG = 0x01;
	static const VALUE SYMBOL_FLAG = 0x0e;

	static const int RSTRING_EMBED_LEN_MAX = ((sizeof(VALUE) * 3) / sizeof(char) - 1);

	struct RBasic {
		VALUE flags;
		VALUE klass;
	};

	struct RString {
		struct RBasic basic;
		union {
			struct {
				long len;
				char *ptr;
				union {
					long capa;
					VALUE shared;
				} aux;
			} heap;
			char ary[RSTRING_EMBED_LEN_MAX + 1];
		} as;
	};

	struct RData {
		struct RBasic basic;
		void(*dmark)(void*);
		void(*dfree)(void*);
		void *data;
	};

	static inline struct RBasic* RBASIC(VALUE obj) { return (struct RBasic*)obj; }
	static inline struct RString* RSTRING(VALUE obj) { return (struct RString*)obj; }
	static inline struct RData* RDATA(VALUE obj) { return (struct RData*)obj; }

	static const VALUE RSTRING_NOEMBED = FL_USER1;
	static const VALUE RSTRING_EMBED_LEN_MASK = (FL_USER2 | FL_USER3 | FL_USER4 | FL_USER5 | FL_USER6);
	static const VALUE RSTRING_EMBED_LEN_SHIFT = (FL_USHIFT + 2);

	static inline char* RSTRING_PTR(VALUE str)
	{
		return RSTRING(str)->as.heap.ptr;
	}

	static inline long RSTRING_LEN(VALUE str)
	{
		return (!(RBASIC(str)->flags & RSTRING_NOEMBED) ? \
			(long)((RBASIC(str)->flags >> RSTRING_EMBED_LEN_SHIFT) & (RSTRING_EMBED_LEN_MASK >> RSTRING_EMBED_LEN_SHIFT)) : RSTRING(str)->as.heap.len);
	}

	static const int RARRAY_EMBED_LEN_MAX = 3;

	struct RArray {
		struct RBasic basic;
		union {
			struct {
				long len;
				union {
					long capa;
					VALUE shared;
				} aux;
				VALUE *ptr;
			} heap;
			VALUE ary[RARRAY_EMBED_LEN_MAX];
		} as;
	};

	static const unsigned long FIXNUM_MAX = (unsigned long)(LONG_MAX >> 1);
	static const long FIXNUM_MIN = ((long)LONG_MIN >> (int)1);

	static inline struct RArray* RARRAY(VALUE obj) { return (struct RArray*)obj; }

	static const VALUE RARRAY_EMBED_FLAG = FL_USER1;
	static const VALUE RARRAY_EMBED_LEN_MASK = (FL_USER4 | FL_USER3);
	static const VALUE RARRAY_EMBED_LEN_SHIFT = (FL_USHIFT + 3);

	static inline VALUE* RARRAY_PTR(VALUE ary)
	{
		return ((RBASIC(ary)->flags & RARRAY_EMBED_FLAG) ? RARRAY(ary)->as.ary : RARRAY(ary)->as.heap.ptr);
	}

	static inline long RARRAY_LEN(VALUE ary)
	{
		return ((RBASIC(ary)->flags & RARRAY_EMBED_FLAG) ? \
			(long)((RBASIC(ary)->flags >> RARRAY_EMBED_LEN_SHIFT) & (RARRAY_EMBED_LEN_MASK >> RARRAY_EMBED_LEN_SHIFT)) : RARRAY(ary)->as.heap.len);
	}

	static const VALUE	RUBY_IMMEDIATE_MASK = 0x03;
	static inline bool	IMMEDIATE_P(VALUE x) { return (x & RUBY_IMMEDIATE_MASK) != 0; }

	static const VALUE	RUBY_SYMBOL_FLAG = 0x0e;
	static const VALUE	RUBY_SPECIAL_SHIFT = 8;
	static inline bool	SYMBOL_P(VALUE x) { return ((x&~(~(VALUE)0 << RUBY_SPECIAL_SHIFT)) == RUBY_SYMBOL_FLAG); }

	static inline int	BUILTIN_TYPE(VALUE x) { return (int)(RBASIC(x)->flags & T_MASK); }
	static inline bool FIXNUM_P(VALUE f) { return (((SIGNED_VALUE)f) & RUBY_FIXNUM_FLAG) != 0; }
	static inline bool POSFIXABLE(unsigned long f) { return (f <= FIXNUM_MAX); }
	static inline bool NEGFIXABLE(long f) { return (f >= FIXNUM_MIN); }
	static inline bool FIXABLE(long f) { return (NEGFIXABLE(f) && (f <= 0 || POSFIXABLE(f))); }

	static inline int rb_type(VALUE obj)
	{
		if (IMMEDIATE_P(obj))
		{
			if (FIXNUM_P(obj))	return T_FIXNUM;
			if (obj == Qtrue)	return T_TRUE;
			if (SYMBOL_P(obj))	return T_SYMBOL;
			if (obj == Qundef)	return T_UNDEF;
		}
		else if (!RTEST(obj))
		{
			if (obj == Qnil)	return T_NIL;
			if (obj == Qfalse)	return T_FALSE;
		}
		return BUILTIN_TYPE(obj);
	}

	struct RGSSTable {
		struct RBasic basic;
		int dimension;
		int xsize;
		int ysize;
		int zsize;
		int data_size;
		WORD *data_ptr;
	};

	struct RGSSColor {
		double red;
		double green;
		double blue;
		double alpha;
	};

};

#define RR RGSS1Runtime
#define RV RR::VALUE
#define RF RR::RubyFunc
#define FIX2NUM RR::FIX2LONG

#define DATA_PTR(dta) (RR::RDATA(dta)->data)
#define Check_Type(v,t) rgss->rb_check_type((RV)(v),(t))
#define Data_Get_Struct(obj,type,sval) do {\
    (sval) = (type*)DATA_PTR(obj);\
} while (0)