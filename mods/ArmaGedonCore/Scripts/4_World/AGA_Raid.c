// ARMA-GEDON – godziny raidów (bazy i namioty przyjmują obrażenia tylko w oknach raidowych)
// Pn–Czw 18:00–00:00, Pt 18:00–02:00, Sob 14:00–02:00, Nd 14:00–00:00 (czas serwera)
class AGA_RaidConfig
{
	bool Enabled = true;
	string Info = "Raid hours (server time): Mon-Thu 18:00-00:00 | Fri 18:00-02:00 | Sat 14:00-02:00 | Sun 14:00-00:00";
}

class AGA_Raid
{
	static ref AGA_RaidConfig s_Cfg;
	static bool s_LastState;
	static bool s_Init;
	static ref map<string, int> s_WarnTime;

	static AGA_RaidConfig Cfg()
	{
		if (!s_Cfg)
		{
			s_Cfg = new AGA_RaidConfig();
			string path = "$profile:ArmaGedonCore/raid.json";
			if (FileExist(path))
				JsonFileLoader<AGA_RaidConfig>.JsonLoadFile(path, s_Cfg);
			else
				JsonFileLoader<AGA_RaidConfig>.JsonSaveFile(path, s_Cfg);
		}
		return s_Cfg;
	}

	// 0 = niedziela ... 6 = sobota (algorytm Sakamoto)
	static int DayOfWeek(int y, int m, int d)
	{
		array<int> t = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
		if (m < 3) y = y - 1;
		return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
	}

	static bool IsRaidTime()
	{
		if (!Cfg().Enabled)
			return true;
		int y, mo, d, h, mi, s;
		GetYearMonthDay(y, mo, d);
		GetHourMinuteSecond(h, mi, s);
		int dow = DayOfWeek(y, mo, d);
		// przedłużenie po północy: sobota 00-02 (po piątku) i niedziela 00-02 (po sobocie)
		if (h < 2 && (dow == 6 || dow == 0))
			return true;
		if (dow >= 1 && dow <= 5)
			return h >= 18;
		return h >= 14;
	}

	static bool IsPlayerDamage(EntityAI source)
	{
		if (!source) return false;
		if (PlayerBase.Cast(source)) return true;
		if (source.GetHierarchyRootPlayer()) return true;
		return false;
	}

	// czy zablokować obrażenia budowli; ostrzega gracza (max raz na 20 s)
	static bool Block(EntityAI target, EntityAI source)
	{
		if (!GetGame().IsServer())
			return false;
		if (IsRaidTime())
			return false;
		PlayerBase pb = PlayerBase.Cast(source);
		if (!pb && source)
			pb = PlayerBase.Cast(source.GetHierarchyRootPlayer());
		if (pb && pb.GetIdentity())
		{
			if (!s_WarnTime)
				s_WarnTime = new map<string, int>;
			string id = pb.GetIdentity().GetPlainId();
			int now = GetGame().GetTime();
			if (!s_WarnTime.Contains(id) || now - s_WarnTime.Get(id) > 20000)
			{
				s_WarnTime.Set(id, now);
				NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 8, "RAIDING IS CLOSED", Cfg().Info, "set:dayz_gui image:icon_info");
			}
		}
		return true;
	}

	// wywoływane co minutę z serwera: ogłoszenie otwarcia / zamknięcia okna
	static void Tick()
	{
		bool now = IsRaidTime();
		if (!s_Init)
		{
			s_Init = true;
			s_LastState = now;
			return;
		}
		if (now == s_LastState)
			return;
		s_LastState = now;
		if (now)
			NotificationSystem.SendNotificationToPlayerIdentityExtended(null, 15, "RAID WINDOW IS OPEN", "Bases can be raided now. " + Cfg().Info, "set:dayz_gui image:icon_info");
		else
			NotificationSystem.SendNotificationToPlayerIdentityExtended(null, 15, "RAID WINDOW IS CLOSED", "Bases are protected until the next raid window. " + Cfg().Info, "set:dayz_gui image:icon_info");
	}
}

modded class BaseBuildingBase
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (AGA_Raid.Block(this, source))
			return false;
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
}

modded class TentBase
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (AGA_Raid.Block(this, source))
			return false;
		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
}
