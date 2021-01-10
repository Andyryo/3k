#include "types.h"
#include "usi.h"
#include "search.h"

#include <sstream>

using namespace std;

// --------------------------------------
//  USI拡張コマンド "bench"(ベンチマーク)
// --------------------------------------

// benchmark用デフォルトの局面集
// これを増やすなら、下のほうの fens.assign のところの局面数も増やすこと。
static const char* BenchSfen[] = {

	// 初期局面に近い局面。
	"r1s1k/2bgp/5/PGB2/K1S1R b - 5",

	"1r1bk/1s1gp/3S1/P1G2/KB2R w - 1",
	"2g1k/3sp/P4/2S2/K1B1b b 2Rg 1",
};

void bench_cmd(Position& cur, istringstream& is)
{
	string token;
	Search::LimitsType limits;
	vector<string> fens;

	string fenFile   = (is >> token) ? token : "default";
	string limitType = (is >> token) ? token : "depth";
	string limit     = (is >> token) ? token : "7";

	if (limitType == "time")
		limits.movetime = (TimePoint)1000 * stoi(limit);

	else if (limitType == "nodes")
		limits.nodes = stoll(limit);

	else
		limits.depth = stoi(limit);

	if (fenFile == "default")
		fens.assign(BenchSfen, BenchSfen + 3);
	else
		fens.push_back(cur.sfen());

	is_ready();

	// ノード数
	int64_t nodes = 0;

	// ベンチの計測用タイマー
	Timer time;
	time.reset();

	Position pos;
	for (size_t i = 0; i < fens.size(); ++i)
	{
		StateListPtr states(new StateList(1));

		pos.set(fens[i], &states->back());

		cout << "\nPosition: " << (i + 1) << '/' << fens.size() << endl;

		// 探索時にnpsが表示されるが、それはこのglobalなTimerに基づくので探索ごとにリセットを行うようにする。
		Time.reset();

		Search::start_thinking(pos, states, limits);

		nodes += Search::Nodes;
	}

	auto elapsed = time.elapsed() + 1;

	cout << "\n==========================="
		   << "\nTotal time (ms) : " << elapsed
		   << "\nNodes searched  : " << nodes
		   << "\nNodes/second    : " << 1000 * nodes / elapsed
	     << endl;
}