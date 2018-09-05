#ifndef _SVenUnitType
#define _SVenUnitType

enum _UnitType
{
	utNone,
	utCurrency,
	utMixed,	// this and the following imply byte unit requests
	utByte,
	utKilo,
	utMega,
	utGiga,
	utTera
};

#endif