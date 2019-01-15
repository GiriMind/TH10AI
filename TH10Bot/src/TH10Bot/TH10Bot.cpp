#include "TH10Bot/Common.h"
#include "TH10Bot/TH10Bot.h"

#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>

namespace th
{
	TH10Bot::TH10Bot() :
		m_process(Process::FindByName("th10.exe")),
		m_window(Window::FindByClassName("BASE")),
		//m_sync(m_process),
		m_reader(m_process),
		m_active(false),
		m_keyUp(VK_UP), m_keyDown(VK_DOWN), m_keyLeft(VK_LEFT), m_keyRight(VK_RIGHT),
		m_keyShift(VK_SHIFT), m_keyZ('Z'), m_keyX('X'),
		m_depthList(DEPTH),
		m_clipList(DEPTH),
		m_bombCooldown(0),
		m_talkCooldown(0),
		m_shootCooldown(0),
		m_pickupCooldown(0),
		m_slowManual(false)
	{
		srand((unsigned int)time(nullptr));
	}

	TH10Bot::~TH10Bot()
	{
	}

	void TH10Bot::start()
	{
		if (!m_active)
		{
			m_active = true;
			std::cout << "开启Bot。" << std::endl;
		}
	}

	void TH10Bot::stop()
	{
		if (m_active)
		{
			m_active = false;
			m_keyUp.release(), m_keyDown.release(), m_keyLeft.release(), m_keyRight.release(),
				m_keyShift.release(), m_keyZ.release(), m_keyX.release();
			std::cout << "停止Bot。" << std::endl;
		}
	}

	void TH10Bot::setSlowManual(bool slowManual)
	{
		m_slowManual = slowManual;
	}

	void TH10Bot::update()
	{
		//m_sync.waitForPresent();

		if (!m_active)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			return;
		}

		Rect rect = m_window.getClientRect();
		if (!m_capturer.capture(m_image, rect))
		{
			std::cout << "抓图失败：桌面没有变化导致超时，或者窗口位置超出桌面范围。" << std::endl;
			return;
		}

		//std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

		m_clock.update();

		m_reader.getPlayer(m_player);
		m_reader.getItems(m_items);
		m_reader.getEnemies(m_enemies);
		m_reader.getBullets(m_bullets);
		m_reader.getLasers(m_lasers);

		m_cutList.clear();
		for (int i = 0; i < m_bullets.size(); ++i)
		{
			const Bullet& bullet = m_bullets[i];
			double angle = bullet.angle(m_player);
			if (angle < 45.0 || m_player.hitTest(bullet, 100.0))
				m_cutList.push_back(i);
		}

		cv::Scalar color1(0, 255, 0);
		cv::Rect rect1(200 + 24 + m_player.x - m_player.w / 2, 16 + m_player.y - m_player.h / 2, m_player.w, m_player.h);
		cv::rectangle(m_image.m_data, rect1, color1, -1);
		cv::Rect rect11(200 + 32 + m_player.x - 200 / 2, 16 + m_player.y - 200 / 2, 200, 200);
		cv::rectangle(m_image.m_data, rect11, color1, 1);

		cv::Scalar color2(255, 0, 0);
		for (const Item& item : m_items)
		{
			cv::Rect rect(200 + 24 + item.x - item.w / 2, 16 + item.y - item.h / 2, item.w, item.h);
			cv::rectangle(m_image.m_data, rect, color2, -1);
		}

		cv::Scalar color3(0, 0, 255);
		for (const Enemy& enemy : m_enemies)
		{
			cv::Rect rect(200 + 24 + enemy.x - enemy.w / 2, 16 + enemy.y - enemy.h / 2, enemy.w, enemy.h);
			cv::rectangle(m_image.m_data, rect, color3, -1);
		}
		for (int i : m_cutList)
		{
			const Bullet& bullet = m_bullets[i];
			cv::Rect rect(200 + 24 + bullet.x - bullet.w / 2, 16 + bullet.y - bullet.h / 2, bullet.w, bullet.h);
			cv::rectangle(m_image.m_data, rect, color3, -1);
		}

		cv::imshow("TH10", m_image.m_data);
		cv::waitKey(1);

		return;

		//std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
		//time_t e1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
		//std::cout << "-----------------------------------------------------------" << std::endl;
		//std::cout << "e1: " << e1 << std::endl;

		handleBomb();
		if (!handleTalk())
			handleShoot();
		handleMove();

		//std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		//time_t e2 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
		//std::cout << "e2: " << e2 << std::endl;
		//std::cout << "-----------------------------------------------------------" << std::endl;
	}

	bool TH10Bot::IsInScene(double x, double y)
	{
		return x > -200.0 && x < 200.0 && y > 0.0 && y < 480.0;
	}

	void TH10Bot::FixPos(double& x, double& y)
	{
		if (x < -200.0)
			x = -200.0;
		if (x > 200.0)
			x = 200.0;
		if (y < 0.0)
			y = 0.0;
		if (y > 480.0)
			y = 480.0;
	}



	// Y轴系数
	double TH10Bot::GetYFactor(const Rect2d& source, const Rect2d& next)
	{
		double factor;

		if (next.y > source.y)
			factor = 1.0;
		else if (next.y < source.y)
			factor = -1.0;
		else
			factor = 0.0;

		return factor;
	}

	// 距离系数，远离加分，靠近减分
	double TH10Bot::GetDistFactor(double source, double next, double target)
	{
		double factor;

		double dSrc = std::abs(source - target);
		double dNext = std::abs(next - target);
		if (dNext > dSrc)
			factor = 1.0;
		else if (dNext < dSrc)
			factor = -1.0;
		else
			factor = 0.0;

		return factor;
	}

	double TH10Bot::GetDistXScore(double xNext, double xTarget)
	{
		double dx = std::abs(xNext - xTarget);
		if (dx > SCENE_WIDTH)
			dx = SCENE_WIDTH;
		return dx / SCENE_WIDTH;
	}

	double TH10Bot::GetDistYScore(double yNext, double yTarget)
	{
		double dy = std::abs(yNext - yTarget);
		if (dy > SCENE_HEIGHT)
			dy = SCENE_HEIGHT;
		return dy / SCENE_HEIGHT;
	}



	// 处理炸弹
	bool TH10Bot::handleBomb()
	{
		if (m_keyX.isPressed())
		{
			m_keyX.release();
			//std::cout << "炸弹 RELEASE" << std::endl;
		}

		// 放了炸弹3秒后再检测碰撞
		if (m_clock.getTimestamp() - m_bombCooldown >= 3000)
		{
			if (hitTestBomb())
			{
				m_bombCooldown = m_clock.getTimestamp();
				m_keyX.press();
				//std::cout << "炸弹 PRESS" << std::endl;
				//std::cout << m_log.str() << std::endl;
				return true;
			}
		}

		return false;
	}

	bool TH10Bot::hitTestBomb()
	{
		if (m_player.hitTest(m_enemies))
			return true;

		if (m_player.hitTest(m_bullets))
			return true;

		if (m_player.hitTestSAT(m_lasers))
			return true;

		return false;
	}

	// 处理对话
	bool TH10Bot::handleTalk()
	{
		if (m_enemies.size() > 1 || !m_bullets.empty() || !m_lasers.empty())
		{
			m_talkCooldown = m_clock.getTimestamp();
		}
		else
		{
			// 延时2秒后对话
			if (m_clock.getTimestamp() - m_talkCooldown >= 2000)
			{
				if (m_keyZ.isPressed())
				{
					m_keyZ.release();
					//std::cout << "对话 RELEASE" << std::endl;
				}
				else
				{
					m_keyZ.press();
					//std::cout << "对话 PRESS" << std::endl;
				}
				return true;
			}
		}

		return false;
	}

	// 处理攻击
	bool TH10Bot::handleShoot()
	{
		if (!m_enemies.empty())
		{
			m_shootCooldown = m_clock.getTimestamp();
			m_keyZ.press();
			//std::cout << "攻击 PRESS" << std::endl;
		}
		else
		{
			// 没有敌人1秒后停止攻击
			if (m_clock.getTimestamp() - m_shootCooldown >= 1000)
			{
				m_keyZ.release();
				//std::cout << "攻击 RELEASE" << std::endl;
				return false;
			}
		}

		return true;
	}

	// 处理移动
	bool TH10Bot::handleMove()
	{
		//std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();

		//m_depthList[0].clear();
		//m_depthList[0].assign(m_bullets.begin(), m_bullets.end());
		//for (int i = 1; i < DEPTH; ++i)
		//{
		//	m_depthList[i].clear();
		//	m_clipList[i].clear();
		//	Rect2d moveAera(m_player.x, m_player.y, m_player.w + MOVE_SPEED[0] * 2 * i, m_player.h + MOVE_SPEED[0] * 2 * i);
		//	for (const Bullet& bullet : m_depthList[i - 1])
		//	{
		//		Bullet next = bullet.advance();
		//		m_depthList[i].push_back(next);
		//		if (moveAera.hitTest(next, 0.0))
		//			m_clipList[i].push_back(next);
		//	}
		//}

		//std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
		//time_t e1 = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
		//std::cout << "-----------------------------------------------------------" << std::endl;
		//std::cout << "e1: " << e1 << std::endl;

		//int powerId = findPower();
		//int enemyId = findEnemy();

		POINT mousePos = getMousePos();
		//std::cout << mousePos.x << " " << mousePos.y << std::endl;
		Rect2d target = { static_cast<double>(mousePos.x), static_cast<double>(mousePos.y), 20.0, 20.0 };
		//if (!IsInScene(target.x, target.y))
		//{
		//	move(DIR_CENTER, false);
		//	return false;
		//}

		int lastDir = DIR_CENTER;
		bool lastSlow = false;
		double maxScore = std::numeric_limits<double>::lowest();
		int depth = 0;

		//m_log.str("");
		//m_log << "----------------------------------------------" << std::endl;
		//for (int m = 0; m < MOVE_SPEED_LEN - 1; ++m)
		//{
		for (int d = 0; d < DIR_LEN; ++d)
		{
			int dir = DIRECTION[d];
			bool slow = false;
			double score = 0.0;

			// 下一个位置
			double xNext = m_player.x + DIR_FACTOR[d].x * MOVE_SPEED[0];
			double yNext = m_player.y + DIR_FACTOR[d].y * MOVE_SPEED[0];

			//FixPos(xNext, yNext);
			if (!IsInScene(xNext, yNext))
			{
				std::cout << "!IsInScene " << xNext << " " << yNext << std::endl;
				continue;
			}

			Player pNext(static_cast<float>(xNext), static_cast<float>(yNext), m_player.w, m_player.h);
			if (hitTestMove(pNext))
			{
				slow = true;

				// 下一个位置
				xNext = m_player.x + DIR_FACTOR[d].x * MOVE_SPEED[1];
				yNext = m_player.y + DIR_FACTOR[d].y * MOVE_SPEED[1];

				//FixPos(xNext, yNext);
				if (!IsInScene(xNext, yNext))
				{
					std::cout << "!IsInScene " << xNext << " " << yNext << std::endl;
					continue;
				}

				pNext = Player(static_cast<float>(xNext), static_cast<float>(yNext), m_player.w, m_player.h);
				if (hitTestMove(pNext))
					continue;
			}

			score += getTargetScore(pNext, target);

			//score += 1.0;

			//score += search(pNext, depth + 1);

			//score += getDodgeEnemyScore(pNext);
			//score += getDodgeBulletScore(pNext);
			//score += getDodgeLaserScore(pNext);
			//if (powerId != -1)
			//	score += getPickupPowerScore(pNext, powerId);
			//else if (enemyId != -1)
			//	score += getShootEnemyScore(pNext, enemyId);
			//else
			//	score += getGobackScore(pNext);

			//std::cout << (int)dir << " " << slow << " " << score << std::endl;
			//m_log << (int)dir << " " << slow << " " << score << std::endl;
			if (score > maxScore)
			{
				maxScore = score;
				lastDir = dir;
				lastSlow = slow;
			}
		}
		//}

		//std::cout << "last " << (int)lastDir << " " << lastSlow << " " << maxScore << std::endl;
		//m_log << "last " << (int)lastDir << " " << lastSlow << " " << maxScore << std::endl;
		//m_log << "----------------------------------------------" << std::endl;
		move(lastDir, lastSlow);

		//std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
		//time_t e2 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
		//std::cout << "e2: " << e2 << std::endl;
		//std::cout << "-----------------------------------------------------------" << std::endl;

		return true;
	}

	POINT TH10Bot::getMousePos()
	{
		POINT mousePos = {};
		GetCursorPos(&mousePos);
		Rect clientRect = m_window.getClientRect();
		return { mousePos.x - clientRect.x - 24 - 200, mousePos.y - clientRect.y - 12 };
	}

	bool TH10Bot::hitTestMove(const Player& player)
	{
		for (const Enemy& enemy : m_enemies)
		{
			if (player.hitTest(enemy, 2.0))
				return true;
		}

		for (const Bullet& bullet : m_bullets)
		{
			if (player.hitTest(bullet, 2.0))
				return true;

			Bullet next = bullet.advance();
			if (player.hitTest(next, 2.0))
				return true;
		}

		for (const Laser& laser : m_lasers)
		{
			if (player.hitTestSAT(laser, 2.0))
				return true;
		}

		return false;
	}

	double TH10Bot::getTargetScore(const Player& pNext, const Rect2d& target)
	{
		double score = 0.0;

		if (pNext.hitTest(target))
		{
			score += 3000.0;
		}
		else
		{
			score += 1500.0 * (1.0 - GetDistXScore(pNext.x, target.x));
			score += 1500.0 * (1.0 - GetDistYScore(pNext.y, target.y));
		}

		return score;
	}

	double TH10Bot::search(const Player& player, int depth)
	{
		double score = 0.0;

		//Rect2d moveAera(player.x, player.y, player.w + MOVE_SPEED[0] * 2.0, player.h + MOVE_SPEED[0] * 2.0);
		//m_clipList[depth].clear();
		//for (const Bullet& bullet : m_depthList[depth])
		//{
		//	if (moveAera.hitTest(bullet, 0.0))
		//		m_clipList[depth].push_back(bullet);
		//}

		for (int m = 0; m < MOVE_SPEED_LEN - 1; ++m)
		{
			for (int d = 1; d < DIR_LEN; ++d)
			{
				int dir = DIRECTION[d];
				bool slow = static_cast<bool>(m);

				// 下一个位置
				double xNext = player.x + DIR_FACTOR[d].x * MOVE_SPEED[m];
				double yNext = player.y + DIR_FACTOR[d].y * MOVE_SPEED[m];

				//FixPos(xNext, yNext);
				if (xNext < -200.0 || xNext > 200.0 || yNext < 0.0 || yNext > 480.0)
					continue;

				Player pNext(static_cast<float>(xNext), static_cast<float>(yNext), player.w, player.h);

				// 剪枝：不折返
				if (m_player.distance(pNext) < m_player.distance(player))
					continue;

				if (hitTestMove(pNext, depth))
					continue;

				score += 1.0;

				//if (powerId != -1)
				//	score += getPickupPowerScore(pNext, powerId);
				//else if (enemyId != -1)
				//	score += getShootEnemyScore(pNext, enemyId);
				//else
				//	score += getGobackScore(pNext);

				if (depth + 1 < DEPTH)
					score += search(pNext, depth + 1);
			}
		}

		return score;
	}

	bool TH10Bot::hitTestMove(const Player& player, int depth)
	{
		if (depth == 0)
		{
			if (player.hitTest(m_enemies, 2.0))
				return true;

			if (player.hitTest(m_bullets, 2.0))
				return true;

			if (player.hitTestSAT(m_lasers, 2.0))
				return true;
		}
		else
		{
			// Enemy缺失

			if (player.hitTest(m_clipList[depth], 2.0))
				return true;

			// Laser缺失
		}

		return false;
	}

	// 查找道具
	int TH10Bot::findPower()
	{
		int id = -1;

		if (m_player.y < SCENE_HEIGHT / 4.0)
			return id;

		double minDist = std::numeric_limits<double>::max();
		int size = static_cast<int>(m_items.size());
		for (int i = 0; i < size; ++i)
		{
			const Item& item = m_items[i];

			// 高于1/5屏
			if (item.y < SCENE_HEIGHT / 5.0)
				continue;

			// 不在自机半屏内
			double dy = std::abs(m_player.y - item.y);
			if (dy > SCENE_HEIGHT / 4.0)
				continue;

			// 与自机距离最近的
			double distance = m_player.distance(item);
			if (distance < minDist)
			{
				minDist = distance;
				id = i;
			}
		}

		return id;
	}

	// 查找敌人
	int TH10Bot::findEnemy()
	{
		int id = -1;

		if (m_player.y < SCENE_HEIGHT / 4.0)
			return id;

		double minDist = std::numeric_limits<double>::max();
		int size = static_cast<int>(m_enemies.size());
		for (int i = 0; i < size; ++i)
		{
			const Enemy& enemy = m_enemies[i];

			//  与自机X轴距离最近
			double dx = std::abs(m_player.x - enemy.x);
			if (dx < minDist)
			{
				minDist = dx;
				id = i;
			}
		}

		return id;
	}

	double TH10Bot::getDodgeEnemyScore(const Player& pNext, double epsilon)
	{
		double score = 0.0;

		for (const Enemy& enemy : m_enemies)
		{
			if (pNext.hitTest(enemy, epsilon))
			{
				score = -10000.0;
				break;
			}
		}

		return score;
	}

	// 躲闪子弹评分
	// 帧同步后还是没法准确地碰撞检测，只好不要靠子弹那么近
	double TH10Bot::getDodgeBulletScore(const Player& pNext, double epsilon)
	{
		double score = 0.0;

		for (const Bullet& bullet : m_bullets)
		{
			if (pNext.hitTest(bullet, epsilon))
			{
				score = -10000.0;
				break;
			}

			Bullet bNext = bullet.advance();
			if (pNext.hitTest(bNext, epsilon))
			{
				score = -10000.0;
				break;
			}
		}

		return score;
	}

	double TH10Bot::getDodgeLaserScore(const Player& pNext, double epsilon)
	{
		double score = 0.0;

		for (const Laser& laser : m_lasers)
		{
			if (pNext.hitTestSAT(laser, epsilon))
			{
				score = -10000.0;
				break;
			}
		}

		return score;
	}

	double TH10Bot::getBulletAngleScore(const Player& pNext)
	{
		double score = 0.0;

		for (const Bullet& bullet : m_bullets)
		{
			double angle = bullet.angle(pNext);
			score = 10000 * (1.0 - angle / 180.0);
		}

		return score;
	}

	// 拾取道具评分
	double TH10Bot::getPickupPowerScore(const Player& pNext, int powerId)
	{
		double score = 0.0;

		if (powerId == -1)
			return score;

		const Item& item = m_items[powerId];
		if (pNext.hitTest(item, 5.0))
		{
			score += 3000.0;
		}
		else
		{
			score += 1500.0 * (1.0 - GetDistXScore(pNext.x, item.x));
			score += 1500.0 * (1.0 - GetDistYScore(pNext.y, item.y));
		}

		return score;
	}

	// 攻击敌人评分
	double TH10Bot::getShootEnemyScore(const Player& pNext, int enemyId)
	{
		double score = 0.0;

		if (enemyId == -1)
			return score;

		const Enemy& enemy = m_enemies[enemyId];
		double dx = std::abs(pNext.x - enemy.x);
		if (dx < 15.0)
		{
			score += 300.0;
		}
		else
		{
			// X轴距离越远得分越少
			if (dx > SCENE_WIDTH)
				dx = SCENE_WIDTH;
			score += 300.0 * (1.0 - dx / SCENE_WIDTH);
		}

		return score;
	}

	double TH10Bot::getGobackScore(const Player& pNext)
	{
		double score = 0.0;

		if (pNext.hitTest(INIT_RECT, 10.0))
		{
			score += 30.0;
		}
		else
		{
			score += 15.0 * (1.0 - GetDistXScore(pNext.x, INIT_RECT.x));
			score += 15.0 * (1.0 - GetDistYScore(pNext.y, INIT_RECT.y));
		}

		return score;
	}

	void TH10Bot::move(int dir, bool slow)
	{
		if (slow || m_slowManual)
		{
			m_keyShift.press();
			//std::cout << "慢速 PRESS" << std::endl;
		}
		if (!slow && !m_slowManual)
		{
			m_keyShift.release();
			//std::cout << "慢速 RELEASE" << std::endl;
		}

		switch (dir)
		{
		case DIR_UP | DIR_LEFT:
			m_keyUp.press(), m_keyDown.release(), m_keyLeft.press(), m_keyRight.release();
			//std::cout << "左上" << std::endl;
			break;

		case DIR_UP:
			m_keyUp.press(), m_keyDown.release(), m_keyLeft.release(), m_keyRight.release();
			//std::cout << "上" << std::endl;
			break;

		case DIR_UP | DIR_RIGHT:
			m_keyUp.press(), m_keyDown.release(), m_keyLeft.release(), m_keyRight.press();
			//std::cout << "右上" << std::endl;
			break;

		case DIR_LEFT:
			m_keyUp.release(), m_keyDown.release(), m_keyLeft.press(), m_keyRight.release();
			//std::cout << "左" << std::endl;
			break;

		case DIR_CENTER:
			m_keyUp.release(), m_keyDown.release(), m_keyLeft.release(), m_keyRight.release();
			//std::cout << "中" << std::endl;
			break;

		case DIR_RIGHT:
			m_keyUp.release(), m_keyDown.release(), m_keyLeft.release(), m_keyRight.press();
			//std::cout << "右" << std::endl;
			break;

		case DIR_DOWN | DIR_LEFT:
			m_keyUp.release(), m_keyDown.press(), m_keyLeft.press(), m_keyRight.release();
			//std::cout << "左下" << std::endl;
			break;

		case DIR_DOWN:
			m_keyUp.release(), m_keyDown.press(), m_keyLeft.release(), m_keyRight.release();
			//std::cout << "下" << std::endl;
			break;

		case DIR_DOWN | DIR_RIGHT:
			m_keyUp.release(), m_keyDown.press(), m_keyLeft.release(), m_keyRight.press();
			//std::cout << "右下" << std::endl;
			break;
		}
	}

	// 处理道具
	//bool TH10Bot::handlePower()
	//{
	//	if (checkPickupStatus())
	//	{
	//		int powerId = findPower();
	//		if (powerId != -1)
	//			return pickupPower(powerId);
	//	}
	//	return false;
	//}

	// 检测拾取状况
	//bool TH10Bot::checkPickupStatus()
	//{
	//	// 拾取冷却中
	//	if (m_clock.getTimestamp() - m_pickupCooldown < 2000)
	//		return false;
	//
	//	// 无道具
	//	if (m_powers.empty())
	//	{
	//		// 进入冷却
	//		m_pickupCooldown = m_clock.getTimestamp();
	//		return false;
	//	}
	//
	//	// 自机在上半屏，道具少于10个，敌人多于2个
	//	if (m_player.y < SCENE_HEIGHT / 2.0 && m_powers.size() < 10 && m_enemies.size() > 2)
	//	{
	//		// 进入冷却
	//		m_pickupCooldown = m_clock.getTimestamp();
	//		return false;
	//	}
	//
	//	// 自机高于1/4屏
	//	if (m_player.y < SCENE_HEIGHT * 0.25)
	//	{
	//		// 进入冷却
	//		m_pickupCooldown = m_clock.getTimestamp();
	//		return false;
	//	}
	//
	//	return true;
	//}

	// 拾取道具
	//bool TH10Bot::pickupPower(int powerId)
	//{
	//	const Power& power = m_powers[powerId];
	//
	//	// 靠近道具了
	//	if (m_player.hitTest(power, 5.0))
	//		return true;
	//
	//	int lastDir = DIR_NONE;
	//	bool lastShift = false;
	//	double maxScore = std::numeric_limits<double>::lowest();
	//	for (int i = 0; i < DIR_LENGHT; ++i)
	//	{
	//		int dir = DIRECTIONS[i];
	//		bool shift = false;
	//		double score = 0.0;
	//
	//		double xNext = m_player.x + MOVE_FACTORS[i].x * MOVE_SPEED;
	//		double yNext = m_player.y + MOVE_FACTORS[i].y * MOVE_SPEED;
	//		FixPos(xNext, yNext);
	//		Player next = { static_cast<float>(xNext), static_cast<float>(yNext), m_player.w, m_player.h };
	//		if (hitTest(next, 0.0))
	//		{
	//			shift = true;
	//			xNext = m_player.x + MOVE_FACTORS[i].x * MOVE_SPEED_SLOW;
	//			yNext = m_player.y + MOVE_FACTORS[i].y * MOVE_SPEED_SLOW;
	//			FixPos(xNext, yNext);
	//			next = { static_cast<float>(xNext), static_cast<float>(yNext), m_player.w, m_player.h };
	//			if (hitTest(next, 0.0))
	//				continue;
	//		}
	//
	//		score += pickupPowerScore(next, power);
	//
	//		if (score > maxScore)
	//		{
	//			maxScore = score;
	//			lastDir = dir;
	//			lastShift = shift;
	//		}
	//	}
	//	if (lastDir != DIR_NONE)
	//		move(lastDir, lastShift);
	//	else
	//		std::cout << "pickupPower()无路可走" << std::endl;
	//
	//	return true;
	//}

	// 处理敌人
	//bool TH10Bot::handleEnemy()
	//{
	//	if (checkShootStatus())
	//	{
	//		int enemyId = findEnemy();
	//		if (enemyId != -1)
	//			return shootEnemy(enemyId);
	//	}
	//	return false;
	//}

	// 检测攻击状况
	//bool TH10Bot::checkShootStatus()
	//{
	//	// 无敌人
	//	if (m_enemies.empty())
	//	{
	//		return false;
	//	}
	//
	//	return true;
	//}

	// 攻击敌人
	//bool TH10Bot::shootEnemy(int enemyId)
	//{
	//	const Enemy& enemy = m_enemies[enemyId];
	//
	//	int lastDir = DIR_NONE;
	//	bool lastShift = false;
	//	double maxScore = 1e-15;
	//	for (int i = 0; i < DIR_LENGHT; ++i)
	//	{
	//		int dir = DIRECTIONS[i];
	//		bool shift = false;
	//		double score = 0.0;
	//
	//		double xNext = m_player.x + MOVE_FACTORS[i].x * MOVE_SPEED;
	//		double yNext = m_player.y + MOVE_FACTORS[i].y * MOVE_SPEED;
	//		FixPos(xNext, yNext);
	//		Player next = { static_cast<float>(xNext), static_cast<float>(yNext), m_player.w, m_player.h };
	//		if (hitTest(next, 0.0))
	//		{
	//			shift = true;
	//			xNext = m_player.x + MOVE_FACTORS[i].x * MOVE_SPEED_SLOW;
	//			yNext = m_player.y + MOVE_FACTORS[i].y * MOVE_SPEED_SLOW;
	//			FixPos(xNext, yNext);
	//			next = { static_cast<float>(xNext), static_cast<float>(yNext), m_player.w, m_player.h };
	//			if (hitTest(next, 0.0))
	//				continue;
	//		}
	//
	//		score += dodgeEnemyScore(next);
	//		score += shootEnemyScore(next, enemy);
	//
	//		if (score > maxScore)
	//		{
	//			maxScore = score;
	//			lastDir = dir;
	//			lastShift = shift;
	//		}
	//	}
	//	if (lastDir != DIR_NONE)
	//		move(lastDir, lastShift);
	//	else
	//		std::cout << "shootEnemy()无路可走" << std::endl;
	//
	//	return true;
	//}

	// 闪避敌人评分
	//double TH10Bot::dodgeEnemyScore(const Player& next)
	//{
	//	double allScore = 0.0;
	//	for (const Enemy& enemy : m_enemies)
	//	{
	//		double score = 0.0;
	//
	//		// 在敌机范围50外 
	//		if (!next.hitTest(enemy, 50.0))
	//		{
	//			score += 100.0;
	//		}
	//		else
	//		{
	//			Point2d dirFactor = GetDirFactor(m_player, next, enemy);
	//			score += (dirFactor.x * 50.0 + dirFactor.y * 50.0);
	//		}
	//
	//		score += GetYFactor(m_player, next) * 100.0;
	//
	//		allScore += score;
	//	}
	//	return allScore;
	//}

	// 归位
	//void TH10Bot::goback()
	//{
	//	// 靠近初始位置了
	//	if (m_player.hitTest(INIT_RECT, 5.0))
	//	{
	//		move(DIR_CENTER, false);
	//		return;
	//	}
	//
	//	int lastDir = DIR_NONE;
	//	bool lastShift = false;
	//	double maxScore = 1e-15;
	//	for (int i = 0; i < DIR_LENGHT; ++i)
	//	{
	//		int dir = DIRECTIONS[i];
	//		bool shift = false;
	//		double score = 0.0;
	//
	//		double nextX = m_player.x + MOVE_FACTORS[i].x * MOVE_SPEED;
	//		double nextY = m_player.y + MOVE_FACTORS[i].y * MOVE_SPEED;
	//		FixPos(nextX, nextY);
	//		Player player = { static_cast<float>(nextX), static_cast<float>(nextY), m_player.w, m_player.h };
	//		if (hitTest(player, 0.0))
	//		{
	//			shift = true;
	//			nextX = m_player.x + MOVE_FACTORS[i].x * MOVE_SPEED_SLOW;
	//			nextY = m_player.y + MOVE_FACTORS[i].y * MOVE_SPEED_SLOW;
	//			FixPos(nextX, nextY);
	//			player = { static_cast<float>(nextX), static_cast<float>(nextY), m_player.w, m_player.h };
	//			if (hitTest(player, 0.0))
	//				continue;
	//		}
	//
	//		score += 1.0 - GetDistScore(player, INIT_RECT);
	//
	//		//std::cout << (int)dir << " " << score << std::endl;
	//		if (score > maxScore)
	//		{
	//			maxScore = score;
	//			lastDir = dir;
	//			lastShift = shift;
	//		}
	//	}
	//	//std::cout << "last " << (int)lastDir << " " << lastShift << std::endl;
	//	if (lastDir != DIR_NONE)
	//		move(lastDir, lastShift);
	//	else
	//		std::cout << "goback()无路可走" << std::endl;
	//}
}
