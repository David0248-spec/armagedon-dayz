// ARMA-GEDON – flaga, terytorium i grupy
// Zestaw flagi: zaostrzony kij + szmata. Postawienie = terytorium (promień 30 m, bez nakładania).
// LVL1 (zestaw) – 5 dni, LVL2 (1 kłoda + lina) – 7 dni, LVL3 (6 kłód + 60 gwoździ + 16 kamieni) + podniesiona flaga – baza nie znika.
// Grupa do 8 osób; obcy nie budują na terytorium i nie rozbierają, gdy flaga jest podniesiona.

modded class PlayerBase
{
	protected int m_AGA_Gid;
	protected bool m_AGA_Staff;

	void PlayerBase()
	{
		RegisterNetSyncVariableInt("m_AGA_Gid", 0, 999999);
		RegisterNetSyncVariableBool("m_AGA_Staff");
	}

	int AGA_Gid() { return m_AGA_Gid; }
	bool AGA_IsStaff() { return m_AGA_Staff; }

	void AGA_SetGroup(int gid, bool staff)
	{
		if (m_AGA_Gid == gid && m_AGA_Staff == staff) return;
		m_AGA_Gid = gid;
		m_AGA_Staff = staff;
		SetSynchDirty();
	}
}

class AGA_Group
{
	int Id;
	string Owner;
	ref array<string> Ids;
	ref array<string> Names;
	vector Pos;
	void AGA_Group()
	{
		Ids = new array<string>;
		Names = new array<string>;
	}
}

class AGA_GroupDb
{
	int Next;
	ref array<ref AGA_Group> Groups;
	void AGA_GroupDb()
	{
		Next = 1;
		Groups = new array<ref AGA_Group>;
	}
}

// ==================================================================== flaga
modded class TerritoryFlag
{
	protected int m_AGA_Gid;
	protected bool m_AGA_Invite;
	protected int m_AGA_Members;

	void TerritoryFlag()
	{
		RegisterNetSyncVariableInt("m_AGA_Gid", 0, 999999);
		RegisterNetSyncVariableBool("m_AGA_Invite");
		RegisterNetSyncVariableInt("m_AGA_Members", 0, 16);
		AGA_Territory.Flags().Insert(this);
	}

	override void EEDelete(EntityAI parent)
	{
		AGA_Territory.Flags().RemoveItem(this);
		super.EEDelete(parent);
	}

	int AGA_Gid() { return m_AGA_Gid; }
	bool AGA_Invite() { return m_AGA_Invite; }
	bool AGA_Raised() { return m_RefresherActive; }

	int AGA_Level()
	{
		if (!GetConstruction()) return 1;
		if (GetConstruction().IsPartConstructed("pole")) return 3;
		if (GetConstruction().IsPartConstructed("support")) return 2;
		return 1;
	}

	void AGA_SetGid(int gid)
	{
		m_AGA_Gid = gid;
		SetSynchDirty();
	}

	void AGA_SetInvite(bool on)
	{
		m_AGA_Invite = on;
		SetSynchDirty();
	}

	void AGA_UpdateCount()
	{
		int n = 0;
		AGA_Group g = AGA_Territory.ById(m_AGA_Gid);
		if (g) n = g.Ids.Count();
		if (n != m_AGA_Members)
		{
			m_AGA_Members = n;
			SetSynchDirty();
		}
	}

	// reset = true przy budowie (pełny czas), false po restarcie (tylko limit)
	void AGA_ApplyLife(bool reset = true)
	{
		if (!GetGame().IsServer()) return;
		float life = AGA_Territory.LIFE_L1;
		if (AGA_Level() >= 2) life = AGA_Territory.LIFE_L2;
		SetLifetimeMax(life);
		if (reset) SetLifetime(life);
	}

	override string GetDisplayName()
	{
		string s = super.GetDisplayName() + " (Lvl " + AGA_Level().ToString() + ")";
		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (me && m_AGA_Gid != 0 && me.AGA_Gid() == m_AGA_Gid)
			s = s + " [" + m_AGA_Members.ToString() + "/" + AGA_Territory.LIMIT.ToString() + "]";
		return s;
	}

	override void OnPartBuiltServer(notnull Man player, string part_name, int action_id)
	{
		super.OnPartBuiltServer(player, part_name, action_id);
		AGA_ApplyLife();
	}

	override void OnPartDismantledServer(notnull Man player, string part_name, int action_id)
	{
		super.OnPartDismantledServer(player, part_name, action_id);
		if (part_name == "base" && m_AGA_Gid != 0)
		{
			int gid = m_AGA_Gid;
			AGA_SetGid(0);
			AGA_Territory.Disband(gid, "the flag pole was dismantled.");
		}
		else AGA_ApplyLife(false);
	}

	override void OnPartDestroyedServer(Man player, string part_name, int action_id, bool destroyed_by_connected_part = false)
	{
		super.OnPartDestroyedServer(player, part_name, action_id, destroyed_by_connected_part);
		if (part_name == "base" && m_AGA_Gid != 0)
		{
			int gid = m_AGA_Gid;
			AGA_SetGid(0);
			AGA_Territory.Disband(gid, "the flag pole was destroyed.");
		}
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionFlagInviteOn);
		AddAction(AGA_ActionFlagInviteOff);
		AddAction(AGA_ActionFlagJoin);
		AddAction(AGA_ActionFlagLeave);
		AddAction(AGA_ActionFlagGroup);
	}
}

class AGA_Territory
{
	static const float RADIUS = 30;
	static const int LIMIT = 8;
	static const int MAX_CRATES = 6;
	static const int MAX_FIREPLACES = 1;
	static const int MAX_VEHICLES = 2;
	static const float LIFE_L1 = 432000;   // 5 dni
	static const float LIFE_L2 = 604800;   // 7 dni

	static ref array<TerritoryFlag> s_Flags;
	static ref AGA_GroupDb s_Db;
	static ref AGA_ZoneDb s_Zones;
	static bool s_OpenMenu;
	static int s_LastMsg;
	static string s_LastText;

	static array<TerritoryFlag> Flags()
	{
		if (!s_Flags) s_Flags = new array<TerritoryFlag>;
		return s_Flags;
	}

	// najbliższa flaga, w której promieniu (x mnożnik) leży pozycja
	static TerritoryFlag FlagAt(vector pos, float mult = 1)
	{
		TerritoryFlag best;
		float bestD = RADIUS * mult;
		pos[1] = 0;
		foreach (TerritoryFlag f : Flags())
		{
			if (!f || f.IsHologram()) continue;
			vector fp = f.GetPosition();
			fp[1] = 0;
			float d = vector.Distance(fp, pos);
			if (d <= bestD)
			{
				best = f;
				bestD = d;
			}
		}
		return best;
	}

	// komunikat na ekranie klienta (bez spamu)
	static void Msg(string text)
	{
		if (GetGame().IsDedicatedServer()) return;
		int now = GetGame().GetTime();
		if (text == s_LastText && now - s_LastMsg < 6000) return;
		if (now - s_LastMsg < 1500) return;
		s_LastMsg = now;
		s_LastText = text;
		NotificationSystem.AddNotificationExtended(4, "TERRITORY", text, "set:dayz_gui image:icon_info");
	}

	static void Tell(PlayerBase p, string text)
	{
		if (p && p.GetIdentity())
			NotificationSystem.SendNotificationToPlayerIdentityExtended(p.GetIdentity(), 5, "TERRITORY", text, "set:dayz_gui image:icon_info");
	}

	// ---------------------------------------------------------------- strefy zakazu budowy
	static AGA_ZoneDb Zones()
	{
		if (!s_Zones)
		{
			s_Zones = new AGA_ZoneDb();
			if (GetGame().IsServer())
			{
				string p = "$profile:ArmaGedonCore/nobuild.json";
				if (FileExist(p)) JsonFileLoader<AGA_ZoneDb>.JsonLoadFile(p, s_Zones);
			}
		}
		return s_Zones;
	}

	static string ZoneAt(vector pos)
	{
		foreach (AGA_ZoneDef z : Zones().Zones)
		{
			float dx = pos[0] - z.X;
			float dz = pos[2] - z.Z;
			if (dx * dx + dz * dz <= z.R * z.R) return z.K;
		}
		return "";
	}

	static bool OnRoad(vector pos)
	{
		float ground = GetGame().SurfaceY(pos[0], pos[2]);
		float road = GetGame().SurfaceRoadY(pos[0], pos[2]);
		return road > ground + 0.005;
	}

	// ---------------------------------------------------------------- stawianie przedmiotów
	static bool IsRaidItem(EntityAI item)
	{
		return item.IsInherited(ExplosivesBase) || item.IsInherited(TrapBase);
	}

	static bool NeedsTerritory(EntityAI item)
	{
		return item.IsInherited(FenceKit) || item.IsInherited(WatchtowerKit) || item.IsInherited(WoodenCrate);
	}

	static bool ZoneLimited(EntityAI item)
	{
		return item.IsInherited(TerritoryFlagKit) || item.IsInherited(KitBase) || item.IsInherited(TentBase) || item.IsInherited(WoodenCrate);
	}

	static int CountNear(TerritoryFlag flag, typename t)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(flag.GetPosition(), RADIUS, objs, null);
		int n = 0;
		foreach (Object o : objs)
		{
			EntityAI e = EntityAI.Cast(o);
			if (!e || e.IsHologram() || e.GetHierarchyParent()) continue;
			if (e.IsInherited(t)) n++;
		}
		return n;
	}

	static bool CanPlace(EntityAI item, Man player, vector pos, out string msg)
	{
		msg = "";
		PlayerBase pb = PlayerBase.Cast(player);
		if (!pb || !item || IsRaidItem(item)) return true;
		if (pb.AGA_IsStaff()) return true;

		if (ZoneLimited(item))
		{
			if (ZoneAt(pos) != "")
			{
				msg = "You can't build here: military, medical, police areas and castles are restricted.";
				return false;
			}
			if (OnRoad(pos))
			{
				msg = "You can't build on roads.";
				return false;
			}
		}

		if (item.IsInherited(TerritoryFlagKit))
		{
			if (pb.AGA_Gid() != 0)
			{
				msg = "You are already part of a territory.";
				return false;
			}
			if (FlagAt(pos, 2))
			{
				msg = "This area cannot be claimed (too close to another territory).";
				return false;
			}
			return true;
		}

		TerritoryFlag flag = FlagAt(pos);
		if (flag)
		{
			if (flag.AGA_Gid() != 0 && flag.AGA_Gid() != pb.AGA_Gid())
			{
				msg = "You are not allowed to build in this territory.";
				return false;
			}
			if (item.IsInherited(WoodenCrate) && CountNear(flag, WoodenCrate) >= MAX_CRATES)
			{
				msg = "Storage limit reached: max " + MAX_CRATES.ToString() + " crates per base.";
				return false;
			}
			if (item.IsInherited(Fireplace) && CountNear(flag, Fireplace) >= MAX_FIREPLACES)
			{
				msg = "Limit reached: max " + MAX_FIREPLACES.ToString() + " fireplace per base.";
				return false;
			}
			return true;
		}

		if (NeedsTerritory(item))
		{
			msg = "Placing " + item.GetDisplayName() + " requires your own territory. Combine a sharpened stick with a rag to make a flag pole kit.";
			return false;
		}
		return true;
	}

	// czy gracz może budować przy tym obiekcie
	static bool CanBuildAt(PlayerBase pb, vector pos, out string msg)
	{
		msg = "";
		if (!pb || pb.AGA_IsStaff()) return true;
		TerritoryFlag flag = FlagAt(pos);
		if (!flag)
		{
			msg = "Building requires a flag pole! Combine a sharpened stick with a rag.";
			return false;
		}
		if (flag.AGA_Gid() != 0 && flag.AGA_Gid() != pb.AGA_Gid())
		{
			msg = "You are not allowed to build in this territory.";
			return false;
		}
		return true;
	}

	static bool CanDismantleAt(PlayerBase pb, vector pos, out string msg)
	{
		msg = "";
		if (!pb || pb.AGA_IsStaff()) return true;
		TerritoryFlag flag = FlagAt(pos);
		if (!flag || flag.AGA_Gid() == 0 || flag.AGA_Gid() == pb.AGA_Gid()) return true;
		if (!flag.AGA_Raised()) return true;
		msg = "You are not allowed to dismantle in this territory while the flag is raised.";
		return false;
	}

	// ================================================================ serwer: grupy
	static AGA_GroupDb Db()
	{
		if (!s_Db)
		{
			s_Db = new AGA_GroupDb();
			string p = "$profile:ArmaGedonCore/groups.json";
			if (FileExist(p)) JsonFileLoader<AGA_GroupDb>.JsonLoadFile(p, s_Db);
		}
		return s_Db;
	}

	static void Save()
	{
		JsonFileLoader<AGA_GroupDb>.JsonSaveFile("$profile:ArmaGedonCore/groups.json", Db());
	}

	static AGA_Group ById(int gid)
	{
		if (gid <= 0) return null;
		foreach (AGA_Group g : Db().Groups)
		{
			if (g.Id == gid) return g;
		}
		return null;
	}

	static AGA_Group Of(string steamId)
	{
		foreach (AGA_Group g : Db().Groups)
		{
			if (g.Ids.Find(steamId) >= 0) return g;
		}
		return null;
	}

	static TerritoryFlag FlagOf(AGA_Group g)
	{
		if (!g) return null;
		foreach (TerritoryFlag f : Flags())
		{
			if (f && f.AGA_Gid() == g.Id) return f;
		}
		return null;
	}

	static void AGA_SyncPl(PlayerBase p)
	{
		if (!p || !p.GetIdentity()) return;
		AGA_Group g = Of(p.GetIdentity().GetPlainId());
		int gid = 0;
		if (g) gid = g.Id;
		p.AGA_SetGroup(gid, AGA_Server.Get().IsStaff(p.GetIdentity()));
	}

	static void SyncAll()
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
			AGA_SyncPl(PlayerBase.Cast(m));
		foreach (TerritoryFlag f : Flags())
		{
			if (f) f.AGA_UpdateCount();
		}
	}

	static void OnFlagPlaced(PlayerBase owner, vector pos)
	{
		if (!owner || !owner.GetIdentity()) return;
		TerritoryFlag flag;
		float bd = 3;
		foreach (TerritoryFlag f : Flags())
		{
			if (!f) continue;
			float d = vector.Distance(f.GetPosition(), pos);
			if (d < bd)
			{
				bd = d;
				flag = f;
			}
		}
		if (!flag) return;
		string id = owner.GetIdentity().GetPlainId();
		AGA_Group old = Of(id);
		if (old) Remove(old, id);
		AGA_Group g = new AGA_Group();
		g.Id = Db().Next;
		Db().Next = Db().Next + 1;
		g.Owner = id;
		g.Ids.Insert(id);
		g.Names.Insert(owner.GetIdentity().GetName());
		g.Pos = flag.GetPosition();
		Db().Groups.Insert(g);
		Save();
		flag.AGA_SetGid(g.Id);
		if (flag.GetConstruction() && !flag.GetConstruction().IsPartConstructed("base"))
			flag.GetConstruction().BuildPartServerEx(owner, "base", AT_BUILD_PART);
		flag.AGA_ApplyLife();
		AGA_SyncPl(owner);
		flag.AGA_UpdateCount();
		AGA_Server.Get().Log("[TERYTORIUM] nowa baza #" + g.Id.ToString() + " " + owner.GetIdentity().GetName() + " (" + id + ") @ " + pos.ToString());
		Tell(owner, "Territory claimed. You are the base owner. Enable invites at the flag pole to add members (max " + LIMIT.ToString() + ").");
	}

	static void Join(PlayerBase p, TerritoryFlag flag)
	{
		if (!p || !p.GetIdentity() || !flag) return;
		AGA_Group g = ById(flag.AGA_Gid());
		if (!g || !flag.AGA_Invite()) return;
		string id = p.GetIdentity().GetPlainId();
		if (Of(id))
		{
			Tell(p, "You are already part of a territory. Leave it first.");
			return;
		}
		if (g.Ids.Count() >= LIMIT)
		{
			Tell(p, "This group is full (" + LIMIT.ToString() + "/" + LIMIT.ToString() + ").");
			return;
		}
		g.Ids.Insert(id);
		g.Names.Insert(p.GetIdentity().GetName());
		Save();
		AGA_SyncPl(p);
		flag.AGA_UpdateCount();
		AGA_Server.Get().Log("[TERYTORIUM] " + p.GetIdentity().GetName() + " (" + id + ") dolaczyl do bazy #" + g.Id.ToString());
		Tell(p, "You joined the group (" + g.Ids.Count().ToString() + "/" + LIMIT.ToString() + ").");
		NotifyGroup(g, p.GetIdentity().GetName() + " joined your group.");
	}

	static void Remove(AGA_Group g, string id)
	{
		int i = g.Ids.Find(id);
		if (i < 0) return;
		g.Ids.Remove(i);
		if (i < g.Names.Count()) g.Names.Remove(i);
		Save();
		PlayerBase p = AGA_Server.Get().FindPlayer(id);
		if (p) AGA_SyncPl(p);
		TerritoryFlag f = FlagOf(g);
		if (f) f.AGA_UpdateCount();
	}

	static void Leave(PlayerBase p)
	{
		if (!p || !p.GetIdentity()) return;
		string id = p.GetIdentity().GetPlainId();
		AGA_Group g = Of(id);
		if (!g) return;
		if (g.Owner == id)
		{
			Tell(p, "The owner can't leave. Lower the flag and dismantle the flag pole to give up the territory.");
			return;
		}
		Remove(g, id);
		AGA_Server.Get().Log("[TERYTORIUM] " + p.GetIdentity().GetName() + " (" + id + ") opuscil baze #" + g.Id.ToString());
		Tell(p, "You left the group.");
		NotifyGroup(g, p.GetIdentity().GetName() + " left your group.");
	}

	static void Kick(PlayerBase owner, string target)
	{
		if (!owner || !owner.GetIdentity()) return;
		AGA_Group g = Of(owner.GetIdentity().GetPlainId());
		if (!g || g.Owner != owner.GetIdentity().GetPlainId() || target == g.Owner) return;
		int i = g.Ids.Find(target);
		if (i < 0) return;
		string name = g.Names[i];
		Remove(g, target);
		AGA_Server.Get().Log("[TERYTORIUM] wlasciciel bazy #" + g.Id.ToString() + " usunal " + name + " (" + target + ")");
		NotifyGroup(g, name + " was removed from the group.");
		PlayerBase t = AGA_Server.Get().FindPlayer(target);
		if (t) Tell(t, "You were removed from the group.");
	}

	static void Disband(int gid, string reason)
	{
		AGA_Group g = ById(gid);
		if (!g) return;
		NotifyGroup(g, "Your territory is gone: " + reason);
		Db().Groups.RemoveItem(g);
		Save();
		AGA_Server.Get().Log("[TERYTORIUM] baza #" + gid.ToString() + " rozwiazana: " + reason);
		SyncAll();
	}

	static void NotifyGroup(AGA_Group g, string text)
	{
		foreach (string id : g.Ids)
		{
			PlayerBase p = AGA_Server.Get().FindPlayer(id);
			if (p) Tell(p, text);
		}
	}

	// przypisanie grup do flag po starcie (po pozycji) + porządki
	static void Restore()
	{
		foreach (TerritoryFlag f : Flags())
		{
			if (!f || f.AGA_Gid() != 0) continue;
			foreach (AGA_Group g : Db().Groups)
			{
				if (vector.Distance(g.Pos, f.GetPosition()) < 2)
				{
					f.AGA_SetGid(g.Id);
					break;
				}
			}
			f.AGA_ApplyLife(false);
		}
		SyncAll();
	}

	static void Cleanup()
	{
		for (int i = Db().Groups.Count() - 1; i >= 0; i--)
		{
			AGA_Group g = Db().Groups[i];
			if (!FlagOf(g)) Disband(g.Id, "the flag pole no longer exists.");
		}
		VehicleCheck();
	}

	static void VehicleCheck()
	{
		foreach (TerritoryFlag f : Flags())
		{
			if (!f || f.AGA_Gid() == 0) continue;
			array<Object> objs = new array<Object>;
			GetGame().GetObjectsAtPosition(f.GetPosition(), RADIUS, objs, null);
			int n = 0;
			foreach (Object o : objs)
			{
				if (o && o.IsInherited(Transport)) n++;
			}
			if (n > MAX_VEHICLES)
			{
				string msg = "[BAZA] " + n.ToString() + " pojazdy na terytorium bazy #" + f.AGA_Gid().ToString() + " @ " + f.GetPosition().ToString() + " (limit " + MAX_VEHICLES.ToString() + ")";
				AGA_Server.Get().Log(msg);
				AGA_Link.StaffAlert(msg);
			}
		}
	}

	static void Init()
	{
		Db();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Restore, 15000, false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Cleanup, 600000, false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Cleanup, 3600000, true);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProbeRoads, 60000, false);
	}

	// jednorazowa sonda wykrywania dróg (log do kalibracji)
	static void ProbeRoads()
	{
		array<vector> pts = AGA_Events.s_Roads;
		if (!pts) return;
		int on = 0;
		int off = 0;
		int n = 0;
		foreach (vector v : pts)
		{
			if (++n > 200) break;
			if (OnRoad(v)) on++;
			vector side = v + "35 0 35";
			if (OnRoad(side)) off++;
		}
		string t;
		GetGame().SurfaceGetType(pts[0][0], pts[0][2], t);
		AGA_Server.Get().Log("[DROGI] sonda: na drodze " + on.ToString() + "/" + Math.Min(n, 200).ToString() + ", obok drogi " + off.ToString() + ", typ " + t + ", roadY-Y " + (GetGame().SurfaceRoadY(pts[0][0], pts[0][2]) - GetGame().SurfaceY(pts[0][0], pts[0][2])).ToString());
	}

	static AGA_GroupView View(PlayerBase p)
	{
		AGA_GroupView v = new AGA_GroupView();
		v.Limit = LIMIT;
		if (!p || !p.GetIdentity()) return v;
		AGA_Group g = Of(p.GetIdentity().GetPlainId());
		if (!g) return v;
		v.Gid = g.Id;
		v.IsOwner = g.Owner == p.GetIdentity().GetPlainId();
		TerritoryFlag f = FlagOf(g);
		if (f)
		{
			v.Level = f.AGA_Level();
			v.Raised = f.AGA_Raised();
			v.Invite = f.AGA_Invite();
		}
		for (int i = 0; i < g.Ids.Count(); i++)
		{
			v.Ids.Insert(g.Ids[i]);
			string nm = "?";
			if (i < g.Names.Count()) nm = g.Names[i];
			PlayerBase m = AGA_Server.Get().FindPlayer(g.Ids[i]);
			if (m && m.GetIdentity()) nm = m.GetIdentity().GetName();
			if (g.Ids[i] == g.Owner) nm = nm + "  (owner)";
			v.Names.Insert(nm);
			int on = 0;
			if (m) on = 1;
			v.Online.Insert(on);
		}
		return v;
	}

	static void OnCmd(ParamsReadContext ctx, PlayerIdentity sender)
	{
		Param2<string, string> data;
		if (!ctx.Read(data) || !sender) return;
		PlayerBase p = AGA_Server.Get().FindPlayer(sender.GetPlainId());
		if (!p) return;
		string cmd = data.param1;
		if (cmd == "kick") Kick(p, data.param2);
		else if (cmd == "leave") Leave(p);
		AGA_GroupView gv = View(p);
		Param1<ref AGA_GroupView> prm = new Param1<ref AGA_GroupView>(gv);
		GetRPCManager().SendRPC("AGA", "AGA_OnGroupView", prm, true, sender);
	}
}

modded class TerritoryFlagKit
{
	override void AssembleKit() {}

	override void DisassembleKit(ItemBase item)
	{
		if (IsHologram()) return;
		ItemBase stick = ItemBase.Cast(GetGame().CreateObjectEx("SharpWoodenStick", GetPosition(), ECE_PLACE_ON_SURFACE));
		if (stick) MiscGameplayFunctions.TransferItemProperties(this, stick);
		GetGame().CreateObjectEx("Rag", GetPosition(), ECE_PLACE_ON_SURFACE);
	}

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);
		if (GetGame().IsServer())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Territory.OnFlagPlaced, 300, false, PlayerBase.Cast(player), position);
	}
}

// ==================================================================== stawianie
modded class ItemBase
{
	override bool CanBePlaced(Man player, vector position)
	{
		if (!super.CanBePlaced(player, position)) return false;
		if (!player) return true;
		string msg;
		if (!AGA_Territory.CanPlace(this, player, position, msg))
		{
			if (msg != "") AGA_Territory.Msg(msg);
			return false;
		}
		return true;
	}
}

modded class TentBase
{
	override bool CanBePlaced(Man player, vector position)
	{
		if (!super.CanBePlaced(player, position)) return false;
		if (!player) return true;
		string msg;
		if (!AGA_Territory.CanPlace(this, player, position, msg))
		{
			if (msg != "") AGA_Territory.Msg(msg);
			return false;
		}
		return true;
	}
}

modded class FireplaceBase
{
	override bool CanBePlaced(Man player, vector position)
	{
		if (!super.CanBePlaced(player, position)) return false;
		if (!player) return true;
		string msg;
		if (!AGA_Territory.CanPlace(this, player, position, msg))
		{
			if (msg != "") AGA_Territory.Msg(msg);
			return false;
		}
		return true;
	}
}

// ==================================================================== budowanie / rozbieranie
modded class ActionBuildPart
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item)) return false;
		BaseBuildingBase bb = BaseBuildingBase.Cast(target.GetObject());
		if (!bb || bb.IsInherited(ShelterSite)) return true;
		string msg;
		if (!AGA_Territory.CanBuildAt(player, bb.GetPosition(), msg))
		{
			AGA_Territory.Msg(msg);
			return false;
		}
		return true;
	}
}

modded class ActionDismantlePart
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item)) return false;
		Object o = target.GetObject();
		if (!o) return true;
		string msg;
		if (!AGA_Territory.CanDismantleAt(player, o.GetPosition(), msg))
		{
			AGA_Territory.Msg(msg);
			return false;
		}
		return true;
	}
}

modded class ActionAttachToConstruction
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item)) return false;
		BaseBuildingBase bb = BaseBuildingBase.Cast(target.GetObject());
		if (!bb || bb.IsInherited(ShelterSite)) return true;
		string msg;
		return AGA_Territory.CanBuildAt(player, bb.GetPosition(), msg);
	}
}

// zamek szyfrowy tylko na bramie we własnym terytorium
modded class Fence
{
	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (!super.CanReceiveAttachment(attachment, slotId)) return false;
		if (!attachment || !attachment.IsInherited(CodeLock) || GetGame().IsDedicatedServer()) return true;
		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (!me || me.AGA_IsStaff()) return true;
		TerritoryFlag flag = AGA_Territory.FlagAt(GetPosition());
		if (!flag || (flag.AGA_Gid() != 0 && flag.AGA_Gid() != me.AGA_Gid()))
		{
			AGA_Territory.Msg("A code lock can only be attached in your own territory.");
			return false;
		}
		return true;
	}
}

// ==================================================================== receptura zestawu flagi
class AGA_CraftFlagKit extends RecipeBase
{
	override void Init()
	{
		m_Name = "#STR_CfgVehicles_TerritoryFlagKit0";
		m_IsInstaRecipe = false;
		m_AnimationLength = 2;
		m_Specialty = -0.02;

		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = 1;
		m_MaxQuantityIngredient[0] = -1;
		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = 1;
		m_MaxQuantityIngredient[1] = -1;

		InsertIngredient(0, "SharpWoodenStick");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = -1;
		m_IngredientDestroy[0] = false;
		m_IngredientUseSoftSkills[0] = false;

		InsertIngredient(1, "Rag");
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = -1;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;

		AddResult("TerritoryFlagKit");
		m_ResultSetFullQuantity[0] = false;
		m_ResultSetQuantity[0] = -1;
		m_ResultSetHealth[0] = -1;
		m_ResultInheritsHealth[0] = -1;
		m_ResultInheritsColor[0] = -1;
		m_ResultToInventory[0] = -2;
		m_ResultUseSoftSkills[0] = false;
		m_ResultReplacesIngredient[0] = -1;
	}
}

modded class CraftTerritoryFlagKit
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		return false;
	}
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new AGA_CraftFlagKit);
	}
}

// ==================================================================== akcje przy fladze
class AGA_ActionFlagBase : ActionInteractBase
{
	void AGA_ActionFlagBase()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
	}

	override bool UseMainItem() { return false; }
	override bool MainItemAlwaysInHands() { return false; }

	TerritoryFlag Flag(ActionTarget target)
	{
		if (!target) return null;
		return TerritoryFlag.Cast(target.GetObject());
	}

	bool IsOwner(PlayerBase player, TerritoryFlag f)
	{
		return f && f.AGA_Gid() != 0 && player.AGA_Gid() == f.AGA_Gid();
	}
}

class AGA_ActionFlagInviteOn : AGA_ActionFlagBase
{
	void AGA_ActionFlagInviteOn() { m_Text = "Enable group invites"; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		TerritoryFlag f = Flag(target);
		return IsOwner(player, f) && !f.AGA_Invite();
	}

	override void OnExecuteServer(ActionData action_data)
	{
		TerritoryFlag f = Flag(action_data.m_Target);
		PlayerBase p = action_data.m_Player;
		if (!f || !p.GetIdentity()) return;
		AGA_Group g = AGA_Territory.ById(f.AGA_Gid());
		if (!g || g.Owner != p.GetIdentity().GetPlainId())
		{
			AGA_Territory.Tell(p, "Only the base owner can manage invites.");
			return;
		}
		f.AGA_SetInvite(true);
		AGA_Territory.Tell(p, "Invites enabled. Players can now join at this flag pole.");
	}
}

class AGA_ActionFlagInviteOff : AGA_ActionFlagBase
{
	void AGA_ActionFlagInviteOff() { m_Text = "Disable group invites"; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		TerritoryFlag f = Flag(target);
		return IsOwner(player, f) && f.AGA_Invite();
	}

	override void OnExecuteServer(ActionData action_data)
	{
		TerritoryFlag f = Flag(action_data.m_Target);
		PlayerBase p = action_data.m_Player;
		if (!f || !p.GetIdentity()) return;
		AGA_Group g = AGA_Territory.ById(f.AGA_Gid());
		if (!g || g.Owner != p.GetIdentity().GetPlainId()) return;
		f.AGA_SetInvite(false);
		AGA_Territory.Tell(p, "Invites disabled.");
	}
}

class AGA_ActionFlagJoin : AGA_ActionFlagBase
{
	void AGA_ActionFlagJoin() { m_Text = "Join group"; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		TerritoryFlag f = Flag(target);
		return f && f.AGA_Gid() != 0 && f.AGA_Invite() && player.AGA_Gid() == 0;
	}

	override void OnExecuteServer(ActionData action_data)
	{
		AGA_Territory.Join(action_data.m_Player, Flag(action_data.m_Target));
	}
}

class AGA_ActionFlagLeave : AGA_ActionFlagBase
{
	void AGA_ActionFlagLeave() { m_Text = "Leave group"; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return IsOwner(player, Flag(target));
	}

	override void OnExecuteServer(ActionData action_data)
	{
		AGA_Territory.Leave(action_data.m_Player);
	}
}

class AGA_ActionFlagGroup : AGA_ActionFlagBase
{
	void AGA_ActionFlagGroup() { m_Text = "Group overview"; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return IsOwner(player, Flag(target));
	}

	override void OnExecuteClient(ActionData action_data)
	{
		AGA_Territory.s_OpenMenu = true;
	}
}
