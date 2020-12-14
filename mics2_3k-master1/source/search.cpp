#include <algorithm>
#include <thread>

#include "search.h"
#include "usi.h"
#include "evaluate.h"
#include "misc.h"
#include "movepick.h"

using namespace std;

namespace Search
{
  RootMoves rootMoves;
  LimitsType Limits;
  uint64_t Nodes;
  bool Stop;
}

namespace
{
  enum NodeType { Root, NonRoot };

  template <NodeType NT>
  Value search(Position& pos, Value alpha, Value beta, int depth, int ply_from_root);

  template <NodeType NT>
  Value qsearch(Position& pos, Value alpha, Value beta, int depth, int ply_from_root);

  Value eval(const Position& pos);
}

// 起動時に呼び出される。時間のかからない探索関係の初期化処理はここに書くこと。
void Search::init() {}

// isreadyコマンドの応答中に呼び出される。時間のかかる処理はここに書くこと。
void Search::clear() {}

void Search::start_thinking(Position rootPos, StateListPtr& states, LimitsType limits)
{
  Limits = limits;
  Nodes = 0;
  rootMoves.clear();
  Stop = false;

#if defined(FOR_TOURNAMENT) 
  auto ml = MoveList<LEGAL>(rootPos);
#else
  auto ml = MoveList<LEGAL_ALL>(rootPos);
#endif

  for (auto m : ml)
    rootMoves.emplace_back(m);

  ASSERT_LV3(states.get());

  search_root(rootPos);
}

void Search::search_root(Position rootPos)
{
  Move bestMove;

  if (rootMoves.size() == 0)
  {
    bestMove = MOVE_RESIGN;
    Stop = true;
    goto ID_END;
  }
  {
    int rootDepth = 0;
    Value alpha, beta;
    StateInfo si;
    Position& pos = rootPos;

    // ---------------------
    //   思考の終了条件
    // ---------------------

    thread* timerThread = nullptr;

    // 探索深さ、ノード数が指定されていない == 探索時間による制限
    if (!(Limits.depth || Limits.nodes))
    {
      // 時間制限があるのでそれに従うために今回の思考時間を計算する。
      // 今回に用いる思考時間 = 残り時間の1/60 + 秒読み時間

      auto us = pos.side_to_move();
      // 2秒未満は2秒として問題ない。(CSAルールにおいて)

      s64 availableTime;
      availableTime = max(s64(100), Limits.time[us] / 60 + Limits.byoyomi[us]);

      // 思考時間は秒単位で繰り上げ
      availableTime = (availableTime / 1000) * 1000;

      // 50msより小さいと思考自体不可能なので下限を50msに。
      auto NetworkDelay = 120;
      availableTime = max(s64(50), availableTime - NetworkDelay);

      auto endTime = availableTime;

      // タイマースレッドを起こして、終了時間を監視させる。

      timerThread = new thread([&] {
        while ((Time.elapsed() < endTime || Limits.infinite) && !Stop)
          this_thread::sleep_for(chrono::milliseconds(100));
        Stop = true;
        });
    }

    // ---------------------
    //   反復深化のループ
    // ---------------------

    while (++rootDepth < MAX_PLY && !Stop && (!Limits.depth || rootDepth <= Limits.depth))
    {
      alpha = -VALUE_INFINITE;
      beta = VALUE_INFINITE;

      ::search<Root>(rootPos, alpha, beta, rootDepth, 0);

      stable_sort(rootMoves.begin(), rootMoves.end());

      cout << USI::pv(pos, rootDepth) << endl;
    }

    bestMove = rootMoves.at(0).pv[0];

    // ---------------------
    // タイマースレッド終了
    // ---------------------

    Stop = true;
    if (timerThread != nullptr)
    {
      timerThread->join();
      delete timerThread;
    }
  }

ID_END:;
  cout << "bestmove " << bestMove << endl;
}

namespace
{
  // -----------------------
  //      通常探索
  // -----------------------

  template <NodeType NT>
  Value search(Position& pos, Value alpha, Value beta, int depth, int ply_from_root)
  {
    if (depth <= 0)
      return qsearch<NT>(pos, alpha, beta, depth, ply_from_root);

    // root nodeであるか
    const bool RootNode = NT == Root;

    Move ttMove = RootNode ? Search::rootMoves[0].pv[0] : MOVE_NONE;

    // -----------------------
    // 1手ずつ指し手を試す
    // -----------------------

    MovePicker mp(pos, ttMove);

    Value value;
    Move move;
    StateInfo si;

    // この局面でdo_move()された合法手の数
    int moveCount = 0;

    while (move = mp.next_move())
    {
      if (RootNode && !std::count(Search::rootMoves.begin(), Search::rootMoves.end(), move))
            continue;

      // legal()のチェック。root nodeだとlegal()だとわかっているのでこのチェックは不要。
      if (!RootNode && !pos.legal(move))
        continue;

      // 1手進める
      pos.do_move(move, si);
      ++moveCount;

      // 再帰的にsearchを呼び出す
      value = -search<NonRoot>(pos, -beta, -alpha, depth - 1, ply_from_root + 1);

      // 1手戻す
      pos.undo_move(move);

      // 停止シグナルが来たら終了
      if (Search::Stop)
        return VALUE_ZERO;

      if (RootNode)
      {
        auto& rm = *find(Search::rootMoves.begin(), Search::rootMoves.end(), move);

        if (moveCount == 1 || value > alpha)
        {
          rm.score = value;
          rm.pv.resize(1);
        }
        else
        {
          rm.score = -VALUE_INFINITE;
        }
      }

      if (value > alpha)
      {
        alpha = value;

        // αがβを上回ったらbeta cut
        if (alpha >= beta)
          break;
      }
    }

    // 合法手がない == 詰まされている ので、rootの局面からの手数で詰まされたという評価値を返す。
    if (moveCount == 0)
      alpha = mated_in(ply_from_root);

    return alpha;
  }


  // -----------------------
  //      静止探索
  // -----------------------

  template <NodeType NT>
  Value qsearch(Position& pos, Value alpha, Value beta, int depth, int ply_from_root)
  {
    // これ以上延長しない
    if (depth <= -3)
      return eval(pos);

    // 王手がかかっているか
    const bool InCheck = pos.in_check();

    // root nodeであるか
    const bool RootNode = NT == Root;

    // 現局面の評価値
    alpha = InCheck ? -VALUE_INFINITE : eval(pos);

    MovePicker mp(pos);
    Move move;
    StateInfo si;

    while (move = mp.next_move())
    {
      // legalのチェック
      if (!RootNode && !pos.legal(move))
        continue;

      // 1手進める
      pos.do_move(move, si);

      // 再帰的にqsearchを呼び出す
      Value value = -qsearch<NT>(pos, -beta, -alpha, depth - 1, ply_from_root + 1);

      // 1手戻す
      pos.undo_move(move);

      // 停止シグナルが来たら終了
      if (Search::Stop)
        return VALUE_ZERO;

      if (value > alpha)
      {
        alpha = value;

        // αがβを上回ったらbeta cut
        if (alpha >= beta)
          break;
      }
    }

    // 王手がかかっている状況で合法手がないので詰んでいる
    if (InCheck && alpha == -VALUE_INFINITE)
      alpha = mated_in(ply_from_root);

    return alpha;
  }


  // -----------------------
  //      評価関数
  // -----------------------

  Value eval(const Position& pos)
  {
    return Eval::evaluate(pos);
  }
}

