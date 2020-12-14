#include "movepick.h"
#include "position.h"

// 通常探索から呼び出されるとき用。
MovePicker::MovePicker(const Position& pos_, Move ttMove) : pos(pos_)
{
  if (pos.in_check())
    endMoves = generateMoves<EVASIONS>(pos, currentMoves);
  else {
    //endMoves = generateMoves<NON_EVASIONS>(pos, currentMoves);
    endMoves = generateMoves<CAPTURES_PRO_PLUS>(pos, currentMoves);
    endMoves = generateMoves<NON_CAPTURES_PRO_MINUS>(pos, endMoves);
  }

  // 置換表の指し手が、この生成された集合のなかにあるなら、その先頭の指し手に置換表の指し手が来るようにしておく。
  if (ttMove != MOVE_NONE && pos.pseudo_legal(ttMove))
  {
    auto p = currentMoves;
    for (; p != endMoves; ++p)
      if (*p == ttMove)
      {
        std::swap(*p, *currentMoves);
        break;
      }
  }
}

// 静止探索から呼び出されるとき用。
MovePicker::MovePicker(const Position& pos_) : pos(pos_)
{
  // 王手がかかっているなら回避手(EVASIONS)、さもなくば、CAPTURESのみ生成。
  if (pos.in_check())
    endMoves = generateMoves<EVASIONS>(pos, currentMoves);
  else
    endMoves = generateMoves<CAPTURES>(pos, currentMoves);
}

// 次の指し手をひとつ返す
// 指し手が尽きればMOVE_NONEが返る。
Move MovePicker::next_move() {
  if (currentMoves == endMoves)
    return MOVE_NONE;
  return *currentMoves++;
}

std::ostream& operator<<(std::ostream& os, const MovePicker& mp) {
  for (auto m : mp)
    os << m.move << ' ';
  return os;
}
