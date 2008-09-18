#ifndef INC_ncoParserTokenTypes_hpp_
#define INC_ncoParserTokenTypes_hpp_

/* $ANTLR 2.7.6 (20071205): "ncoGrammer.g" -> "ncoParserTokenTypes.hpp"$ */

#ifndef CUSTOM_API
# define CUSTOM_API
#endif

#ifdef __cplusplus
struct CUSTOM_API ncoParserTokenTypes {
#endif
	enum {
		EOF_ = 1,
		NULL_NODE = 4,
		BLOCK = 5,
		ARG_LIST = 6,
		DMN_LIST = 7,
		DMN_ARG_LIST = 8,
		LMT_LIST = 9,
		VALUE_LIST = 10,
		FUNC_ARG = 11,
		LMT = 12,
		EXPR = 13,
		FEXPR = 14,
		POST_INC = 15,
		POST_DEC = 16,
		UTIMES = 17,
		SQR2 = 18,
		PROP = 19,
		FOR2 = 20,
		NORET = 21,
		ATAN2 = 22,
		WHERE_ASSIGN = 23,
		MISS2ZERO = 24,
		SEMI = 25,
		DEFDIM = 26,
		LPAREN = 27,
		NSTRING = 28,
		COMMA = 29,
		RPAREN = 30,
		WHILE = 31,
		BREAK = 32,
		CONTINUE = 33,
		IF = 34,
		ELSE = 35,
		WHERE = 36,
		ELSEWHERE = 37,
		PRINT = 38,
		VAR_ID = 39,
		ATT_ID = 40,
		LCURL = 41,
		RCURL = 42,
		FOR = 43,
		COLON = 44,
		LSQUARE = 45,
		DIM_ID = 46,
		RSQUARE = 47,
		DIVIDE = 48,
		DIM_MTD_ID = 49,
		DOT = 50,
		FUNC = 51,
		INC = 52,
		DEC = 53,
		LNOT = 54,
		PLUS = 55,
		MINUS = 56,
		TIMES = 57,
		CARET = 58,
		MOD = 59,
		FLTHAN = 60,
		FGTHAN = 61,
		LTHAN = 62,
		GTHAN = 63,
		GEQ = 64,
		LEQ = 65,
		EQ = 66,
		NEQ = 67,
		LAND = 68,
		LOR = 69,
		QUESTION = 70,
		ASSIGN = 71,
		PLUS_ASSIGN = 72,
		MINUS_ASSIGN = 73,
		TIMES_ASSIGN = 74,
		DIVIDE_ASSIGN = 75,
		FLOAT = 76,
		DOUBLE = 77,
		INT = 78,
		BYTE = 79,
		UBYTE = 80,
		SHORT = 81,
		USHORT = 82,
		UINT = 83,
		INT64 = 84,
		UINT64 = 85,
		DIM_ID_SIZE = 86,
		NRootAST = 87,
		SHIFTL = 88,
		SHIFTR = 89,
		QUOTE = 90,
		DGT = 91,
		LPH = 92,
		LPHDGT = 93,
		XPN = 94,
		VAR_NM = 95,
		VAR_NM_QT = 96,
		BLASTOUT = 97,
		UNUSED_OPS = 98,
		Whitespace = 99,
		CXX_COMMENT = 100,
		C_COMMENT = 101,
		NUMBER_DOT = 102,
		NUMBER = 103,
		VAR_ATT = 104,
		DIM_QT = 105,
		LMT_DMN = 106,
		NULL_TREE_LOOKAHEAD = 3
	};
#ifdef __cplusplus
};
#endif
#endif /*INC_ncoParserTokenTypes_hpp_*/
