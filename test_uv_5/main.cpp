#include "xx_uv.h"
#include "../gens/output/PKG_class.h"
#include <iostream>

#define var decltype(auto)

struct GameEnv {
	std::unordered_map<int, PKG::Player_s> players;
	PKG::Scene_s scene;
};

int main() {
	PKG::AllTypesRegister();

	GameEnv env;
	xx::MakeShared(env.scene);
	xx::MakeShared(env.scene->monsters);
	xx::MakeShared(env.scene->players);

	var p1 = PKG::Player::MakeShared();
	var p2 = PKG::Player::MakeShared();
	var m1 = PKG::Monster::MakeShared();
	var m2 = PKG::Monster::MakeShared();

	p1->id = 123;
	xx::MakeShared(p1->token, "asdf");
	p1->target = m1;

	p2->id = 234;
	xx::MakeShared(p1->token, "qwer");
	p2->target = m2;

	env.players[p1->id] = p1;
	env.players[p2->id] = p2;

	env.scene->monsters->Add(m1, m2);
	env.scene->players->Add(p1, p2);

	std::cout << env.scene << std::endl;

	xx::BBuffer bb;
	{
		var sync = PKG::Sync::MakeShared();
		xx::MakeShared(sync->players);
		for (var player_w : *env.scene->players) {
			if (var player = player_w.lock()) {
				sync->players->Add(player);
			}
		}
		sync->scene = env.scene;
		bb.WriteRoot(sync);
	}
	std::cout << bb << std::endl;

	//env.players
	return 0;
}


























//int main() {
//	PKG::AllTypesRegister();
//	std::cout << PKG::PkgGenMd5::value << std::endl;
//
//	auto o = std::make_shared<PKG::Foo>();
//	o->parent = o;
//	o->childs = o->childs->Create();
//	o->childs->Add(o);
//	std::cout << o << std::endl;
//
//	xx::BBuffer bb;
//	bb.WriteRoot(o);
//	std::cout << bb << std::endl;
//
//	auto o2 = PKG::Foo::Create();
//	int r = bb.ReadRoot(o2);
//	std::cout << r << std::endl;
//	std::cout << o2 << std::endl;
//	
//	return 0;
//}
