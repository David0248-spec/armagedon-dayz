// ARMA-GEDON Admin – logika serwera
class AGA_Server
{
	static ref AGA_Server s_Instance;

	protected string m_Dir = "$profile:ArmaGedonAdmin";
	protected ref AGA_Config m_Config;
	protected ref AGA_Bans m_Bans;
	protected ref map<string, bool> m_God;
	protected ref map<string, bool> m_Invis;
	protected ref array<string> m_Recent;   // ostatnie wpisy admina
	protected ref array<string> m_Combat;   // ostatnie trafienia / zabójstwa
	protected float m_SyncTimer;
	protected ref array<PlayerBase> m_Bots;
	protected ref map<string, vector> m_AfkPos;
	protected ref map<string, int> m_AfkMin;

	static AGA_Server Get()
	{
		if (!s_Instance)
			s_Instance = new AGA_Server();
		return s_Instance;
	}

	void AGA_Server()
	{
		m_God = new map<string, bool>;
		m_Invis = new map<string, bool>;
		m_Recent = new array<string>;
		m_Combat = new array<string>;
		m_Bots = new array<PlayerBase>;
		m_AfkPos = new map<string, vector>;
		m_AfkMin = new map<string, int>;
		if (!FileExist(m_Dir))
			MakeDirectory(m_Dir);
		LoadConfig();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(SyncInvis, 2000, true);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RaidTick, 60000, true);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ProbeHeliSpots, 90000, false);
		AGA_Raid.Cfg();
	}

	void LoadConfig()
	{
		string cfgPath = m_Dir + "/admins.json";
		m_Config = new AGA_Config();
		if (FileExist(cfgPath))
		{
			JsonFileLoader<AGA_Config>.JsonLoadFile(cfgPath, m_Config);
		}
		else
		{
			m_Config.Admins.Insert("76561199562774488");
			JsonFileLoader<AGA_Config>.JsonSaveFile(cfgPath, m_Config);
		}

		string banPath = m_Dir + "/bans.json";
		m_Bans = new AGA_Bans();
		if (FileExist(banPath))
			JsonFileLoader<AGA_Bans>.JsonLoadFile(banPath, m_Bans);
		Log("Załadowano adminów: " + m_Config.Admins.Count().ToString() + ", banów: " + m_Bans.Banned.Count().ToString());
	}

	bool IsAdmin(PlayerIdentity id)
	{
		if (!id || !m_Config)
			return false;
		return m_Config.Admins.Find(id.GetPlainId()) != -1;
	}

	bool IsBanned(string steamId)
	{
		return m_Bans.Banned.Find(steamId) != -1;
	}

	string Stamp()
	{
		int y, mo, d, h, mi, s;
		GetYearMonthDay(y, mo, d);
		GetHourMinuteSecond(h, mi, s);
		return string.Format("%1-%2-%3 %4:%5:%6", y, mo.ToStringLen(2), d.ToStringLen(2), h.ToStringLen(2), mi.ToStringLen(2), s.ToStringLen(2));
	}

	void Log(string line)
	{
		string full = Stamp() + " | " + line;
		FileHandle f = OpenFile(m_Dir + "/admin_log.txt", FileMode.APPEND);
		if (f != 0)
		{
			FPrintln(f, full);
			CloseFile(f);
		}
		m_Recent.Insert(full);
		if (m_Recent.Count() > 60)
			m_Recent.Remove(0);
		Print("[ArmaGedonAdmin] " + line);
	}

	void CombatLog(string line)
	{
		string full = Stamp() + " | " + line;
		FileHandle f = OpenFile(m_Dir + "/combat_log.txt", FileMode.APPEND);
		if (f != 0)
		{
			FPrintln(f, full);
			CloseFile(f);
		}
		m_Combat.Insert(full);
		if (m_Combat.Count() > 60)
			m_Combat.Remove(0);
	}

	PlayerBase FindPlayer(string steamId)
	{
		for (int b = 0; b < m_Bots.Count(); b++)
		{
			if (m_Bots[b] && BotId(b) == steamId)
				return m_Bots[b];
		}
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (pb && pb.GetIdentity() && pb.GetIdentity().GetPlainId() == steamId)
				return pb;
		}
		return null;
	}

	void Notify(PlayerIdentity id, string text)
	{
		NotificationSystem.SendNotificationToPlayerIdentityExtended(id, 5, "ARMA-GEDON ADMIN", text, "set:dayz_gui image:icon_info");
	}

	string BotId(int idx)
	{
		return "BOT" + (idx + 1).ToString();
	}

	string BotName(PlayerBase p)
	{
		int idx = m_Bots.Find(p);
		if (idx == -1) return "?";
		return "BOT TESTOWY " + (idx + 1).ToString();
	}

	void SpawnBot(PlayerBase p, PlayerIdentity id)
	{
		vector at = p.GetPosition() + p.GetDirection() * 6;
		at = GroundAt(at);
		PlayerBase bot = PlayerBase.Cast(GetGame().CreateObjectEx("SurvivorM_Mirek", at, ECE_PLACE_ON_SURFACE));
		if (!bot)
		{
			Notify(id, "Nie udało się stworzyć bota");
			return;
		}
		vector o = p.GetOrientation();
		o[0] = o[0] + 180;
		bot.SetOrientation(o);
		array<string> gear = {"TShirt_Red", "Jeans_Blue", "AthleticShoes_Black", "BallisticHelmet_Green", "PlateCarrierVest"};
		foreach (string g : gear)
			bot.GetInventory().CreateInInventory(g);
		m_Bots.Insert(bot);
		Notify(id, "Bot testowy stworzony: " + BotName(bot));
		Log(Who(p) + " stworzył bota testowego");
	}

	void ClearBots(PlayerBase p, PlayerIdentity id)
	{
		int n = 0;
		foreach (PlayerBase b : m_Bots)
		{
			if (b)
			{
				GetGame().ObjectDelete(b);
				n++;
			}
		}
		m_Bots.Clear();
		Notify(id, "Usunięto botów: " + n.ToString());
		Log(Who(p) + " usunął boty testowe");
	}

	string Who(PlayerBase p)
	{
		if (p && p.GetIdentity())
			return p.GetIdentity().GetName() + " (" + p.GetIdentity().GetPlainId() + ")";
		return "?";
	}

	string NameOf(PlayerBase p)
	{
		if (p && p.GetIdentity())
			return p.GetIdentity().GetName();
		if (p)
			return BotName(p);
		return "?";
	}

	void RaidTick()
	{
		AGA_Raid.Tick();
		AfkTick();
	}

	// wyrzucanie AFK po 15 minutach bez ruchu (admini pominięci)
	void AfkTick()
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (!pb || !pb.GetIdentity() || !pb.IsAlive() || IsAdmin(pb.GetIdentity())) continue;
			string id = pb.GetIdentity().GetPlainId();
			vector now = pb.GetPosition() + pb.GetOrientation();
			int mins = 0;
			if (m_AfkPos.Contains(id) && vector.Distance(m_AfkPos.Get(id), now) < 0.5)
				mins = m_AfkMin.Get(id) + 1;
			m_AfkPos.Set(id, now);
			m_AfkMin.Set(id, mins);
			if (mins == 13)
				NotificationSystem.SendNotificationToPlayerIdentityExtended(pb.GetIdentity(), 10, "AFK WARNING", "You will be kicked in 2 minutes for being AFK.", "set:dayz_gui image:icon_info");
			if (mins >= 15)
			{
				Log("AFK kick: " + Who(pb));
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DoKick, 500, false, id);
				m_AfkPos.Remove(id);
				m_AfkMin.Remove(id);
			}
		}
	}

	void WelcomeInfo(string steamId)
	{
		PlayerBase p = FindPlayer(steamId);
		if (!p || !p.GetIdentity()) return;
		string state = "Raiding is CLOSED right now.";
		if (AGA_Raid.IsRaidTime()) state = "Raiding is OPEN right now.";
		NotificationSystem.SendNotificationToPlayerIdentityExtended(p.GetIdentity(), 14, "WELCOME TO ARMA-GEDON", state + " " + AGA_Raid.Cfg().Info, "set:dayz_gui image:icon_info");
	}

	// jednorazowa sonda: płaskie, wolne miejsca na lotniskach pod spawn helikopterów
	void ProbeHeliSpots()
	{
		string path = m_Dir + "/heli_spots.txt";
		if (FileExist(path)) return;
		array<vector> centers = {Vector(4800, 0, 2500), Vector(4700, 0, 10100), Vector(11950, 0, 12550), Vector(12100, 0, 9150), Vector(7400, 0, 4900)};
		array<vector> found = new array<vector>;
		foreach (vector c : centers)
		{
			int got = 0;
			for (int i = 0; i < 400 && got < 6; i++)
			{
				vector p = c + Vector(Math.RandomFloat(-380, 380), 0, Math.RandomFloat(-380, 380));
				if (GetGame().SurfaceIsSea(p[0], p[2]) || GetGame().SurfaceIsPond(p[0], p[2])) continue;
				float y0 = GetGame().SurfaceY(p[0], p[2]);
				float y1 = GetGame().SurfaceY(p[0] + 7, p[2]);
				float y2 = GetGame().SurfaceY(p[0] - 7, p[2]);
				float y3 = GetGame().SurfaceY(p[0], p[2] + 7);
				float y4 = GetGame().SurfaceY(p[0], p[2] - 7);
				float mx = Math.Max(Math.Max(y1, y2), Math.Max(y3, y4));
				float mn = Math.Min(Math.Min(y1, y2), Math.Min(y3, y4));
				if (Math.AbsFloat(mx - y0) > 0.4 || Math.AbsFloat(mn - y0) > 0.4) continue;
				p[1] = y0;
				array<Object> objs = new array<Object>;
				array<CargoBase> cargos = new array<CargoBase>;
				GetGame().GetObjectsAtPosition(p, 11, objs, cargos);
				if (objs.Count() > 0) continue;
				bool near = false;
				foreach (vector f : found)
				{
					if (vector.Distance(f, p) < 45) near = true;
				}
				if (near) continue;
				found.Insert(p);
				got++;
			}
		}
		FileHandle fh = OpenFile(path, FileMode.WRITE);
		if (fh != 0)
		{
			foreach (vector v : found)
				FPrintln(fh, Math.Round(v[0]).ToString() + " " + Math.Round(v[1]).ToString() + " " + Math.Round(v[2]).ToString());
			CloseFile(fh);
		}
		Log("Sonda helikopterow: znaleziono miejsc: " + found.Count().ToString());
	}

	// ---------- niewidzialność: rozsyłanie listy do wszystkich ----------
	void SyncInvis()
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		array<int> lows = new array<int>;
		array<int> highs = new array<int>;
		foreach (Man m : players)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (!pb || !pb.GetIdentity()) continue;
			if (!m_Invis.Contains(pb.GetIdentity().GetPlainId())) continue;
			int lo, hi;
			pb.GetNetworkID(lo, hi);
			lows.Insert(lo);
			highs.Insert(hi);
		}
		foreach (Man r : players)
		{
			PlayerBase rp = PlayerBase.Cast(r);
			if (!rp || !rp.GetIdentity()) continue;
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(lows.Count());
			for (int i = 0; i < lows.Count(); i++)
			{
				rpc.Write(lows[i]);
				rpc.Write(highs[i]);
			}
			rpc.Send(rp, AGA_RPC_INVIS_SYNC, false, rp.GetIdentity());
		}
	}

	// ---------- strzały: przekazanie do adminów online ----------
	void OnShot(PlayerBase shooter, vector from, vector dir, string ammo, string weapon)
	{
		vector to = from + dir * 900;
		vector contactPos, contactDir;
		int comp;
		set<Object> hit = new set<Object>;
		if (DayZPhysics.RaycastRV(from + dir * 0.5, to, contactPos, contactDir, comp, hit, null, shooter, false, false, ObjIntersectFire, 0.05))
			to = contactPos;

		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			PlayerBase adm = PlayerBase.Cast(m);
			if (!adm || !IsAdmin(adm.GetIdentity())) continue;
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(from + dir * 1.2);
			rpc.Write(to);
			rpc.Write(NameOf(shooter));
			rpc.Write(weapon);
			rpc.Send(adm, AGA_RPC_SHOT, false, adm.GetIdentity());
		}
	}

	// znacznik prawdziwego trafienia w gracza / bota (pozycja z systemu obrażeń)
	void OnHitMarker(vector worldPos, string victim)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			PlayerBase adm = PlayerBase.Cast(m);
			if (!adm || !IsAdmin(adm.GetIdentity())) continue;
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(worldPos);
			rpc.Write(worldPos);
			rpc.Write(victim);
			rpc.Write("TRAFIENIE");
			rpc.Send(adm, AGA_RPC_SHOT, false, adm.GetIdentity());
		}
	}

	// ---------- obsługa RPC ----------
	void OnAuth(PlayerBase sender, PlayerIdentity id)
	{
		bool ok = IsAdmin(id);
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(ok);
		rpc.Send(sender, AGA_RPC_AUTH_RESULT, true, id);
		Log("Prośba o panel: " + Who(sender) + " -> admin: " + ok.ToString());
	}

	void OnPlayers(PlayerBase sender, PlayerIdentity id)
	{
		if (!IsAdmin(id))
			return;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		array<PlayerBase> list = new array<PlayerBase>;
		foreach (Man c : players)
		{
			PlayerBase cp = PlayerBase.Cast(c);
			if (cp && cp.GetIdentity())
				list.Insert(cp);
		}
		array<string> ids = new array<string>;
		array<string> names = new array<string>;
		foreach (PlayerBase lp : list)
		{
			ids.Insert(lp.GetIdentity().GetPlainId());
			names.Insert(lp.GetIdentity().GetName());
		}
		for (int bi = 0; bi < m_Bots.Count(); bi++)
		{
			if (!m_Bots[bi]) continue;
			list.Insert(m_Bots[bi]);
			ids.Insert(BotId(bi));
			names.Insert(BotName(m_Bots[bi]));
		}
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(list.Count());
		for (int li = 0; li < list.Count(); li++)
		{
			PlayerBase pb = list[li];
			string sid = ids[li];
			int lo, hi;
			pb.GetNetworkID(lo, hi);
			string hands = "";
			EntityAI inHands = pb.GetHumanInventory().GetEntityInHands();
			if (inHands)
				hands = inHands.GetDisplayName();
			float water = 0;
			float energy = 0;
			if (pb.GetStatWater()) water = pb.GetStatWater().Get();
			if (pb.GetStatEnergy()) energy = pb.GetStatEnergy().Get();
			rpc.Write(names[li]);
			rpc.Write(sid);
			rpc.Write(pb.GetPosition());
			rpc.Write(pb.GetHealth("", ""));
			rpc.Write(pb.GetHealth("", "Blood"));
			rpc.Write(m_God.Contains(sid));
			rpc.Write(lo);
			rpc.Write(hi);
			rpc.Write(hands);
			rpc.Write(water);
			rpc.Write(energy);
			rpc.Write(m_Invis.Contains(sid));
			rpc.Write(pb.IsInVehicle());
		}
		rpc.Send(sender, AGA_RPC_PLAYERS_RES, true, id);
	}

	void OnLogs(PlayerBase sender, PlayerIdentity id)
	{
		if (!IsAdmin(id))
			return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(m_Recent.Count());
		foreach (string a : m_Recent)
			rpc.Write(a);
		rpc.Write(m_Combat.Count());
		foreach (string b : m_Combat)
			rpc.Write(b);
		rpc.Send(sender, AGA_RPC_LOGS_RES, true, id);
	}

	void OnAction(PlayerBase sender, PlayerIdentity id, ParamsReadContext ctx)
	{
		if (!IsAdmin(id))
		{
			Log("ODRZUCONO akcję od nie-admina: " + Who(sender));
			return;
		}
		int act;
		string target;
		string text;
		vector pos;
		int num;
		float val;
		if (!ctx.Read(act) || !ctx.Read(target) || !ctx.Read(text) || !ctx.Read(pos) || !ctx.Read(num) || !ctx.Read(val))
			return;

		PlayerBase t;
		if (target != "")
			t = FindPlayer(target);

		switch (act)
		{
			case AGA_ACT_TP_POS:
			{
				TeleportTo(sender, pos);
				Log(Who(sender) + " teleport na " + Math.Round(pos[0]).ToString() + " / " + Math.Round(pos[2]).ToString());
				break;
			}
			case AGA_ACT_TP_TO:
			{
				if (!t) { Notify(id, "Gracz offline"); return; }
				TeleportTo(sender, t.GetPosition() + Vector(1, 0, 1));
				Log(Who(sender) + " teleport do " + Who(t));
				break;
			}
			case AGA_ACT_BRING:
			{
				if (!t) { Notify(id, "Gracz offline"); return; }
				TeleportTo(t, sender.GetPosition() + Vector(1, 0, 1));
				Notify(id, "Przyciągnięto " + NameOf(t));
				Log(Who(sender) + " przyciągnął " + Who(t));
				break;
			}
			case AGA_ACT_HEAL:
			{
				if (!t) t = sender;
				t.AGA_FullHeal();
				Notify(id, "Uleczono " + NameOf(t));
				Log(Who(sender) + " uleczył " + Who(t));
				break;
			}
			case AGA_ACT_HEAL_ALL:
			{
				array<Man> all = new array<Man>;
				GetGame().GetPlayers(all);
				foreach (Man hm : all)
				{
					PlayerBase hp = PlayerBase.Cast(hm);
					if (hp) hp.AGA_FullHeal();
				}
				Notify(id, "Uleczono wszystkich graczy");
				Log(Who(sender) + " uleczył wszystkich");
				break;
			}
			case AGA_ACT_KILL:
			{
				if (!t) return;
				t.SetAllowDamage(true);
				if (t.GetIdentity()) m_God.Remove(t.GetIdentity().GetPlainId());
				t.SetHealth("", "", 0);
				Log(Who(sender) + " zabił " + Who(t));
				break;
			}
			case AGA_ACT_KICK:
			{
				if (!t || !t.GetIdentity()) return;
				Log(Who(sender) + " wyrzucił " + Who(t) + " | powód: " + text);
				Notify(t.GetIdentity(), "Zostałeś wyrzucony: " + text);
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DoKick, 1500, false, t.GetIdentity().GetPlainId());
				break;
			}
			case AGA_ACT_BAN:
			{
				if (target == "") return;
				if (m_Bans.Banned.Find(target) == -1)
				{
					m_Bans.Banned.Insert(target);
					JsonFileLoader<AGA_Bans>.JsonSaveFile(m_Dir + "/bans.json", m_Bans);
				}
				Log(Who(sender) + " zbanował " + target + " | powód: " + text);
				if (t)
					GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DoKick, 500, false, target);
				Notify(id, "Ban nadany: " + target);
				break;
			}
			case AGA_ACT_GOD:
			{
				if (!t) t = sender;
				string gid = target;
				if (t.GetIdentity()) gid = t.GetIdentity().GetPlainId();
				bool on = (num == 1);
				t.SetAllowDamage(!on);
				if (on)
				{
					m_God.Set(gid, true);
					t.AGA_FullHeal();
				}
				else
				{
					m_God.Remove(gid);
				}
				Notify(id, "God mode " + NameOf(t) + ": " + on.ToString());
				Log(Who(sender) + " god mode " + on.ToString() + " dla " + Who(t));
				break;
			}
			case AGA_ACT_INVIS:
			{
				if (!t) t = sender;
				if (!t.GetIdentity()) { Notify(id, "Boty nie obsługują niewidzialności"); return; }
				string iid = t.GetIdentity().GetPlainId();
				bool inv = (num == 1);
				if (inv)
					m_Invis.Set(iid, true);
				else
					m_Invis.Remove(iid);
				SyncInvis();
				Notify(id, "Niewidzialność " + NameOf(t) + ": " + inv.ToString());
				Log(Who(sender) + " niewidzialność " + inv.ToString() + " dla " + Who(t));
				break;
			}
			case AGA_ACT_FREEZE:
			{
				if (!t || !t.GetIdentity()) return;
				ScriptRPC fr = new ScriptRPC();
				fr.Write(num == 1);
				fr.Send(t, AGA_RPC_FREEZE, true, t.GetIdentity());
				Notify(id, "Zamrożenie " + NameOf(t) + ": " + (num == 1).ToString());
				Log(Who(sender) + " zamrożenie " + (num == 1).ToString() + " dla " + Who(t));
				break;
			}
			case AGA_ACT_STRIP:
			{
				if (!t) return;
				t.RemoveAllItems();
				Notify(id, "Wyczyszczono ekwipunek: " + NameOf(t));
				Log(Who(sender) + " wyczyścił ekwipunek " + Who(t));
				break;
			}
			case AGA_ACT_KIT:
			{
				if (!t) t = sender;
				GiveKit(t, text);
				Notify(id, "Zestaw " + text + " dla " + NameOf(t));
				Log(Who(sender) + " dał zestaw " + text + " dla " + Who(t));
				break;
			}
			case AGA_ACT_PM:
			{
				if (!t || !t.GetIdentity() || text == "") return;
				NotificationSystem.SendNotificationToPlayerIdentityExtended(t.GetIdentity(), 15, "WIADOMOŚĆ OD ADMINISTRACJI", text, "set:dayz_gui image:icon_info");
				Notify(id, "Wysłano wiadomość do " + NameOf(t));
				Log(Who(sender) + " wiadomość do " + Who(t) + ": " + text);
				break;
			}
			case AGA_ACT_SPAWN:
			{
				SpawnItem(sender, id, text, num, val, pos);
				break;
			}
			case AGA_ACT_CAR:
			{
				SpawnVehicle(sender, id, text, pos);
				break;
			}
			case AGA_ACT_REPAIR_CAR:
			{
				RepairNearestVehicle(sender, id);
				break;
			}
			case AGA_ACT_DELETE_OBJ:
			{
				array<string> parts = new array<string>;
				text.Split(",", parts);
				if (parts.Count() != 2) return;
				Object obj = GetGame().GetObjectByNetworkId(parts[0].ToInt(), parts[1].ToInt());
				if (!obj || obj.IsMan()) { Notify(id, "Nie można usunąć tego obiektu"); return; }
				Log(Who(sender) + " usunął obiekt " + obj.GetType() + " na " + obj.GetPosition().ToString());
				GetGame().ObjectDelete(obj);
				Notify(id, "Usunięto: " + obj.GetType());
				break;
			}
			case AGA_ACT_SPAWN_AI:
			{
				SpawnAI(sender, id, text, num, pos);
				break;
			}
			case AGA_ACT_BOT:
			{
				if (num == 1)
					SpawnBot(sender, id);
				else
					ClearBots(sender, id);
				break;
			}
			case AGA_ACT_CLEAR_AI:
			{
				ClearAI(sender, id, num);
				break;
			}
			case AGA_ACT_TIME:
			{
				int yy, mm, dd, hh, mn;
				GetGame().GetWorld().GetDate(yy, mm, dd, hh, mn);
				GetGame().GetWorld().SetDate(yy, mm, dd, num, 0);
				Notify(id, "Godzina: " + num.ToString() + ":00");
				Log(Who(sender) + " ustawił godzinę " + num.ToString());
				break;
			}
			case AGA_ACT_WEATHER:
			{
				SetWeather(num);
				Notify(id, "Pogoda zmieniona");
				Log(Who(sender) + " ustawił pogodę " + num.ToString());
				break;
			}
			case AGA_ACT_MESSAGE:
			{
				if (text == "") return;
				NotificationSystem.SendNotificationToPlayerIdentityExtended(null, 12, "ARMA-GEDON", text, "set:dayz_gui image:icon_info");
				Log(Who(sender) + " ogłoszenie: " + text);
				break;
			}
		}
	}

	void DoKick(string steamId)
	{
		PlayerBase p = FindPlayer(steamId);
		if (p && p.GetIdentity())
			GetGame().DisconnectPlayer(p.GetIdentity(), steamId);
	}

	void TeleportTo(PlayerBase p, vector pos)
	{
		if (!p)
			return;
		float ground = GetGame().SurfaceY(pos[0], pos[2]);
		if (pos[1] < ground)
			pos[1] = ground;
		p.SetPosition(pos);
	}

	bool IsValidClass(string cls)
	{
		return GetGame().ConfigIsExisting("CfgVehicles " + cls) || GetGame().ConfigIsExisting("CfgWeapons " + cls) || GetGame().ConfigIsExisting("CfgMagazines " + cls);
	}

	vector GroundAt(vector at)
	{
		at[1] = GetGame().SurfaceY(at[0], at[2]);
		return at;
	}

	void SpawnItem(PlayerBase p, PlayerIdentity id, string cls, int count, float mode, vector cursor)
	{
		if (cls == "" || !IsValidClass(cls))
		{
			Notify(id, "Nieznana klasa: " + cls);
			return;
		}
		if (count < 1) count = 1;
		if (count > 50) count = 50;
		int m = mode;
		bool full = false;
		if (m >= 10)
		{
			full = true;
			m = m - 10;
		}
		int made = 0;
		for (int i = 0; i < count; i++)
		{
			EntityAI e;
			if (m == AGA_SPAWN_INV)
				e = p.GetInventory().CreateInInventory(cls);
			if (!e)
			{
				vector at = p.GetPosition() + p.GetDirection() * 1.5;
				if (m == AGA_SPAWN_CURSOR && cursor != vector.Zero)
					at = GroundAt(cursor);
				e = EntityAI.Cast(GetGame().CreateObjectEx(cls, at, ECE_PLACE_ON_SURFACE));
			}
			if (e)
			{
				made++;
				if (full)
					e.OnDebugSpawn();
				ItemBase ib = ItemBase.Cast(e);
				if (ib && ib.HasQuantity())
					ib.SetQuantityMax();
			}
		}
		Notify(id, "Utworzono " + made.ToString() + "x " + cls);
		Log(Who(p) + " spawn " + made.ToString() + "x " + cls);
	}

	void GiveKit(PlayerBase p, string kit)
	{
		array<string> items = new array<string>;
		array<string> weapons = new array<string>;
		switch (kit)
		{
			case "SZTURM":
				items = {"PlateCarrierVest", "BallisticHelmet_Black", "TacticalGloves_Black", "GorkaEJacket_Flat", "GorkaPants_Flat", "MilitaryBoots_Black", "AssaultBag_Black", "Mag_STANAG_30Rnd", "Mag_STANAG_30Rnd", "Mag_STANAG_30Rnd", "Ammo_556x45", "BandageDressing", "BandageDressing"};
				weapons = {"M4A1"};
				break;
			case "SNAJPER":
				items = {"GhillieSuit_Woodland", "HuntingJacket_Spring", "HunterPants_Spring", "MilitaryBoots_Brown", "HuntingBag", "Mag_SVD_10Rnd", "Mag_SVD_10Rnd", "Ammo_762x54", "Ammo_762x54", "Rangefinder", "BandageDressing"};
				weapons = {"SVD"};
				break;
			case "MEDYK":
				items = {"MedicalScrubsShirt_Green", "MedicalScrubsPants_Green", "FirstAidKit", "BandageDressing", "BandageDressing", "BandageDressing", "Morphine", "Epinephrine", "SalineBagIV", "BloodBagIV", "BloodTestKit", "TetracyclineAntibiotics", "PainkillerTablets", "CharcoalTablets", "VitaminBottle", "DisinfectantSpray", "Splint"};
				break;
			case "BUDOWA":
				items = {"MountainBag_Green", "Hammer", "Hatchet", "Shovel", "Pliers", "Hacksaw", "NailBox", "NailBox", "NailBox", "MetalWire", "CombinationLock", "Flashlight", "Battery9V"};
				break;
			case "SURVIVAL":
				items = {"Canteen", "WaterBottle", "TacticalBaconCan", "BakedBeansCan", "SpaghettiCan", "CanOpener", "HuntingKnife", "Matchbox", "Compass", "ChernarusMap", "BandageDressing", "Heatpack", "Rope"};
				break;
			case "UBRANIE":
				items = {"M65Jacket_Olive", "CargoPants_Green", "MilitaryBoots_Bluerock", "WorkingGloves_Brown", "BeanieHat_Green", "AliceBag_Green", "MilitaryBelt", "SmershVest", "Balaclava3Holes_Green"};
				break;
		}
		foreach (string cls : items)
		{
			EntityAI e = p.GetInventory().CreateInInventory(cls);
			if (!e)
				e = EntityAI.Cast(GetGame().CreateObjectEx(cls, p.GetPosition(), ECE_PLACE_ON_SURFACE));
			ItemBase ib = ItemBase.Cast(e);
			if (ib && ib.HasQuantity())
				ib.SetQuantityMax();
		}
		foreach (string wcls : weapons)
		{
			EntityAI w = p.GetHumanInventory().CreateInHands(wcls);
			if (!w)
				w = p.GetInventory().CreateInInventory(wcls);
			if (!w)
				w = EntityAI.Cast(GetGame().CreateObjectEx(wcls, p.GetPosition(), ECE_PLACE_ON_SURFACE));
			if (w)
				w.OnDebugSpawn();
		}
	}

	void SpawnVehicle(PlayerBase p, PlayerIdentity id, string cls, vector pick)
	{
		if (cls == "" || !GetGame().ConfigIsExisting("CfgVehicles " + cls))
		{
			Notify(id, "Nieznany pojazd: " + cls);
			return;
		}
		vector at = p.GetPosition() + p.GetDirection() * 7;
		if (pick != vector.Zero)
			at = pick;
		at = GroundAt(at);
		if (GetGame().IsKindOf(cls, "BoatScript"))
		{
			float sea = g_Game.SurfaceGetSeaLevel();
			if (at[1] < sea)
				at[1] = sea;
		}
		at[1] = at[1] + 0.5;
		EntityAI veh = EntityAI.Cast(GetGame().CreateObjectEx(cls, at, ECE_PLACE_ON_SURFACE));
		if (!veh)
		{
			Notify(id, "Nie udało się stworzyć: " + cls);
			return;
		}
		veh.SetOrientation(p.GetOrientation());
		veh.OnDebugSpawn();
		FillVehicle(veh);
		Notify(id, "Pojazd gotowy: " + veh.GetDisplayName());
		Log(Who(p) + " spawn pojazdu " + cls);
	}

	void FillVehicle(EntityAI veh)
	{
		CarScript car = CarScript.Cast(veh);
		if (car)
		{
			car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
			car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
			car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
			car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
		}
		BoatScript boat = BoatScript.Cast(veh);
		if (boat)
			boat.Fill(BoatFluid.FUEL, boat.GetFluidCapacity(BoatFluid.FUEL));
		MotorbikeScript bike = MotorbikeScript.Cast(veh);
		if (bike)
		{
			bike.Fill(MotorbikeFluid.FUEL, bike.GetFluidCapacity(MotorbikeFluid.FUEL));
		}
	}

	void RepairNearestVehicle(PlayerBase p, PlayerIdentity id)
	{
		array<Object> objs = new array<Object>;
		array<CargoBase> cargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition(p.GetPosition(), 25, objs, cargos);
		EntityAI best;
		float bestDist = 1000;
		foreach (Object o : objs)
		{
			EntityAI e = EntityAI.Cast(o);
			if (!e) continue;
			if (!CarScript.Cast(e) && !BoatScript.Cast(e) && !MotorbikeScript.Cast(e)) continue;
			float dist = vector.Distance(p.GetPosition(), e.GetPosition());
			if (dist < bestDist)
			{
				bestDist = dist;
				best = e;
			}
		}
		if (!best)
		{
			Notify(id, "Brak pojazdu w promieniu 25 m");
			return;
		}
		best.SetHealthMax("", "");
		array<string> zones = new array<string>;
		best.GetDamageZones(zones);
		foreach (string z : zones)
			best.SetHealthMax(z, "");
		for (int i = 0; i < best.GetInventory().AttachmentCount(); i++)
		{
			EntityAI att = best.GetInventory().GetAttachmentFromIndex(i);
			if (att)
				att.SetHealthMax("", "");
		}
		FillVehicle(best);
		Notify(id, "Naprawiono i zatankowano: " + best.GetDisplayName());
		Log(Who(p) + " naprawił pojazd " + best.GetType());
	}

	void SpawnAI(PlayerBase p, PlayerIdentity id, string cls, int count, vector pick)
	{
		if (!GetGame().ConfigIsExisting("CfgVehicles " + cls))
		{
			Notify(id, "Nieznana klasa AI: " + cls);
			return;
		}
		if (count < 1) count = 1;
		if (count > 30) count = 30;
		vector center = p.GetPosition() + p.GetDirection() * 15;
		if (pick != vector.Zero)
			center = pick;
		for (int i = 0; i < count; i++)
		{
			vector at = center + Vector(Math.RandomFloat(-6, 6), 0, Math.RandomFloat(-6, 6));
			at = GroundAt(at);
			GetGame().CreateObjectEx(cls, at, ECE_PLACE_ON_SURFACE | ECE_INITAI | ECE_EQUIP_ATTACHMENTS);
		}
		Notify(id, "Stworzono AI: " + count.ToString() + "x " + cls);
		Log(Who(p) + " spawn AI " + count.ToString() + "x " + cls);
	}

	void ClearAI(PlayerBase p, PlayerIdentity id, int radius)
	{
		if (radius < 10) radius = 100;
		array<Object> objs = new array<Object>;
		array<CargoBase> cargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition(p.GetPosition(), radius, objs, cargos);
		int removed = 0;
		foreach (Object o : objs)
		{
			if (DayZInfected.Cast(o) || DayZAnimal.Cast(o))
			{
				GetGame().ObjectDelete(o);
				removed++;
			}
		}
		Notify(id, "Usunięto AI: " + removed.ToString());
		Log(Who(p) + " usunął AI w promieniu " + radius.ToString() + " m: " + removed.ToString());
	}

	void SetWeather(int preset)
	{
		Weather w = GetGame().GetWeather();
		w.SetWeatherUpdateFreeze(false);
		float hold = 3600;
		switch (preset)
		{
			case 0:
				w.GetOvercast().Set(0.05, 30, hold);
				w.GetRain().Set(0, 10, hold);
				w.GetFog().Set(0, 30, hold);
				break;
			case 1:
				w.GetOvercast().Set(0.6, 30, hold);
				w.GetRain().Set(0, 10, hold);
				w.GetFog().Set(0.05, 30, hold);
				break;
			case 2:
				w.GetOvercast().Set(0.85, 30, hold);
				w.GetRain().Set(0.7, 30, hold);
				w.GetFog().Set(0.1, 30, hold);
				break;
			case 3:
				w.GetOvercast().Set(1, 30, hold);
				w.GetRain().Set(1, 30, hold);
				w.GetFog().Set(0.15, 30, hold);
				w.SetStorm(1, 0.7, 20);
				break;
			case 4:
				w.GetOvercast().Set(0.5, 30, hold);
				w.GetRain().Set(0, 10, hold);
				w.GetFog().Set(0.6, 30, hold);
				break;
		}
	}
}
