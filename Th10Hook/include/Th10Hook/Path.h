#pragma once

#include "Th10Hook/Common.h"

#include <boost/optional.hpp>

#include "Th10Hook/Status.h"
#include "Th10Hook/Scene.h"

namespace th
{
	struct Action
	{
		vec2 fromPos;
		DIR fromDir;
		float_t frame;

		//int_t willCollideCount;
		//float_t minCollideFrame;
		float_t score;
	};

	struct Result
	{
		bool valid;
		bool slow;		// ʵ���Ƿ�����
		float_t score;
		//int_t ttd;
	};

	class Path
	{
	public:
		Path(Status& status, Scene& scene,
			const boost::optional<Item>& itemTarget,
			const boost::optional<Enemy>& enemyTarget,
			bool underEnemy);

		Result find(DIR dir);
		Result dfs(const Action& action);

	//private:
		static float_t CalcFarScore(vec2 player, vec2 target);
		static float_t CalcNearScore(vec2 player, vec2 target);
		static float_t CalcShootScore(vec2 player, vec2 target);

		static const DIR FIND_DIRS[enum_cast(DIR::MAX_COUNT)][5];
		static const int_t FIND_DIR_COUNTS[enum_cast(DIR::MAX_COUNT)];
		static const int_t FIND_LIMIT;
		static const float_t FIND_DEPTH;
		static const vec2 RESET_POS;

		Status& m_status;
		Scene& m_scene;
		const boost::optional<Item>& m_itemTarget;
		const boost::optional<Enemy>& m_enemyTarget;
		bool m_underEnemy;

		DIR m_dir;
		bool m_slowFirst;			// �Ƿ���������

		float_t m_bestScore;
		int_t m_count;
	};
}