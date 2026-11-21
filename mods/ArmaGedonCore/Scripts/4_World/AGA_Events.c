// ARMA-GEDON: dynamiczne eventy (obozy myśliwych, obozy cywilów, katastrofa samolotu, konwój medyczny)
// oraz automatyczne restarty serwera 2x dziennie.

class AGA_Event
{
	string Kind;
	vector Pos;
	int Born;
	int LifeMin;
	ref array<Object> Objs = new array<Object>;
}

class AGA_Events
{
	static ref array<ref AGA_Event> s_Active = new array<ref AGA_Event>;
	static int s_Minute;
	static int s_LastRestartWarn = -1;
	static bool s_Restarting;

	static const int RESTART_H1 = 5;
	static const int RESTART_H2 = 17;

	static ref array<string> HUNTER;
	static ref array<string> CIVIL;
	static ref array<string> MEDIC;
	static ref array<string> MILITARY;


	static void Init()
	{
		array<string> lHUNTER = {"Mosin9130", "Winchester70", "CZ527", "Izh18", "Ammo_762x54", "Ammo_308Win", "Ammo_762x39", "Mag_CZ527_5rnd", "HuntingKnife", "HuntingBag", "GhillieHood_Woodland", "GhillieBushrag_Woodland", "HuntingOptic", "Binoculars", "Compass", "HuntingJacket_Brown", "HunterPants_Brown", "Ammo_12gaRifled", "Ammo_12gaPellets", "Rope", "Canteen", "DeerSteakMeat", "WoodAxe"};
		HUNTER = lHUNTER;
		array<string> lCIVIL = {"TunaCan", "BakedBeansCan", "SpaghettiCan", "PeachesCan", "Canteen", "WaterBottle", "Matchbox", "Hatchet", "CanOpener", "Flashlight", "Battery9V", "Rag", "BandageDressing", "Hoodie_Blue", "Jeans_Blue", "AthleticShoes_Black", "TaloonBag_Blue", "MKII", "Ammo_22", "Hammer", "NailBox", "Pliers", "SewingKit", "CombinationLock"};
		CIVIL = lCIVIL;
		array<string> lMEDIC = {"BandageDressing", "Morphine", "Epinephrine", "SalineBagIV", "BloodBagIV", "BloodTestKit", "FirstAidKit", "TetracyclineAntibiotics", "PainkillerTablets", "CharcoalTablets", "VitaminBottle", "DisinfectantSpray", "GasMask", "NBCJacketGray", "NBCPantsGray", "NBCBootsGray", "NBCHoodGray", "NBCGlovesGray", "AntiChemInjector", "SurgicalGloves_Blue", "MedicalScrubsShirt_Green", "M4A1", "Mag_STANAG_30Rnd", "Ammo_556x45"};
		MEDIC = lMEDIC;
		array<string> lMILITARY = {"M4A1", "AKM", "SVD", "FAL", "Mag_STANAG_30Rnd", "Mag_AKM_30Rnd", "Mag_SVD_10Rnd", "Mag_FAL_20Rnd", "Ammo_556x45", "Ammo_762x39", "Ammo_762x54", "Ammo_308Win", "PlateCarrierVest", "BallisticHelmet_Green", "ACOGOptic", "PSO1Optic", "GorkaEJacket_Summer", "GorkaPants_Summer", "MilitaryBoots_Black", "NVGoggles", "M67Grenade", "AssaultBag_Green", "MilitaryBelt", "Mag_STANAG_30Rnd"};
		MILITARY = lMILITARY;
	}

	// ---------- pętla (co minutę z AGA_Server.RaidTick) ----------
	static void Tick()
	{
		if (!HUNTER) Init();
		s_Minute++;
		RestartTick();
		if (s_Minute < 3 || s_Restarting) return;

		for (int i = s_Active.Count() - 1; i >= 0; i--)
		{
			AGA_Event e = s_Active[i];
			if (s_Minute - e.Born >= e.LifeMin && !PlayerNear(e.Pos, 150))
			{
				Despawn(e);
				s_Active.Remove(i);
			}
		}
		Ensure("HUNTER", 4);
		Ensure("CIVIL", 3);
		Ensure("PLANE", 1);
		Ensure("CONVOY", 1);
	}

	static void Ensure(string kind, int want)
	{
		int have = 0;
		foreach (AGA_Event e : s_Active) if (e.Kind == kind) have++;
		if (have >= want) return;
		if (Math.RandomInt(0, 100) > 40) return; // rozłożenie w czasie
		Spawn(kind);
	}

	static int Count()
	{
		return s_Active.Count();
	}

	// ---------- szukanie miejsca ----------
	static bool PlayerNear(vector pos, float r)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
			if (m && vector.Distance(m.GetPosition(), pos) < r) return true;
		return false;
	}

	static vector FindSpot(int minTrees, int maxTrees)
	{
		for (int t = 0; t < 120; t++)
		{
			float x = Math.RandomFloat(1500, 13800);
			float z = Math.RandomFloat(1800, 14000);
			if (GetGame().SurfaceIsSea(x, z) || GetGame().SurfaceIsPond(x, z)) continue;
			float y = GetGame().SurfaceY(x, z);
			if (y < 5) continue;
			if (Math.AbsFloat(GetGame().SurfaceY(x + 4, z) - y) > 1.2 || Math.AbsFloat(GetGame().SurfaceY(x, z + 4) - y) > 1.2) continue;
			vector p = Vector(x, y, z);
			if (PlayerNear(p, 400)) continue;
			bool bad = false;
			foreach (AGA_Event e : s_Active) if (vector.Distance(e.Pos, p) < 1500) bad = true;
			if (bad) continue;
			array<Object> objs = new array<Object>;
			GetGame().GetObjectsAtPosition(p, 30, objs, null);
			int trees = 0;
			foreach (Object o : objs)
			{
				if (o.IsTree()) trees++;
				else if (o.IsBuilding() && vector.Distance(o.GetPosition(), p) < 30) bad = true;
			}
			if (bad || trees < minTrees || trees > maxTrees) continue;
			array<Object> far = new array<Object>;
			GetGame().GetObjectsAtPosition(p, 90, far, null);
			foreach (Object b : far) if (b.IsBuilding()) bad = true;
			if (bad) continue;
			return p;
		}
		return vector.Zero;
	}

	// ---------- tworzenie obiektów ----------
	static Object Put(AGA_Event e, string cls, vector off, float yaw)
	{
		if (!GetGame().ConfigIsExisting("CfgVehicles " + cls)) return null;
		vector p = e.Pos + off;
		p[1] = GetGame().SurfaceY(p[0], p[2]);
		Object o = GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE | ECE_CREATEPHYSICS);
		if (!o) return null;
		o.SetOrientation(Vector(yaw, 0, 0));
		o.SetPosition(p);
		EntityAI ent = EntityAI.Cast(o);
		if (ent) ent.SetLifetime(5400);
		e.Objs.Insert(o);
		return o;
	}

	static Object PutFirst(AGA_Event e, array<string> classes, vector off, float yaw)
	{
		foreach (string c : classes)
		{
			Object o = Put(e, c, off, yaw);
			if (o) return o;
		}
		return null;
	}

	static void Crate(AGA_Event e, vector off, array<string> pool, int count)
	{
		EntityAI box = EntityAI.Cast(Put(e, "WoodenCrate", off, Math.RandomFloat(0, 360)));
		for (int i = 0; i < count; i++)
		{
			string cls = pool.GetRandomElement();
			EntityAI it = null;
			if (box) it = box.GetInventory().CreateInInventory(cls);
			if (!it)
			{
				vector p = e.Pos + off + Vector(Math.RandomFloat(-2, 2), 0, Math.RandomFloat(-2, 2));
				p[1] = GetGame().SurfaceY(p[0], p[2]);
				it = EntityAI.Cast(GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE));
			}
			if (it) it.SetLifetime(5400);
		}
	}

	static void Loose(AGA_Event e, array<string> pool, int count, float r)
	{
		for (int i = 0; i < count; i++)
		{
			vector p = e.Pos + Vector(Math.RandomFloat(-r, r), 0, Math.RandomFloat(-r, r));
			p[1] = GetGame().SurfaceY(p[0], p[2]);
			EntityAI it = EntityAI.Cast(GetGame().CreateObjectEx(pool.GetRandomElement(), p, ECE_PLACE_ON_SURFACE));
			if (it) it.SetLifetime(5400);
		}
	}

	static void Fire(AGA_Event e, vector off)
	{
		FireplaceBase fp = FireplaceBase.Cast(Put(e, "Fireplace", off, 0));
		if (!fp) return;
		fp.GetInventory().CreateAttachment("Firewood");
		fp.GetInventory().CreateAttachment("WoodenStick");
		fp.GetInventory().CreateAttachment("Rag");
		fp.StartFire(true);
	}

	static void Tent(AGA_Event e, string cls, vector off, float yaw)
	{
		TentBase tent = TentBase.Cast(Put(e, cls, off, yaw));
		if (tent) tent.Pitch(true, true);
	}

	static void Zeds(AGA_Event e, array<string> classes, int n, float r)
	{
		for (int i = 0; i < n; i++)
		{
			vector p = e.Pos + Vector(Math.RandomFloat(-r, r), 0, Math.RandomFloat(-r, r));
			p[1] = GetGame().SurfaceY(p[0], p[2]);
			GetGame().CreateObjectEx(classes.GetRandomElement(), p, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP);
		}
	}

	// ---------- eventy ----------
	static void Spawn(string kind)
	{
		AGA_Event e = new AGA_Event;
		e.Kind = kind;
		e.Born = s_Minute;
		e.LifeMin = 60;
		if (kind == "HUNTER" || kind == "CIVIL") e.Pos = FindSpot(12, 400);
		else e.Pos = FindSpot(0, 3);
		if (e.Pos == vector.Zero) return;

		float yaw = Math.RandomFloat(0, 360);
		if (kind == "HUNTER")
		{
			Fire(e, "0 0 0");
			Tent(e, "MediumTent", Vector(-5, 0, 3), yaw);
			Put(e, "CamoNet", Vector(4, 0, -3), yaw);
			Put(e, "Firewood", Vector(2, 0, 2), yaw);
			Put(e, "WoodenLog", Vector(-2, 0, -3), yaw);
			Put(e, "WoodenLog", Vector(-2.6, 0, -3.4), yaw + 20);
			Crate(e, Vector(3, 0, 1), HUNTER, 7);
			Loose(e, HUNTER, 3, 6);
		}
		else if (kind == "CIVIL")
		{
			Fire(e, "0 0 0");
			Tent(e, "MediumTent", Vector(-5, 0, 2), yaw);
			Tent(e, "CarTent", Vector(5, 0, 4), yaw + 90);
			Put(e, "SeaChest", Vector(2, 0, -3), yaw);
			Put(e, "Firewood", Vector(-1.5, 0, 2.5), yaw);
			Crate(e, Vector(3, 0, -1), CIVIL, 8);
			Loose(e, CIVIL, 4, 7);
		}
		else if (kind == "PLANE")
		{
			e.LifeMin = 90;
			array<string> c1 = {"Land_Wreck_C130J", "Land_Wreck_C130J_2", "Land_Wreck_C130_Cargo", "Wreck_Mi8_Crashed"};
			Object w = PutFirst(e, c1, "0 0 0", yaw);
			if (w) AGA_Server.Get().Log("EVENT samolot: wrak " + w.GetType());
			Crate(e, Vector(10, 0, 6), MILITARY, 8);
			Crate(e, Vector(-9, 0, -7), MILITARY, 6);
			Loose(e, MILITARY, 5, 14);
			array<string> c2 = {"ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_PilotNormal"};
			Zeds(e, c2, 10, 25);
			Announce("PLANE CRASH", "A cargo plane went down somewhere in Chernarus. Military cargo reported on board.");
		}
		else if (kind == "CONVOY")
		{
			e.LifeMin = 75;
			array<string> c3 = {"Land_Wreck_V3S", "Land_Wreck_Ural"};
			PutFirst(e, c3, "0 0 0", yaw);
			array<string> c4 = {"Land_Wreck_Uaz", "Land_Wreck_Volha_Police", "Land_Wreck_V3S"};
			PutFirst(e, c4, Vector(9, 0, 4), yaw + 15);
			array<string> c5 = {"Land_Wreck_Ural", "Land_Wreck_V3S"};
			PutFirst(e, c5, Vector(-10, 0, -3), yaw - 10);
			Crate(e, Vector(4, 0, -4), MEDIC, 9);
			Crate(e, Vector(-4, 0, 5), MEDIC, 7);
			Loose(e, MEDIC, 5, 12);
			array<string> c6 = {"ZmbM_DoctorFat", "ZmbF_NurseFat", "ZmbF_DoctorSkinny", "ZmbM_SoldierNormal", "ZmbM_ParamedicNormal_Red"};
			Zeds(e, c6, 9, 20);
			Announce("MEDICAL CONVOY", "A medical convoy was ambushed. Medical supplies are scattered at the site.");
		}
		s_Active.Insert(e);
		AGA_Server.Get().Log("EVENT " + kind + " @ " + e.Pos.ToString() + " obiektow " + e.Objs.Count());
	}

	static void Despawn(AGA_Event e)
	{
		foreach (Object o : e.Objs)
			if (o) GetGame().ObjectDelete(o);
		AGA_Server.Get().Log("EVENT koniec " + e.Kind + " @ " + e.Pos.ToString());
	}

	static void Announce(string title, string text)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
			if (m && m.GetIdentity())
				NotificationSystem.SendNotificationToPlayerIdentityExtended(m.GetIdentity(), 12, title, text, "set:dayz_gui image:icon_info");
	}

	// ---------- restarty 05:00 i 17:00 (czas serwera) ----------
	static void RestartTick()
	{
		int h, m, s;
		GetHourMinuteSecond(h, m, s);
		int now = h * 60 + m;
		int left = 9999;
		array<int> hours = {RESTART_H1, RESTART_H2};
		foreach (int rh : hours)
		{
			int d = rh * 60 - now;
			if (d <= 0) d += 1440;
			if (d < left) left = d;
		}
		if (left > 30 && left < 1430) { s_LastRestartWarn = -1; return; }
		if (left == 1440 || left <= 0) left = 0;
		if ((left == 30 || left == 15 || left == 10 || left == 5 || left == 1) && s_LastRestartWarn != left)
		{
			s_LastRestartWarn = left;
			Announce("SERVER RESTART", "Server restart in " + left.ToString() + " min. Find a safe spot.");
		}
		if (left == 0 && !s_Restarting && s_Minute > 10)
		{
			s_Restarting = true;
			Announce("SERVER RESTART", "Restarting now. See you in a minute!");
			AGA_Server.Get().Log("Planowy restart serwera");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DoExit, 15000, false);
		}
	}

	static void DoExit()
	{
		GetGame().RequestExit(0);
	}
}
