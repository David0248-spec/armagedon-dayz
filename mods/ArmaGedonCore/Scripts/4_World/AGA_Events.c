// ARMA-GEDON: dynamiczne eventy (katastrofa samolotu, konwój medyczny, obozy myśliwych i cywilów, plac budowy)
// Każdy obiekt jest osadzany wg własnego obrysu i nachylenia terenu (bez lewitowania / zapadania).
// Zombie i loot zależą od tieru okolicy. Do tego automatyczne restarty serwera 2x dziennie.

// słup dymu i trzask ognia – mechanika wraku heli z gry, bez modelu
class AGA_SmokeColumn extends Wreck_UH1Y
{
}

// skrzynia z lootem eventu: da się przeszukać, nie da się zabrać
class AGA_LootCrate extends WoodenCrate
{
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
}

class AGA_Event
{
	string Kind;
	string Variant;
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
		if (s_Minute == 1) LoadRoads();
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
		Ensure("HUNTER", 3);
		Ensure("CIVIL", 3);
		Ensure("BUILD", 2);
		Ensure("HELI", 2);
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

	// ---------------- drogi: pozycje aut / radiowozów / konwojów z cfgeventspawns ----------------
	static ref array<vector> s_Roads = new array<vector>; // x, kąt, z

	static string Attr(string line, string key)
	{
		int i = line.IndexOf(key + "=\"");
		if (i < 0) return "";
		int st = i + key.Length() + 2;
		string rest = line.Substring(st, line.Length() - st);
		int en = rest.IndexOf("\"");
		if (en < 0) return "";
		return rest.Substring(0, en);
	}

	static void LoadRoads()
	{
		if (s_Roads.Count() > 0) return;
		FileHandle fh = OpenFile("$mission:cfgeventspawns.xml", FileMode.READ);
		if (!fh) return;
		string line;
		bool use = false;
		while (FGets(fh, line) >= 0)
		{
			if (line.Contains("<event name="))
			{
				string ev = Attr(line, "name");
				use = (ev == "StaticPoliceCar" || ev == "StaticMilitaryConvoy" || ev == "VehicleCivilianSedan" || ev == "VehicleOffroadHatchback" || ev == "VehicleTruck01" || ev == "VehicleSedan02" || ev == "VehicleHatchback02");
				continue;
			}
			if (!use || !line.Contains("<pos ")) continue;
			string sx = Attr(line, "x");
			string sz = Attr(line, "z");
			string sa = Attr(line, "a");
			if (sx == "" || sz == "") continue;
			s_Roads.Insert(Vector(sx.ToFloat(), sa.ToFloat(), sz.ToFloat()));
		}
		CloseFile(fh);
		AGA_Server.Get().Log("Eventy: punktow drogowych " + s_Roads.Count().ToString());
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
			float qx = p[0] + Math.Cos(ang) * r;
			float qz = p[2] + Math.Sin(ang) * r;
			if (Math.AbsFloat(GetGame().SurfaceY(qx, qz) - y) > tol) return false;
			if (GetGame().SurfaceIsSea(qx, qz) || GetGame().SurfaceIsPond(qx, qz)) return false;
		}
		return true;
	}

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
			else if ((o.IsBuilding() || o.IsTransport()) && d < nearBuilding)
				nearBuilding = d;
		}
	}

	static bool FarFromOthers(vector p)
	{
		if (PlayerNear(p, 350)) return false;
		foreach (AGA_Event e : s_Active) if (vector.Distance(e.Pos, p) < 1200) return false;
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

	static int TierOf(vector p)
	{
		float x = p[0];
		float z = p[2];
		if (x < 6000 && z > 10500) return 4;
		if (z < 4300 || (x > 12000 && z < 10500)) return 1;
		if (z > 9000 || x < 4500) return 3;
		return 2;
	}

	// zwraca pozycję; yaw przez out (dla eventów drogowych = kierunek jezdni)
	static vector FindSpot(string kind, out float yaw)
	{
		yaw = Math.RandomFloat(0, 360);
		int trees;
		float nt, nb;
		for (int t = 0; t < 400; t++)
		{
			if (kind == "CONVOY" || kind == "BUILD")
			{
				if (s_Roads.Count() == 0) return vector.Zero;
				vector rp = s_Roads.GetRandomElement();
				vector p = Vector(rp[0], 0, rp[2]);
				yaw = rp[1];
				if (kind == "BUILD")
				{
					// obok drogi, kawałek od jezdni
					float side = 1;
					if (Math.RandomInt(0, 2) == 0) side = -1;
					vector rr = Vector(yaw + 90, 0, 0).AnglesToVector();
					p = p + rr * (side * Math.RandomFloat(20, 30));
				}
				p[1] = GetGame().SurfaceY(p[0], p[2]);
				if (!FarFromOthers(p)) continue;
				if (kind == "CONVOY")
				{
					if (!Flat(p, 10, 1.2)) continue;
					Scan(p, 20, trees, nt, nb);
					if (nb < 9) continue;
					return p;
				}
				if (!Flat(p, 13, 1.0) || !Flat(p, 7, 0.45)) continue;
				Scan(p, 200, trees, nt, nb);
				if (nt < 10 || nb < 16 || nb > 200) continue;
				return p;
			}
			vector q = RandomLand();
			if (q == vector.Zero || !FarFromOthers(q)) continue;
			string s = SurfAt(q[0], q[2]);
			if (kind == "PLANE")
			{
				if (!s.Contains("grass") && !s.Contains("dirt")) continue;
				if (!Flat(q, 30, 2.0) || !Flat(q, 15, 1.0)) continue;
				Scan(q, 130, trees, nt, nb);
				if (nt < 38 || nb < 130) continue;
				return q;
			}
			if (kind == "HELI")
			{
				if (!Flat(q, 14, 1.2) || !Flat(q, 7, 0.6)) continue;
				Scan(q, 90, trees, nt, nb);
				if (nt < 16 || nb < 90) continue;
				return q;
			}
			if (kind == "CIVIL")
			{
				if (!s.Contains("grass") && !s.Contains("dirt")) continue;
				if (!Flat(q, 14, 0.8) || !Flat(q, 7, 0.35)) continue;
				Scan(q, 90, trees, nt, nb);
				if (nt < 15 || trees < 6 || nb < 160) continue;
				return q;
			}
			if (kind == "HUNTER")
			{
				if (!Flat(q, 9, 0.6) || !Flat(q, 5, 0.3)) continue;
				Scan(q, 45, trees, nt, nb);
				if (nt < 8 || trees < 14) continue;
				Scan(q, 250, trees, nt, nb);
				if (nb < 250) continue;
				return q;
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

	static void Fire(AGA_Event e, float right, float fwd)
	{
		FireplaceBase fp = FireplaceBase.Cast(Put(e, "Fireplace", right, fwd, 0, 0));
		if (!fp) return;
		fp.GetInventory().CreateAttachment("Firewood");
		fp.GetInventory().CreateAttachment("Firewood");
		fp.GetInventory().CreateAttachment("WoodenStick");
		fp.GetInventory().CreateAttachment("Rag");
		fp.StartFire(true);
	}

	static void Smoke(AGA_Event e, float right, float fwd)
	{
		Put(e, "AGA_SmokeColumn", right, fwd, 0, 0);
	}

	// wybuch słyszalny z daleka
	static void Boom(AGA_Event e)
	{
		vector p = Off(e, 0, 0);
		p[1] = p[1] + 1;
		ExplosivesBase ex = ExplosivesBase.Cast(GetGame().CreateObjectEx("M67Grenade", p, ECE_NONE));
		if (ex) ex.Explode(DamageType.EXPLOSION);
	}

	// ---------------- loot ----------------
	static bool Chance(float p)
	{
		return Math.RandomFloat01() < p;
	}

	static bool Valid(string c)
	{
		return GetGame().ConfigIsExisting("CfgVehicles " + c) || GetGame().ConfigIsExisting("CfgWeapons " + c) || GetGame().ConfigIsExisting("CfgMagazines " + c);
	}

	static EntityAI LootCrate(AGA_Event e, float right, float fwd)
	{
		return EntityAI.Cast(Put(e, "AGA_LootCrate", right, fwd, Math.RandomFloat(-20, 20), 0));
	}

	static void Give(EntityAI c, string cls)
	{
		if (!c || cls == "" || !Valid(cls)) return;
		EntityAI it = c.GetInventory().CreateInInventory(cls);
		if (it) it.SetLifetime(5400);
	}

	static void GiveFrom(EntityAI c, array<string> pool, int n)
	{
		for (int i = 0; i < n; i++) Give(c, pool.GetRandomElement());
	}

	// przedmiot leżący na statycznej skrzyni / palecie
	static void OnTop(Object baseObj, string cls)
	{
		if (!baseObj || !Valid(cls)) return;
		vector mm[2];
		if (!baseObj.GetCollisionBox(mm)) baseObj.ClippingInfo(mm);
		vector bmax = mm[1];
		vector p = baseObj.GetPosition();
		p[0] = p[0] + Math.RandomFloat(-0.3, 0.3);
		p[2] = p[2] + Math.RandomFloat(-0.3, 0.3);
		p[1] = p[1] + bmax[1];
		EntityAI it = EntityAI.Cast(GetGame().CreateObjectEx(cls, p, ECE_NONE));
		if (!it) return;
		vector im[2];
		it.ClippingInfo(im);
		vector imin = im[0];
		p[1] = p[1] - imin[1] + 0.01;
		it.SetPosition(p);
		it.SetOrientation(Vector(Math.RandomFloat(0, 360), 0, 0));
		it.SetLifetime(5400);
	}

	// przedmiot na ziemi – fizyka sama go położy
	static void Drop(AGA_Event e, string cls, float right, float fwd)
	{
		if (!Valid(cls)) return;
		vector p = Off(e, right, fwd);
		p[1] = p[1] + 0.25;
		EntityAI it = EntityAI.Cast(GetGame().CreateObjectEx(cls, p, ECE_CREATEPHYSICS));
		if (it) it.SetLifetime(5400);
	}

	// ---------------- zombie ----------------
	static ZombieBase Zed(AGA_Event e, string cls, float rMin, float rMax)
	{
		if (!GetGame().ConfigIsExisting("CfgVehicles " + cls)) return null;
		float a = Math.RandomFloat(0, 360);
		float d = Math.RandomFloat(rMin, rMax);
		vector p = Off(e, Math.Sin(a * Math.DEG2RAD) * d, Math.Cos(a * Math.DEG2RAD) * d);
		return ZombieBase.Cast(GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP));
	}

	static void Zeds(AGA_Event e, array<string> pool, int n, float rMin, float rMax)
	{
		for (int i = 0; i < n; i++) Zed(e, pool.GetRandomElement(), rMin, rMax);
	}

	// opancerzony żołnierz – po zabiciu można zabrać plate carrier
	static void PlateZed(AGA_Event e, float rMin, float rMax)
	{
		ZombieBase z = Zed(e, "ZmbM_usSoldier_Heavy_Woodland", rMin, rMax);
		if (!z) z = Zed(e, "ZmbM_SoldierNormal", rMin, rMax);
		if (!z) return;
		EntityAI v = z.GetInventory().CreateInInventory("PlateCarrierVest");
		if (!v) z.GetInventory().CreateAttachment("PlateCarrierVest");
	}

	// ---------------- eventy ----------------
	static void Spawn(string kind)
	{
		float yaw;
		vector pos = FindSpot(kind, yaw);
		if (pos == vector.Zero) return;
		AGA_Event e = new AGA_Event;
		e.Kind = kind;
		e.Pos = pos;
		e.Yaw = yaw;
		e.Tier = TierOf(pos);
		e.Born = s_Minute;
		e.LifeMin = 60;

		if (kind == "PLANE") BuildPlane(e);
		else if (kind == "HELI") BuildHeli(e);
		else if (kind == "CONVOY") BuildConvoy(e);
		else if (kind == "HUNTER") BuildHunter(e);
		else if (kind == "CIVIL") BuildCivil(e);
		else if (kind == "BUILD") BuildSite(e);

		if (kind == "PLANE") e.Tp = Off(e, 0, 32);
		else if (kind == "HELI") e.Tp = Off(e, 0, 16);
		else if (kind == "CONVOY") e.Tp = Off(e, 7, 3);
		else if (kind == "HUNTER") e.Tp = Off(e, 0, 7);
		else if (kind == "CIVIL") e.Tp = Off(e, -9, -6);
		else e.Tp = Off(e, 6, -13);
		e.Tp[1] = e.Tp[1] + 0.3;

		s_Active.Insert(e);
		AGA_Server.Get().Log("EVENT " + kind + e.Variant + " T" + e.Tier.ToString() + " @ " + e.Pos.ToString() + " obiektow " + e.Objs.Count().ToString());
	}

	// samolot transportowy w polu, za nim pas szczątków i rozsypany ładunek
	static void BuildPlane(AGA_Event e)
	{
		e.LifeMin = 90;
		if (e.Tier < 3) e.Tier = 3;
		array<string> hull = {"Land_Wreck_C130J", "Land_Wreck_C130J_2"};
		PutAny(e, hull, 0, 0, 0, 0.6);
		Put(e, "StaticObj_Wreck_C130J_Engine", -9, -26, 35, 0.4);
		Put(e, "StaticObj_Wreck_C130J_Wing", 10, -34, -20, 0.3);
		Put(e, "StaticObj_Wreck_C130J_Engine", 4, -48, 110, 0.4);
		Smoke(e, 3, 2);
		Smoke(e, -9, -26);
		Fire(e, 8, -18);
		Fire(e, 4, -46);
		// rozsypany ładunek – statyczne skrzynie i palety z rzeczami na wierzchu
		Object s1 = Put(e, "StaticObj_Misc_WoodenCrate_5x", -5, -20, 25, 0.02);
		Object s2 = Put(e, "StaticObj_Misc_WoodenCrate_3x", 6, -28, -40, 0.02);
		Object s3 = Put(e, "StaticObj_Misc_BoxWooden", -2, -37, 70, 0.02);
		Put(e, "StaticObj_Misc_Pallets1", 8, -41, 15, 0.02);
		array<string> top = {"Mag_STANAG_30Rnd", "Mag_AKM_30Rnd", "Mag_AK74_30Rnd", "Ammo_556x45", "Ammo_762x39", "Ammo_545x39", "BandageDressing", "Canteen", "M67Grenade"};
		OnTop(s1, top.GetRandomElement());
		OnTop(s2, top.GetRandomElement());
		if (Chance(0.5)) OnTop(s3, top.GetRandomElement());
		// skrzynie zaopatrzeniowe (do przeszukania, nie do zabrania)
		array<string> guns = {"M4A1", "AKM", "AK74", "FAL", "AK101"};
		array<string> rare = {"SVD", "VSS", "M249"};
		array<string> gear = {"PlateCarrierVest", "BallisticHelmet_Green", "GorkaEJacket_Summer", "GorkaPants_Summer", "MilitaryBoots_Black", "AssaultBag_Green", "TacticalGloves_Green", "MilitaryBelt", "TTSKOJacket_Camo", "TTSKOPants"};
		array<string> extra = {"ACOGOptic", "PSO1Optic", "KobraOptic", "M4_Suppressor", "NVGoggles", "M67Grenade", "RGD5Grenade"};
		EntityAI c1 = LootCrate(e, -3, -30);
		if (Chance(0.12)) Give(c1, rare.GetRandomElement());
		else if (Chance(0.65)) Give(c1, guns.GetRandomElement());
		GiveFrom(c1, top, 1);
		GiveFrom(c1, gear, 1);
		EntityAI c2 = LootCrate(e, 5, -44);
		GiveFrom(c2, gear, 2);
		if (Chance(0.35)) GiveFrom(c2, extra, 1);
		// tylko wojskowe zombie + szansa na opancerzonych
		array<string> mil = {"ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_PatrolNormal_PautRev", "ZmbM_usSoldier_normal_Woodland"};
		Zeds(e, mil, 6 + e.Tier, 12, 45);
		if (Chance(0.6)) PlateZed(e, 15, 40);
		if (Chance(0.25)) PlateZed(e, 15, 40);
		Boom(e);
		Announce("PLANE CRASH", "A military cargo plane went down somewhere in Chernarus. Follow the smoke.");
	}

	// katastrofa helikoptera: wojskowa / cywilna / medyczna
	static void BuildHeli(AGA_Event e)
	{
		e.LifeMin = 75;
		int v = Math.RandomInt(0, 100);
		EntityAI c;
		if (v < 45)
		{
			e.Variant = "_MIL";
			array<string> mw = {"Wreck_UH1Y", "Wreck_Mi8_Crashed"};
			PutAny(e, mw, 0, 0, 0, 0.3);
			array<string> mguns = {"M4A1", "AKM", "AK74", "SKS", "Mosin9130", "UMP45"};
			array<string> mgear = {"Mag_STANAG_30Rnd", "Mag_AKM_30Rnd", "Ammo_556x45", "Ammo_762x39", "BallisticHelmet_Green", "MilitaryBoots_Black", "TacticalGloves_Green", "AssaultBag_Green", "PlateCarrierVest", "BandageDressing", "M67Grenade"};
			c = LootCrate(e, 6, -5);
			if (Chance(0.55)) Give(c, mguns.GetRandomElement());
			GiveFrom(c, mgear, 2);
			Drop(e, mgear.GetRandomElement(), -5, 4);
			array<string> mz = {"ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_usSoldier_normal_Woodland"};
			Zeds(e, mz, 3 + e.Tier, 8, 30);
			if (e.Tier >= 3 && Chance(0.3)) PlateZed(e, 10, 25);
		}
		else if (v < 75)
		{
			e.Variant = "_CIV";
			Put(e, "Land_Wreck_Mi8_Civilian", 0, 0, 0, 0.3);
			Smoke(e, 2, 1);
			array<string> cl = {"Hoodie_Blue", "Jeans_Blue", "HikingBootsLow_Black", "MountainBag_Green", "TunaCan", "BakedBeansCan", "WaterBottle", "Flashlight", "Battery9V", "Binoculars", "CZ75", "Mag_CZ75_15Rnd", "Ammo_9x19", "Compass", "Matchbox"};
			c = LootCrate(e, 7, -4);
			GiveFrom(c, cl, 3);
			Drop(e, cl.GetRandomElement(), -6, 3);
			array<string> cz = {"ZmbM_CitizenASkinny_Blue", "ZmbF_CitizenANormal_Blue", "ZmbM_HikerSkinny_Blue", "ZmbM_PilotNormal", "ZmbM_OffshoreWorker_Green"};
			Zeds(e, cz, 3 + e.Tier, 8, 30);
		}
		else
		{
			e.Variant = "_MED";
			Put(e, "Wreck_UH1Y", 0, 0, 0, 0.3);
			array<string> md = {"BandageDressing", "BandageDressing", "Morphine", "Epinephrine", "SalineBagIV", "BloodBagIV", "BloodTestKit", "FirstAidKit", "TetracyclineAntibiotics", "PainkillerTablets", "CharcoalTablets", "DisinfectantSpray", "SurgicalGloves_Blue", "MedicalScrubsShirt_Green"};
			c = LootCrate(e, 6, -5);
			GiveFrom(c, md, 3);
			if (Chance(0.15)) Give(c, "NBCGlovesGray");
			array<string> mz2 = {"ZmbM_DoctorFat", "ZmbF_NurseFat", "ZmbF_DoctorSkinny", "ZmbM_ParamedicNormal_Red", "ZmbM_ParamedicNormal_Blue", "ZmbF_ParamedicNormal_Red"};
			Zeds(e, mz2, 3 + e.Tier, 8, 30);
		}
		Fire(e, -4, -6);
		Boom(e);
	}

	// konwój medyczny na drodze – wzdłuż jezdni
	static void BuildConvoy(AGA_Event e)
	{
		e.LifeMin = 75;
		Put(e, "Land_Wreck_S1023_Medic_Beige", 0.4, 0, 2, 0.15);
		array<string> trucks = {"Land_Wreck_V3S", "StaticObj_Wreck_Ural"};
		PutAny(e, trucks, -0.3, 11, -3, 0.2);
		Put(e, "Land_Wreck_Offroad02_Medic_TK", 0.5, -9, 4, 0.15);
		array<string> escort = {"Land_Wreck_UAZ_Aban1_Police", "Land_Wreck_Uaz", "Land_Wreck_Volha_Police"};
		PutAny(e, escort, -0.6, -18, 9, 0.15);
		Smoke(e, -0.3, 11);
		array<string> basic = {"BandageDressing", "BandageDressing", "Rag", "PainkillerTablets", "CharcoalTablets", "TetracyclineAntibiotics", "VitaminBottle", "DisinfectantAlcohol", "Morphine", "Epinephrine", "SalineBagIV", "BloodTestKit", "SurgicalGloves_Blue", "SurgicalMask", "FirstAidKit", "Iodine"};
		array<string> nbc = {"NBCGlovesGray", "NBCBootsGray", "NBCHoodGray", "NBCPantsGray", "NBCJacketGray", "GasMask"};
		EntityAI c1 = LootCrate(e, 4.5, 2);
		GiveFrom(c1, basic, 3);
		EntityAI c2 = LootCrate(e, 4.5, -7);
		GiveFrom(c2, basic, 2);
		if (Chance(0.2)) Give(c2, nbc.GetRandomElement());
		if (Chance(0.1)) Give(c2, "Grenade_ChemGas");
		array<string> doc = {"ZmbM_DoctorFat", "ZmbF_NurseFat", "ZmbF_DoctorSkinny", "ZmbM_ParamedicNormal_Red", "ZmbM_ParamedicNormal_Blue", "ZmbF_ParamedicNormal_Red", "ZmbF_ParamedicNormal_Blue"};
		Zeds(e, doc, 4 + e.Tier, 6, 28);
		Announce("MEDICAL CONVOY", "A medical convoy was ambushed on the road. Medical supplies reported.");
	}

	// obóz myśliwych na leśnej polanie
	static void BuildHunter(AGA_Event e)
	{
		Fire(e, 0, 0);
		Put(e, "StaticObj_Mil_CamoNet_Side_east", 4.5, 2, -90, 0);
		Put(e, "StaticObj_Misc_WoodPile_Forest1", -1, -4.5, 10, 0.05);
		Put(e, "Land_Misc_FireBarrel_Green", 2.5, -3, 0, 0.05);
		Object bx = Put(e, "StaticObj_Misc_BoxWooden", -4, 2, 30, 0.02);
		Put(e, "StaticObj_Misc_Pallet", 3.5, 4.5, 15, 0.02);
		array<string> guns = {"Mosin9130", "CZ527", "Izh18", "Winchester70", "Izh43Shotgun"};
		array<string> ammo = {"Ammo_762x54", "Ammo_308Win", "Ammo_22", "Ammo_12gaPellets", "Ammo_12gaSlug", "Mag_CZ527_5rnd"};
		array<string> wear = {"HuntingJacket_Autumn", "HuntingJacket_Brown", "HunterPants_Autumn", "HunterPants_Brown", "BoonieHat_DPM", "HuntingBag", "WoolGloves_Green"};
		array<string> misc = {"HuntingKnife", "Compass", "Rope", "Canteen", "Matchbox", "DeerSteakMeat", "Binoculars", "HuntingOptic"};
		EntityAI c = LootCrate(e, 3.2, 2.5);
		if (Chance(0.5)) Give(c, guns.GetRandomElement());
		GiveFrom(c, ammo, 1);
		GiveFrom(c, wear, 1);
		GiveFrom(c, misc, 1);
		OnTop(bx, misc.GetRandomElement());
		array<string> hz = {"ZmbM_HunterOld_Autumn", "ZmbM_HunterOld_Spring", "ZmbM_HunterOld_Summer", "ZmbM_HikerSkinny_Green", "ZmbM_HikerSkinny_Grey", "ZmbM_ForestManOld"};
		Zeds(e, hz, 2 + e.Tier / 2, 10, 28);
	}

	// obóz cywilów: przyczepy, porzucone auto, ognisko
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
		Object bx = Put(e, "StaticObj_Misc_BoxWooden", -1.5, -6, 10, 0.02);
		array<string> food = {"TunaCan", "BakedBeansCan", "SpaghettiCan", "PeachesCan", "SardinesCan", "WaterBottle", "SodaCan_Cola", "Canteen"};
		array<string> stuff = {"Hoodie_Blue", "Jeans_Blue", "AthleticShoes_Black", "WoolCoat_Black", "Sweater_Gray", "Matchbox", "Flashlight", "Battery9V", "Rag", "KitchenKnife", "CanOpener", "Hatchet", "SewingKit", "TaloonBag_Blue"};
		array<string> pist = {"CZ75", "MKII", "Izh43Shotgun"};
		EntityAI c = LootCrate(e, -4, 6);
		GiveFrom(c, food, 1);
		GiveFrom(c, stuff, 2);
		if (e.Tier >= 2 && Chance(0.25)) Give(c, pist.GetRandomElement());
		OnTop(bx, food.GetRandomElement());
		array<string> civ = {"ZmbM_CitizenASkinny_Blue", "ZmbF_CitizenANormal_Blue", "ZmbM_HikerSkinny_Blue", "ZmbF_HikerSkinny_Red", "ZmbM_FarmerFat_Brown", "ZmbF_JoggerSkinny_Red"};
		Zeds(e, civ, 2 + e.Tier / 2, 10, 30);
	}

	// plac budowy przy drodze, na skraju miejscowości
	static void BuildSite(AGA_Event e)
	{
		e.LifeMin = 75;
		Put(e, "StaticObj_Wreck_Bulldozer", -8, 6, 20, 0.15);
		array<string> conts = {"Land_Container_1Bo", "Land_Container_1Mo", "Land_Container_1Aoh", "Land_Container_1Moh"};
		PutAny(e, conts, 8, 8, 90, 0.02);
		PutAny(e, conts, 9, -2, 90, 0.02);
		Put(e, "StaticObj_Misc_ConcretePipe", -3, -6, 75, 0.05);
		Put(e, "StaticObj_Misc_ConcretePipe", -3, -9, 80, 0.05);
		Object pal = Put(e, "StaticObj_Misc_Pallets1", 2, 9, 0, 0.02);
		Put(e, "StaticObj_Misc_Pallets2", -1, 11, 10, 0.02);
		Put(e, "StaticObj_Misc_WoodPile1", 3, -8, 5, 0.03);
		Put(e, "Land_Misc_Toilet_Mobile", -10, -6, 160, 0.02);
		array<string> mats = {"NailBox", "WoodenPlank", "MetalPlate", "MetalWire", "Rope", "BarbedWire", "CombinationLock"};
		array<string> tools = {"Hammer", "Shovel", "Pliers", "Hacksaw", "HandSaw", "Hatchet", "Crowbar", "Pickaxe"};
		array<string> wear = {"ConstructionHelmet_Orange", "ConstructionHelmet_Yellow", "ReflexVest", "WorkingBoots_Yellow", "WorkingGloves_Yellow", "Overalls_Blue", "Overalls_Gray"};
		EntityAI c1 = LootCrate(e, 4, 2);
		GiveFrom(c1, mats, 2);
		GiveFrom(c1, tools, 1);
		EntityAI c2 = LootCrate(e, -2, 3);
		GiveFrom(c2, wear, 1);
		GiveFrom(c2, tools, 1);
		// rzadki zestaw wybuchowy – max jeden na event
		if (e.Tier >= 2 && Chance(0.12))
		{
			Give(c2, "PlasticExplosive");
			if (Chance(0.5)) Give(c2, "RemoteDetonator");
		}
		OnTop(pal, mats.GetRandomElement());
		array<string> workers = {"ZmbM_ConstrWorkerNormal_Beige", "ZmbM_ConstrWorkerNormal_Black", "ZmbM_ConstrWorkerNormal_Green", "ZmbM_ConstrWorkerNormal_Grey", "ZmbM_ConstrWorkerNormal_Beige", "ZmbM_CitizenBFat_Blue", "ZmbM_HandymanNormal_Beige"};
		Zeds(e, workers, 5 + e.Tier, 8, 26);
	}

	static string Label(AGA_Event e, int n)
	{
		string name = "Event";
		if (e.Kind == "PLANE") name = "Katastrofa samolotu";
		else if (e.Kind == "HELI")
		{
			name = "Helicrash";
			if (e.Variant == "_MIL") name = "Helicrash wojskowy";
			else if (e.Variant == "_CIV") name = "Helicrash cywilny";
			else if (e.Variant == "_MED") name = "Helicrash medyczny";
		}
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
