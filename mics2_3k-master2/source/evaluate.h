#ifndef _EVALUATE_H_
#define _EVALUATE_H_

#include "types.h"
#include "position.h"

namespace Eval
{
	// Apery(WCSC26)の駒割り
	enum {
		PawnValue = 100,
		SilverValue = 750,
		GoldValue = 850,
		BishopValue = 1205,
		RookValue = 1560,
		ProPawnValue = 835,
		ProSilverValue = 870,
		HorseValue = 1855,
		DragonValue = 2250,
		KingValue = 150000,
	};

	// 駒の価値のテーブル(後手の駒は負の値)
	extern int PieceValue[PIECE_NB];

  Value evaluate(const Position& pos);
}

#endif