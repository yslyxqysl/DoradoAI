#include "sdk.h"
#include "const.h"
#include "console.h"
#include "filter.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <set>
#include <memory>
#include <queue>
#include <map>
#include <cmath>
using namespace std;

#define myCon AIController::ins()->console()

class Strategy;
class AIHero;

class AIController  // 控制ai的主类
{
public:
	AIController(const PMap &map, const PPlayerInfo &info, PCommand &cmd);
	~AIController() { delete _console; }
public:
	void globalStrategy();
	void levelupHero();
	void buyNewHero();
	void assignBaseAttack();
	void action();

	static AIController *ins() { return instance; }

	Console *console() const { return _console; }
	const PMap &getMap() const { return *map; }

	const std::vector<shared_ptr<AIHero>> &decisionPool() { return AIHeroList; }
	const std::vector<Strategy *> &strategyPool() { return allStrategy; }

private:
	static AIController *instance;

	Console *_console;
	const PMap *map;
	const PPlayerInfo *info;
	PCommand *cmd;

	vector<shared_ptr<AIHero>> AIHeroList;
	vector<Strategy *> allStrategy;

public:
	vector<PUnit*> myHeros, enemyHeros, monsters;
	PUnit *myBase, *enemyBase;
};

AIController *AIController::instance = nullptr;

class Memory
{
public:
	static Memory *ins() { if (_instance) return _instance; else return _instance = new Memory; }
	static void updateMemory();

private:
	Memory()
	{
		srand((unsigned int)time(nullptr));
		badSituation = false;
		enemyBaseLastHp = 3000;
		enemyBaseLastSeen = 1000000000;
		lastRoundEnemyOccuCent = 1000000000;
		lastRoundEnemyOccuMine1 = 1000000000;
		lastRoundEnemyProtBase = 1000000000;
		catchJungleRound = 1000000000;
		randMine = vector<int>({ 1, 6 }).at(rand() % 2);
	}
	static Memory *_instance;
public:
	bool badSituation;
	std::map<PUnit *, std::string> lastStrategy;
	int enemyBaseLastHp;
	int enemyBaseLastSeen;
	int lastRoundEnemyOccuCent;
	int lastRoundEnemyOccuMine1;
	int lastRoundEnemyProtBase;
	int catchJungleRound;
	int randMine;

public:
	int currentEnemyBaseHp() { return min(2 * enemyBaseLastSeen + enemyBaseLastHp, 3000); }
};

Memory *Memory::_instance = nullptr;
/********************************************************************************/


/********************************* Constants ************************************/
namespace lsy
{
	static int heroRank = 1234;
	static int newHeroFirst = 1;
	static int attackArg = 200, supportRange = 600, monsterAvoid = 45, enemyAvoid = 30, deltaFightArg = 100;
	static int outOfRangeArg = 100;
	static int miningArg = 100, sameMineArg = 10, centerMineArg = 30;
	static int goBackHomeHp = 100, goBackHomeArg = 300;
	static int hammerDizzy = 50;
	static int strategyDisabled = -1 << 30;

	static const std::vector<std::string> HERO_NAME = { "Hammerguard", "Master", "Berserker", "Scouter" };
	static const std::set<std::string> ACTIVE_SKILLS = { "HammerAttack", "Blink", "Sacrifice", "SetObserver" };

	static const std::string &strHammerguard = HERO_NAME[0];
	static const std::string &strMaster = HERO_NAME[1];
	static const std::string &strBerserker = HERO_NAME[2];
	static const std::string &strScouter = HERO_NAME[3];

	static const std::vector<Pos> initPath[8] = {
	{ Pos(19,16),Pos(21,20),Pos(24,24),Pos(28,27),Pos(30,29),Pos(32,31),Pos(36,34),Pos(39,38),Pos(42,42),Pos(45,46),Pos(48,50),Pos(52,53),Pos(55,57),Pos(59,60),Pos(62,64),Pos(66,67),Pos(69,71),Pos(72,71) },
	{ Pos(24,12),Pos(28,20),Pos(29,23),Pos(30,26),Pos(31,29),Pos(32,32),Pos(34,34),Pos(36,36),Pos(38,38),Pos(40,40),Pos(42,42),Pos(44,44),Pos(46,46),Pos(48,48),Pos(50,50),Pos(52,52),Pos(53,55),Pos(55,57),Pos(58,58),Pos(60,60),Pos(62,62),Pos(64,64),Pos(66,66),Pos(67,69),Pos(69,71),Pos(71,73) },
	{ Pos(22,14),Pos(25,18),Pos(28,22),Pos(31,26),Pos(34,30),Pos(37,34),Pos(40,38),Pos(43,42),Pos(46,45),Pos(49,49),Pos(53,52),Pos(56,56),Pos(60,59),Pos(63,63),Pos(67,66),Pos(70,70) },
	{ Pos(21,15),Pos(24,20),Pos(27,25),Pos(30,30),Pos(34,34),Pos(38,38),Pos(42,42),Pos(46,46),Pos(50,50),Pos(54,54),Pos(58,58),Pos(62,62),Pos(66,66),Pos(70,70) },
	{ Pos(127,138),Pos(127,134),Pos(127,130),Pos(123,127),Pos(121,123),Pos(118,119),Pos(116,117),Pos(112,114),Pos(109,110),Pos(106,106),Pos(103,102),Pos(99,99),Pos(96,95),Pos(92,92),Pos(89,88),Pos(85,85),Pos(82,81),Pos(78,78),Pos(78,75),Pos(76,75) },
	{ Pos(129,135),Pos(121,131),Pos(120,128),Pos(119,125),Pos(118,122),Pos(117,119),Pos(116,116),Pos(114,114),Pos(112,112),Pos(110,110),Pos(108,108),Pos(106,106),Pos(104,104),Pos(102,102),Pos(100,100),Pos(98,98),Pos(96,96),Pos(94,94),Pos(92,92),Pos(90,90),Pos(89,87),Pos(87,85),Pos(85,83),Pos(83,81),Pos(81,79) },
	{ Pos(128,136),Pos(126,132),Pos(123,128),Pos(120,124),Pos(117,120),Pos(114,116),Pos(111,112),Pos(107,109),Pos(104,105),Pos(100,102),Pos(98,98),Pos(95,94),Pos(91,91),Pos(88,87),Pos(84,84),Pos(81,80),Pos(77,77) },
	{ Pos(127,137),Pos(127,131),Pos(124,126),Pos(121,121),Pos(117,117),Pos(113,113),Pos(109,109),Pos(105,105),Pos(101,101),Pos(97,97),Pos(93,93),Pos(89,89),Pos(85,85),Pos(81,81),Pos(77,77) }
	};  // 游戏开始时打表寻路
}

using namespace lsy;
/********************************************************************************/


/******************************* Simple Functions *******************************/
inline bool isEqual(const char *str1, const char *str2) { return !strcmp(str1, str2); }

bool isHero(string unitName)
{
	return find(HERO_NAME.begin(), HERO_NAME.end(), unitName) != HERO_NAME.end();
}

bool isMonster(string unitName)
{
	return unitName == "Dragon" || unitName == "Roshan";
}

bool isActiveSkill(PSkill* mySkill)  // 判断技能是否为主动技能
{
	return ACTIVE_SKILLS.count(mySkill->name) != 0;
}

Pos campRotate(int x, int y)
{
	if (myCon->camp())
		return Pos(149 - x, 149 - y);
	else
		return Pos(x, y);
}

Pos campRotate(Pos p)
{
	if (myCon->camp())
		return Pos(149 - p.x, 149 - p.y);
	else
		return p;
}

Pos getMineByCamp(int cid)
{
	// cid is the index of MINE_POS when in camp 0
	// this function will convert the position when you in camp 1
	if (myCon->camp() == 0)
		return MINE_POS[cid];
	if (cid == 0)
		return MINE_POS[0];
	if (cid >= 1 && cid <= 4)
		return MINE_POS[5 - cid];
	else
		return MINE_POS[11 - cid];
}

int getHeroType(string name)
{
	for (int i = 0; i < 4; ++i)
		if (HERO_NAME[i] == name)
			return i;
}

int friendsInRange(PUnit *worker)
{
	int res = 0;
	UnitFilter filter;
	filter.setAreaFilter(new Circle(worker->pos, worker->view));
	for (auto x : myCon->friendlyUnits(filter))
		if (x->isHero() || x->isBase())
			++res;
	return res;
}

int enemiesInRange(PUnit *worker, int range2 = 256)
{
	int res = 0;
	UnitFilter filter;
	filter.setAreaFilter(new Circle(worker->pos, range2));
	for (auto x : myCon->enemyUnits(filter))
		if (x->isHero())
			++res;
	return res;
}
/********************************************************************************/

void Memory::updateMemory()
{
	if (AIController::ins()->enemyBase)
	{
		ins()->enemyBaseLastHp = AIController::ins()->enemyBase->hp;
		ins()->enemyBaseLastSeen = myCon->round();
	}

	UnitFilter filter;

	int enemyCount = 0;
	filter.setAreaFilter(new Circle(MINE_POS[0], 144), "w");
	for (auto x : myCon->enemyUnits(filter))
		if (x->isHero())
			++enemyCount;
	int friendCount = 0;
	for (auto x : myCon->friendlyUnits(filter))
		if (x->isHero())
			++friendCount;
	if (enemyCount > 2 && friendCount == 0)
		Memory::ins()->lastRoundEnemyOccuCent = myCon->round();

	enemyCount = 0;
	filter.setAreaFilter(new Circle(getMineByCamp(Memory::ins()->randMine), 324), "w");
	for (auto x : myCon->enemyUnits(filter))
		if (x->isHero())
			++enemyCount;
	friendCount = 0;
	for (auto x : myCon->friendlyUnits(filter))
		if (x->isHero())
			++friendCount;
	if (enemyCount > 3 && friendCount <= 1)
		Memory::ins()->lastRoundEnemyOccuMine1 = myCon->round();
}

/******************************** Path Function *********************************/
void findPath(const PMap &map, Pos start, Pos dest, const vector<Pos> &blocks, vector<Pos> &path)
{
	struct Node
	{
		int x, y;
		int level;
		int priority;

		Node() : x(0), y(0), level(0), priority(0) {}
		Node(int xp, int yp, int d, int p) : x(xp), y(yp), level(d), priority(p) {}
		void updatePriority(int xDest, int yDest)
		{
			priority = level + estimate(xDest, yDest);
		}
		void nextLevel()
		{
			level += 100;
		}
		int estimate(const int &xDest, const int &yDest) const
		{
			int xd = xDest - x, yd = yDest - y;
			return static_cast<int>(10.0 * (sqrt(2.0) * std::min(xd, yd) + 1.0 * std::abs(xd - yd)));
		}
		bool operator < (const Node &b) const
		{
			return priority > b.priority;
		}
	};
	const int dx[] = { 1, 0, -1, 0 };
	const int dy[] = { 0, 1, 0, -1 };
	static unsigned char closed_nodes_map[MAP_SIZE][MAP_SIZE];
	static int open_nodes_map[MAP_SIZE][MAP_SIZE];
	static unsigned char dir_map[MAP_SIZE][MAP_SIZE];
	priority_queue<Node> pq[2];
	int pqi;
	int i, j, x, y, xdx, ydy;
	memset(closed_nodes_map, 0, sizeof(closed_nodes_map));
	memset(open_nodes_map, 0, sizeof(open_nodes_map));
	if (start == dest)
		return;
	for (size_t i = 0; i < blocks.size(); ++i)
		if (blocks[i].x >= 0 && blocks[i].x < MAP_SIZE && blocks[i].y >= 0 && blocks[i].y < MAP_SIZE)
			closed_nodes_map[blocks[i].x][blocks[i].y] = 1;
	if (closed_nodes_map[dest.x][dest.y] == 1) {
		xdx = start.x < dest.x ? -1 : 1;
		ydy = start.y < dest.y ? -1 : 1;
		x = dest.x;
		y = dest.y;
		while (closed_nodes_map[x][y] == 1) {
			if (checkOnLine(start, dest, Pos(x + xdx, y)))
				x += xdx;
			else
				y += ydy;
		}
		dest = Pos(x, y);
	}
	Node n0(start.x, start.y, 0, 0);
	n0.updatePriority(dest.x, dest.y);
	pqi = 0;
	pq[pqi].push(n0);
	open_nodes_map[start.x][start.y] = n0.priority;
	while (!pq[pqi].empty()) {
		n0 = pq[pqi].top();
		pq[pqi].pop();
		x = n0.x;
		y = n0.y;
		open_nodes_map[x][y] = 0;
		closed_nodes_map[x][y] = 1;
		if (x == dest.x && y == dest.y) {
			while (!(x == start.x && y == start.y)) {
				j = dir_map[x][y];
				path.push_back(Pos(x, y));
				x += dx[j];
				y += dy[j];
			}
			path.push_back(start);
			reverse(path.begin(), path.end());
			return;
		}
		for (i = 0; i < 4; i++) {
			xdx = x + dx[i];
			ydy = y + dy[i];
			if (!(xdx < 0 || xdx >= MAP_SIZE || ydy < 0 || ydy >= MAP_SIZE
				|| map.getHeight(xdx, ydy) - map.getHeight(x, y) > 1
				|| map.getHeight(xdx, ydy) - map.getHeight(x, y) < -1
				|| closed_nodes_map[xdx][ydy] == 1)) {
				Node m0(n0);
				m0.x = xdx;
				m0.y = ydy;
				m0.nextLevel();
				m0.updatePriority(dest.x, dest.y);
				if (open_nodes_map[xdx][ydy] == 0) {
					open_nodes_map[xdx][ydy] = m0.priority;
					pq[pqi].push(m0);
					dir_map[xdx][ydy] = (i + 2) % 4;
				}
				else if (open_nodes_map[xdx][ydy] > m0.priority) {
					open_nodes_map[xdx][ydy] = m0.priority;
					dir_map[xdx][ydy] = (i + 2) % 4;
					while (!(pq[pqi].top().x == xdx && pq[pqi].top().y == ydy)) {
						pq[1 - pqi].push(pq[pqi].top());
						pq[pqi].pop();
					}
					pq[pqi].pop();
					if (pq[pqi].size() > pq[1 - pqi].size())
						pqi = 1 - pqi;
					while (!pq[pqi].empty()) {
						pq[1 - pqi].push(pq[pqi].top());
						pq[pqi].pop();
					}
					pqi = 1 - pqi;
					pq[pqi].push(m0);
				}
			}
		}
	}
}
/********************************** Strategies **********************************/


/********************************** Strategies **********************************/
class Strategy  // 策略基类
{
public:
	Strategy() { }
	Strategy(PUnit *worker, std::string name) : worker(worker), name(name) { }
	virtual int countWorth() = 0;  // 计算某种策略的实行价值
	virtual void work() = 0;  // 实现这种策略

public:
	void setWorker(PUnit* worker) { this->worker = worker; }
	int getWorth() { return worth; }
	void setWorth(int worth) { this->worth = worth; }
	PUnit* getWorker() { return worker; }
	std::string getName() { return name; }

protected:
	PUnit *worker;
	int worth;
	std::string name;
};

class AIHero  // 对应每个英雄的策略决策
{
public:
	AIHero(PUnit* newHero) { mHero = newHero; mStrategy = nullptr; }  // 默认选择回家的策略
	~AIHero() { if (mStrategy != nullptr) delete mStrategy; }

public:
	void chooseStrategy(const vector<Strategy *> strats)
	{
		int maxWorth = -1000000000;
		for (auto x : strats)
		{
			if (x->getWorker() != this->mHero)
				continue;
			int stratWorth = x->countWorth();
			if (stratWorth > maxWorth)
				mStrategy = x, maxWorth = stratWorth;
		}
	}

	void action() { if (mStrategy) mStrategy->work(); }  // 进行当前的最优策略
	PUnit *getHero() { return mHero; }
	Strategy *getStrategy() { return mStrategy; }
private:
	PUnit *mHero;
	Strategy *mStrategy;
};

class Unavalable : public Strategy
{
public:
	Unavalable(PUnit* worker) : Strategy(worker, "Unavalable") { }
public:
	int countWorth()
	{
		if (worker->findBuff("Dizzy") != nullptr)
		{
			name = "Dizzy";
			return worth = 2147483640;
		}
		if (worker->findBuff("Reviving") != nullptr)
		{
			name = "Dead";
			return worth = 2147483640;
		}
		return worth = strategyDisabled;
	}
	void work() { }
};

class NormalAttack : public Strategy
{
public:
	NormalAttack(PUnit *worker, PUnit *target) : Strategy(worker, "NorAtk") { setTarget(target); }
	int countWorth()
	{
		this->worth = 0;
		if (!worker->canUseSkill("Attack")
			|| target->findBuff("Reviving") != nullptr || target->hp <= 0
			|| dis2(worker->pos, target->pos) > worker->range)
			return this->worth = strategyDisabled;
		return this->worth;
	}

	void work()
	{
		myCon->selectUnit(worker);
		myCon->attack(target);
	}

	void setTarget(PUnit* target) { this->target = target; }
private:
	PUnit* target;
};

class Chase : public Strategy
{
public:
	Chase(PUnit *worker, PUnit *target) : Strategy(worker, "Chase") { setTarget(target); }
	int countWorth()
	{
		this->worth = 0;
		if (target->findBuff("Reviving") != nullptr || target->hp <= 0
			|| dis2(worker->pos, target->pos) <= worker->range)
			return this->worth = strategyDisabled;
		return this->worth;
	}

	void work()
	{
		myCon->selectUnit(worker);
		myCon->attack(target);
	}

	void setTarget(PUnit* target) { this->target = target; }
private:
	PUnit* target;
};

class Attack : public Strategy  // 攻击某个单位的策略
{
public:
	Attack(PUnit* worker, PUnit* target) : Strategy(worker, "Attack") { setTarget(target); }
public:
	int countWorth()
	{
		this->worth = 0;
		if (target->findBuff("Reviving") != nullptr || target->hp <= 0)
			return this->worth = strategyDisabled;

		if (dis2(worker->pos, target->pos) <= supportRange)
		{
			if (target->hp < 80 && dis2(worker->pos, target->pos) > worker->range && target->speed >= worker->speed && !target->findBuff("Dizzy"))
				this->worth += 230;
			else
				this->worth += int(((double)target->max_hp / target->hp) * attackArg);
			if (worker->findBuff("WinOrDie"))
			{
				if (dis2(worker->pos, target->pos) <= worker->range)
					worth += 2000;
				else
					worth = 200;
			}
			if (dis2(target->pos, AIController::ins()->myBase->pos) <= AIController::ins()->myBase->view)
				worth += 300;
			if (dis2(worker->pos, target->pos) <= worker->range)
			{
				if (target->name == strBerserker)
				{
					if (target->canUseSkill("Sacrifice"))
						worth += 300;
					else
						worth -= 20;
				}
				if (worker->atk - target->def >= target->hp)
					worth += 3000;
			}
			else
			{
				this->worth -= outOfRangeArg;
			}
			if (target->isWild())
			{
				this->worth -= monsterAvoid;
				if (friendsInRange(target) <= 1)
					this->worth -= 1000;
			}
		}
		return this->worth;
	}

	void work() {
		myCon->selectUnit(worker);
		myCon->attack(target);
	}

	void setTarget(PUnit* target) { this->target = target; }
private:
	PUnit* target;
};

class Tempt : public Strategy  // 引诱敌人（野怪）的策略-即几乎保证自己不受伤的情况下进行hit-and-run
{
public:
	Tempt(PUnit* worker, PUnit* target) : Strategy(worker, "Tempt") { setTarget(target); }
public:
	int countWorth()
	{
		return this->worth = 0;
		if (worker->findSkill("Attack") && worker->findSkill("Attack")->cd > 0)
			this->worth = 300;
		return this->worth;
	}
	void work() {
		myCon->selectUnit(worker);
		myCon->attack(target);
	}
	void setTarget(PUnit* target) { this->target = target; }
private:
	PUnit* target;
};

class UseHammerAttack : public Strategy
{
public:
	UseHammerAttack(PUnit* worker, PUnit* target) : Strategy(worker, "UseHamAtk") { setTarget(target); }

	int countWorth()
	{
		this->worth = 0;
		if (!worker->canUseSkill("HammerAttack"))
			return this->worth = strategyDisabled;
		if (dis2(worker->pos, target->pos) > HAMMERATTACK_RANGE || target->findBuff("Reviving"))
			return this->worth;

		this->worth += int(((double)target->max_hp / target->hp) * attackArg);
		if (dis2(target->pos, AIController::ins()->myBase->pos) <= AIController::ins()->myBase->view)
			worth += 300;
		if (target->findBuff("Dizzy") != nullptr)
			this->worth -= hammerDizzy;
		return this->worth;
	}

	void work()
	{
		myCon->selectUnit(worker);
		myCon->useSkill("HammerAttack", target);
	}

	void setTarget(PUnit* target) { this->target = target; }
private:
	PUnit *target;
};

class UseBlink : public Strategy
{
public:
	UseBlink(PUnit* worker) : Strategy(worker, "UseBlink") { }

	int countWorth()
	{
		this->worth = 0;
		if (!worker->canUseSkill("Blink"))
			return this->worth = strategyDisabled;

		if (myCon->round() == 1)
		{
			pos = initPath[myCon->camp() * 4 + 1][myCon->round()];
			this->worth += 100000001;
		}

		int enemyTotalAtk = 0;
		UnitFilter filter;
		filter.setAreaFilter(new Circle(worker->pos, worker->view));
		for (auto x : myCon->enemyUnits(filter))
			if (dis2(worker->pos, x->pos) <= x->range)
				enemyTotalAtk += x->atk;
		if (enemyTotalAtk < worker->hp)
			return worth;
		std::vector<Pos> homePath;
		if (myCon->camp())
			pos = Pos(worker->pos.x + 6, worker->pos.y + 6);
		else
			pos = Pos(worker->pos.x - 6, worker->pos.y - 6);
		findShortestPath(AIController::ins()->getMap(), pos, myCon->getMilitaryBase()->pos, homePath);
		if (homePath.back() == myCon->getMilitaryBase()->pos)
			this->worth += 2000;

		return this->worth;
	}

	void work() {
		myCon->selectUnit(worker);
		myCon->useSkill("Blink", pos);
	}

	void setPos(PUnit* target) { this->pos = pos; }
private:
	Pos pos;
};

class UseSacrifice : public Strategy
{
public:
	UseSacrifice(PUnit* worker, PUnit* target) : Strategy(worker, "UseSacri") { setTarget(target); }

	int countWorth()
	{
		this->worth = 0;
		if (!worker->canUseSkill("Sacrifice"))
			return this->worth = strategyDisabled;
		if (target->findBuff("Reviving"))
			return this->worth;
		UnitFilter filter;
		filter.setAreaFilter(new Circle(worker->pos, 144));
		if(worker->findSkill("Attack")->cd > 1)
			return worth;
		for (auto x : myCon->enemyUnits(filter))
			if (dis2(x->pos, worker->pos) <= x->range && !x->findBuff("Dizzy"))
				return worth;
		if (dis(worker->pos, target->pos) <= sqrt(worker->range) + sqrt(worker->speed) / 2)
		{
			this->worth += int(((double)target->max_hp / target->hp) * attackArg * 2);
			if (dis2(worker->pos, target->pos) > worker->view)
				this->worth -= outOfRangeArg * 2;
			if (target->findBuff("Dizzy") != nullptr)
				this->worth += hammerDizzy;
		}
		if (dis2(worker->pos, target->pos) <= worker->range && dis2(target->pos, AIController::ins()->myBase->pos) <= AIController::ins()->myBase->view)
			worth += 300;
		return this->worth;
	}

	void work() {
		myCon->selectUnit(worker);
		myCon->useSkill("Sacrifice", target);
	}

	void setTarget(PUnit* target) { this->target = target; }
private:
	PUnit *target;
};

class PlugEye : public Strategy  // 放眼策略
{
public:
	PlugEye(PUnit* worker) : Strategy(worker, "PlugEye") { }
public:
	void sklSetObserver(PSkill *skill)
	{
		this->worth = 0;
		if (myCon->round() < 30)
			return;
		if (dis2(worker->pos, campRotate(138, 115)) <= 100)
		{
			targetPos = myCon->randPosInArea(campRotate(138, 125), 4);
			this->worth += 205;
		}
		else
		{
			int fHeroNearCenter = 0;
			UnitFilter filter;
			filter.setAreaFilter(new Circle(MINE_POS[0], 225));
			for (auto x : myCon->enemyUnits(filter))
			{
				if (x->isHero())
					return;
			}
			for (auto x : AIController::ins()->myHeros)
			{
				if (x != worker && dis2(x->pos, MINE_POS[0]) <= 144 && x->findBuff("BeAttacked") == nullptr)
					fHeroNearCenter++;
			}
			if (fHeroNearCenter >= 2)
			{
				targetPos = campRotate(99, 84);
				//this->worth += 190;
			}
		}
	}

	int countWorth()
	{
		this->worth = 0;
		myCon->selectUnit(worker);
		if (!worker->canUseSkill("SetObserver"))
			return this->worth = strategyDisabled;
		else
			sklSetObserver(myCon->getSkill("SetObserver", worker));
		return this->worth;
	}

	void work()
	{
		myCon->selectUnit(worker);
		if (dis2(worker->pos, targetPos) <= 25)
			myCon->useSkill(myCon->getSkill("SetObserver", worker), myCon->randPosInArea(targetPos, 4));
		else
			myCon->move(targetPos);
	}

private:
	Pos targetPos;
};

class GoBackHome : public Strategy  // 英雄步行回家的策略
{
public:
	GoBackHome(PUnit* worker) : Strategy(worker, "GoHome") { }
public:
	int countWorth()
	{
		this->worth = 0;
		if (worker->hp <= goBackHomeHp && strcmp(worker->name, "Berserker") != 0)
			this->worth += goBackHomeArg;
		if (friendsInRange(worker) < AIController::ins()->myHeros.size()
			&& friendsInRange(worker) + 1 < enemiesInRange(worker))
		{
			this->worth = goBackHomeArg;
			if (friendsInRange(worker) + 3 < enemiesInRange(worker))
				this->worth = goBackHomeArg;
		}
		if (AIController::ins()->myBase->hp < 3000)
			this->worth = 198;
		return this->worth;
	}

	void work()
	{
		myCon->selectUnit((const PUnit*)worker);
		if (AIController::ins()->myBase->hp < 3000)
			myCon->move(campRotate(20, 19));
		else
			myCon->move(myCon->getMilitaryBase()->pos);
	}
};

class CallBackHome : public Strategy  // 召回英雄的策略
{
public:
	CallBackHome(PUnit* worker) : Strategy(worker, "CallBack") { }
public:
	int countWorth()
	{
		this->worth = 0;
		if (worker->findBuff("BeAttacked") || Memory::ins()->lastStrategy[worker] == "CallBack")
			return worth = strategyDisabled;
		if (dis2(worker->pos, AIController::ins()->myBase->pos) <= MILITARY_BASE_RANGE)
			return this->worth;
		int enemyAttackBase = 0;
		int friendProtectBase = 0;
		int callBackSpent = 0;
		UnitFilter filter;
		filter.setAreaFilter(new Circle(AIController::ins()->myBase->pos, MILITARY_BASE_RANGE));
		for (auto x : myCon->enemyUnits(filter))
			enemyAttackBase++;
		for (auto x : myCon->friendlyUnits(filter))
			if (x->isHero())
				friendProtectBase++;
		for (auto x : AIController::ins()->decisionPool())
			if (x->getStrategy() && x->getStrategy()->getName() == "CallBack")
			{
				friendProtectBase++;
				callBackSpent += myCon->callBackCost(x->getHero()->level);
			}

		if (myCon->gold() - callBackSpent < myCon->callBackCost(worker->level))
			return worth = strategyDisabled;

		if (enemyAttackBase > friendProtectBase)
			this->worth += 300;
		return this->worth;
	}

	void work()
	{
		myCon->callBackHero(worker, myCon->randPosInArea(campRotate(15, 9), 2));
	}
};

class GoCenterMining : public Strategy  // 采中心矿的策略
{
public:
	GoCenterMining(PUnit* worker) : Strategy(worker, "GoCenMine") { }
public:
	int countWorth()
	{
		this->worth = miningArg;
		if (myCon->round() <= initPath[myCon->camp() * 4 + getHeroType(worker->name)].size() - 1)
			return this->worth = 100000000;

		const auto &currentDecisionPool = AIController::ins()->decisionPool();
		for (auto x : currentDecisionPool)
			if (x->getStrategy() != nullptr)
				if (x->getStrategy()->getName() == "GoCenterMining")
					this->worth -= sameMineArg;

		this->worth -= int(dis(worker->pos, MINE_POS[0]) / 5);
		this->worth += centerMineArg;
		return this->worth;
	}

	void work() {
		myCon->selectUnit(worker);
		if(myCon->round() <= initPath[myCon->camp() * 4 + getHeroType(worker->name)].size() - 1)
			myCon->move(initPath[myCon->camp() * 4 + getHeroType(worker->name)][myCon->round()]);
		else
			myCon->move(MINE_POS[0]);
	}
private:
};

class GoMining : public Strategy  // 打野的策略
{
public:
	GoMining(PUnit* worker, Pos target) : Strategy(worker, "GoMining") { setTarget(target); }
public:
	int countWorth()
	{
		UnitFilter filter;

		this->worth = 0;
		if (myCon->round() <= 30)
			return worth;

		int enemyCount = 0;
		filter.setAreaFilter(new Circle(getMineByCamp(Memory::ins()->randMine), 324), "w");
		for (auto x : myCon->enemyUnits(filter))
			if (x->isHero())
				++enemyCount;
		if (dis2(worker->pos, getMineByCamp(Memory::ins()->randMine)) <= worker->view && enemyCount == 0)
			Memory::ins()->lastRoundEnemyOccuMine1 = 1000000000;

		if (Memory::ins()->lastRoundEnemyOccuMine1 <= 1000 && myCon->round() - Memory::ins()->lastRoundEnemyOccuMine1 >= 30)
			return worth;

		filter.setAreaFilter(new Circle(MINE_POS[0], 144));
		enemyCount = 0;
		for (auto x : myCon->enemyUnits(filter))
			if (x->isHero())
				++enemyCount;
		if (enemyCount > AIController::ins()->myHeros.size())
			Memory::ins()->badSituation = true;

		filter.setAreaFilter(new Circle(MINE_POS[0], 144), "w");
		int friendsInCenter = 0;
		for (auto x : myCon->friendlyUnits(filter))
			if (x->isHero())
				friendsInCenter++;

		if (dis2(worker->pos, MINE_POS[0]) <= worker->view && enemyCount == 0)
			Memory::ins()->lastRoundEnemyOccuCent = 1000000000;

		if (Memory::ins()->badSituation || friendsInCenter == 0 && Memory::ins()->lastRoundEnemyOccuCent <= 1000)
			this->worth += miningArg;

		if (target == getMineByCamp(4))
		{
			enemyCount = 0;
			filter.setAreaFilter(new Circle(target, 324), "w");
			for (auto x : myCon->enemyUnits(filter))
				if (x->isHero())
					++enemyCount;
			UnitFilter bigFilter;
			bigFilter.setAreaFilter(new Circle(MINE_POS[0], 441));
			friendsInCenter = 0;
			for (auto x : myCon->friendlyUnits(bigFilter))
				if (x->isHero())
					friendsInCenter++;
			if (enemyCount >= 1 && enemyCount < friendsInCenter && Memory::ins()->catchJungleRound > 1000)
				Memory::ins()->catchJungleRound = myCon->round();
			if (enemyCount == 0 || myCon->round() - Memory::ins()->catchJungleRound > 15)
				Memory::ins()->catchJungleRound = 1000000000;
			if (Memory::ins()->catchJungleRound <= 1000)
				this->worth = 175;
			else
				this->worth = 0;
		}
		else if (target == getMineByCamp(Memory::ins()->randMine))
			this->worth += 50;

		return this->worth;
	}

	void work() {
		if (worker->name == strScouter)
		{
			if (Memory::ins()->catchJungleRound > 1000
				|| dis2(worker->pos, myCon->getMilitaryBase()->pos) <= 3600
				|| myCon->round() - Memory::ins()->catchJungleRound >= 6)
			{
				myCon->selectUnit(worker);
				myCon->move(target);
			}
		}
		else
		{
			myCon->selectUnit(worker);
			myCon->move(target);
		}
	}

	void setTarget(Pos target) { this->target = target; }
	Pos getTarget() { return target; }
private:
	Pos target;
};

class ProtectBase : public Strategy  // 保护老家策略
{
public:
	ProtectBase(PUnit *worker) : Strategy(worker, "ProtectBase") { }
public:
	int countWorth()
	{
		return worth = 0;
	}
	void work() { }
};

class PushBase : public Strategy  // 推老家策略
{
public:
	PushBase(PUnit *worker) : Strategy(worker, "PushBase") { }
public:
	int countWorth()
	{
		this->worth = 0;
		if (AIController::ins()->myHeros.size() < HERO_LIMIT - 1)
			return this->worth;

		bool inBaseRange = false;
		for (auto x : AIController::ins()->myHeros)
			if (dis2(x->pos, campRotate(MILITARY_BASE_POS[1])) <= 144)
				inBaseRange = true;
		if (inBaseRange && dis2(campRotate(MILITARY_BASE_POS[1]), worker->pos) <= 625)
			this->worth += 10000;
		else
		{
			int enemyByBaseCount = 0;
			for (auto x : AIController::ins()->enemyHeros)
				if (dis2(x->pos, campRotate(MILITARY_BASE_POS[1])) <= 625)
					enemyByBaseCount++;
			if (enemyByBaseCount >= 3 && (Memory::ins()->enemyBaseLastSeen > 1000 || myCon->round() - Memory::ins()->enemyBaseLastSeen >= 20))
				Memory::ins()->lastRoundEnemyProtBase = myCon->round();
			if (enemyByBaseCount >= 3 && Memory::ins()->lastStrategy[worker] == "PlugEye")
				Memory::ins()->lastRoundEnemyProtBase = myCon->round() + 100;
			if (Memory::ins()->lastRoundEnemyProtBase <= 1000 && myCon->round() - Memory::ins()->lastRoundEnemyProtBase <= 35 && Memory::ins()->currentEnemyBaseHp() >= 300)
				return worth;
			if (enemiesInRange(worker) <= 1 && myCon->gold() >= 150)
				this->worth += 200;
			if (AIController::ins()->myBase->hp < 1500 && Memory::ins()->lastStrategy[worker] != "PushBase")
				this->worth -= 100;
			for (auto x : AIController::ins()->strategyPool())
				if (x->getWorker() == worker && x->getName() == "GoHome")
					if (x->getWorth() >= goBackHomeArg)
						this->worth += 150;
		}

		return this->worth;
	}

	void work()
	{
		myCon->selectUnit(worker);
		if (AIController::ins()->enemyBase)
		{
			if (worker->name == strBerserker && worker->canUseSkill("Sacrifice") && worker->findSkill("Attack")->cd <= 1 && dis2(AIController::ins()->enemyBase->pos, worker->pos) <= worker->range)
				myCon->useSkill("Sacrifice", AIController::ins()->enemyBase);
			else
				myCon->attack(AIController::ins()->enemyBase);
		}
		else
		{
			int friendHeroCount = 0;
			for (auto x : AIController::ins()->myHeros)
				if (dis2(x->pos, worker->pos) <= worker->range || campRotate(x->pos).y >= 110 || x->findBuff("Reviving"))
					friendHeroCount++;

			if (campRotate(worker->pos).x <= 108 && campRotate(worker->pos).y <= 65)
				myCon->move(campRotate(116, 46));
			else if (friendHeroCount >= HERO_LIMIT - 1
				|| worker->pos.y > 126
				|| enemiesInRange(AIController::ins()->myBase) > 0
				|| myCon->round() >= 950)
				myCon->move(MILITARY_BASE_POS[1 - myCon->camp()]);
			else
				myCon->move(campRotate(138, 115));
		}
	}
};
/********************************************************************************/


/********************************************************************************/
AIController::AIController(const PMap &map, const PPlayerInfo &info, PCommand &cmd)
{
	instance = this;
	_console = new Console(map, info, cmd);
	_console->changeShortestPathFunc(findPath);

	this->map = &map;
	this->info = &info;
	this->cmd = &cmd;
	myHeros.clear();
	std::vector<PUnit*> myUnits = myCon->friendlyUnits();
	std::vector<PUnit*> enemyUnits = myCon->enemyUnits();
	enemyBase = nullptr;
	for (size_t i = 0; i < myUnits.size(); ++i)
	{
		if (isHero(myUnits[i]->name))
			myHeros.push_back(myUnits[i]);
		else if (isEqual(myUnits[i]->name, "MilitaryBase"))
			myBase = myUnits[i];
	}
	for (size_t i = 0; i < enemyUnits.size(); ++i)
	{
		if (isHero(enemyUnits[i]->name) || isMonster(enemyUnits[i]->name))
			enemyHeros.push_back(enemyUnits[i]);
		else if (isEqual(enemyUnits[i]->name, "MilitaryBase"))
			enemyBase = enemyUnits[i];
	}
}

void AIController::globalStrategy()
{
	if (newHeroFirst)
	{
		ins()->buyNewHero();
		ins()->levelupHero();
	}
	else
	{
		ins()->levelupHero();
		ins()->buyNewHero();
	}

	assignBaseAttack();
}

void AIController::levelupHero()
{
	UnitFilter filter;
	filter.setAreaFilter(new Circle(myBase->pos, LEVELUP_RANGE), "a");
	vector<PUnit*> heroToLevelUp = myCon->friendlyUnits(filter);

	int heroCount = 0;
	for (auto x : myCon->friendlyUnits())
		if (x->isHero())
			heroCount++;

	for (auto x : heroToLevelUp)
		if (heroCount == HERO_LIMIT - 1 && myCon->gold() - myCon->levelUpCost(x->level) >= myCon->callBackCost(0))
			myCon->buyHeroLevel(x);
}

void AIController::buyNewHero()
{
	if (myCon->round() == 0 || AIController::myHeros.size() == 0)
	{
		myCon->chooseHero(HERO_NAME[0], initPath[myCon->camp() * 4][0]);
		myCon->chooseHero(HERO_NAME[1], initPath[myCon->camp() * 4 + 1][0]);
		myCon->chooseHero(HERO_NAME[2], initPath[myCon->camp() * 4 + 2][0]);
		myCon->chooseHero(HERO_NAME[3], initPath[myCon->camp() * 4 + 3][0]);
	}
	else
	{
		int heroCount = myHeros.size();
		int newHero = heroCount % 3;
		if (newHero == 1 || newHero == 2)
			newHero = 3 - newHero;
		if (heroCount == 7)
			newHero = 0;
		myCon->chooseHero(HERO_NAME[newHero]);
	}
}

void AIController::assignBaseAttack()
{
	PUnit *minBloodHero = nullptr;
	UnitFilter filter;
	filter.setAreaFilter(new Circle(myBase->pos, MILITARY_BASE_RANGE));
	for (auto x : myCon->enemyUnits(filter))
	{
		if (x->hp <= 0)
			continue;
		if (!minBloodHero)
			minBloodHero = x;
		else if (x->canUseSkill("Sacrifice"))
		{
			minBloodHero = x;
			break;
		}
		else if (minBloodHero->name != strHammerguard && x->name == strHammerguard)
			minBloodHero = x;
		else if (x->hp < minBloodHero->hp)
			minBloodHero = x;
	}

	if (!minBloodHero)
		if (enemiesInRange(AIController::ins()->myBase) == 0)
			for (auto x : myCon->friendlyUnits(filter))
				if (x->isHero() && x->hp < 100 && x->hp > 0)
					minBloodHero = x;
	myCon->baseAttack(minBloodHero);
}

void AIController::action()
{
	globalStrategy();

	for (size_t i = 0; i < myHeros.size(); ++i)
		AIHeroList.push_back(make_shared<AIHero>(myHeros[i]));
	for (size_t i = 0; i < myHeros.size(); ++i)
	{
		allStrategy.push_back(new Unavalable(myHeros[i]));
		if (myHeros[i]->findBuff("Reviving") != nullptr || myHeros[i]->findBuff("Dizzy") != nullptr)
		{
			AIHeroList[i]->chooseStrategy(allStrategy);
			continue;
		}

		if (myHeros[i]->name == strScouter)
			allStrategy.push_back(new PlugEye(myHeros[i]));
		else if (myHeros[i]->name == strMaster)
			allStrategy.push_back(new UseBlink(myHeros[i]));
		else if (myHeros[i]->name == strHammerguard)
			for (size_t j = 0; j < enemyHeros.size(); ++j)
				allStrategy.push_back(new UseHammerAttack(myHeros[i], enemyHeros[j]));
		else if (myHeros[i]->name == strBerserker)
			for (size_t j = 0; j < enemyHeros.size(); ++j)
				allStrategy.push_back(new UseSacrifice(myHeros[i], enemyHeros[j]));
		for (size_t j = 0; j < enemyHeros.size(); ++j)
			allStrategy.push_back(new Attack(myHeros[i], enemyHeros[j]));
		for (size_t j = 0; j < enemyHeros.size(); ++j)
			allStrategy.push_back(new Tempt(myHeros[i], enemyHeros[j]));
		allStrategy.push_back(new GoBackHome(myHeros[i]));
		allStrategy.push_back(new CallBackHome(myHeros[i]));
		allStrategy.push_back(new GoCenterMining(myHeros[i]));
		for (size_t j = 1; j < MINE_NUM; ++j)
			allStrategy.push_back(new GoMining(myHeros[i], MINE_POS[j]));
		allStrategy.push_back(new PushBase(myHeros[i]));
		AIHeroList[i]->chooseStrategy(allStrategy);
		Memory::ins()->lastStrategy[AIHeroList[i]->getHero()] = AIHeroList[i]->getStrategy()->getName();
	}
	for (size_t i = 0; i < AIHeroList.size(); ++i)
	{
		cout << AIHeroList[i]->getStrategy()->getName() << ' ' << AIHeroList[i]->getStrategy()->getWorth() << '\t';
		AIHeroList[i]->action();
	}
}
/********************************************************************************/


/****************************** Entry Function **********************************/
void player_ai(const PMap &map, const PPlayerInfo &info, PCommand &cmd)
{
	AIController controller(map, info, cmd);
	Memory::updateMemory();

	if (info.round == 0)
	{
		controller.buyNewHero();
		return;
	}

	controller.action();
}
/********************************************************************************/
