// ARMA-GEDON: dynamiczne eventy (katastrofa samolotu, konwój medyczny, obozy myśliwych i cywilów, plac budowy)
// Każdy obiekt jest osadzany wg własnego obrysu i nachylenia terenu (bez lewitowania / zapadania).
// Zombie i loot zależą od tieru okolicy. Do tego automatyczne restarty serwera 2x dziennie.

class AGA_SmokeColumn extends House
{
	protected Particle m_Smoke;

	void AGA_SmokeColumn()
	{
		if (!GetGame().IsDedicatedServer())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(StartSmoke, 300, false);
	}

	void ~AGA_SmokeColumn()
	{
		if (m_Smoke) m_Smoke.Stop();
	}

	void StartSmoke()
	{
		m_Smoke = ParticleManager.GetInstance().PlayOnObject(ParticleList.SMOKING_HELI_WRECK, this, "0 0 0");
	}
}

class AGA_Event
{
	string Kind;
	vector Pos;
	float Yaw;
	int Tier;
	int Born;
	int LifeMin;
	vector Tp;
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

	// ---------------- pętla (co minutę z AGA_Server) ----------------
	static void Tick()
	{
		s_Minute++;
		RestartTick();
		if (s_Minute < 3 || s_Restarting) return;

		for (int i = s_Active.Count() - 1; i >= 0; i--)
		{
			AGA_Event e = s_Active[i];
			if (s_Minute - e.Born >= e.LifeMin && !PlayerNear(e.Pos, 200))
			{
				Despawn(e);
				s_Active.Remove(i);
			}
		}
		Ensure("HUNTER", 4);
		Ensure("CIVIL", 3);
		Ensure("BUILD", 2);
		Ensure("CONVOY", 1);
		Ensure("PLANE", 1);
	}

	static void Ensure(string kind, int want)
	{
		int have = 0;
		foreach (AGA_Event e : s_Active) if (e.Kind == kind) have++;
		if (have >= want) return;
		if (Math.RandomInt(0, 100) > 45) return;
		Spawn(kind);
	}

	// ---------------- teren ----------------
	static bool PlayerNear(vector pos, float r)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
			if (m && vector.Distance(m.GetPosition(), pos) < r) return true;
		return false;
	}

	static string SurfAt(float x, float z)
	{
		string t;
		GetGame().SurfaceGetType3D(x, GetGame().SurfaceY(x, z) + 1.5, z, t);
		return t;
	}

	static bool Flat(vector p, float r, float tol)
	{
		float y = GetGame().SurfaceY(p[0], p[2]);
		for (int a = 0; a < 8; a++)
		{
			float ang = a * 45 * Math.DEG2RAD;
			float h = GetGame().SurfaceY(p[0] + Math.Cos(ang) * r, p[2] + Math.Sin(ang) * r);
			if (Math.AbsFloat(h - y) > tol) return false;
			if (GetGame().SurfaceIsSea(p[0] + Math.Cos(ang) * r, p[2] + Math.Sin(ang) * r)) return false;
		}
		return true;
	}

	// liczy drzewa w promieniu r, odległość do najbliższego drzewa i budynku
	static void Scan(vector p, float r, out int trees, out float nearTree, out float nearBuilding)
	{
		trees = 0;
		nearTree = 9999;
		nearBuilding = 9999;
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(p, r, objs, null);
		foreach (Object o : objs)
		{
			float d = vector.Distance(o.GetPosition(), p);
			if (o.IsTree() || o.IsBush())
			{
				if (o.IsTree()) trees++;
				if (d < nearTree) nearTree = d;
			}
			else if (o.IsBuilding() && d < nearBuilding)
				nearBuilding = d;
		}
	}

	static bool FarFromOthers(vector p)
	{
		if (PlayerNear(p, 350)) return false;
		foreach (AGA_Event e : s_Active) if (vector.Distance(e.Pos, p) < 1500) return false;
		return true;
	}

	static vector RandomLand()
	{
		float x = Math.RandomFloat(1200, 14200);
		float z = Math.RandomFloat(1800, 14600);
		vector p = Vector(x, GetGame().SurfaceY(x, z), z);
		if (p[1] < 4 || GetGame().SurfaceIsSea(x, z) || GetGame().SurfaceIsPond(x, z)) return vector.Zero;
		return p;
	}

	// tier okolicy jak w ekonomii Chernarusi: 1 wybrzeże … 4 północny zachód
	static int TierOf(vector p)
	{
		float x = p[0];
		float z = p[2];
		if (x < 6000 && z > 10500) return 4;
		if (z < 4300 || (x > 12000 && z < 10500)) return 1;
		if (z > 9000 || x < 4500) return 3;
		return 2;
	}

	static vector FindSpot(string kind)
	{
		for (int t = 0; t < 400; t++)
		{
			vector p = RandomLand();
			if (p == vector.Zero || !FarFromOthers(p)) continue;
			string s = SurfAt(p[0], p[2]);
			int trees;
			float nt, nb;
			if (kind == "PLANE")
			{
				if (!s.Contains("grass") && !s.Contains("dirt")) continue;
				if (!Flat(p, 30, 2.0) || !Flat(p, 15, 1.0)) continue;
				Scan(p, 130, trees, nt, nb);
				if (nt < 38 || nb < 130) continue;
				return p;
			}
			if (kind == "CONVOY" || kind == "BUILD")
			{
				if (!s.Contains("gravel") && !s.Contains("dirt") && !s.Contains("grass")) continue;
				if (!Flat(p, 16, 0.9) || !Flat(p, 8, 0.45)) continue;
				Scan(p, 170, trees, nt, nb);
				if (nt < 14 || nb < 28 || nb > 160) continue;
				return p;
			}
			if (kind == "CIVIL")
			{
				if (!s.Contains("grass") && !s.Contains("dirt")) continue;
				if (!Flat(p, 14, 0.8) || !Flat(p, 7, 0.35)) continue;
				Scan(p, 90, trees, nt, nb);
				if (nt < 15 || trees < 6 || nb < 160) continue;
				return p;
			}
			if (kind == "HUNTER")
			{
				if (!Flat(p, 9, 0.6) || !Flat(p, 5, 0.3)) continue;
				Scan(p, 45, trees, nt, nb);
				if (nt < 8 || trees < 14) continue;
				Scan(p, 250, trees, nt, nb);
				if (nb < 250) continue;
				return p;
			}
		}
		return vector.Zero;
	}

	// ---------------- osadzanie obiektów ----------------
	static vector Off(AGA_Event e, float right, float fwd)
	{
		vector f = Vector(e.Yaw, 0, 0).AnglesToVector();
		vector r = Vector(e.Yaw + 90, 0, 0).AnglesToVector();
		vector p = e.Pos + f * fwd + r * right;
		p[1] = GetGame().SurfaceY(p[0], p[2]);
		return p;
	}

	static vector Cross(vector a, vector b)
	{
		return Vector(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
	}

	// ustawia obiekt tak, by spód leżał na terenie i był pochylony zgodnie ze zboczem
	static void Align(Object o, vector pos, float yaw, float sink)
	{
		vector mm[2];
		if (!o.GetCollisionBox(mm)) o.ClippingInfo(mm);
		vector bmin = mm[0];
		vector bmax = mm[1];
		float halfL = Math.Max(0.6, (bmax[2] - bmin[2]) * 0.4);
		float halfW = Math.Max(0.6, (bmax[0] - bmin[0]) * 0.4);
		float lift = 0 - bmin[1] - sink;
		vector f = Vector(yaw, 0, 0).AnglesToVector();
		vector r = Vector(yaw + 90, 0, 0).AnglesToVector();
		vector pf = pos + f * halfL;
		vector pb = pos - f * halfL;
		vector pr = pos + r * halfW;
		vector pl = pos - r * halfW;
		pf[1] = GetGame().SurfaceY(pf[0], pf[2]);
		pb[1] = GetGame().SurfaceY(pb[0], pb[2]);
		pr[1] = GetGame().SurfaceY(pr[0], pr[2]);
		pl[1] = GetGame().SurfaceY(pl[0], pl[2]);
		float yc = GetGame().SurfaceY(pos[0], pos[2]);
		vector up = Cross(pf - pb, pr - pl);
		up.Normalize();
		if (up[1] < 0) up = up * -1;
		if (up[1] < 0.88) up = Vector(0, 1, 0);
		vector fw = f - up * vector.Dot(f, up);
		fw.Normalize();
		vector mat[4];
		Math3D.DirectionAndUpMatrix(fw, up, mat);
		// najniższy z punktów podparcia – brak szpar pod obiektem
		float ground = Math.Min(Math.Min(pf[1], pb[1]), Math.Min(pr[1], pl[1]));
		ground = (ground + yc) * 0.5;
		mat[3] = Vector(pos[0], ground, pos[2]) + up * lift;
		o.SetTransform(mat);
		o.Update();
	}

	static Object Put(AGA_Event e, string cls, float right, float fwd, float yawAdd, float sink = 0.05)
	{
		if (!GetGame().ConfigIsExisting("CfgVehicles " + cls)) return null;
		vector p = Off(e, right, fwd);
		Object o = GetGame().CreateObjectEx(cls, p, ECE_CREATEPHYSICS | ECE_UPDATEPATHGRAPH);
		if (!o) return null;
		Align(o, p, e.Yaw + yawAdd, sink);
		EntityAI ent = EntityAI.Cast(o);
		if (ent) ent.SetLifetime(5400);
		e.Objs.Insert(o);
		return o;
	}

	static Object PutAny(AGA_Event e, array<string> classes, float right, float fwd, float yawAdd, float sink = 0.05)
	{
		Object o = Put(e, classes.GetRandomElement(), right, fwd, yawAdd, sink);
		if (o) return o;
		foreach (string c : classes)
		{
			o = Put(e, c, right, fwd, yawAdd, sink);
			if (o) return o;
		}
		return null;
	}

	static void Tent(AGA_Event e, string cls, float right, float fwd, float yawAdd)
	{
		TentBase tent = TentBase.Cast(Put(e, cls, right, fwd, yawAdd, 0));
		if (!tent) return;
		tent.Pitch(true, true);
		Align(tent, tent.GetPosition(), e.Yaw + yawAdd, 0);
	}

	static void Fire(AGA_Event e, float right, float fwd)
	{
		FireplaceBase fp = FireplaceBase.Cast(Put(e, "Fireplace", right, fwd, 0, 0));
		if (!fp) return;
		fp.GetInventory().CreateAttachment("Firewood");
		fp.GetInventory().CreateAttachment("Firewood");
		fp.GetInventory().CreateAttachment("WoodenStick");
		fp.GetInventory().CreateAttachment("Rag");
		fp.StartFire(true);
		// kamienny krąg
		fp.GetInventory().CreateAttachment("Stone");
	}

	static string Pick(array<string> pool)
	{
		for (int i = 0; i < 6; i++)
		{
			string c = pool.GetRandomElement();
			if (GetGame().ConfigIsExisting("CfgVehicles " + c) || GetGame().ConfigIsExisting("CfgWeapons " + c) || GetGame().ConfigIsExisting("CfgMagazines " + c))
				return c;
		}
		return "";
	}

	static void Crate(AGA_Event e, string box, float right, float fwd, array<string> pool, int count)
	{
		EntityAI c = EntityAI.Cast(Put(e, box, right, fwd, Math.RandomFloat(-25, 25), 0));
		for (int i = 0; i < count; i++)
		{
			string cls = Pick(pool);
			if (cls == "") continue;
			EntityAI it = null;
			if (c) it = c.GetInventory().CreateInInventory(cls);
			if (!it) it = Drop(e, cls, right + Math.RandomFloat(-1.5, 1.5), fwd + Math.RandomFloat(-1.5, 1.5));
			if (it) it.SetLifetime(5400);
		}
	}

	static EntityAI Drop(AGA_Event e, string cls, float right, float fwd)
	{
		vector p = Off(e, right, fwd);
		EntityAI it = EntityAI.Cast(GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE));
		if (it)
		{
			it.PlaceOnSurface();
			it.SetLifetime(5400);
		}
		return it;
	}

	static void Scatter(AGA_Event e, array<string> pool, int count, float r0, float r1, float f0, float f1)
	{
		for (int i = 0; i < count; i++)
		{
			string cls = Pick(pool);
			if (cls != "") Drop(e, cls, Math.RandomFloat(r0, r1), Math.RandomFloat(f0, f1));
		}
	}

	static void Zeds(AGA_Event e, array<string> pool, int n, float rMin, float rMax)
	{
		for (int i = 0; i < n; i++)
		{
			string cls = pool.GetRandomElement();
			if (!GetGame().ConfigIsExisting("CfgVehicles " + cls)) continue;
			float a = Math.RandomFloat(0, 360);
			float d = Math.RandomFloat(rMin, rMax);
			vector p = Off(e, Math.Sin(a * Math.DEG2RAD) * d, Math.Cos(a * Math.DEG2RAD) * d);
			GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP);
		}
	}

	// zombie zależne od tieru: im wyższy tier, tym więcej i twardsze
	static void TierZeds(AGA_Event e, array<string> civ, int base, float rMin, float rMax)
	{
		array<string> mil = {"ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_PatrolNormal_PautRev", "ZmbM_usSoldier_normal_Woodland"};
		array<string> heavy = {"ZmbM_usSoldier_Heavy_Woodland", "ZmbM_NBC_Yellow", "ZmbM_NBC_Grey", "ZmbM_SoldierNormal"};
		Zeds(e, civ, base + e.Tier, rMin, rMax);
		if (e.Tier >= 2) Zeds(e, mil, e.Tier - 1, rMin, rMax);
		if (e.Tier >= 3) Zeds(e, heavy, e.Tier - 2, rMin, rMax);
	}

	// ---------------- pule lootu ----------------
	static array<string> LootHunter(int tier)
	{
		array<string> l = {"Winchester70", "CZ527", "Izh18", "Mosin9130", "Ammo_308Win", "Ammo_762x54", "Ammo_22", "Mag_CZ527_5rnd", "HuntingKnife", "HuntingBag", "HuntingOptic", "Binoculars", "Compass", "HuntingJacket_Autumn", "HuntingJacket_Brown", "HunterPants_Autumn", "HunterPants_Brown", "BoonieHat_DPM", "Canteen", "Rope", "DeerSteakMeat", "BakedBeansCan", "Matchbox", "WoodAxe", "Ammo_12gaPellets", "Ammo_12gaSlug"};
		if (tier >= 3)
		{
			array<string> rare = {"GhillieHood_Woodland", "GhillieBushrag_Woodland", "GhillieTop_Woodland", "SSG82", "Mag_SSG82_5rnd", "Ammo_545x39", "SVD", "PSO1Optic", "Mag_SVD_10Rnd"};
			l.InsertAll(rare);
		}
		return l;
	}

	static array<string> LootCivil(int tier)
	{
		array<string> l = {"TunaCan", "BakedBeansCan", "SpaghettiCan", "PeachesCan", "SardinesCan", "Canteen", "WaterBottle", "SodaCan_Cola", "Matchbox", "Hatchet", "CanOpener", "Flashlight", "Battery9V", "Rag", "BandageDressing", "Hoodie_Blue", "Hoodie_Black", "Jeans_Blue", "Jeans_Black", "AthleticShoes_Black", "HikingBootsLow_Black", "TaloonBag_Blue", "MountainBag_Green", "Sweater_Gray", "WoolCoat_Black", "Raincoat_Orange", "KitchenKnife", "Pot", "SewingKit", "Ammo_380", "MKII", "Ammo_22"};
		if (tier >= 2)
		{
			array<string> guns = {"CZ75", "Mag_CZ75_15Rnd", "Ammo_9x19", "Izh43Shotgun", "Ammo_12gaPellets", "Ruger1022", "Mag_Ruger1022_15Rnd", "Glock19", "Mag_Glock_15Rnd"};
			l.InsertAll(guns);
		}
		return l;
	}

	static array<string> LootBuild(int tier)
	{
		array<string> l = {"NailBox", "NailBox", "WoodenPlank", "WoodenPlank", "MetalPlate", "Hammer", "Shovel", "Pliers", "Hacksaw", "HandSaw", "Hatchet", "MetalWire", "BarbedWire", "Rope", "CombinationLock", "Fence_Kit", "Watchtower_Kit", "ConstructionHelmet_Orange", "ConstructionHelmet_Yellow", "ReflexVest", "WorkingBoots_Yellow", "WorkingGloves_Yellow", "Overalls_Blue", "Overalls_Gray", "Crowbar", "Pickaxe"};
		if (tier >= 2)
		{
			array<string> boom = {"PlasticExplosive", "RemoteDetonator", "RemoteDetonatorTrigger", "ImprovisedExplosive"};
			l.InsertAll(boom);
		}
		return l;
	}

	static array<string> LootMedic()
	{
		array<string> l = {"BandageDressing", "BandageDressing", "Bandana_RedPattern", "Morphine", "Epinephrine", "SalineBagIV", "BloodBagIV", "BloodTestKit", "FirstAidKit", "TetracyclineAntibiotics", "PainkillerTablets", "CharcoalTablets", "VitaminBottle", "DisinfectantSpray", "DisinfectantAlcohol", "AntiChemInjector", "SurgicalGloves_Blue", "SurgicalMask", "MedicalScrubsShirt_Green", "MedicalScrubsPants_Green", "GasMask", "GP5GasMask", "NBCJacketGray", "NBCPantsGray", "NBCBootsGray", "NBCHoodGray", "NBCGlovesGray", "Grenade_ChemGas", "Iodine"};
		return l;
	}

	static array<string> LootMilitary(int tier)
	{
		array<string> l = {"M4A1", "AKM", "AK74", "FAL", "Mag_STANAG_30Rnd", "Mag_AKM_30Rnd", "Mag_AK74_30Rnd", "Mag_FAL_20Rnd", "Ammo_556x45", "Ammo_762x39", "Ammo_545x39", "Ammo_308Win", "PlateCarrierVest", "BallisticHelmet_Green", "ACOGOptic", "M4_RISHndgrd", "GorkaEJacket_Summer", "GorkaPants_Summer", "MilitaryBoots_Black", "M67Grenade", "AssaultBag_Green", "MilitaryBelt", "TacticalGloves_Green", "Canteen", "BandageDressing", "Morphine"};
		if (tier >= 3)
		{
			array<string> top = {"SVD", "Mag_SVD_10Rnd", "Ammo_762x54", "PSO1Optic", "NVGoggles", "NVGHeadstrap", "M4_Suppressor", "AK_Suppressor", "PlateCarrierHolster", "PlateCarrierPouches", "Mag_STANAG_60Rnd", "VSS", "Mag_VSS_10Rnd", "Ammo_9x39"};
			l.InsertAll(top);
		}
		return l;
	}

	// ---------------- eventy ----------------
	static void Spawn(string kind)
	{
		vector pos = FindSpot(kind);
		if (pos == vector.Zero) return;
		AGA_Event e = new AGA_Event;
		e.Kind = kind;
		e.Pos = pos;
		e.Yaw = Math.RandomFloat(0, 360);
		e.Tier = TierOf(pos);
		e.Born = s_Minute;
		e.LifeMin = 60;

		if (kind == "PLANE") BuildPlane(e);
		else if (kind == "CONVOY") BuildConvoy(e);
		else if (kind == "HUNTER") BuildHunter(e);
		else if (kind == "CIVIL") BuildCivil(e);
		else if (kind == "BUILD") BuildSite(e);

		// punkt teleportu dla adminów – obok eventu, nie w środku obiektów
		if (kind == "PLANE") e.Tp = Off(e, 0, 32);
		else if (kind == "CONVOY") e.Tp = Off(e, 9, 2);
		else if (kind == "HUNTER") e.Tp = Off(e, 0, 7);
		else if (kind == "CIVIL") e.Tp = Off(e, -9, -6);
		else e.Tp = Off(e, 6, -13);
		e.Tp[1] = e.Tp[1] + 0.3;

		s_Active.Insert(e);
		AGA_Server.Get().Log("EVENT " + kind + " T" + e.Tier.ToString() + " @ " + e.Pos.ToString() + " obiektow " + e.Objs.Count().ToString());
	}

	// samolot transportowy rozbity w polu, za nim pas szczątków z najlepszym lootem
	static void BuildPlane(AGA_Event e)
	{
		e.LifeMin = 90;
		if (e.Tier < 3) e.Tier = 3;
		array<string> hull = {"Land_Wreck_C130J", "Land_Wreck_C130J_2"};
		PutAny(e, hull, 0, 0, 0, 0.6);
		Put(e, "StaticObj_Wreck_C130J_Engine", -9, -26, 35, 0.4);
		Put(e, "StaticObj_Wreck_C130J_Wing", 10, -34, -20, 0.3);
		Put(e, "StaticObj_Wreck_C130J_Engine", 4, -45, 110, 0.4);
		Put(e, "AGA_SmokeColumn", 2, 4, 0, 0);
		Put(e, "AGA_SmokeColumn", -6, -26, 0, 0);
		Fire(e, 8, -18);
		Fire(e, -4, -38);
		array<string> mil = LootMilitary(4);
		Crate(e, "WoodenCrate", -3, -22, mil, 8);
		Crate(e, "WoodenCrate", 6, -40, mil, 8);
		Crate(e, "WoodenCrate", 1, -52, mil, 6);
		Scatter(e, mil, 10, -10, 10, -55, -15);
		array<string> crew = {"ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_SoldierNormal", "ZmbM_usSoldier_normal_Woodland"};
		TierZeds(e, crew, 6, 12, 45);
		Announce("PLANE CRASH", "A military cargo plane went down somewhere in Chernarus. Follow the smoke.");
	}

	// rozbity konwój medyczny przy wiosce: karetka, ciężarówki, eskorta
	static void BuildConvoy(AGA_Event e)
	{
		e.LifeMin = 75;
		Put(e, "Land_Wreck_S1023_Medic_Beige", 0, 0, 0, 0.15);
		array<string> trucks = {"Land_Wreck_V3S", "StaticObj_Wreck_Ural"};
		PutAny(e, trucks, 0.5, 11, 4, 0.2);
		Put(e, "Land_Wreck_Offroad02_Medic_TK", -0.5, -9, -6, 0.15);
		array<string> escort = {"Land_Wreck_UAZ_Aban1_Police", "Land_Wreck_Uaz", "Land_Wreck_Volha_Police"};
		PutAny(e, escort, 3.5, -17, 25, 0.15);
		Put(e, "StaticObj_Misc_BagFence_3m", -5, 4, 90, 0);
		Put(e, "Land_Misc_FireBarrel_Red", 4, 3, 0, 0.05);
		array<string> med = LootMedic();
		Crate(e, "WoodenCrate", -3.5, 1, med, 8);
		Crate(e, "WoodenCrate", 3.5, 8, med, 7);
		Scatter(e, med, 8, -6, 6, -14, 16);
		array<string> doc = {"ZmbM_DoctorFat", "ZmbF_NurseFat", "ZmbF_DoctorSkinny", "ZmbM_ParamedicNormal_Red", "ZmbM_ParamedicNormal_Blue", "ZmbF_ParamedicNormal_Red"};
		TierZeds(e, doc, 5, 6, 30);
		Announce("MEDICAL CONVOY", "A medical convoy was ambushed near a settlement. Medical supplies and NBC gear reported.");
	}

	// obóz myśliwych na leśnej polanie
	static void BuildHunter(AGA_Event e)
	{
		Fire(e, 0, 0);
		Tent(e, "MediumTent", -4.5, 2.5, 90);
		Put(e, "StaticObj_Mil_CamoNet_Side_east", 4.5, 2, -90, 0);
		Put(e, "StaticObj_Misc_WoodPile_Forest1", -1, -4.5, 10, 0.05);
		Put(e, "Land_Misc_FireBarrel_Green", 2.5, -3, 0, 0.05);
		Put(e, "StaticObj_Misc_Pallet", 3.5, 4, 15, 0.02);
		array<string> loot = LootHunter(e.Tier);
		Crate(e, "WoodenCrate", 3.2, 2.5, loot, 4 + e.Tier);
		Scatter(e, loot, 2 + e.Tier, -3, 3, -3, 3);
		array<string> hunters = {"ZmbM_HunterOld_Autumn", "ZmbM_HunterOld_Spring", "ZmbM_HunterOld_Summer", "ZmbM_HikerSkinny_Green", "ZmbM_HikerSkinny_Grey"};
		TierZeds(e, hunters, 1, 10, 28);
	}

	// obóz cywilów: przyczepy kempingowe, porzucone auto, ognisko
	static void BuildCivil(AGA_Event e)
	{
		array<string> caravans = {"Land_Wreck_Caravan_MGreen", "Land_Wreck_Caravan_MRust", "Land_Wreck_Caravan_WBrown", "Land_Wreck_Caravan_WGreen"};
		PutAny(e, caravans, -8, 3, 80, 0.1);
		PutAny(e, caravans, 7, 5, -75, 0.1);
		array<string> cars = {"Land_Wreck_sed01_aban1_black", "Land_Wreck_hb01_aban1_blue", "Land_Wreck_Lada_Green", "Land_Wreck_Volha_Blue", "Land_Wreck_Pickup01_Aban_Red"};
		PutAny(e, cars, 1, 11, 160, 0.1);
		Fire(e, 0, 0);
		Put(e, "StaticObj_Misc_WoodPile1", -3, -4, 30, 0.05);
		Put(e, "Land_Misc_FireBarrel_Blue", 3.5, -2.5, 0, 0.05);
		Tent(e, "CarTent", 0, -8, 180);
		array<string> loot = LootCivil(e.Tier);
		Crate(e, "WoodenCrate", -4, 6, loot, 5 + e.Tier);
		Crate(e, "SeaChest", 4, 7, loot, 4);
		Scatter(e, loot, 3 + e.Tier, -6, 6, -5, 8);
		array<string> civ = {"ZmbM_CitizenASkinny_Blue", "ZmbF_CitizenANormal_Blue", "ZmbM_HikerSkinny_Blue", "ZmbF_HikerSkinny_Red", "ZmbM_FarmerFat_Brown", "ZmbF_JoggerSkinny_Red"};
		TierZeds(e, civ, 2, 10, 30);
	}

	// plac budowy na skraju wioski
	static void BuildSite(AGA_Event e)
	{
		e.LifeMin = 75;
		Put(e, "StaticObj_Wreck_Bulldozer", -8, 6, 20, 0.15);
		array<string> conts = {"Land_Container_1Bo", "Land_Container_1Mo", "Land_Container_1Aoh", "Land_Container_1Moh"};
		PutAny(e, conts, 8, 8, 90, 0.02);
		PutAny(e, conts, 9, -2, 90, 0.02);
		Put(e, "StaticObj_Misc_ConcretePipe", -3, -6, 75, 0.05);
		Put(e, "StaticObj_Misc_ConcretePipe", -3, -9, 80, 0.05);
		Put(e, "StaticObj_Misc_Pallets1", 2, 9, 0, 0.02);
		Put(e, "StaticObj_Misc_Pallets2", -1, 11, 10, 0.02);
		Put(e, "StaticObj_Misc_WoodPile1", 3, -8, 5, 0.03);
		Put(e, "Land_Misc_Toilet_Mobile", -10, -6, 160, 0.02);
		array<string> loot = LootBuild(e.Tier);
		Crate(e, "WoodenCrate", 4, 2, loot, 6 + e.Tier);
		Crate(e, "WoodenCrate", -2, 3, loot, 5 + e.Tier);
		Scatter(e, loot, 4 + e.Tier, -6, 6, -6, 10);
		array<string> workers = {"ZmbM_ConstrWorkerNormal_Beige", "ZmbM_ConstrWorkerNormal_Black", "ZmbM_ConstrWorkerNormal_Green", "ZmbM_ConstrWorkerNormal_Grey", "ZmbM_HeavyIndustryWorker"};
		TierZeds(e, workers, 3, 8, 26);
	}

	static string Label(AGA_Event e, int n)
	{
		string name = "Event";
		if (e.Kind == "PLANE") name = "Katastrofa samolotu";
		else if (e.Kind == "CONVOY") name = "Konwoj medyczny";
		else if (e.Kind == "HUNTER") name = "Oboz mysliwych";
		else if (e.Kind == "CIVIL") name = "Oboz cywilow";
		else if (e.Kind == "BUILD") name = "Plac budowy";
		return "[EVENT] " + name + " " + n.ToString() + " (T" + e.Tier.ToString() + ")";
	}

	static void Despawn(AGA_Event e)
	{
		foreach (Object o : e.Objs)
		{
			if (!o) continue;
			EntityAI ent = EntityAI.Cast(o);
			// nie kasuj rzeczy, które ktoś już zabrał do ekwipunku
			if (ent && ent.GetHierarchyRootPlayer()) continue;
			GetGame().ObjectDelete(o);
		}
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

	// ---------------- restarty 05:00 i 17:00 (czas serwera) ----------------
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
