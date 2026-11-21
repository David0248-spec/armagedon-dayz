// ARMA-GEDON Admin – okno panelu
class AGA_Menu extends UIScriptedMenu
{
	protected MapWidget m_Map;
	protected Widget m_Markers;
	protected TextWidget m_Status;
	protected TextWidget m_PickTxt;
	protected TextListboxWidget m_PlayerList;
	protected TextListboxWidget m_ItemList;
	protected EditBoxWidget m_Search;
	protected EditBoxWidget m_Qty;
	protected EditBoxWidget m_Reason;
	protected EditBoxWidget m_Msg;
	protected ref array<Widget> m_Pages = new array<Widget>;
	protected ref array<Widget> m_MarkerW = new array<Widget>;
	protected ref array<string> m_RowIds = new array<string>;
	protected ref array<string> m_ItemRows = new array<string>;
	protected string m_LastSearch = "-";
	protected string m_Selected;
	protected vector m_Pick;
	protected float m_Tick;
	protected bool m_Centered;
	protected ref AGA_MapHandler m_MapHandler;

	static ref array<string> s_AllItems;
	static ref array<string> s_AllNames;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_menu.layout");
		m_Map = MapWidget.Cast(layoutRoot.FindAnyWidget("Map"));
		m_Markers = layoutRoot.FindAnyWidget("Markers");
		m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));
		m_PickTxt = TextWidget.Cast(layoutRoot.FindAnyWidget("PickTxt"));
		m_PlayerList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("PlayerList"));
		m_ItemList = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("ItemList"));
		m_Search = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("SearchBox"));
		m_Qty = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("QtyBox"));
		m_Reason = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("ReasonBox"));
		m_Msg = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("MsgBox"));
		for (int i = 0; i < 4; i++)
			m_Pages.Insert(layoutRoot.FindAnyWidget("Page" + i.ToString()));
		if (m_Map)
		{
			m_MapHandler = new AGA_MapHandler(this);
			m_Map.SetHandler(m_MapHandler);
			PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
			if (p)
			{
				m_Map.SetScale(0.3);
				m_Map.SetMapPos(p.GetPosition());
			}
		}
		ShowPage(0);
		BuildItems();
		return layoutRoot;
	}

	override void OnShow()
	{
		super.OnShow();
		AGA_Client.Get().WantPlayers = true;
		AGA_Client.Get().RequestPlayers();
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
	}

	override void OnHide()
	{
		super.OnHide();
		AGA_Client.Get().WantPlayers = false;
		GetGame().GetMission().PlayerControlEnable(false);
	}

	void ShowPage(int idx)
	{
		for (int i = 0; i < m_Pages.Count(); i++)
		{
			if (m_Pages[i])
				m_Pages[i].Show(i == idx);
		}
	}

	// ---------- lista przedmiotów ----------
	void BuildItems()
	{
		if (s_AllItems) return;
		s_AllItems = new array<string>;
		s_AllNames = new array<string>;
		array<string> roots = {"CfgVehicles", "CfgWeapons", "CfgMagazines"};
		foreach (string root : roots)
		{
			int n = GetGame().ConfigGetChildrenCount(root);
			for (int i = 0; i < n; i++)
			{
				string cls;
				GetGame().ConfigGetChildName(root, i, cls);
				string path = root + " " + cls;
				if (GetGame().ConfigGetInt(path + " scope") != 2)
					continue;
				string dn;
				GetGame().ConfigGetText(path + " displayName", dn);
				s_AllItems.Insert(cls);
				s_AllNames.Insert(dn);
			}
		}
	}

	void FilterItems(string q)
	{
		m_ItemList.ClearItems();
		m_ItemRows.Clear();
		q.ToLower();
		for (int i = 0; i < s_AllItems.Count(); i++)
		{
			string cls = s_AllItems[i];
			string dn = s_AllNames[i];
			string hay = cls + " " + dn;
			hay.ToLower();
			if (q != "" && !hay.Contains(q))
				continue;
			m_ItemList.AddItem(dn + "   [" + cls + "]", null, 0);
			m_ItemRows.Insert(cls);
			if (m_ItemRows.Count() >= 300)
				break;
		}
	}

	// ---------- odświeżanie ----------
	override void Update(float timeslice)
	{
		super.Update(timeslice);
		AGA_Client c = AGA_Client.Get();

		string s = m_Search.GetText();
		if (s != m_LastSearch)
		{
			m_LastSearch = s;
			FilterItems(s);
		}

		m_Tick -= timeslice;
		if (m_Tick <= 0)
		{
			m_Tick = 1;
			RefreshPlayerList();
		}
		UpdateMarkers();

		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (me && m_Status)
		{
			vector mp = me.GetPosition();
			m_Status.SetText("Online: " + c.Players.Count().ToString() + "   |   Ja: " + Math.Round(mp[0]).ToString() + " / " + Math.Round(mp[2]).ToString() + "   |   Latanie: " + c.IsFlying().ToString());
		}
	}

	void RefreshPlayerList()
	{
		AGA_Client c = AGA_Client.Get();
		int sel = m_PlayerList.GetSelectedRow();
		m_PlayerList.ClearItems();
		m_RowIds.Clear();
		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		foreach (AGA_PlayerInfo info : c.Players)
		{
			string line = info.Name + "   HP " + Math.Round(info.Health).ToString() + "   krew " + Math.Round(info.Blood).ToString();
			if (me)
				line = line + "   " + Math.Round(vector.Distance(me.GetPosition(), info.Pos)).ToString() + "m";
			if (info.God)
				line = line + "   [GOD]";
			m_PlayerList.AddItem(line, null, 0);
			m_RowIds.Insert(info.SteamId);
		}
		if (sel >= 0 && sel < m_RowIds.Count())
			m_PlayerList.SelectRow(sel);
	}

	void UpdateMarkers()
	{
		if (!m_Map || !m_Markers) return;
		AGA_Client c = AGA_Client.Get();
		while (m_MarkerW.Count() < c.Players.Count())
			m_MarkerW.Insert(GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_marker.layout", m_Markers));
		float mx, my, mw, mh;
		m_Map.GetScreenPos(mx, my);
		m_Map.GetScreenSize(mw, mh);
		for (int i = 0; i < m_MarkerW.Count(); i++)
		{
			Widget w = m_MarkerW[i];
			if (i >= c.Players.Count()) { w.Show(false); continue; }
			AGA_PlayerInfo info = c.Players[i];
			vector sp = m_Map.MapToScreen(info.Pos);
			bool inside = sp[0] > mx && sp[0] < mx + mw - 200 && sp[1] > my && sp[1] < my + mh - 20;
			w.Show(inside);
			if (!inside) continue;
			w.SetPos(sp[0] - 5, sp[1] - 11);
			TextWidget t = TextWidget.Cast(w.FindAnyWidget("Text"));
			if (t) t.SetText(info.Name);
		}
	}

	string SelectedPlayer()
	{
		int r = m_PlayerList.GetSelectedRow();
		if (r < 0 || r >= m_RowIds.Count()) return "";
		return m_RowIds[r];
	}

	// ---------- zdarzenia ----------
	bool OnMapClick(Widget w, int x, int y, int button)
	{
		if (w == m_Map)
		{
			vector world = m_Map.ScreenToMap(Vector(x, y, 0));
			world[1] = 0;
			if (button == MouseState.RIGHT)
			{
				AGA_Client.Get().Action(AGA_ACT_TP_POS, "", "", world);
				return true;
			}
			if (button == MouseState.LEFT)
			{
				m_Pick = world;
				m_PickTxt.SetText("Wybrany punkt: " + Math.Round(world[0]).ToString() + " / " + Math.Round(world[2]).ToString());
				SelectNearestPlayer(x, y);
			}
		}
		return false;
	}

	void SelectNearestPlayer(int x, int y)
	{
		AGA_Client c = AGA_Client.Get();
		for (int i = 0; i < c.Players.Count(); i++)
		{
			vector sp = m_Map.MapToScreen(c.Players[i].Pos);
			if (Math.AbsFloat(sp[0] - x) < 12 && Math.AbsFloat(sp[1] - y) < 12)
			{
				m_PlayerList.SelectRow(i);
				ShowPage(0);
				return;
			}
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		string n = w.GetName();
		AGA_Client c = AGA_Client.Get();
		string sp = SelectedPlayer();

		if (n.IndexOf("Tab") == 0) { ShowPage(n.Substring(3, 1).ToInt()); return true; }

		switch (n)
		{
			case "BtnTpTo": c.Action(AGA_ACT_TP_TO, sp); return true;
			case "BtnBring": c.Action(AGA_ACT_BRING, sp); return true;
			case "BtnHeal": c.Action(AGA_ACT_HEAL, sp); return true;
			case "BtnKill": c.Action(AGA_ACT_KILL, sp); return true;
			case "BtnKick": c.Action(AGA_ACT_KICK, sp, m_Reason.GetText()); return true;
			case "BtnBan": c.Action(AGA_ACT_BAN, sp, m_Reason.GetText()); return true;
			case "BtnGodP":
			{
				int gp = 1;
				foreach (AGA_PlayerInfo pi : c.Players)
				{
					if (pi.SteamId == sp && pi.God) gp = 0;
				}
				c.Action(AGA_ACT_GOD, sp, "", "0 0 0", gp);
				return true;
			}
			case "BtnLocate":
			{
				foreach (AGA_PlayerInfo pl : c.Players)
				{
					if (pl.SteamId == sp) { m_Map.SetScale(0.1); m_Map.SetMapPos(pl.Pos); }
				}
				return true;
			}
			case "BtnSpawnInv": SpawnSelected(AGA_SPAWN_INV); return true;
			case "BtnSpawnGround": SpawnSelected(AGA_SPAWN_GROUND); return true;
			case "BtnSpawnPoint": SpawnSelected(AGA_SPAWN_CURSOR); return true;
			case "Car0": c.Action(AGA_ACT_CAR, "", "OffroadHatchback"); return true;
			case "Car1": c.Action(AGA_ACT_CAR, "", "CivilianSedan"); return true;
			case "Car2": c.Action(AGA_ACT_CAR, "", "Hatchback_02"); return true;
			case "Car3": c.Action(AGA_ACT_CAR, "", "Sedan_02"); return true;
			case "Car4": c.Action(AGA_ACT_CAR, "", "Offroad_02"); return true;
			case "Car5": c.Action(AGA_ACT_CAR, "", "Truck_01_Covered"); return true;
			case "Time0": c.Action(AGA_ACT_TIME, "", "", "0 0 0", 6); return true;
			case "Time1": c.Action(AGA_ACT_TIME, "", "", "0 0 0", 12); return true;
			case "Time2": c.Action(AGA_ACT_TIME, "", "", "0 0 0", 18); return true;
			case "Time3": c.Action(AGA_ACT_TIME, "", "", "0 0 0", 0); return true;
			case "W0": c.Action(AGA_ACT_WEATHER, "", "", "0 0 0", 0); return true;
			case "W1": c.Action(AGA_ACT_WEATHER, "", "", "0 0 0", 1); return true;
			case "W2": c.Action(AGA_ACT_WEATHER, "", "", "0 0 0", 2); return true;
			case "W3": c.Action(AGA_ACT_WEATHER, "", "", "0 0 0", 3); return true;
			case "W4": c.Action(AGA_ACT_WEATHER, "", "", "0 0 0", 4); return true;
			case "BtnSend": c.Action(AGA_ACT_MESSAGE, "", m_Msg.GetText()); m_Msg.SetText(""); return true;
			case "BtnFly": Close(); c.ToggleFly(); return true;
			case "BtnGod":
			{
				c.God = !c.God;
				int g = 0;
				if (c.God) g = 1;
				c.Action(AGA_ACT_GOD, "", "", "0 0 0", g);
				return true;
			}
			case "BtnHealMe": c.Action(AGA_ACT_HEAL); return true;
			case "BtnTpPoint": if (m_Pick != vector.Zero) c.Action(AGA_ACT_TP_POS, "", "", m_Pick); return true;
		}
		return false;
	}

	void SpawnSelected(int mode)
	{
		int r = m_ItemList.GetSelectedRow();
		if (r < 0 || r >= m_ItemRows.Count()) return;
		int qty = m_Qty.GetText().ToInt();
		if (qty < 1) qty = 1;
		vector at = m_Pick;
		AGA_Client.Get().Action(AGA_ACT_SPAWN, "", m_ItemRows[r], at, qty, mode);
	}
}

class AGA_MapHandler extends ScriptedWidgetEventHandler
{
	protected AGA_Menu m_Menu;
	void AGA_MapHandler(AGA_Menu menu) { m_Menu = menu; }
	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (m_Menu)
			return m_Menu.OnMapClick(w, x, y, button);
		return false;
	}
}
