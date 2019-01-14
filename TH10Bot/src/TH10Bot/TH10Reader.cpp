#include "TH10Bot/Common.h"
#include "TH10Bot/TH10Reader.h"

namespace th
{
	TH10Reader::TH10Reader(Process& process) :
		m_process(process)
	{
	}

	bool TH10Reader::getPlayer(Player& player)
	{
		uint32_t baseAddr = readMemory<uint32_t>(0x00477834);
		if (baseAddr == 0)
		{
			player = Player();
			return false;
		}
		player.x = readMemory<float>(baseAddr + 0x3C0);
		player.y = readMemory<float>(baseAddr + 0x3C4);
		player.dx = readMemory<int>(baseAddr + 0x3F0) / 100.0f;
		player.dy = readMemory<int>(baseAddr + 0x3F4) / 100.0f;
		player.w = readMemory<float>(baseAddr + 0x41C) * 2;
		player.h = player.w;
		player.slow = readMemory<int>(baseAddr + 0x4474);
		player.powers = readMemory<int>(0x00474C48) / 20.0f;
		player.type = readMemory<int>(0x00474C68);
		player.life = readMemory<int>(0x00474C70) + 1;
		player.itemObtainRange = readMemory<float>(0x00476FB0) + player.type * 4;
		if (player.slow)
			player.itemObtainRange *= 2.5;
		player.status = readMemory<int>(baseAddr + 0x458);
		player.invinibleTime = readMemory<int>(baseAddr + 0x4310);
		return true;
	}

	bool TH10Reader::getItems(std::vector<Item>& items)
	{
		items.clear();
		uint32_t baseAddr = readMemory<uint32_t>(0x00477818);
		if (baseAddr == 0)
			return false;
		uint32_t esi = baseAddr + 0x14;
		uint32_t ebp = esi + 0x3B0;
		for (int i = 0; i < 2000; ++i)
		{
			uint32_t eax = readMemory<uint32_t>(ebp + 0x2C);
			// 点的类型分为以下几种
			// eax == 0 不存在
			// eax == 1 正常的可收点
			// eax == 2 放B消除子弹产生的点
			// eax == 3 到达收点线、放B等自动回收的点
			// eax == 4 到达点的收取范围，自动回收的点
			if (eax == 1)
			{
				float x = readMemory<float>(ebp - 0x4);
				float y = readMemory<float>(ebp);
				float dx = readMemory<float>(ebp + 0x8);
				float dy = readMemory<float>(ebp + 0xC);
				int32_t type = readMemory<int32_t>(ebp + 0x30);
				// 正常点分为以下几种
				// type == 1 Power Items P点（红点）
				// type == 2 Point Items 得分点（蓝点）
				// type == 3 Faith Items 信仰点（绿点）
				// type == 4 Large Power Items 大P点（红点）
				// type == 5 Large Point Items 大得分点（带黄色边线的蓝点），由BOSS掉落
				// type == 6 Unknown
				// type == 7 Life Items 续命点（紫点、1UP点）
				// type == 8 Unknown
				// type == 9 Faith Items 信仰点（绿点），满灵力时由P点转化而来
				// type == 10 Power Items P点（红点），由BOSS掉落
				// 点没有宽度和高度，自机靠近点时会自动收取，为了方便显示设定为6
				items.emplace_back(x, y, 6.0, 6.0);
			}
			ebp += 0x3F0;
		}
		return true;
	}

	bool TH10Reader::getEnemies(std::vector<Enemy>& enemies)
	{
		enemies.clear();
		uint32_t baseAddr = readMemory<uint32_t>(0x00477704);
		if (baseAddr == 0)
			return false;
		uint32_t objBase = readMemory<uint32_t>(baseAddr + 0x58);
		if (objBase == 0)
			return false;
		while (true)
		{
			uint32_t objAddr = readMemory<uint32_t>(objBase);
			uint32_t objNext = readMemory<uint32_t>(objBase + 0x4);
			objAddr += 0x103C;
			uint32_t t = readMemory<uint32_t>(objAddr + 0x1444);
			if ((t & 0x40) == 0)
			{
				if ((t & 0x12) == 0)
				{
					float x = readMemory<float>(objAddr + 0x2C);
					float y = readMemory<float>(objAddr + 0x30);
					float dx = readMemory<float>(objAddr + 0x38);
					float dy = readMemory<float>(objAddr + 0x3C);
					float w = readMemory<float>(objAddr + 0xB8);
					float h = readMemory<float>(objAddr + 0xBC);
					enemies.emplace_back(x, y, w, h);
				}
			}
			if (objNext == 0)
				break;
			objBase = objNext;
		}
		return true;
	}

	bool TH10Reader::getBullets(std::vector<Bullet>& bullets)
	{
		bullets.clear();
		uint32_t baseAddr = readMemory<uint32_t>(0x004776F0);
		if (baseAddr == 0)
			return false;
		uint32_t ebx = baseAddr + 0x60;
		for (int i = 0; i < 2000; ++i)
		{
			uint32_t edi = ebx + 0x400;
			uint32_t bp = readMemory<uint32_t>(edi + 0x46);
			bp &= 0x0000FFFF;
			if (bp)
			{
				uint32_t eax = readMemory<uint32_t>(0x477810);
				if (eax)
				{
					eax = readMemory<uint32_t>(eax + 0x58);
					if (!(eax & 0x00000400))
					{
						float x = readMemory<float>(ebx + 0x3B4);
						float y = readMemory<float>(ebx + 0x3B8);
						float dx = readMemory<float>(ebx + 0x3C0);
						float dy = readMemory<float>(ebx + 0x3C4);
						float w = readMemory<float>(ebx + 0x3F0);
						float h = readMemory<float>(ebx + 0x3F4);
						bullets.emplace_back(x, y, w, h, dx, dy);
					}
				}
			}
			ebx += 0x7F0;
		}
		return true;
	}

	bool TH10Reader::getLasers(std::vector<Laser>& lasers)
	{
		lasers.clear();
		uint32_t baseAddr = readMemory<uint32_t>(0x0047781C);
		if (baseAddr == 0)
			return false;
		uint32_t objAddr = readMemory<uint32_t>(baseAddr + 0x18);
		if (objAddr == 0)
			return false;
		while (true)
		{
			uint32_t objNext = readMemory<uint32_t>(objAddr + 0x8);
			float x = readMemory<float>(objAddr + 0x24);
			float y = readMemory<float>(objAddr + 0x28);
			float dx = readMemory<float>(objAddr + 0x30);
			float dy = readMemory<float>(objAddr + 0x34);
			float arc = readMemory<float>(objAddr + 0x3C);
			float h = readMemory<float>(objAddr + 0x40);
			float w = readMemory<float>(objAddr + 0x44);
			lasers.emplace_back(x, y, w, h, arc);
			if (objNext == 0)
				break;
			objAddr = objNext;
		}
		return true;
	}
}
