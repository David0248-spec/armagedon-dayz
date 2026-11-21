// ARMA-GEDON Admin – stan klienta: latanie, ESP, niewidzialność, spectate, narzędzia
class AGA_Shot
{
	vector From;
	vector To;
	float Age;
	string Who;
}

class AGA_Client
{
	static ref AGA_Client s_Instance;
	protected bool m_IsAdmin;
	protected float m_AuthTimer;
	protected float m_ListTimer;
	protected float m_InvisTimer;
	ref array<ref AGA_PlayerInfo> Players = new array<ref AGA_PlayerInfo>;
	ref array<string> LogAdmin = new array<string>;
	ref array<string> LogCombat = new array<string>;
	bool WantPlayers;
	bool God;
	bool Invis;

	// ustawienia ESP
	bool EspOn;
	bool EspNames = true;
	bool EspBoxes = true;
	bool EspLines;
	bool EspShots = true;
	bool EspAi;
	float EspRange = 1500;

	// narzędzia
	bool DeleteMode;

	protected Camera m_Cam;
	protected string m_SpecId;
	protected bool m_SpaceDown;
	protected bool m_DelDown;
	protected bool m_PgUpDown;

	protected Widget m_EspRoot;
	protected CanvasWidget m_Canvas;
	protected ref array<Widget> m_Labels = new array<Widget>;
	protected ref array<ref AGA_Shot> m_Shots = new array<ref AGA_Shot>;
	protected ref array<Object> m_AiObjs = new array<Object>;
	protected float m_AiScan;
	protected bool m_EspFailed;

	protected ref array<int> m_InvisLow = new array<int>;
	protected ref array<int> m_InvisHigh = new array<int>;
	protected ref array<EntityAI> m_Hidden = new array<EntityAI>;

	static AGA_Client Get()
	{
		if (!s_Instance)
			s_Instance = new AGA_Client();
		return s_Instance;
	}

	bool IsAdmin() { return m_IsAdmin; }

	void SetAdmin(bool ok)
	{
		bool was = m_IsAdmin;
		m_IsAdmin = ok;
		CLog("odpowiedz autoryzacji dotarla do gry: admin=" + ok.ToString());
		if (ok && !was)
			NotificationSystem.AddNotificationExtended(6, "ARMA-GEDON ADMIN", "Panel aktywny: HOME / INSERT panel, END latanie", "set:dayz_gui image:icon_info");
	}

	// ---------------- komunikacja ----------------
	protected PlayerBase Me()
	{
		return PlayerBase.Cast(GetGame().GetPlayer());
	}

	void CLog(string msg)
	{
		Print("[ArmaGedonAdmin] " + msg);
		PlayerBase p = Me();
		if (!p) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(msg);
		rpc.Send(p, AGA_RPC_CLOG, true, null);
	}

	void RequestAuth()
	{
		PlayerBase p = Me();
		if (!p) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Send(p, AGA_RPC_AUTH, true, null);
	}

	void RequestPlayers()
	{
		PlayerBase p = Me();
		if (!p || !m_IsAdmin) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Send(p, AGA_RPC_PLAYERS, true, null);
	}

	void RequestLogs()
	{
		PlayerBase p = Me();
		if (!p || !m_IsAdmin) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Send(p, AGA_RPC_LOGS, true, null);
	}

	void Action(int act, string target = "", string text = "", vector pos = "0 0 0", int num = 0, float val = 0)
	{
		PlayerBase p = Me();
		if (!p || !m_IsAdmin) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(act);
		rpc.Write(target);
		rpc.Write(text);
		rpc.Write(pos);
		rpc.Write(num);
		rpc.Write(val);
		rpc.Send(p, AGA_RPC_ACTION, true, null);
	}

	void ReadPlayers(ParamsReadContext ctx)
	{
		int count;
		if (!ctx.Read(count)) return;
		Players.Clear();
		for (int i = 0; i < count; i++)
		{
			string nm, sid, hands;
			vector ps;
			float hp, bl, wat, en;
			bool gd, inv, veh;
			int lo, hi;
			ctx.Read(nm);
			ctx.Read(sid);
			ctx.Read(ps);
			ctx.Read(hp);
			ctx.Read(bl);
			ctx.Read(gd);
			ctx.Read(lo);
			ctx.Read(hi);
			ctx.Read(hands);
			ctx.Read(wat);
			ctx.Read(en);
			ctx.Read(inv);
			ctx.Read(veh);
			AGA_PlayerInfo info = new AGA_PlayerInfo();
			info.Name = nm;
			info.SteamId = sid;
			info.Pos = ps;
			info.Health = hp;
			info.Blood = bl;
			info.God = gd;
			info.NetLow = lo;
			info.NetHigh = hi;
			info.InHands = hands;
			info.Water = wat;
			info.Energy = en;
			info.Invis = inv;
			info.InVehicle = veh;
			Players.Insert(info);
		}
	}

	void ReadLogs(ParamsReadContext ctx)
	{
		int a, b;
		LogAdmin.Clear();
		LogCombat.Clear();
		if (!ctx.Read(a)) return;
		for (int i = 0; i < a; i++)
		{
			string la;
			ctx.Read(la);
			LogAdmin.Insert(la);
		}
		if (!ctx.Read(b)) return;
		for (int j = 0; j < b; j++)
		{
			string lb;
			ctx.Read(lb);
			LogCombat.Insert(lb);
		}
	}

	void ReadShot(ParamsReadContext ctx)
	{
		vector from, to;
		string who, weapon;
		if (!ctx.Read(from) || !ctx.Read(to) || !ctx.Read(who) || !ctx.Read(weapon)) return;
		AGA_Shot s = new AGA_Shot();
		s.From = from;
		s.To = to;
		s.Who = who + " | " + weapon;
		m_Shots.Insert(s);
		if (m_Shots.Count() > 80)
			m_Shots.Remove(0);
	}

	void ReadInvis(ParamsReadContext ctx)
	{
		int n;
		if (!ctx.Read(n)) return;
		m_InvisLow.Clear();
		m_InvisHigh.Clear();
		for (int i = 0; i < n; i++)
		{
			int lo, hi;
			ctx.Read(lo);
			ctx.Read(hi);
			m_InvisLow.Insert(lo);
			m_InvisHigh.Insert(hi);
		}
		ApplyInvis();
	}

	// ---------------- niewidzialność (wykonywane u każdego gracza) ----------------
	protected void HideRecursive(EntityAI e, bool hide)
	{
		if (!e) return;
		e.SetInvisible(hide);
		GameInventory inv = e.GetInventory();
		if (!inv) return;
		for (int i = 0; i < inv.AttachmentCount(); i++)
			HideRecursive(inv.GetAttachmentFromIndex(i), hide);
	}

	protected void HidePlayer(PlayerBase pb, bool hide)
	{
		HideRecursive(pb, hide);
		EntityAI hands = pb.GetHumanInventory().GetEntityInHands();
		if (hands)
			HideRecursive(hands, hide);
	}

	void ApplyInvis()
	{
		PlayerBase me = Me();
		array<EntityAI> now = new array<EntityAI>;
		for (int i = 0; i < m_InvisLow.Count(); i++)
		{
			PlayerBase pb = PlayerBase.Cast(GetGame().GetObjectByNetworkId(m_InvisLow[i], m_InvisHigh[i]));
			if (!pb || pb == me) continue;
			HidePlayer(pb, true);
			now.Insert(pb);
		}
		foreach (EntityAI old : m_Hidden)
		{
			PlayerBase op = PlayerBase.Cast(old);
			if (op && now.Find(old) == -1)
				HidePlayer(op, false);
		}
		m_Hidden.Clear();
		m_Hidden.Copy(now);
	}

	// ---------------- pętla ----------------
	void Update(float dt)
	{
		PlayerBase me = Me();
		if (!me) return;

		m_InvisTimer -= dt;
		if (m_InvisTimer <= 0)
		{
			m_InvisTimer = 0.5;
			ApplyInvis();
		}

		if (!m_IsAdmin)
		{
			m_AuthTimer -= dt;
			if (m_AuthTimer <= 0)
			{
				m_AuthTimer = 15;
				RequestAuth();
			}
			return;
		}

		m_ListTimer -= dt;
		if ((WantPlayers || m_Cam || EspOn) && m_ListTimer <= 0)
		{
			m_ListTimer = 1.0;
			RequestPlayers();
		}

		for (int s = m_Shots.Count() - 1; s >= 0; s--)
		{
			m_Shots[s].Age = m_Shots[s].Age + dt;
			if (m_Shots[s].Age > 12)
				m_Shots.Remove(s);
		}

		bool menuOpen = GetGame().GetUIManager().GetMenu() != null;

		if (m_Cam && !menuOpen)
		{
			if (m_SpecId != "")
				UpdateSpectate(dt);
			else
				UpdateFly(dt);
		}

		if (!menuOpen)
			UpdateHotkeys();

		DrawEsp(menuOpen, dt);
	}

	// ---------------- skróty: Delete (tryb usuwania), Page Up (TP tam gdzie patrzysz) ----------------
	protected Object RaycastLook(out vector hitPos)
	{
		vector from = GetGame().GetCurrentCameraPosition();
		vector dir = GetGame().GetCurrentCameraDirection();
		vector to = from + dir * 1500;
		vector contactDir;
		int comp;
		set<Object> results = new set<Object>;
		hitPos = vector.Zero;
		if (!DayZPhysics.RaycastRV(from, to, hitPos, contactDir, comp, results, null, Me(), true, false, ObjIntersectView, 0.1))
			return null;
		if (results.Count() > 0)
			return results.Get(0);
		return null;
	}

	protected void UpdateHotkeys()
	{
		bool del = KeyState(KeyCode.KC_DELETE) != 0;
		if (del && !m_DelDown && DeleteMode)
		{
			vector hp;
			Object o = RaycastLook(hp);
			if (o && !o.IsMan())
			{
				int lo, hi;
				o.GetNetworkID(lo, hi);
				if (lo == 0 && hi == 0)
					NotificationSystem.AddNotificationExtended(3, "ARMA-GEDON ADMIN", "To obiekt mapy, nie da sie go usunac: " + o.GetType());
				else
					Action(AGA_ACT_DELETE_OBJ, "", lo.ToString() + "," + hi.ToString());
			}
		}
		m_DelDown = del;

		bool pgup = KeyState(KeyCode.KC_PRIOR) != 0;
		if (pgup && !m_PgUpDown)
		{
			vector tp;
			RaycastLook(tp);
			if (tp != vector.Zero)
			{
				if (m_Cam) StopFly(false);
				Action(AGA_ACT_TP_POS, "", "", tp);
			}
		}
		m_PgUpDown = pgup;
	}

	// ---------------- LATANIE ----------------
	bool IsFlying() { return m_Cam != null; }
	bool IsSpectating() { return m_SpecId != ""; }

	void ToggleFly()
	{
		if (m_Cam) StopFly(false);
		else StartFly();
	}

	void StartFly()
	{
		PlayerBase p = Me();
		if (!p || m_Cam) return;
		vector pos = p.GetPosition() + Vector(0, 2, 0);
		m_Cam = Camera.Cast(GetGame().CreateObject("staticcamera", pos, true));
		if (!m_Cam) return;
		vector po = p.GetOrientation();
		po[1] = 0;
		po[2] = 0;
		m_Cam.SetOrientation(po);
		m_Cam.SetActive(true);
		p.GetInputController().SetDisabled(true);
	}

	void StopFly(bool teleport)
	{
		m_SpecId = "";
		if (!m_Cam) return;
		vector pos = m_Cam.GetPosition();
		m_Cam.SetActive(false);
		GetGame().ObjectDelete(m_Cam);
		m_Cam = null;
		PlayerBase p = Me();
		if (p) p.GetInputController().SetDisabled(false);
		if (teleport)
		{
			pos[1] = 0;
			Action(AGA_ACT_TP_POS, "", "", pos);
		}
	}

	void StartSpectate(string steamId)
	{
		if (!m_Cam) StartFly();
		if (!m_Cam) return;
		m_SpecId = steamId;
	}

	protected PlayerBase LocalPlayerOf(AGA_PlayerInfo info)
	{
		if (!info) return null;
		return PlayerBase.Cast(GetGame().GetObjectByNetworkId(info.NetLow, info.NetHigh));
	}

	protected AGA_PlayerInfo InfoOf(string steamId)
	{
		foreach (AGA_PlayerInfo i : Players)
		{
			if (i.SteamId == steamId) return i;
		}
		return null;
	}

	protected void UpdateSpectate(float dt)
	{
		PlayerBase t = LocalPlayerOf(InfoOf(m_SpecId));
		bool space = KeyState(KeyCode.KC_SPACE) != 0;
		if (space && !m_SpaceDown)
		{
			m_SpaceDown = true;
			StopFly(false);
			return;
		}
		m_SpaceDown = space;
		if (!t)
		{
			AGA_PlayerInfo far = InfoOf(m_SpecId);
			if (far)
			{
				m_Cam.SetPosition(far.Pos + Vector(0, 25, 0) - Vector(15, 0, 15));
				m_Cam.LookAt(far.Pos);
			}
			return;
		}
		vector head = t.GetBonePositionWS(t.GetBoneIndexByName("Head"));
		vector back = t.GetDirection();
		vector pos = head - back * 2.6 + Vector(0, 0.6, 0);
		m_Cam.SetPosition(pos);
		m_Cam.LookAt(head + back * 4);
	}

	protected void UpdateFly(float dt)
	{
		float fwd = GetUApi().GetInputByID(UAMoveForward).LocalValue() - GetUApi().GetInputByID(UAMoveBack).LocalValue();
		float side = GetUApi().GetInputByID(UAMoveRight).LocalValue() - GetUApi().GetInputByID(UAMoveLeft).LocalValue();
		float up = 0;
		if (KeyState(KeyCode.KC_E)) up = up + 1;
		if (KeyState(KeyCode.KC_Q)) up = up - 1;
		float speed = 12;
		if (KeyState(KeyCode.KC_LSHIFT)) speed = 60;
		if (KeyState(KeyCode.KC_LMENU)) speed = 250;
		if (KeyState(KeyCode.KC_LCONTROL)) speed = 3;

		vector dir = m_Cam.GetDirection();
		vector aside = vector.Up * dir;
		vector pos = m_Cam.GetPosition() + dir * fwd * speed * dt + aside * side * speed * dt + vector.Up * up * speed * dt;
		float minY = GetGame().SurfaceRoadY(pos[0], pos[2]) + 0.3;
		if (pos[1] < minY) pos[1] = minY;
		m_Cam.SetPosition(pos);

		float yaw = GetUApi().GetInputByID(UAAimLeft).LocalValue() - GetUApi().GetInputByID(UAAimRight).LocalValue();
		float pitch = GetUApi().GetInputByID(UAAimDown).LocalValue() - GetUApi().GetInputByID(UAAimUp).LocalValue();
		vector o = m_Cam.GetOrientation();
		o[0] = o[0] - Math.RAD2DEG * yaw * dt;
		o[1] = Math.Clamp(o[1] - Math.RAD2DEG * pitch * dt, -89, 89);
		m_Cam.SetOrientation(o);

		bool space = KeyState(KeyCode.KC_SPACE) != 0;
		if (space && !m_SpaceDown)
		{
			m_SpaceDown = true;
			StopFly(true);
			return;
		}
		m_SpaceDown = space;
	}

	// ---------------- ESP ----------------
	protected void EnsureEsp()
	{
		if (m_EspRoot || m_EspFailed) return;
		m_EspRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_esp.layout");
		if (m_EspRoot)
			m_Canvas = CanvasWidget.Cast(m_EspRoot.FindAnyWidget("Canvas"));
		if (!m_EspRoot || !m_Canvas)
		{
			m_EspFailed = true;
			CLog("BLAD: warstwa ESP nie wczytala sie (root=" + (m_EspRoot != null).ToString() + ", canvas=" + (m_Canvas != null).ToString() + ")");
			NotificationSystem.AddNotificationExtended(6, "ARMA-GEDON ADMIN", "ESP: blad warstwy rysowania");
		}
		else
		{
			CLog("ESP: warstwa rysowania gotowa");
		}
	}

	protected void ScanAi(vector cam, float dt)
	{
		m_AiScan -= dt;
		if (m_AiScan > 0) return;
		m_AiScan = 1.0;
		m_AiObjs.Clear();
		float r = EspRange;
		if (r > 400) r = 400;
		array<Object> objs = new array<Object>;
		array<CargoBase> cargos = new array<CargoBase>;
		GetGame().GetObjectsAtPosition(cam, r, objs, cargos);
		foreach (Object o : objs)
		{
			if (DayZInfected.Cast(o) || DayZAnimal.Cast(o))
			{
				EntityAI e = EntityAI.Cast(o);
				if (e && e.IsAlive())
					m_AiObjs.Insert(o);
			}
			if (m_AiObjs.Count() >= 150)
				break;
		}
	}

	protected void Box(float x0, float y0, float x1, float y1, int col)
	{
		m_Canvas.DrawLine(x0, y0, x1, y0, 1.5, col);
		m_Canvas.DrawLine(x1, y0, x1, y1, 1.5, col);
		m_Canvas.DrawLine(x1, y1, x0, y1, 1.5, col);
		m_Canvas.DrawLine(x0, y1, x0, y0, 1.5, col);
	}

	protected Widget Label(int idx)
	{
		while (m_Labels.Count() <= idx)
		{
			Widget w = GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_label.layout", m_EspRoot);
			if (!w) return null;
			m_Labels.Insert(w);
		}
		return m_Labels[idx];
	}

	protected void DrawEsp(bool menuOpen, float dt)
	{
		bool active = EspOn || m_Cam != null;
		if (!active || menuOpen)
		{
			if (m_EspRoot)
				m_EspRoot.Show(false);
			return;
		}
		EnsureEsp();
		if (!m_EspRoot || !m_Canvas) return;
		m_EspRoot.Show(true);
		m_Canvas.Clear();

		int sw, sh;
		GetScreenSize(sw, sh);
		vector cam = GetGame().GetCurrentCameraPosition();
		PlayerBase me = Me();
		string myId = "";
		if (me && me.GetIdentity()) myId = me.GetIdentity().GetPlainId();

		int used = 0;
		foreach (AGA_PlayerInfo info : Players)
		{
			if (info.SteamId == myId) continue;
			vector feet = info.Pos;
			vector head = info.Pos + Vector(0, 1.75, 0);
			PlayerBase lp = LocalPlayerOf(info);
			if (lp)
			{
				feet = lp.GetPosition();
				head = lp.GetBonePositionWS(lp.GetBoneIndexByName("Head")) + Vector(0, 0.15, 0);
			}
			float dist = vector.Distance(cam, feet);
			if (dist > EspRange) continue;
			vector sf = GetGame().GetScreenPos(feet);
			vector sh2 = GetGame().GetScreenPos(head);
			if (sf[2] <= 0 || sh2[2] <= 0) continue;

			int col = 0xFFFF3B3B;
			if (info.God) col = 0xFFFFC52E;
			if (info.Invis) col = 0xFFB46CFF;

			float h = sf[1] - sh2[1];
			if (h < 6) h = 6;
			float w = h * 0.45;
			if (EspBoxes)
				Box(sh2[0] - w * 0.5, sh2[1], sh2[0] + w * 0.5, sf[1], col);
			if (EspLines)
				m_Canvas.DrawLine(sw * 0.5, sh, sf[0], sf[1], 1, col);
			if (EspNames)
			{
				Widget lw = Label(used);
				if (lw)
				{
					lw.Show(true);
					lw.SetPos(sh2[0] - 110, sh2[1] - 30);
					TextWidget tw = TextWidget.Cast(lw.FindAnyWidget("Text"));
					if (tw)
					{
						string extra = "";
						if (info.InHands != "") extra = " | " + info.InHands;
						tw.SetText(info.Name + "  " + Math.Round(dist).ToString() + "m  HP " + Math.Round(info.Health).ToString() + extra);
					}
					used++;
				}
			}
		}
		for (int k = used; k < m_Labels.Count(); k++)
			m_Labels[k].Show(false);

		if (EspAi)
		{
			ScanAi(cam, dt);
			foreach (Object ao : m_AiObjs)
			{
				if (!ao) continue;
				vector af = ao.GetPosition();
				float ah = 1.7;
				int acol = 0xFF3DD16A;
				if (DayZAnimal.Cast(ao))
				{
					ah = 1.2;
					acol = 0xFF3DA5FF;
				}
				vector asf = GetGame().GetScreenPos(af);
				vector ash = GetGame().GetScreenPos(af + Vector(0, ah, 0));
				if (asf[2] <= 0 || ash[2] <= 0) continue;
				float abh = asf[1] - ash[1];
				if (abh < 4) abh = 4;
				float abw = abh * 0.5;
				if (DayZAnimal.Cast(ao)) abw = abh * 1.2;
				Box(ash[0] - abw * 0.5, ash[1], ash[0] + abw * 0.5, asf[1], acol);
			}
		}

		if (EspShots)
		{
			foreach (AGA_Shot s : m_Shots)
			{
				float fade = 1 - s.Age / 12.0;
				int a = Math.Clamp(fade * 255, 30, 255);
				int scol = ARGB(a, 255, 150, 40);
				vector prev = GetGame().GetScreenPos(s.From);
				for (int seg = 1; seg <= 24; seg++)
				{
					vector wp = s.From + (s.To - s.From) * (seg / 24.0);
					vector cur = GetGame().GetScreenPos(wp);
					if (prev[2] > 0 && cur[2] > 0)
						m_Canvas.DrawLine(prev[0], prev[1], cur[0], cur[1], 2, scol);
					prev = cur;
				}
				vector hitS = GetGame().GetScreenPos(s.To);
				if (hitS[2] > 0)
					Box(hitS[0] - 4, hitS[1] - 4, hitS[0] + 4, hitS[1] + 4, scol);
			}
		}
	}
}
