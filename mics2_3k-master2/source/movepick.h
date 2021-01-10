#ifndef _MOVEPICK_H_
#define _MOVEPICK_H_

#include "types.h"

// -----------------------
//   指し手オーダリング
// -----------------------

struct MovePicker
{
  // このクラスは指し手生成バッファが大きいので、コピーして使うような使い方は禁止。
  MovePicker(const MovePicker&) = delete;
  MovePicker& operator=(const MovePicker&) = delete;

  // 通常探索から呼び出されるとき用。
  MovePicker(const Position& pos_, Move ttMove);

  // 静止探索から呼び出される時用。
  MovePicker(const Position& pos_);

  // 次の指し手をひとつ返す
  // 指し手が尽きればMOVE_NONEが返る。
  Move next_move();

  friend std::ostream& operator<<(std::ostream& os, const MovePicker& mp);

private:
  const Position& pos;

  // range-based forを使いたいので。
  const ExtMove* begin() const { return moves; }
  const ExtMove* end() const { return endMoves; }

  // currentMoves  : 次に返す指し手
  // endMoves      : 生成された指し手の末尾
  ExtMove* currentMoves = moves, * endMoves = moves;

  // 指し手生成バッファ
  ExtMove moves[MAX_MOVES];
};

#endif
