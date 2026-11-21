// ARMA-GEDON Admin – stan klienta i latanie
class AGA_Client
{
	static ref AGA_Client s_Instance;
	protected bool m_IsAdmin;
	protected float m_AuthTimer;
	protected float m_ListTimer;
	ref array<ref AGA_PlayerInfo> Players = new array<ref AGA_PlayerInfo>;
	bool WantPlayers;
	bool God;

	protected Camera m_Cam;
	protected bool m_SpaceDown;
	protected Widget m_EspRoot;
	protected ref array<Widget> m_Esp = new array<Widget>;

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
		if (ok && !was)
			NotificationSystem.AddNotificationExtended(6, "ARMA-GEDON ADMIN", "Panel aktywny: HOME otwiera panel, END wlacza latanie", "set:dayz_gui image:icon_info");
	}

	void RequestAuth()
	{
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (!p) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Send(p, AGA_RPC_AUTH, true, null);
	}

	void RequestPlayers()
	{
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (!p || !m_IsAdmin) return;
		ScriptRPC rpc = new ScriptRPC();
		rpc.Send(p, AGA_RPC_PLAYERS, true, null);
	}

	void Action(int act, string target = "", string text = "", vector pos = "0 0 0", int num = 0, float val = 0)
	{
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
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
			AGA_PlayerInfo info = new AGA_PlayerInfo();
			string nm, sid;
			vector ps;
			float hp, bl;
			bool gd;
			ctx.Read(nm);
			ctx.Read(sid);
			ctx.Read(ps);
			ctx.Read(hp);
			ctx.Read(bl);
			ctx.Read(gd);
			info.Name = nm;
			info.SteamId = sid;
			info.Pos = ps;
			info.Health = hp;
			info.Blood = bl;
			info.God = gd;
			Players.Insert(info);
		}
	}

	void Update(float dt)
	{
		if (!GetGame().GetPlayer()) return;
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
		if ((WantPlayers || m_Cam) && m_ListTimer <= 0)
		{
			m_ListTimer = 1.5;
			RequestPlayers();
		}
		if (m_Cam)
			UpdateFly(dt);
	}

	// ---------------- LATANIE ----------------
	bool IsFlying() { return m_Cam != null; }

	void ToggleFly()
	{
		if (m_Cam) StopFly(false);
		else StartFly();
	}

	void StartFly()
	{
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
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
		m_EspRoot = GetGame().GetWorkspace().CreateWidget(FrameWidgetTypeID, 0, 0, 1, 1, WidgetFlags.VISIBLE | WidgetFlags.IGNOREPOINTER, 0xffffffff, 0);
	}

	void StopFly(bool teleport)
	{
		if (!m_Cam) return;
		vector pos = m_Cam.GetPosition();
		m_Cam.SetActive(false);
		GetGame().ObjectDelete(m_Cam);
		m_Cam = null;
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (p) p.GetInputController().SetDisabled(false);
		if (teleport)
		{
			pos[1] = 0;
			Action(AGA_ACT_TP_POS, "", "", pos);
		}
		if (m_EspRoot) m_EspRoot.Unlink();
		m_EspRoot = null;
		m_Esp.Clear();
	}

	protected void UpdateFly(float dt)
	{
		if (GetGame().GetUIManager().GetMenu())
			return;
		float fwd = GetUApi().GetInputByID(UAMoveForward).LocalValue() - GetUApi().GetInputByID(UAMoveBack).LocalValue();
		float side = GetUApi().GetInputByID(UAMoveRight).LocalValue() - GetUApi().GetInputByID(UAMoveLeft).LocalValue();
		float up = 0;
		if (KeyState(KeyCode.KC_E)) up = up + 1;
		if (KeyState(KeyCode.KC_Q)) up = up - 1;
		float speed = 12;
		if (KeyState(KeyCode.KC_LSHIFT)) speed = 60;
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
		UpdateEsp(pos);
	}

	protected void UpdateEsp(vector camPos)
	{
		if (!m_EspRoot) return;
		while (m_Esp.Count() < Players.Count())
			m_Esp.Insert(GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_label.layout", m_EspRoot));
		for (int i = 0; i < m_Esp.Count(); i++)
		{
			Widget w = m_Esp[i];
			if (i >= Players.Count()) { w.Show(false); continue; }
			AGA_PlayerInfo info = Players[i];
			vector s = GetGame().GetScreenPos(info.Pos + Vector(0, 2, 0));
			if (s[2] <= 0) { w.Show(false); continue; }
			w.Show(true);
			w.SetPos(s[0] - 100, s[1] - 20);
			TextWidget t = TextWidget.Cast(w.FindAnyWidget("Text"));
			if (t)
				t.SetText(info.Name + " [" + Math.Round(vector.Distance(camPos, info.Pos)).ToString() + "m]");
		}
	}
}
