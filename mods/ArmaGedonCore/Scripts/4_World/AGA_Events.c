// ARMA-GEDON: dynamiczne eventy – samolot i heli (MAYDAY + katastrofa slyszalna z 4 km), konwoje, posterunek,
// obozy mysliwych i cywilow, plac budowy (Plate Carrier + czyste bronie + losowe paczki lub luzna amunicja).

class AGA_SmokeColumn extends Wreck_UH1Y
{
	static bool s_AGASnd = AGA_RegSnd();
	static bool AGA_RegSnd()
	{
		CrashSoundSets.RegisterSoundSet("AGA_PlaneCrash_Far_SoundSet");
		CrashSoundSets.RegisterSoundSet("AGA_HeliCrash_Far_SoundSet");
		return true;
	}
}

class AGA_LootCrate extends WoodenCrate
{
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool CanBeSplit() { return false; }
	override bool IsHeavyBehaviour() { return true; }
	override bool IsTakeable() { return false; }
	override bool IsDeployable() { return false; }
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (GetGame().IsServer()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().ObjectDelete, 2000, false, this);
	}
}
class AGA_LootCrateMil extends AGA_LootCrate {}
class AGA_LootCrateMed extends AGA_LootCrate {}

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
	int LootedAt = -1;
	ref array<ZombieBase> Zs = new array<ZombieBase>;
	ref array<EntityAI> Crates = new array<EntityAI>;
	ref array<Object> Objs = new array<Object>;
	ref array<EntityAI> Items = new array<EntityAI>;
}

class AGA_Events
{
	static AGA_Event s_Cur;
	static ref array<ref AGA_Event> s_Active = new array<ref AGA_Event>;
	static int s_Minute;
	static int s_LastRestartWarn = -1;
	static bool s_Restarting;

	static const int RESTART_H1 = 5;
	static const int RESTART_H2 = 17;

	static void Tick()
	{
		s_Minute++;
		RestartTick();
		if (s_Minute == 1) { LoadRoads(); LoadPhase(); }
		if (s_Minute == 2 && FileExist("$profile:ArmaGedonCore/evtest.txt")) DebugAll();
		if (s_Minute < 3 || s_Restarting) return;

		for (int i = s_Active.Count() - 1; i >= 0; i--)
		{
			AGA_Event e = s_Active[i];
			Leash(e);
			if (e.LootedAt < 0 && IsLooted(e)) e.LootedAt = s_Minute;
			bool done = (e.LootedAt >= 0 && s_Minute - e.LootedAt >= 5);
			if ((done || s_Minute - e.Born >= e.LifeMin) && !PlayerNear(e.Pos, 150))
			{
				Despawn(e);
				s_Active.Remove(i);
			}
		}
		Ensure("HUNTER", 2);
		Ensure("CIVIL", 2);
		Ensure("BUILD", 2);
		Ensure("HELI", 2);
		Ensure("CONVOY", 1);
		Ensure("MILCONVOY", 1);
		Ensure("POLICE", 1);
		Ensure("PLANE", 1);
	}

	static ref array<string> s_Incoming = new array<string>;

	static void Mayday(string kind, vector pos, float yaw)
	{
		float hdg = Math.RandomFloat(0, 360);
		vector dir = Vector(hdg, 0, 0).AnglesToVector();
		float dist = 2600;
		int ms = 32000;
		int type = 1;
		if (kind == "HELI") { dist = 2000; ms = 42000; type = 2; }
		vector from = pos + dir * dist;
		from[1] = GetGame().SurfaceY(from[0], from[2]) + 180;
		vector to = pos;
		to[1] = to[1] + 25;
		GetRPCManager().SendRPC("AGA", "AGA_OnFlyBy", new Param4<vector, vector, int, int>(from, to, ms, type), true, null);
		s_Incoming.Insert(kind);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Arrive, ms, false, kind, pos, yaw);
	}

	static void Arrive(string kind, vector pos, float yaw)
	{
		int i = s_Incoming.Find(kind);
		if (i >= 0) s_Incoming.Remove(i);
		if (s_Restarting) return;
		SpawnAt(kind, pos, yaw);
	}

	static bool IsLooted(AGA_Event e)
	{
		// eventy wojskowe nie mają już skrzyni - wtedy liczy się tylko loot leżący na miejscu
		if (e.Crates.Count() == 0 && e.Items.Count() == 0) return false;
		int left = 0;
		foreach (EntityAI li : e.Items) if (li && !li.GetHierarchyParent()) left++;
		if (left > e.Items.Count() * 0.3) return false;
		foreach (EntityAI c : e.Crates)
		{
			if (!c || !c.GetInventory()) continue;
			if (c.GetInventory().AttachmentCount() > 0) return false;
			if (c.GetInventory().GetCargo() && c.GetInventory().GetCargo().GetItemCount() > 0) return false;
		}
		return true;
	}

	static void Leash(AGA_Event e)
	{
		foreach (ZombieBase z : e.Zs)
		{
			if (!z || !z.IsAlive()) continue;
			if (vector.Distance(z.GetPosition(), e.Pos) < 28) continue;
			if (PlayerNear(z.GetPosition(), 60)) continue;
			float a = Math.RandomFloat(0, 360);
			float d = Math.RandomFloat(3, 14);
			z.SetPosition(Off(e, Math.Sin(a * Math.DEG2RAD) * d, Math.Cos(a * Math.DEG2RAD) * d));
		}
	}

	static int s_Phase = 2;

	static void LoadPhase()
	{
		s_Phase = 2;
	}

	static void Ensure(string kind, int want)
	{
		int chance = 45;
		int have = 0;
		foreach (AGA_Event e : s_Active) if (e.Kind == kind) have++;
		foreach (string pk : s_Incoming) if (pk == kind) have++;
		if (have >= want) return;
		if (Math.RandomInt(0, 100) > chance) return;
		Spawn(kind);
	}

	static ref array<vector> s_Roads = new array<vector>;

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

	static vector FindSpot(string kind, out float yaw)
	{
		yaw = Math.RandomFloat(0, 360);
		int trees;
		float nt, nb;
		for (int t = 0; t < 400; t++)
		{
			if (kind == "CONVOY" || kind == "BUILD" || kind == "MILCONVOY" || kind == "POLICE")
			{
				if (s_Roads.Count() == 0) return vector.Zero;
				vector rp = s_Roads.GetRandomElement();
				vector p = Vector(rp[0], 0, rp[2]);
				yaw = rp[1];
				if (kind == "BUILD")
				{
					float side = 1;
					if (Math.RandomInt(0, 2) == 0) side = -1;
					vector rr = Vector(yaw + 90, 0, 0).AnglesToVector();
					p = p + rr * (side * Math.RandomFloat(20, 30));
				}
				p[1] = GetGame().SurfaceY(p[0], p[2]);
				if (!FarFromOthers(p)) continue;
				if (kind == "CONVOY" || kind == "MILCONVOY" || kind == "POLICE")
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
				if (!s.Contains("conifer") && !s.Contains("broadleaf") && !s.Contains("forest")) continue;
				if (NearRoad(q, 90)) continue;
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

	static bool NearRoad(vector p, float r)
	{
		foreach (vector v : s_Roads)
		{
			float dx = v[0] - p[0];
			float dz = v[2] - p[2];
			if (dx * dx + dz * dz < r * r) return true;
		}
		for (int a = 0; a < 8; a++)
		{
			float ang = a * 45 * Math.DEG2RAD;
			float x = p[0] + Math.Sin(ang) * 20;
			float z = p[2] + Math.Cos(ang) * 20;
			string t = SurfAt(x, z);
			if (t.Contains("concrete") || t.Contains("gravel") || t.Contains("asphalt")) return true;
		}
		return false;
	}

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
		if (!GetGame().ConfigIsExisting("CfgVehicles " + cls)) { AGA_Server.Get().Log("[EVENT] brak klasy " + cls); return null; }
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

	static void CrashSound(AGA_Event e, string soundSet)
	{
		Param3<bool, vector, int> ps = new Param3<bool, vector, int>(true, Off(e, 0, 0), soundSet.Hash());
		GetGame().RPCSingleParam(null, ERPCs.RPC_SOUND_HELICRASH, ps, true);
	}

	static void Boom(AGA_Event e)
	{
		vector p = Off(e, 0, 0);
		p[1] = p[1] + 1;
		ExplosivesBase ex = ExplosivesBase.Cast(GetGame().CreateObjectEx("M67Grenade", p, ECE_NONE));
		if (ex) ex.Explode(DamageType.EXPLOSION);
	}

	static bool Chance(float p)
	{
		return Math.RandomFloat01() < p;
	}

	static bool Valid(string c)
	{
		return GetGame().ConfigIsExisting("CfgVehicles " + c) || GetGame().ConfigIsExisting("CfgWeapons " + c) || GetGame().ConfigIsExisting("CfgMagazines " + c);
	}

	// Losowa ilość amunicji dla luźnych nabojów i magazynków na eventach
	static void RandomizeAmmo(EntityAI it)
	{
		if (!it) return;
		ItemBase ib = ItemBase.Cast(it);
		if (!ib) return;
		string cls = it.GetType();
		if (cls.Contains("AmmoBox") || cls.Contains("Ammobox")) return;
		if (cls.Contains("Ammo_"))
		{
			int mx = ib.GetQuantityMax();
			if (mx > 1)
			{
				int qLo = 4;
				if (mx < qLo) qLo = mx;
				int qHi = 24;
				if (mx < qHi) qHi = mx;
				if (qHi < qLo) qHi = qLo;
				ib.SetQuantity(Math.RandomIntInclusive(qLo, qHi));
			}
		}
		else if (cls.IndexOf("Mag_") == 0 || cls.IndexOf("AGMag_") == 0)
		{
			Magazine mag = Magazine.Cast(it);
			if (mag && mag.GetAmmoMax() > 1)
			{
				// 1 na 5 magazynków pusty, reszta 10-80% - pełne magazynki były zbyt łaskawe
				int am = mag.GetAmmoMax();
				int mLo = am / 10;
				if (mLo < 1) mLo = 1;
				int mHi = am * 8 / 10;
				if (mHi < mLo) mHi = mLo;
				int mq = 0;
				if (Math.RandomInt(0, 5) > 0) mq = Math.RandomIntInclusive(mLo, mHi);
				mag.ServerSetAmmoCount(mq);
			}
		}
	}

	static string CrateClass(AGA_Event e)
	{
		if (e.Kind == "PLANE") return "AGA_EvWeapons";
		if (e.Kind == "CONVOY" || e.Kind == "MILCONVOY") return "AGA_EvArmy";
		if (e.Kind == "HELI" && e.Variant == "_MIL") return "AGA_EvWeapons";
		if (e.Kind == "HELI" && e.Variant == "_MED") return "AGA_EvMedic";
		return "AGA_LootCrate";
	}

	static EntityAI LootCrate(AGA_Event e, float right, float fwd)
	{
		EntityAI c = EntityAI.Cast(Put(e, CrateClass(e), right, fwd, Math.RandomFloat(-20, 20), 0));
		if (c)
		{
			e.Crates.Insert(c);
			ItemBase ib = ItemBase.Cast(c);
			if (ib && ib.ConfigGetBool("itemIsOpenable"))
			{
				ib.Open();
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ib.Close, 4000, false);
			}
		}
		return c;
	}

	static void Give(EntityAI c, string cls)
	{
		if (!c || cls == "" || !Valid(cls)) return;
		EntityAI it = c.GetInventory().CreateInInventory(cls);
		if (it)
		{
			it.SetLifetime(5400);
			RandomizeAmmo(it);
		}
	}

	static void GiveFrom(EntityAI c, array<string> pool, int n)
	{
		for (int i = 0; i < n; i++) Give(c, pool.GetRandomElement());
	}

	static void GiveGun(EntityAI c, array<string> pool)
	{
		string pick = pool.GetRandomElement();
		array<string> parts = new array<string>;
		pick.Split("|", parts);
		if (c)
		{
			vector ori = c.GetOrientation();
			vector side = Vector(ori[0] + 90, 0, 0).AnglesToVector();
			vector p = c.GetPosition() + side * Math.RandomFloat(1.1, 1.6);
			EntityAI gun = DropAt(p, parts[0]);
			if (gun && c.IsKindOf("AGAStorage_openable_noLock_base"))
			{
				if (!c.ServerTakeEntityToTargetAttachment(c, gun))
					c.ServerTakeEntityToCargo(gun);
			}
		}
		if (parts.Count() > 1) { Give(c, parts[1]); }
	}

	static EntityAI DropAt(vector p, string cls)
	{
		if (!Valid(cls)) return null;
		p[1] = GetGame().SurfaceY(p[0], p[2]);
		EntityAI it = EntityAI.Cast(GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE));
		if (!it) return null;
		it.SetOrientation(Vector(Math.RandomFloat(0, 360), 0, 0));
		it.PlaceOnSurface();
		vector mm[2];
		it.ClippingInfo(mm);
		vector lo = mm[0];
		vector q = it.GetPosition();
		q[1] = GetGame().SurfaceY(q[0], q[2]) - lo[1] + 0.01;
		it.SetPosition(q);
		it.SetLifetime(5400);
		RandomizeAmmo(it);
		if (s_Cur) s_Cur.Items.Insert(it);
		return it;
	}

	static void DropFrom(AGA_Event e, array<string> pool, int n, float r0, float r1, float f0, float f1)
	{
		for (int i = 0; i < n; i++) Drop(e, pool.GetRandomElement(), Math.RandomFloat(r0, r1), Math.RandomFloat(f0, f1));
	}

	static EntityAI LootOn(Object b, string cls)
	{
		if (!Valid(cls)) return null;
		if (!b) return null;
		vector mm[2];
		b.ClippingInfo(mm);
		vector lo = mm[0];
		vector hi = mm[1];
		for (int k = 0; k < 5; k++)
		{
			float x = Math.RandomFloat(lo[0] * 0.55, hi[0] * 0.55);
			float z = Math.RandomFloat(lo[2] * 0.55, hi[2] * 0.55);
			vector top = b.ModelToWorld(Vector(x, hi[1] + 0.6, z));
			vector bot = b.ModelToWorld(Vector(x, lo[1] - 0.2, z));
			vector hit;
			vector dir;
			int comp;
			set<Object> res = new set<Object>;
			if (!DayZPhysics.RaycastRV(top, bot, hit, dir, comp, res, null, null, false, false, ObjIntersectGeom, 0.0)) continue;
			if (res.Find(b) < 0) continue;
			if (hit[1] < GetGame().SurfaceY(hit[0], hit[2]) + 0.05) continue;
			s_OnHit++;
			return PlaceAt(hit, cls);
		}
		s_OnMiss++;
		vector p = b.GetPosition();
		p[0] = p[0] + Math.RandomFloat(-1.2, 1.2);
		p[2] = p[2] + Math.RandomFloat(-1.2, 1.2);
		return DropAt(p, cls);
	}

	static EntityAI PlaceAt(vector hit, string cls)
	{
		// Promień mógł trafić w dym, krzak albo liść - wtedy przedmiot zostawał w powietrzu.
		// Jeśli punkt jest wyżej niż 2.5 m nad terenem, kładziemy rzecz na ziemi.
		// Skrzynie z amunicją i stoły mają do ~1.5 m. Wyżej to dach wraku, dym albo gałąź -
		// tam przedmiot wisiał w powietrzu, więc kładziemy go na ziemi obok.
		float ground = GetGame().SurfaceY(hit[0], hit[2]);
		if (hit[1] - ground > 1.6) return DropAt(hit, cls);
		EntityAI it = EntityAI.Cast(GetGame().CreateObjectEx(cls, hit, ECE_KEEPHEIGHT));
		if (!it) return null;
		it.SetOrientation(Vector(Math.RandomFloat(0, 360), 0, 0));
		vector mm[2];
		it.ClippingInfo(mm);
		vector lo = mm[0];
		vector q = hit;
		q[1] = hit[1] - lo[1] + 0.01;
		it.SetPosition(q);
		it.SetLifetime(5400);
		RandomizeAmmo(it);
		if (s_Cur) s_Cur.Items.Insert(it);
		return it;
	}

	static void LootOnFrom(Object b, array<string> pool, int n)
	{
		for (int i = 0; i < n; i++) LootOn(b, pool.GetRandomElement());
	}

	static void GunOn(Object b, array<string> pool)
	{
		string pick = pool.GetRandomElement();
		array<string> parts = new array<string>;
		pick.Split("|", parts);
		LootOn(b, parts[0]);
		if (parts.Count() > 1 && Chance(0.5)) { LootOn(b, parts[1]); }
	}

	// Amunicja przy broni z eventu: 35% nic, inaczej magazynek albo naboje.
	// Ilość losuje RandomizeAmmo (magazynek 0-80%, naboje 4-24 szt.), więc nigdy nie jest od razu pełny stack.
	static void AmmoFor(Object b, string mag)
	{
		if (mag == "" || Math.RandomInt(0, 100) < 35) return;
		LootOn(b, mag);
	}

	// Broń wojskowa z eventu leży NA ZIEMI / obiektach (bez skrzyni z gwarantowanym rzadkim).
	// Na JEDEN event wypada najwyżej JEDNA rzadka broń - wczesniej kazde losowanie liczylo
	// szanse osobno, wiec przy kolumnie wojskowej lezaly FAL, SVD i SVT-40 naraz.
	static bool s_RareDone;

	static void MilGun(AGA_Event e, Object b, array<string> guns, array<string> rare, float rareBase)
	{
		array<string> pool = guns;
		int tOver = e.Tier - 2;
		if (tOver < 0) tOver = 0;
		float rc = rareBase + 0.03 * tOver;
		if (!s_RareDone && rare && rare.Count() > 0 && Chance(rc))
		{
			pool = rare;
			s_RareDone = true;
		}
		string pick = pool.GetRandomElement();
		array<string> parts = new array<string>;
		pick.Split("|", parts);
		LootOn(b, parts[0]);
		if (parts.Count() > 1) AmmoFor(b, parts[1]);
	}

	static void OnTop(Object baseObj, string cls)
	{
		LootOn(baseObj, cls);
	}

	static void Drop(AGA_Event e, string cls, float right, float fwd)
	{
		DropAt(Off(e, right, fwd), cls);
	}

	static ZombieBase Zed(AGA_Event e, string cls, float rMin, float rMax)
	{
		if (!GetGame().ConfigIsExisting("CfgVehicles " + cls)) return null;
		// Zombie co najmniej 9 m od środka - bliżej stoi wrak helikoptera/samolotu i zombie
		// pojawiały się w jego teksturach. Wcześniej Math.Min(rMin, 3) ściągało je do 3 m.
		if (rMin < 9) rMin = 9;
		if (rMax > 22) rMax = 22;
		if (rMax < rMin + 4) rMax = rMin + 4;
		vector p;
		for (int t = 0; t < 6; t++)
		{
			float a = Math.RandomFloat(0, 360);
			float d = Math.RandomFloat(rMin, rMax);
			p = Off(e, Math.Sin(a * Math.DEG2RAD) * d, Math.Cos(a * Math.DEG2RAD) * d);
			if (!AGA_InsideObj(e, p)) break;
		}
		ZombieBase z = ZombieBase.Cast(GetGame().CreateObjectEx(cls, p, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP));
		if (z) e.Zs.Insert(z);
		return z;
	}

	// czy punkt wypada w obrysie któregoś obiektu eventu (wrak, kontener, skrzynie)
	static bool AGA_InsideObj(AGA_Event e, vector p)
	{
		foreach (Object o : e.Objs)
		{
			if (!o) continue;
			vector mm[2];
			o.ClippingInfo(mm);
			vector lo = mm[0];
			vector hi = mm[1];
			vector lp = o.WorldToModel(p);
			if (lp[0] > lo[0] - 0.5 && lp[0] < hi[0] + 0.5 && lp[2] > lo[2] - 0.5 && lp[2] < hi[2] + 0.5) return true;
		}
		return false;
	}

	static void Zeds(AGA_Event e, array<string> pool, int n, float rMin, float rMax)
	{
		for (int i = 0; i < n; i++) Zed(e, pool.GetRandomElement(), rMin, rMax);
	}

	static void PlateZed(AGA_Event e, float rMin, float rMax)
	{
		ZombieBase z = Zed(e, "ZmbM_usSoldier_Heavy_Woodland", rMin, rMax);
		if (!z) z = Zed(e, "ZmbM_SoldierNormal", rMin, rMax);
		if (!z) return;
		EntityAI v = z.GetInventory().CreateInInventory("PlateCarrierVest");
		if (!v) z.GetInventory().CreateAttachment("PlateCarrierVest");
	}

	static int s_OnHit;
	static int s_OnMiss;

	static void DebugAll()
	{
		array<string> kinds = {"PLANE", "HELI", "HELI", "HELI", "CONVOY", "CONVOY", "HUNTER", "HUNTER", "CIVIL", "CIVIL", "BUILD", "BUILD", "MILCONVOY", "POLICE"};
		foreach (string k : kinds)
		{
			float yaw;
			vector pos = FindSpot(k, yaw);
			if (pos == vector.Zero) { AGA_Server.Get().Log("[EVTEST] brak miejsca " + k); continue; }
			SpawnAt(k, pos, yaw);
		}
	}

	static void Spawn(string kind)
	{
		float yaw;
		vector pos = FindSpot(kind, yaw);
		if (pos == vector.Zero) return;
		if (kind == "PLANE" || kind == "HELI") { Mayday(kind, pos, yaw); return; }
		SpawnAt(kind, pos, yaw);
	}

	static void SpawnAt(string kind, vector pos, float yaw)
	{
		AGA_Event e = new AGA_Event;
		e.Kind = kind;
		e.Pos = pos;
		e.Yaw = yaw;
		e.Tier = TierOf(pos);
		e.Born = s_Minute;
		e.LifeMin = 45;

		s_Cur = e;
		s_RareDone = false;   // limit "jedna rzadka bron na event" liczony od nowa
		if (kind == "PLANE") BuildPlane(e);
		else if (kind == "HELI") BuildHeli(e);
		else if (kind == "CONVOY") BuildConvoy(e);
		else if (kind == "HUNTER") BuildHunter(e);
		else if (kind == "CIVIL") BuildCivil(e);
		else if (kind == "BUILD") BuildSite(e);
		else if (kind == "MILCONVOY") BuildMilConvoy(e);
		else if (kind == "POLICE") BuildPolice(e);

		if (kind == "PLANE") e.Tp = Off(e, 0, 32);
		else if (kind == "HELI") e.Tp = Off(e, 0, 16);
		else if (kind == "CONVOY") e.Tp = Off(e, 7, 3);
		else if (kind == "HUNTER") e.Tp = Off(e, 0, 7);
		else if (kind == "CIVIL") e.Tp = Off(e, -9, -6);
		else if (kind == "MILCONVOY") e.Tp = Off(e, 8, -4);
		else if (kind == "POLICE") e.Tp = Off(e, 7, 0);
		else e.Tp = Off(e, 6, -13);
		e.Tp[1] = e.Tp[1] + 0.3;

		s_Cur = null;
		s_Active.Insert(e);
		AGA_Server.Get().Log("EVENT " + kind + e.Variant + " T" + e.Tier.ToString() + " @ " + e.Pos.ToString() + " obiektow " + e.Objs.Count().ToString() + " loot " + e.Items.Count().ToString() + " na obiektach " + s_OnHit.ToString() + "/" + (s_OnHit + s_OnMiss).ToString());
		s_OnHit = 0;
		s_OnMiss = 0;
	}

	// ================= kompozycje eventów =================

	// samolot transportowy: 1 pewna broń + szansa na drugą + Plate Carrier + losowe paczki lub luźna amunicja
	static void BuildPlane(AGA_Event e)
	{
		e.LifeMin = 90;
		if (e.Tier < 3) e.Tier = 3;
		bool split = Chance(0.5);
		if (split)
		{
			e.Variant = "_B";
			Put(e, "Land_Wreck_C130J_2", 0, 0, 0, 0.6);
			Put(e, "StaticObj_Wreck_C130J_Wing", -14, -18, 140, 0.3);
			Put(e, "StaticObj_Wreck_C130J_Engine", 12, 9, -60, 0.4);
		}
		else
		{
			e.Variant = "_A";
			Put(e, "Land_Wreck_C130J", 0, 0, 0, 0.6);
			Put(e, "StaticObj_Wreck_C130J_Engine", -9, -26, 35, 0.4);
			Put(e, "StaticObj_Wreck_C130J_Wing", 10, -34, -20, 0.3);
		}
		Put(e, "StaticObj_ShellCrater2_Large", 0, 16, 0, 0.3);
		Put(e, "StaticObj_ShellCrater2_Debris", 3, 12, 40, 0.1);
		Put(e, "StaticObj_Wreck_Decal_Big", 0, -22, 0, 0.02);
		Put(e, "StaticObj_Wreck_Decal_Big", 1, -38, 10, 0.02);
		Put(e, "StaticObj_ShellCrater1_Decal", -2, -52, 0, 0.02);
		Put(e, "StaticObj_Wreck_C130J_Engine", 5, -50, 110, 0.4);
		Smoke(e, 3, 2);
		Smoke(e, -9, -26);
		Fire(e, 8, -18);
		Fire(e, 4, -46);
		Fire(e, -6, 14);
		Object a1 = Put(e, "StaticObj_ammoboxes_stacked", -5, -28, 25, 0.02);
		Object a2 = Put(e, "StaticObj_ammoboxes_stacked", 6, -33, -35, 0.02);
		Object a3 = Put(e, "StaticObj_ammoboxes_big", -1, -42, 80, 0.02);
		Object a4 = Put(e, "StaticObj_ammoboxes_single", 9, -45, 15, 0.02);
		Object w1 = Put(e, "StaticObj_Misc_WoodenCrate_5x", -7, -20, 30, 0.02);
		Put(e, "StaticObj_Misc_Pallets1", 3, -55, 15, 0.02);
		Fire(e, -11, -38);
		array<string> guns = {"M16A2|Mag_STANAG_30Rnd", "M16A4|Mag_STANAG_30Rnd", "AKS74U|Mag_AK74_30Rnd", "AK74|Mag_AK74_30Rnd", "AKM|Mag_AKM_30Rnd", "AK101|Mag_AK101_30Rnd", "G36|Mag_G36_30Rnd", "G36C|Mag_G36_30Rnd", "FAMAS|Mag_FAMAS_25Rnd", "VBB_FAMAS|Mag_FAMAS_35Rnd", "SA58|Mag_FAL_20Rnd", "SA58P|Mag_FAL_20Rnd", "sz_AKS74UN|Mag_AK74_30Rnd", "sz_AKS74UN_Tactical|Mag_AK74_30Rnd", "RPK74|Mag_AK74_45Rnd", "AG_RPK_762x39|Mag_AKM_30Rnd", "UMP45|Mag_UMP_25Rnd", "MP5K|Mag_MP5_30Rnd", "M14|Mag_M14_20Rnd", "Vikhr|Mag_Vikhr_30Rnd", "Saiga|Mag_Saiga_5Rnd", "M1014|Ammo_12gaPellets", "AG_PP19|Mag_PP19_30Rnd", "PP19|Mag_PP19_64Rnd", "Glock18C|Mag_Glock_15Rnd", "FNX45|Mag_FNX45_15Rnd", "AG_ADAR|Mag_STANAG_30Rnd"};
		array<string> rare = {"M4A1|Mag_STANAG_30Rnd", "FAL|Mag_FAL_20Rnd", "SVD|Mag_SVD_10Rnd", "VSS|Mag_VSS_10Rnd", "ASVAL|Mag_VAL_20Rnd", "AEK971|Mag_AK74_30Rnd", "AN94|Mag_AK74_30Rnd", "AG_SR2m|Mag_SR2_30rnd", "M24|Ammo_308Win", "SVT40|Mag_SVT40_10Rnd", "SV_98|Mag_SV_98_10Rnd", "AG_FN_SPR_A3G|Mag_fn_spr_a3g_5rnd", "MK12Mod1|Mag_MK12Mod1_20Rnd"};
		array<string> gear = {"PlateCarrierVest", "PlateCarrierPouches", "BallisticHelmet_Green", "GorkaEJacket_Summer", "GorkaPants_Summer", "MilitaryBoots_Black", "AssaultBag_Green", "TacticalGloves_Green", "MilitaryBelt", "TTSKOJacket_Camo", "TTSKOPants", "UKAssVest_Olive", "Mich2001Helmet"};
		array<string> att = {"KobraOptic", "M68Optic", "M4_OEBttstck", "M4_RISHndgrd", "AK_PlasticBttstck", "AK_RailHndgrd", "AK_TacticalBttstck", "RPK_Bttstck", "RPK_Hndgrd", "RPK_Bipod", "M24_Bipod", "AG_Rail_Bipod", "OKP7_Optic", "UniversalLight"};
		array<string> extra = {"ACOGOptic", "PSO1Optic", "KobraOptic", "M68Optic", "M4_Suppressor", "AK_Suppressor", "AK_Suppressor545", "PBS4_suppressor", "Aimpoint_Optic", "holo_Optic", "Hamr_Optic", "Accupoint_Trijicon", "MRADOptic", "Specter_optic", "SV98Optic"};
		array<string> ammo = {"Mag_STANAG_30Rnd", "Mag_AKM_30Rnd", "Mag_AK74_30Rnd", "Mag_G36_30Rnd", "Ammo_556x45", "AmmoBox_556x45_20Rnd", "Ammo_762x39", "AmmoBox_762x39_20Rnd", "Ammo_545x39", "AmmoBox_545x39_20Rnd", "Ammo_308Win", "AmmoBox_308Win_20Rnd", "AG_Ammo_9x21", "AG_Ammobox_9x21_30rnd", "M67Grenade", "BandageDressing"};
		// samolot to najwieksze znalezisko na mapie: 3-4 bronie wojskowe
		MilGun(e, a1, guns, rare, 0.07);
		MilGun(e, a2, guns, rare, 0.05);
		if (Chance(0.70)) MilGun(e, a3, guns, rare, 0.04);
		if (Chance(0.45)) MilGun(e, w1, guns, rare, 0.03);
		LootOnFrom(a2, ammo, 2);
		LootOnFrom(a3, gear, 1);
		if (Chance(0.6)) LootOnFrom(a4, att, 1);
		LootOnFrom(w1, gear, 1);
		DropFrom(e, ammo, 1, -5, 5, -38, -26);
		if (Chance(0.25)) LootOn(a3, "PlateCarrierVest");
		if (Chance(0.30)) LootOnFrom(w1, extra, 1);
		if (Chance(0.03)) LootOn(a4, "NVGoggles");
		array<string> mil = {"AG_ZmbM_Spetsnaz", "ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_PatrolNormal_PautRev", "ZmbM_usSoldier_normal_Woodland"};
		Zeds(e, mil, 6 + e.Tier, 12, 45);
		if (Chance(0.5)) PlateZed(e, 15, 40);
		Boom(e);
		CrashSound(e, "AGA_PlaneCrash_Far_SoundSet");
	}

	// katastrofa helikoptera: wojskowa / cywilna / medyczna
	static void BuildHeli(AGA_Event e)
	{
		e.LifeMin = 75;
		int v = Math.RandomInt(0, 100);
		EntityAI c;
		Put(e, "StaticObj_ShellCrater2_Small", 0, 7, 0, 0.2);
		Put(e, "StaticObj_Wreck_Decal_Small1", -2, -6, 30, 0.02);
		if (v < 45)
		{
			e.Variant = "_MIL";
			array<string> mw = {"Wreck_UH1Y", "Wreck_Mi8_Crashed"};
			PutAny(e, mw, 0, 0, 0, 0.3);
			Object b1 = Put(e, "StaticObj_ammoboxes_single", -7, -5, 40, 0.02);
			Object b2 = Put(e, "StaticObj_ammoboxes_single", 8, 4, -25, 0.02);
			Object b3 = Put(e, "StaticObj_ammoboxes_big", -5, 7, 70, 0.02);
			array<string> mg2 = {"M16A2|Mag_STANAG_30Rnd", "M16A4|Mag_STANAG_30Rnd", "AKS74U|Mag_AK74_30Rnd", "AK74|Mag_AK74_30Rnd", "AKM|Mag_AKM_30Rnd", "AK101|Mag_AK101_30Rnd", "G36|Mag_G36_30Rnd", "G36C|Mag_G36_30Rnd", "FAMAS|Mag_FAMAS_25Rnd", "VBB_FAMAS|Mag_FAMAS_35Rnd", "SA58|Mag_FAL_20Rnd", "SA58P|Mag_FAL_20Rnd", "sz_AKS74UN|Mag_AK74_30Rnd", "sz_AKS74UN_Tactical|Mag_AK74_30Rnd", "RPK74|Mag_AK74_45Rnd", "AG_RPK_762x39|Mag_AKM_30Rnd", "UMP45|Mag_UMP_25Rnd", "MP5K|Mag_MP5_30Rnd", "M14|Mag_M14_20Rnd", "Vikhr|Mag_Vikhr_30Rnd", "Saiga|Mag_Saiga_5Rnd", "M1014|Ammo_12gaPellets", "AG_PP19|Mag_PP19_30Rnd", "PP19|Mag_PP19_64Rnd", "Glock18C|Mag_Glock_15Rnd", "FNX45|Mag_FNX45_15Rnd", "AG_ADAR|Mag_STANAG_30Rnd"};
			array<string> mgear = {"PlateCarrierVest", "PlateCarrierPouches", "Mag_STANAG_30Rnd", "Mag_AKM_30Rnd", "Mag_G36_30Rnd", "Ammo_556x45", "AmmoBox_556x45_20Rnd", "Ammo_762x39", "AmmoBox_762x39_20Rnd", "Ammo_545x39", "AmmoBox_545x39_20Rnd", "BallisticHelmet_Green", "MilitaryBoots_Black", "TacticalGloves_Green", "AssaultBag_Green", "BandageDressing", "M67Grenade"};
			array<string> mrare = {"M4A1|Mag_STANAG_30Rnd", "FAL|Mag_FAL_20Rnd", "SVD|Mag_SVD_10Rnd", "VSS|Mag_VSS_10Rnd", "ASVAL|Mag_VAL_20Rnd", "AEK971|Mag_AK74_30Rnd", "AN94|Mag_AK74_30Rnd", "AG_SR2m|Mag_SR2_30rnd", "M24|Ammo_308Win", "SVT40|Mag_SVT40_10Rnd", "SV_98|Mag_SV_98_10Rnd", "AG_FN_SPR_A3G|Mag_fn_spr_a3g_5rnd", "MK12Mod1|Mag_MK12Mod1_20Rnd"};
			array<string> matt = {"KobraOptic", "M68Optic", "AK_PlasticBttstck", "M4_OEBttstck", "AK_TacticalBttstck", "M24_Bipod", "UniversalLight", "holo_Optic", "Hamr_Optic", "Specter_optic"};
			MilGun(e, b1, mg2, mrare, 0.05);
			if (Chance(0.40)) MilGun(e, b3, mg2, mrare, 0.03);
			LootOnFrom(b2, mgear, 2);
			if (Chance(0.5)) LootOnFrom(b3, matt, 1);
			if (Chance(0.25)) LootOn(b2, "PlateCarrierVest");
			if (Chance(0.2)) LootOn(b1, "M67Grenade");
			if (Chance(0.02)) LootOn(b3, "NVGoggles");
			DropFrom(e, mgear, 2, -8, 8, -8, 8);
			array<string> mz = {"AG_ZmbM_Spetsnaz", "ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_usSoldier_normal_Woodland"};
			Zeds(e, mz, 3 + e.Tier, 8, 30);
			if (e.Tier >= 3 && Chance(0.25)) PlateZed(e, 10, 25);
		}
		else if (v < 75)
		{
			e.Variant = "_CIV";
			Put(e, "Land_Wreck_Mi8_Civilian", 0, 0, 0, 0.3);
			Object s1 = Put(e, "StaticObj_Furniture_bag_DZ", -6, -4, 30, 0.02);
			Object s2 = Put(e, "StaticObj_Furniture_box_c", 7, -3, -40, 0.02);
			Smoke(e, 2, 1);
			array<string> cl = {"Hoodie_Blue", "Jeans_Blue", "HikingBootsLow_Black", "MountainBag_Green", "TunaCan", "BakedBeansCan", "WaterBottle", "Flashlight", "Battery9V", "Binoculars", "CZ75", "Mag_CZ75_15Rnd", "Ammo_9x19", "AmmoBox_9x19_25rnd", "Compass", "Matchbox"};
			LootOnFrom(s1, cl, 1);
			LootOnFrom(s2, cl, 1);
			c = LootCrate(e, 7, 3);
			GiveFrom(c, cl, 2);
			if (Chance(0.12)) { Give(c, "Glock18C"); if (Chance(0.5)) Give(c, "Mag_Glock_15Rnd"); }
			DropFrom(e, cl, 2, -8, 8, -8, 8);
			array<string> cz = {"ZmbM_CitizenASkinny_Blue", "ZmbF_CitizenANormal_Blue", "ZmbM_HikerSkinny_Blue", "ZmbM_PilotNormal", "ZmbM_OffshoreWorker_Green"};
			Zeds(e, cz, 3 + e.Tier, 8, 30);
		}
		else
		{
			e.Variant = "_MED";
			Put(e, "Wreck_UH1Y", 0, 0, 0, 0.3);
			// UH1Y ma ok. 17 m dlugosci i 3 m szerokosci - nosze i worek odsuniete na bok,
			// wczesniej (6-7 m od srodka) wchodzily w kadlub
			Object st1 = Put(e, "StaticObj_Stretcher_Red", -7, -2, 20, 0.02);
			Object hb = Put(e, "StaticObj_Decon_HazmatBag_01_F", 7.5, 2, -10, 0.02);
			array<string> md = {"BandageDressing", "Morphine", "Epinephrine", "SalineBagIV", "BloodBagIV", "BloodTestKit", "FirstAidKit", "TetracyclineAntibiotics", "PainkillerTablets", "CharcoalTablets", "DisinfectantSpray"};
			array<string> nbc = {"NBCGlovesGray", "NBCBootsGray", "NBCHoodGray", "NBCPantsGray", "NBCJacketGray", "GasMask"};
			LootOnFrom(st1, md, 2);
			LootOnFrom(hb, md, 1);
			if (Chance(0.3)) LootOn(hb, nbc.GetRandomElement());
			c = LootCrate(e, 6, 3);
			GiveFrom(c, md, 2);
			if (Chance(0.25)) Give(c, nbc.GetRandomElement());
			if (Chance(0.50)) LootOn(st1, "AntiChemInjector");
			if (Chance(0.08)) LootOn(hb, "InjectionVial");
			DropFrom(e, md, 1, -7, 7, -7, 7);
			array<string> mz2 = {"ZmbM_DoctorFat", "ZmbF_NurseFat", "ZmbF_DoctorSkinny", "ZmbM_ParamedicNormal_Red", "ZmbM_ParamedicNormal_Blue", "ZmbF_ParamedicNormal_Red"};
			Zeds(e, mz2, 3 + e.Tier, 8, 30);
		}
		Fire(e, -4, -6);
		Boom(e);
		CrashSound(e, "AGA_HeliCrash_Far_SoundSet");
	}

	// konwój medyczny
	static void BuildConvoy(AGA_Event e)
	{
		e.LifeMin = 75;
		array<string> trucks = {"Land_Wreck_V3S", "StaticObj_Wreck_Ural"};
		array<string> escort = {"Land_Wreck_UAZ_Aban1_Police", "Land_Wreck_Uaz", "Land_Wreck_Lada_Green"};
		Object bed1; Object bed2; Object str1; Object hz1; Object hz2; Object cont;
		if (Chance(0.5))
		{
			e.Variant = "_A";
			Put(e, "Land_Wreck_S1023_Medic_Beige", 0.4, 0, 2, 0.15);
			PutAny(e, trucks, -0.3, 11, -3, 0.2);
			PutAny(e, escort, -0.6, -12, 9, 0.15);
			cont = Put(e, "Land_Container_1Mo", 9, 4, 90, 0.02);
			bed1 = Put(e, "StaticObj_Furniture_hospital_transport_bed", 5, -2, 80, 0.02);
			bed2 = Put(e, "StaticObj_Furniture_hospital_transport_bed", 5.5, 1.5, 95, 0.02);
			str1 = Put(e, "StaticObj_Stretcher_Green", 4.5, -6, 70, 0.02);
			hz1 = Put(e, "StaticObj_Decon_HazmatBag_01_F", 6.5, 8, 20, 0.02);
			hz2 = Put(e, "StaticObj_Decon_HazmatBag_01_F", 7.5, 9.5, 60, 0.02);
			Put(e, "StaticObj_Wreck_Decal_Small2", 0, 11, 0, 0.02);
			Smoke(e, -0.3, 11);
		}
		else
		{
			e.Variant = "_B";
			Put(e, "Land_Wreck_S1023_Medic_Beige", 0, 2, 70, 0.15);
			PutAny(e, trucks, 5, -9, 25, 0.2);
			Put(e, "Land_Wreck_Offroad02_Medic_TK", -0.5, 12, -10, 0.15);
			cont = Put(e, "Land_Container_1Mo", -10, -3, 0, 0.02);
			bed1 = Put(e, "StaticObj_Furniture_hospital_transport_bed", -6, 4, 10, 0.02);
			bed2 = Put(e, "StaticObj_Furniture_hospital_transport_bed", -7.5, 6, -5, 0.02);
			str1 = Put(e, "StaticObj_Stretcher_Red", -5, -6, 30, 0.02);
			hz1 = Put(e, "StaticObj_Decon_HazmatBag_01_F", -8, -9, 40, 0.02);
			hz2 = Put(e, "StaticObj_Decon_HazmatBag_01_roll_F", -4, 9, 0, 0.02);
			Put(e, "StaticObj_Wreck_Decal_Small1", 5, -9, 0, 0.02);
			Smoke(e, 5, -9);
		}
		Put(e, "StaticObj_Furniture_MedicalGarbage_Bandage", 3, -3, 0, 0.02);
		Put(e, "StaticObj_Sign_Roadblock_Stop", 1, -22, 0, 0.02);
		Put(e, "Land_Misc_FireBarrel_Red", 5.5, -12, 0, 0.05);
		array<string> basic = {"BandageDressing", "Rag", "PainkillerTablets", "CharcoalTablets", "TetracyclineAntibiotics", "VitaminBottle", "DisinfectantAlcohol", "Morphine", "Epinephrine", "SalineBagIV", "BloodTestKit", "FirstAidKit", "Iodine"};
		array<string> nbc = {"NBCGlovesGray", "NBCBootsGray", "NBCHoodGray", "NBCPantsGray", "NBCJacketGray", "GasMask", "GP5GasMask", "AirborneMask"};
		LootOnFrom(bed1, basic, 2);
		LootOnFrom(bed2, basic, 1);
		LootOnFrom(str1, basic, 1);
		LootOn(hz1, nbc.GetRandomElement());
		if (Chance(0.5)) LootOn(hz2, nbc.GetRandomElement());
		DropFrom(e, basic, 2, -4, 6, -8, 8);
		array<string> police = {"MP5K|Mag_MP5_30Rnd", "Mp133Shotgun|Ammo_12gaPellets", "FNX45|Mag_FNX45_15Rnd", "Glock19|Mag_Glock_15Rnd", "M1014|Ammo_12gaPellets", "Saiga9|Mag_PP19_30Rnd", "AG_ADAR|Mag_STANAG_30Rnd", "Kedr|Mag_Kedr_20Rnd", "AG_PP19|Mag_PP19_30Rnd"};
		// POX: antidotum czesto, vialka (probka wirusa) rzadko
		if (Chance(0.55)) LootOn(bed1, "AntiChemInjector");
		if (Chance(0.10)) LootOn(hz1, "InjectionVial");
		EntityAI c = LootCrate(e, 3, 5);
		GiveFrom(c, basic, 2);
		int tPol = e.Tier - 1;
		if (tPol < 0) tPol = 0;
		if (Chance(0.12 + 0.04 * tPol)) GiveGun(c, police);
		if (Chance(0.35)) Give(c, nbc.GetRandomElement());
		array<string> doc = {"ZmbM_DoctorFat", "ZmbF_NurseFat", "ZmbF_DoctorSkinny", "ZmbM_ParamedicNormal_Red", "ZmbM_ParamedicNormal_Blue", "ZmbF_ParamedicNormal_Red", "ZmbF_ParamedicNormal_Blue", "ZmbM_NBC_Grey", "ZmbM_NBC_Yellow"};
		Zeds(e, doc, 4 + e.Tier, 6, 28);
	}

	// rozbita kolumna wojskowa + Plate Carrier + losowa amunicja
	static void BuildMilConvoy(AGA_Event e)
	{
		e.LifeMin = 80;
		if (e.Tier < 3) e.Tier = 3;
		array<string> lead = {"StaticObj_Wreck_BRDM", "StaticObj_Wreck_BMP1"};
		PutAny(e, lead, 0.3, 14, -6, 0.2);
		array<string> mid = {"StaticObj_Wreck_MTVR_Cargo", "StaticObj_Wreck_Ural", "Land_Wreck_V3S"};
		PutAny(e, mid, -0.2, 0, 3, 0.2);
		array<string> tail = {"StaticObj_Wreck_BMP2", "StaticObj_Wreck_T72_Chassis", "StaticObj_Wreck_HMMWV"};
		PutAny(e, tail, 0.4, -15, 8, 0.25);
		Put(e, "StaticObj_ShellCrater1_Decal", 3, 6, 0, 0.02);
		Smoke(e, 0.3, 14);
		Smoke(e, 0.4, -15);
		Fire(e, -4, 6);
		Put(e, "StaticObj_Mil_HBarrier_4m", 7, 22, 90, 0);
		Put(e, "StaticObj_Mil_HBarrier_Round", -7, -24, 0, 0);
		Put(e, "StaticObj_Misc_BagFence_3m", 7, -6, 90, 0);
		Put(e, "StaticObj_Mil_CamoNet_Side_east", 8, 3, 90, 0);
		Object b1 = Put(e, "StaticObj_ammoboxes_stacked", 6, 4, 20, 0.02);
		Object b2 = Put(e, "StaticObj_ammoboxes_single", -6, -8, 70, 0.02);
		Object b3 = Put(e, "StaticObj_ammoboxes_big", 6.5, -1, 85, 0.02);
		Object b4 = Put(e, "StaticObj_Misc_Table_Camp", -6, 2, 10, 0.02);
		array<string> top = {"Mag_STANAG_30Rnd", "Mag_AK74_30Rnd", "Mag_G36_30Rnd", "Ammo_556x45", "AmmoBox_556x45_20Rnd", "Ammo_545x39", "AmmoBox_545x39_20Rnd", "Ammo_762x39", "AmmoBox_762x39_20Rnd", "M67Grenade", "BandageDressing"};
		array<string> guns = {"M16A2|Mag_STANAG_30Rnd", "M16A4|Mag_STANAG_30Rnd", "AKS74U|Mag_AK74_30Rnd", "AK74|Mag_AK74_30Rnd", "AKM|Mag_AKM_30Rnd", "AK101|Mag_AK101_30Rnd", "G36|Mag_G36_30Rnd", "G36C|Mag_G36_30Rnd", "FAMAS|Mag_FAMAS_25Rnd", "VBB_FAMAS|Mag_FAMAS_35Rnd", "SA58|Mag_FAL_20Rnd", "SA58P|Mag_FAL_20Rnd", "sz_AKS74UN|Mag_AK74_30Rnd", "sz_AKS74UN_Tactical|Mag_AK74_30Rnd", "RPK74|Mag_AK74_45Rnd", "AG_RPK_762x39|Mag_AKM_30Rnd", "UMP45|Mag_UMP_25Rnd", "MP5K|Mag_MP5_30Rnd", "M14|Mag_M14_20Rnd", "Vikhr|Mag_Vikhr_30Rnd", "Saiga|Mag_Saiga_5Rnd", "M1014|Ammo_12gaPellets", "AG_PP19|Mag_PP19_30Rnd", "PP19|Mag_PP19_64Rnd", "Glock18C|Mag_Glock_15Rnd", "FNX45|Mag_FNX45_15Rnd", "AG_ADAR|Mag_STANAG_30Rnd"};
		array<string> rare = {"M4A1|Mag_STANAG_30Rnd", "FAL|Mag_FAL_20Rnd", "SVD|Mag_SVD_10Rnd", "VSS|Mag_VSS_10Rnd", "ASVAL|Mag_VAL_20Rnd", "AEK971|Mag_AK74_30Rnd", "AN94|Mag_AK74_30Rnd", "AG_SR2m|Mag_SR2_30rnd", "M24|Ammo_308Win", "SVT40|Mag_SVT40_10Rnd", "SV_98|Mag_SV_98_10Rnd", "AG_FN_SPR_A3G|Mag_fn_spr_a3g_5rnd", "MK12Mod1|Mag_MK12Mod1_20Rnd"};
		array<string> gear = {"PlateCarrierVest", "PlateCarrierPouches", "BallisticHelmet_Green", "Mich2001Helmet", "UKAssVest_Olive", "GorkaEJacket_Summer", "GorkaPants_Summer", "MilitaryBoots_Black", "TacticalGloves_Green", "AssaultBag_Green"};
		array<string> extra = {"PSO1Optic", "KobraOptic", "PKAS_Optic", "OKP7_Optic", "RGD5Grenade", "AK_PlasticBttstck", "AK_RailHndgrd", "AK_TacticalBttstck", "RPK_Bipod", "RPK_Bttstck", "RPK_Hndgrd", "Mag_AK74_45Rnd", "UniversalLight"};
		// bez skrzyni: broń i sprzęt leżą na obiektach kolumny, rzadka broń tylko z małą szansą
		MilGun(e, b1, guns, rare, 0.05);
		LootOnFrom(b1, top, 1);
		if (Chance(0.45)) MilGun(e, b3, guns, rare, 0.03);
		LootOnFrom(b3, gear, 1);
		LootOnFrom(b2, top, 1);
		if (Chance(0.6)) LootOnFrom(b4, extra, 1);
		DropFrom(e, top, 1, -6, 6, -8, 8);
		if (Chance(0.20)) LootOn(b2, "PlateCarrierVest");
		array<string> mil = {"ZmbM_SoldierNormal", "ZmbM_PatrolNormal_Autumn", "ZmbM_PatrolNormal_Flat", "ZmbM_usSoldier_normal_Woodland", "AG_ZmbM_Spetsnaz"};
		Zeds(e, mil, 6 + e.Tier, 10, 40);
		if (Chance(0.45)) PlateZed(e, 12, 35);
		Boom(e);
	}

	// posterunek policji przy drodze (bez KS23)
	static void BuildPolice(AGA_Event e)
	{
		e.LifeMin = 60;
		Put(e, "Land_Wreck_Volha_Police", 0.3, 4, 15, 0.15);
		Put(e, "Land_Wreck_UAZ_Aban1_Police", -0.4, -7, -20, 0.15);
		Put(e, "StaticObj_Sign_Roadblock_Stop", 0, 14, 180, 0.02);
		Put(e, "StaticObj_Sign_Roadblock_Arrow", 0, -16, 0, 0.02);
		Put(e, "StaticObj_Misc_BagFence_3m", 6, 10, 90, 0);
		Put(e, "StaticObj_Misc_BagFence_Round", -6, -12, 0, 0);
		Put(e, "Land_Misc_FireBarrel_Red", 5, -3, 0, 0.05);
		Object t = Put(e, "StaticObj_Misc_Table_Camp", 5, 2, 90, 0.02);
		Object r = Put(e, "StaticObj_Furniture_radio_b", 5, 2.4, 90, 0.02);
		Object bx = Put(e, "StaticObj_Furniture_box_c", -5, 3, 40, 0.02);
		array<string> top = {"Mag_MP5_30Rnd", "Ammo_12gaPellets", "AmmoBox_00buck_10rnd", "Mag_Glock_15Rnd", "Ammo_9x19", "AmmoBox_9x19_25rnd", "Ammo_762x25", "AmmoBox_762x25_25rnd", "Handcuffs", "PoliceVest", "PersonalRadio", "TearGas_SmokeGrenade"};
		array<string> guns = {"MP5K|Mag_MP5_30Rnd", "M1014|Ammo_12gaPellets", "Mp133Shotgun|Ammo_12gaPellets", "Saiga9|Mag_PP19_30Rnd", "AG_PP19|Mag_PP19_30Rnd", "Kedr|Mag_Kedr_20Rnd", "Glock18C|Mag_Glock_15Rnd", "APS|Mag_APS_20Rnd", "PB|Mag_PB_8Rnd", "AG_ADAR|Mag_STANAG_30Rnd", "UMP45|Mag_UMP_25Rnd"};
		array<string> gear = {"PoliceVest", "PoliceJacket", "PolicePants", "PoliceCap", "Handcuffs", "HandcuffKeys", "PersonalRadio", "Flashlight", "BandageDressing", "PP19_RailHndgrd", "SurefireLight", "Aimpoint_Optic", "holo_Optic"};
		// posterunek: broń policyjna, w wyższych tierach szansa na lepszy sprzęt policyjny
		array<string> rarePol = {"M1014|Ammo_12gaPellets", "UMP45|Mag_UMP_25Rnd", "Glock18C|Mag_Glock_15Rnd"};
		MilGun(e, t, guns, rarePol, 0.10);
		LootOnFrom(t, top, 2);
		LootOnFrom(bx, gear, 1);
		if (r) LootOn(r, "Battery9V");
		DropFrom(e, top, 1, -5, 5, -6, 6);
		if (Chance(0.40)) MilGun(e, bx, guns, rarePol, 0.05);
		LootOnFrom(bx, gear, 1);
		array<string> cops = {"ZmbM_PolicemanFat", "ZmbF_PoliceWomanNormal", "ZmbM_PolicemanSpecForce", "ZmbM_PolicemanSpecForce_Heavy"};
		Zeds(e, cops, 4 + e.Tier, 6, 25);
	}

	// obóz myśliwych na leśnej polanie
	static void BuildHunter(AGA_Event e)
	{
		Fire(e, 0, 0);
		Object tb; Object mt; Object bx;
		if (Chance(0.5))
		{
			e.Variant = "_A";
			Put(e, "StaticObj_Misc_Timbers_Log1", -2.2, 0.4, 80, 0.12);
			Put(e, "StaticObj_Misc_Timbers_Log2", 2.3, -0.6, 100, 0.12);
			Put(e, "StaticObj_Misc_Chair_Camp1", 0.6, 2.4, 190, 0.02);
			Put(e, "StaticObj_Mil_CamoNet_Tent2", -6, 5, 15, 0);
			mt = Put(e, "StaticObj_Furniture_matress_white", -6.5, 5.5, 105, 0.02);
			tb = Put(e, "StaticObj_Misc_Table_Camp", 3.8, 3, -20, 0.02);
			Put(e, "StaticObj_Misc_Chair_Camp2", 3.6, 1.6, 10, 0.02);
			Put(e, "StaticObj_Misc_Gunrack", -3.5, 7.5, 20, 0.02);
			Put(e, "StaticObj_Misc_WoodPile_Forest1", -1, -4.5, 10, 0.05);
			Put(e, "StaticObj_Misc_FeedRack", 9, -6, 40, 0.05);
			bx = Put(e, "StaticObj_Furniture_box_c", 2, -3.5, 30, 0.02);
			Put(e, "StaticObj_Furniture_canister_DZ", 4.8, -2, 0, 0.02);
		}
		else
		{
			e.Variant = "_B";
			Put(e, "StaticObj_Mil_CamoNet_Side_east", 0, 6, 0, 0);
			mt = Put(e, "StaticObj_Furniture_foldingbed_open", -2, 6, 90, 0.02);
			tb = Put(e, "StaticObj_Misc_WoodTable_Outdoor", 2.5, 5.5, 0, 0.02);
			Put(e, "StaticObj_Misc_Timbers_Log3", 0, -2.4, 0, 0.12);
			Put(e, "StaticObj_Misc_Timbers_Log1", 2.5, -0.5, 60, 0.12);
			bx = Put(e, "StaticObj_Furniture_bag_DZ", -3.5, 3.5, 40, 0.02);
			Put(e, "StaticObj_Furniture_bucket", 4, 2.5, 0, 0.02);
			Put(e, "StaticObj_Furniture_canister_DZ", -4.5, 1.5, 0, 0.02);
			Put(e, "StaticObj_Misc_WoodPile_Forest1", 6, 3, 80, 0.05);
			Put(e, "StaticObj_Misc_Gunrack", 4.5, 7, -10, 0.02);
		}
		Put(e, "StaticObj_Forest_DeerSkeleton_Full", 6.5, -3.5, 120, 0.02);
		Put(e, "StaticObj_Forest_DeerSkeleton_Skull", -4.6, -4.4, 60, 0);
		array<string> ammo = {"Ammo_762x54", "AmmoBox_762x54_20Rnd", "Ammo_308Win", "AmmoBox_308Win_20Rnd", "Ammo_22", "AmmoBox_22_50Rnd", "Ammo_12gaPellets", "AmmoBox_00buck_10rnd", "Mag_CZ527_5rnd", "AG_Ammo_792x57", "AG_AmmoBox_792x57_20Rnd"};
		array<string> wear = {"HuntingJacket_Autumn", "HuntingJacket_Brown", "HunterPants_Autumn", "HunterPants_Brown", "BoonieHat_DPM", "HuntingBag", "WoolGloves_Green"};
		array<string> misc = {"HuntingKnife", "Compass", "Rope", "Canteen", "Matchbox", "DeerSteakMeat", "Binoculars", "HuntingOptic", "PUHuntingOptic", "BurlapWrap", "GrassBurlapWrap"};
		array<string> hg = {"Mosin9130|Ammo_762x54", "CZ527|Mag_CZ527_5rnd", "Izh18|Ammo_762x39", "Winchester70|Ammo_308Win", "Izh43Shotgun|Ammo_12gaPellets", "Ruger1022|Mag_Ruger1022_15Rnd", "kar98k|AG_Ammo_792x57", "VEPR_SOK94|Mag_VEPR_SOK94_10rnd", "MP153|Ammo_12gaPellets", "Izh27|Ammo_12gaPellets"};
		if (Chance(0.55)) GunOn(tb, hg); else LootOnFrom(tb, ammo, 1);
		LootOnFrom(tb, misc, 1);
		LootOnFrom(mt, wear, 1);
		LootOnFrom(bx, misc, 1);
		LootOnFrom(bx, ammo, 1);
		// obóz myśliwych: broń myśliwska na stole/pryczy, bez skrzyni; SVT-40 rzadko i tylko wyżej
		if (Chance(0.45)) LootOnFrom(mt, wear, 1);
		if (Chance(0.35)) LootOnFrom(bx, ammo, 1);
		array<string> svt = {"SVT40|Mag_SVT40_10Rnd"};
		int tHun = e.Tier - 2;
		if (tHun < 0) tHun = 0;
		if (Chance(0.06 + 0.02 * tHun)) MilGun(e, bx, hg, svt, 0.35);
		array<string> hz = {"ZmbM_HunterOld_Autumn", "ZmbM_HunterOld_Spring", "ZmbM_HunterOld_Summer", "ZmbM_HikerSkinny_Green", "ZmbM_HikerSkinny_Grey", "ZmbM_ForestManOld"};
		Zeds(e, hz, 2 + e.Tier / 2, 10, 28);
	}

	// obóz cywilów (tabor)
	static void BuildCivil(AGA_Event e)
	{
		array<string> caravans = {"Land_Wreck_Caravan_MGreen", "Land_Wreck_Caravan_MRust", "Land_Wreck_Caravan_WBrown", "Land_Wreck_Caravan_WGreen"};
		array<string> cars = {"Land_Wreck_sed01_aban1_black", "Land_Wreck_hb01_aban1_blue", "Land_Wreck_Lada_Green", "Land_Wreck_Volha_Blue", "Land_Wreck_Pickup01_Aban_Red"};
		Object t1; Object t2; Object m1; Object b1;
		if (Chance(0.5))
		{
			e.Variant = "_A";
			PutAny(e, caravans, -8, 3, 80, 0.1);
			PutAny(e, caravans, 7, 5, -75, 0.1);
			PutAny(e, cars, 1, 11, 160, 0.1);
			Put(e, "StaticObj_Slum_Roof4", -2, 5.5, 0, 0.02);
			t1 = Put(e, "StaticObj_Misc_WoodTable_Outdoor", -1.5, 5, 90, 0.02);
			Put(e, "StaticObj_Furniture_lobby_chair", -3, 4, 60, 0.02);
			Put(e, "StaticObj_Furniture_lobby_chair", 0, 3.6, -40, 0.02);
			m1 = Put(e, "StaticObj_Furniture_matress_white", 3.5, -4, 30, 0.02);
			b1 = Put(e, "StaticObj_Furniture_box_c", -4, -5, 10, 0.02);
			Put(e, "StaticObj_Garbage_Pile8", 10, -6, 0, 0.02);
			Put(e, "Land_wreck_truck01_aban1_orange", -13, -7, 120, 0.1);
		}
		else
		{
			e.Variant = "_B";
			PutAny(e, caravans, 0, 9, 0, 0.1);
			PutAny(e, caravans, -9, -2, 100, 0.1);
			PutAny(e, cars, 9, -3, 70, 0.1);
			Put(e, "Land_Wreck_Trailer_Closed", 8, 8, -30, 0.1);
			Put(e, "StaticObj_Slum_Roof2", 3, 3, 20, 0.02);
			t1 = Put(e, "StaticObj_Misc_Table_Market", 3, 3.5, 20, 0.02);
			Put(e, "StaticObj_Furniture_lobby_chair", 4.5, 1.8, 160, 0.02);
			m1 = Put(e, "StaticObj_Furniture_foldingbed_matress", -4, 4, 10, 0.02);
			b1 = Put(e, "StaticObj_Furniture_bag_DZ", -3, -5, 40, 0.02);
			Put(e, "StaticObj_Garbage_Pile5", -10, 7, 0, 0.02);
			Put(e, "StaticObj_Garbage_GroundSq_5m", 0, -6, 0, 0.02);
		}
		Fire(e, 0, 0);
		t2 = Put(e, "StaticObj_Furniture_radio_b", 0, 0, 0, 0.02);
		if (t2 && t1) { vector rp = t1.GetPosition(); t2.SetPosition(rp + "0 1.0 0"); }
		Put(e, "StaticObj_Misc_WoodPile1", -3, -2.5, 30, 0.05);
		Put(e, "Land_Misc_FireBarrel_Blue", 3.5, -2.5, 0, 0.05);
		Put(e, "StaticObj_Furniture_canister_DZ", 5, -1, 0, 0.02);
		Put(e, "StaticObj_Furniture_bucket", -5, 1, 0, 0.02);
		array<string> food = {"TunaCan", "BakedBeansCan", "SpaghettiCan", "PeachesCan", "SardinesCan", "WaterBottle", "SodaCan_Cola", "Canteen"};
		array<string> stuff = {"Hoodie_Blue", "Jeans_Blue", "AthleticShoes_Black", "WoolCoat_Black", "Sweater_Gray", "Matchbox", "Flashlight", "Battery9V", "Rag", "KitchenKnife", "CanOpener", "Hatchet", "SewingKit", "TaloonBag_Blue"};
		array<string> civAmmo = {"Ammo_9x19", "AmmoBox_9x19_25rnd", "Ammo_22", "Ammo_12gaPellets", "Ammo_762x25", "AmmoBox_762x25_25rnd"};
		LootOnFrom(t1, food, 2);
		LootOnFrom(m1, stuff, 1);
		LootOnFrom(b1, food, 1);
		LootOnFrom(b1, stuff, 1);
		DropFrom(e, stuff, 1, -6, 6, -5, 8);
		EntityAI c = LootCrate(e, -4, 6);
		GiveFrom(c, stuff, 2);
		GiveFrom(c, civAmmo, 1);
		array<string> civMats = {"NailBox", "MetalWire", "Rope", "WoodenPlank", "DuctTape", "Nail"};
		if (Chance(0.45)) LootOnFrom(b1, civMats, 1);
		if (Chance(0.30)) DropFrom(e, civMats, 1, -5, 5, -4, 6);
		array<string> pist = {"CZ75|Mag_CZ75_15Rnd", "MKII|Mag_MKII_10Rnd", "Izh43Shotgun|Ammo_12gaPellets", "TT_33|Mag_TT_33_8Rnd", "MakarovPM|Mag_MakarovPM_8Rnd", "M92|Mag_M92_15Rnd", "LugerP08|Mag_LugerP08_8Rnd", "PPSH41|Mag_PPSH41_35Rnd", "Vepr_KM|AGMag_AKM_10Rnd"};
		array<string> civHunt = {"Mosin9130|Ammo_762x54", "Izh18|Ammo_762x39", "Izh43Shotgun|Ammo_12gaPellets", "Repeater|Ammo_357", "Ruger1022|Mag_Ruger1022_15Rnd", "MP153|Ammo_12gaPellets"};
		if (Chance(0.35)) GiveGun(c, pist);
		else if (Chance(0.30)) GiveGun(c, civHunt);
		array<string> civ = {"ZmbM_CitizenASkinny_Blue", "ZmbF_CitizenANormal_Blue", "ZmbM_HikerSkinny_Blue", "ZmbF_HikerSkinny_Red", "ZmbM_FarmerFat_Brown", "ZmbF_JoggerSkinny_Red"};
		Zeds(e, civ, 2 + e.Tier / 2, 10, 30);
	}

	// plac budowy: przycięte gwoździe, blachy i maksymalnie 1 rzadki CodeLock w skrzyni (25%)
	static void BuildSite(AGA_Event e)
	{
		e.LifeMin = 75;
		array<string> conts = {"Land_Container_1Bo", "Land_Container_1Mo", "Land_Container_1Aoh", "Land_Container_1Moh"};
		array<string> trk = {"Land_Wreck_Truck02_Yellow", "Land_Wreck_Truck02_White", "Land_wreck_truck01_aban1_orange"};
		Object sh1; Object sh2; Object rk; Object wb; Object tt;
		if (Chance(0.5))
		{
			e.Variant = "_A";
			Put(e, "StaticObj_Wreck_Bulldozer", -8, 6, 20, 0.15);
			PutAny(e, conts, 8, 8, 90, 0.02);
			PutAny(e, conts, 9, -2, 90, 0.02);
			PutAny(e, trk, -14, 14, 110, 0.15);
			sh1 = Put(e, "StaticObj_Furniture_shelf_DZ", 5.5, 4, 90, 0.02);
			sh2 = Put(e, "StaticObj_Furniture_shelf_DZ", 5.5, 1.5, 90, 0.02);
			rk = Put(e, "StaticObj_Furniture_tools_racking_DZ", 5.5, -5, 90, 0.02);
			wb = Put(e, "StaticObj_Furniture_workbench_DZ", 2, -2, 0, 0.02);
			tt = Put(e, "StaticObj_ToolTrolley_02_F", 0, -3, 40, 0.02);
			Put(e, "StaticObj_Misc_ConcretePipe", -3, -6, 75, 0.05);
			Put(e, "StaticObj_Misc_ConcretePipe", -3, -9, 80, 0.05);
			Put(e, "StaticObj_Misc_ConcreteBlock2", -6, -13, 0, 0.02);
			Put(e, "StaticObj_Misc_ConcreteBlock2", -2.5, -13.5, 0, 0.02);
		}
		else
		{
			e.Variant = "_B";
			Put(e, "StaticObj_Wreck_Digger", 7, 9, -30, 0.15);
			PutAny(e, conts, -9, 3, 0, 0.02);
			PutAny(e, trk, 10, -8, 200, 0.15);
			sh1 = Put(e, "StaticObj_Furniture_shelf_DZ", -6, -2, 0, 0.02);
			sh2 = Put(e, "StaticObj_Furniture_shelfs_small", -3.5, -2.2, 0, 0.02);
			rk = Put(e, "StaticObj_Furniture_tools_racking_DZ", -6.5, 8, 0, 0.02);
			wb = Put(e, "StaticObj_Furniture_Workbench", 0, 3, 90, 0.02);
			tt = Put(e, "StaticObj_PalletTrolley_01_yellow_F", 2, -4, 30, 0.02);
			Put(e, "StaticObj_Misc_Pipes", 4, 2, 5, 0.03);
			Put(e, "StaticObj_Misc_ConcreteBlock2_Stripes", 0, -12, 90, 0.02);
			Put(e, "StaticObj_Misc_ConcretePanels", -2, 12, 0, 0.02);
			Put(e, "StaticObj_Decon_PowerCable_01_Roll_F", 1.5, 5, 0, 0.02);
		}
		Object pal = Put(e, "StaticObj_Misc_Pallets1", 2, 9, 0, 0.02);
		Put(e, "StaticObj_Misc_Pallets2", -1, 11, 10, 0.02);
		Put(e, "StaticObj_Misc_WoodPile1", 3, -8, 5, 0.03);
		Put(e, "Land_Misc_Toilet_Mobile", -10, -6, 160, 0.02);
		Put(e, "Land_Misc_FireBarrel_Yellow", 12, 3, 0, 0.05);
		// Materiały budowlane: 1-2 sztuki na cały plac, max 1 pudełko gwoździ (wcześniej ~4 + skrzynia).
		array<string> mats = {"WoodenPlank", "MetalPlate", "MetalWire", "Rope", "BarbedWire"};
		array<string> tools = {"Hammer", "Shovel", "Pliers", "Hacksaw", "HandSaw", "Hatchet", "Crowbar", "Pickaxe", "SharpeningStone"};
		array<string> wear = {"ConstructionHelmet_Orange", "ConstructionHelmet_Yellow", "ReflexVest", "WorkingBoots_Yellow", "WorkingGloves_Yellow", "Overalls_Blue", "Overalls_Gray"};
		if (Chance(0.45)) LootOn(sh1, "NailBox");
		LootOnFrom(sh2, mats, 1);
		if (Chance(0.35)) LootOnFrom(pal, mats, 1);
		LootOnFrom(rk, tools, 1);
		if (Chance(0.5)) LootOnFrom(wb, tools, 1);
		LootOnFrom(tt, wear, 1);
		DropFrom(e, wear, 1, -6, 8, -6, 12);
		// bez skrzyni z gwarantowanym lootem: CodeLock i ładunki rzadko, leżą na stole warsztatowym
		if (Chance(0.08)) LootOn(wb, "CodeLock");
		if (e.Tier >= 3 && Chance(0.06))
		{
			LootOn(wb, "ImprovisedExplosive");
			if (Chance(0.4)) LootOn(wb, "RemoteDetonator");
		}
		array<string> workers = {"ZmbM_ConstrWorkerNormal_Beige", "ZmbM_ConstrWorkerNormal_Black", "ZmbM_ConstrWorkerNormal_Green", "ZmbM_ConstrWorkerNormal_Grey", "ZmbM_CitizenBFat_Blue", "ZmbM_HandymanNormal_Beige"};
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
		else if (e.Kind == "MILCONVOY") name = "Kolumna wojskowa";
		else if (e.Kind == "POLICE") name = "Posterunek policji";
		return "[EVENT] " + name + " " + n.ToString() + " (T" + e.Tier.ToString() + ")";
	}

	static void Despawn(AGA_Event e)
	{
		foreach (Object o : e.Objs)
		{
			if (!o) continue;
			EntityAI ent = EntityAI.Cast(o);
			if (ent && ent.GetHierarchyRootPlayer()) continue;
			GetGame().ObjectDelete(o);
		}
		foreach (EntityAI it : e.Items)
		{
			if (!it || it.GetHierarchyParent()) continue;
			GetGame().ObjectDelete(it);
		}
		foreach (ZombieBase z : e.Zs)
			if (z && !PlayerNear(z.GetPosition(), 100)) GetGame().ObjectDelete(z);
		AGA_Server.Get().Log("EVENT koniec " + e.Kind + " @ " + e.Pos.ToString());
	}

	static string Dir8(vector from, vector to)
	{
		float a = Math.Atan2(to[0] - from[0], to[2] - from[2]) * Math.RAD2DEG;
		if (a < 0) a += 360;
		array<string> n = {"north", "north-east", "east", "south-east", "south", "south-west", "west", "north-west"};
		int r = Math.Round(a / 45);
		int i = r % 8;
		return n[i];
	}

	static void Announce(string title, string text)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			if (!m || !m.GetIdentity()) continue;
			string t = text;
			if (s_Cur)
			{
				float km = vector.Distance(m.GetPosition(), s_Cur.Pos) / 1000;
				if (km < 0.4) t = text + " It's right next to you.";
				else t = text + " Roughly " + (Math.Round(km * 2) / 2).ToString() + " km " + Dir8(m.GetPosition(), s_Cur.Pos) + " of you.";
			}
			NotificationSystem.SendNotificationToPlayerIdentityExtended(m.GetIdentity(), 12, title, t, "set:dayz_gui image:icon_info");
		}
	}

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
