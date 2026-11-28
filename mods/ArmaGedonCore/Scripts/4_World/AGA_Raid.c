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

// ============================================================== obrażenia raidowe (tabela jak KarmaKrew)
// Liczby z tabeli to przebicie całego przejścia: połowa (drewno / metal) + rama.
//   kule: każde trafienie uderza w jedną strefę -> rama 40% liczby z kolumny drewna,
//         poszycie drewniane 60%, poszycie metalowe = liczba z kolumny metalu - rama
//   wybuchy: jeden wybuch rani wszystkie strefy w zasięgu naraz -> każda strefa dostaje pełną liczbę
// Ładunek improwizowany wybucha ładunkami w środku (plastik, granaty), więc liczy się jak one.
class AGA_RaidDmg
{
	static const float FRAME_SHARE = 0.4;

	// liczba trafień na przebicie (połowa + rama); explosive = czy to wybuch
	static void Table(string ammo, out float wood, out float metal, out bool explosive)
	{
		string a = ammo;
		a.ToLower();
		explosive = true;
		wood = 0; metal = 0;
		if (a.IndexOf("plastic") >= 0) { wood = 3; metal = 3; return; }
		if (a.IndexOf("claymore") >= 0) { wood = 10; metal = 15; return; }
		if (a.IndexOf("landmine") >= 0) { wood = 50; metal = 60; return; }
		if (a.IndexOf("grenade") >= 0 || a.IndexOf("rgd") >= 0 || a.IndexOf("m67") >= 0 || a.IndexOf("40mm_explosive") >= 0) { wood = 20; metal = 20; return; }
		explosive = false;
		if (a.IndexOf("pellet") >= 0) { wood = 150 * 8; metal = 215 * 8; return; }
		if (a.IndexOf("slug") >= 0) { wood = 350; metal = 450; return; }
		if (a.IndexOf("545") >= 0) { wood = 500; metal = 720; return; }
		if (a.IndexOf("556") >= 0) { wood = 550; metal = 720; return; }
		if (a.IndexOf("762x39") >= 0) { wood = 400; metal = 660; return; }
		if (a.IndexOf("762x54") >= 0 || a.IndexOf("308") >= 0 || a.IndexOf("mauser") >= 0 || a.IndexOf("792x57") >= 0 || a.IndexOf("9x39") >= 0) { wood = 250; metal = 360; return; }
		if (a.IndexOf("bullet") >= 0) { wood = 800; metal = 1100; return; }
	}

	// rodzaj strefy budowli bazy: 0 = nie dotyczy (zostaje config), 1 = rama, 2 = drewno, 3 = metal, 4 = cała ściana z bali
	static int Kind(string zone)
	{
		string z = zone;
		z.ToLower();
		if (z.IndexOf("spike") >= 0 || z.IndexOf("stairs") >= 0 || z.IndexOf("body") >= 0 || z.IndexOf("furniture") >= 0 || z.IndexOf("shelf") >= 0) return 0;
		if (z.IndexOf("woodenlogs") >= 0 || z.IndexOf("wire") >= 0 || z.IndexOf("camonet") >= 0) return 0;
		if (z == "tr_gate") return 2;
		if (z.IndexOf("metal") >= 0 || z.IndexOf("rollets") >= 0) return 3;
		if (z.IndexOf("_base") >= 0 || z.IndexOf("frame") >= 0 || z.IndexOf("stand") >= 0) return 1;
		if (z.IndexOf("wood") >= 0 || z.IndexOf("sashes") >= 0) return 2;
		if (z.IndexOf("gate") >= 0 || z.IndexOf("wall") >= 0) return 4;
		return 0;
	}

	// ile trafień tej amunicji niszczy daną strefę (0 = zostaje zwykły system obrażeń)
	static float HitsFor(string ammo, int kind)
	{
		float wood, metal;
		bool expl;
		Table(ammo, wood, metal, expl);
		if (wood <= 0 || kind <= 0) return 0;
		if (expl)
		{
			if (kind == 3) return metal;
			return wood;
		}
		if (kind == 1) return wood * FRAME_SHARE;
		if (kind == 2) return wood * (1 - FRAME_SHARE);
		if (kind == 3) return metal - wood * FRAME_SHARE;
		return wood;
	}

	// wybuch: pełne obrażenia do 2,5 m od ładunku, potem coraz mniej, od 6 m nic
	static float Falloff(EntityAI target, EntityAI source, vector modelPos)
	{
		if (!source) return 1;
		vector hit = target.ModelToWorld(modelPos);
		if (modelPos == vector.Zero) hit = target.GetPosition();
		float d = vector.Distance(source.GetPosition(), hit);
		if (d <= 2.5) return 1;
		if (d >= 6) return 0;
		return 1 - (d - 2.5) / 3.5;
	}

	// stara tabela (inne budowle): liczba trafień na jedną część
	static float Hits(string ammo, bool metal)
	{
		float w, m;
		bool e;
		Table(ammo, w, m, e);
		if (metal) return m;
		return w;
	}
}

modded class BaseBuildingBase
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (AGA_Raid.Block(this, source))
			return false;
		if (!GetGame().IsServer() || IsInherited(TerritoryFlag) || dmgZone == "")
			return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		// budowle bazy: stała liczba trafień wg tabeli
		if (ConfigGetBool("AG_BaseBuilding"))
		{
			int kind = AGA_RaidDmg.Kind(dmgZone);
			if (kind == 0)
				return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
			// ścian i drzwi nie da się rozbić bronią białą
			if (damageType == DamageType.CLOSE_COMBAT)
				return false;
			float n = AGA_RaidDmg.HitsFor(ammo, kind);
			if (n <= 0)
				return false;
			float f = 1;
			if (damageType == DamageType.EXPLOSION)
				f = AGA_RaidDmg.Falloff(this, source, modelPos);
			float mx = GetMaxHealth(dmgZone, "Health");
			if (mx > 0 && f > 0)
				DecreaseHealth(dmgZone, "Health", mx * f / n);
			return false;
		}

		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;
		if (damageType == DamageType.CLOSE_COMBAT)
			return true;
		string z = dmgZone;
		z.ToLower();
		float hits = AGA_RaidDmg.Hits(ammo, z.IndexOf("metal") >= 0);
		if (hits <= 0) return true;
		float m2 = GetMaxHealth(dmgZone, "Health");
		if (m2 <= 0) return true;
		DecreaseHealth(dmgZone, "Health", m2 / hits);
		return false;
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
