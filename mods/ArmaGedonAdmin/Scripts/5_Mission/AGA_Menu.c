// ARMA-GEDON Admin – okno panelu
class AGA_ItemDef
{
	string Cls;
	string Name;
	string Low;
	int Cat;
}

class AGA_ItemDb
{
	static ref array<ref AGA_ItemDef> s_Items;

	static array<ref AGA_ItemDef> Get()
	{
		if (!s_Items)
			Build();
		return s_Items;
	}

	static void Build()
	{
		s_Items = new array<ref AGA_ItemDef>;
		array<string> roots = {"CfgWeapons", "CfgMagazines", "CfgVehicles"};
		array<string> medical = {"bandage", "morphine", "epinephrine", "saline", "bloodbag", "bloodtest", "tetracycline", "painkiller", "charcoal", "vitamin", "splint", "disinfect", "iodine", "rag", "startkit", "firstaid", "pills", "defibrillator", "chelating", "cotton"};
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
				TStringArray parents = new TStringArray;
				GetGame().ConfigGetFullPath(path, parents);
				int cat = 7;
				if (parents.Find("Weapon_Base") != -1)
					cat = 1;
				else if (parents.Find("Magazine_Base") != -1)
					cat = 2;
				else if (parents.Find("Clothing_Base") != -1 || parents.Find("Clothing") != -1)
				{
					string slot;
					GetGame().ConfigGetText(path + " inventorySlot", slot);
					cat = 3;
					if (slot == "Back")
						cat = 4;
					else
					{
						TStringArray slots = new TStringArray;
						GetGame().ConfigGetTextArray(path + " inventorySlot", slots);
						if (slots.Find("Back") != -1)
							cat = 4;
					}
				}
				else if (parents.Find("Edible_Base") != -1)
					cat = 5;
				else if (parents.Find("Inventory_Base") == -1 && parents.Find("ItemBase") == -1)
					continue; // budynki, AI, efekty itp.

				string low = cls;
				low.ToLower();
				if (cat == 7)
				{
					foreach (string m : medical)
					{
						if (low.Contains(m))
						{
							cat = 6;
							break;
						}
					}
				}
				string dn;
				GetGame().ConfigGetText(path + " displayName", dn);
				if (dn == "")
					dn = cls;
				AGA_ItemDef d = new AGA_ItemDef();
				d.Cls = cls;
				d.Name = dn;
				string dl = dn;
				dl.ToLower();
				d.Low = low + " " + dl;
				d.Cat = cat;
				s_Items.Insert(d);
			}
		}
	}
}

class AGA_Menu extends UIScriptedMenu
{
	static int s_Page;

	protected MapWidget m_Map;
	protected Widget m_Markers;
	protected EditBoxWidget m_Search;
	protected EditBoxWidget m_Qty;
	protected EditBoxWidget m_Reason;
	protected EditBoxWidget m_Msg;
	protected EditBoxWidget m_CoordX;
	protected EditBoxWidget m_CoordZ;

	protected ref map<string, int> m_BaseColor = new map<string, int>;
	protected ref array<Widget> m_MarkerW = new array<Widget>;

	protected string m_SelId;
	protected int m_MPage;
	protected int m_PPage;
	protected vector m_Pick;

	protected int m_Cat;
	protected string m_LastSearch = "-";
	protected int m_IPage;
	protected ref array<AGA_ItemDef> m_Filtered = new array<AGA_ItemDef>;
	protected ref array<EntityAI> m_TileEnt = new array<EntityAI>;
	protected ref array<string> m_TileCls = new array<string>;
	protected string m_SelItem;
	protected EntityAI m_DetailEnt;
	protected float m_Rot;

	protected int m_SelCar = -1;
	protected ref array<EntityAI> m_CarEnt = new array<EntityAI>;
	protected EntityAI m_CarDetailEnt;
	protected ref array<string> m_CarCls;
	protected ref array<string> m_CarName;

	void AGA_Menu()
	{
		m_CarCls = {"OffroadHatchback", "CivilianSedan", "Hatchback_02", "Sedan_02", "Offroad_02", "Truck_01_Covered"};
		m_CarName = {"ADA 4X4", "OLGA 24", "GUNTER 2", "SARKA 120", "M1025 HUMVEE", "V3S CARGO"};
	}

	protected float m_Tick;

	const int COL_HOVER = 0xFF2A2F38;
	const int COL_RED = 0xFFDB2626;

	// ------------------------------------------------------------
	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_menu.layout");
		if (!layoutRoot)
		{
			NotificationSystem.AddNotificationExtended(8, "ARMA-GEDON ADMIN", "Blad wczytania layoutu panelu");
			return null;
		}
		m_Map = MapWidget.Cast(layoutRoot.FindAnyWidget("Map"));
		m_Markers = layoutRoot.FindAnyWidget("Markers");
		m_Search = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("ISearch"));
		m_Qty = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("IQty"));
		m_Reason = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("CReason"));
		m_Msg = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("MsgBox"));
		m_CoordX = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("CoordX"));
		m_CoordZ = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("CoordZ"));
		if (m_Qty)
			m_Qty.SetText("1");

		CollectColors(layoutRoot.GetChildren());

		for (int i = 0; i < 18; i++)
		{
			m_TileEnt.Insert(null);
			m_TileCls.Insert("");
		}

		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (m_Map && me)
		{
			m_Map.SetScale(0.25);
			m_Map.SetMapPos(me.GetPosition());
		}
		if (me && me.GetIdentity())
			SetTxt("Who", "ADMIN: " + me.GetIdentity().GetName());

		BuildCars();
		ShowPage(s_Page);
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
		ClearPreviews();
	}

	void ~AGA_Menu()
	{
		ClearPreviews();
	}

	// ------------------------------------------------------------ pomocnicze
	Widget F(string n)
	{
		if (!layoutRoot) return null;
		return layoutRoot.FindAnyWidget(n);
	}

	void SetTxt(string n, string t)
	{
		TextWidget tw = TextWidget.Cast(F(n));
		if (tw) tw.SetText(t);
	}

	void ShowW(string n, bool s)
	{
		Widget w = F(n);
		if (w) w.Show(s);
	}

	void SetBg(string n, int col)
	{
		Widget w = F(n + "_bg");
		if (w) w.SetColor(col);
	}

	void ResetBg(string n)
	{
		if (m_BaseColor.Contains(n))
			SetBg(n, m_BaseColor.Get(n));
	}

	void CollectColors(Widget w)
	{
		while (w)
		{
			if (ButtonWidget.Cast(w))
			{
				Widget bg = w.FindAnyWidget(w.GetName() + "_bg");
				if (bg)
					m_BaseColor.Set(w.GetName(), bg.GetColor());
			}
			if (w.GetChildren())
				CollectColors(w.GetChildren());
			w = w.GetSibling();
		}
	}

	EntityAI MakeLocal(string cls)
	{
		EntityAI e = EntityAI.Cast(GetGame().CreateObjectEx(cls, "0 0 0", ECE_LOCAL | ECE_CREATEPHYSICS | ECE_TRACE));
		if (e)
		{
			dBodyDestroy(e);
			e.DisableSimulation(true);
			e.SetAllowDamage(false);
		}
		return e;
	}

	void KillEnt(EntityAI e)
	{
		if (e)
			e.Delete();
	}

	void SetPreview(string widgetName, EntityAI e, vector orient)
	{
		ItemPreviewWidget pv = ItemPreviewWidget.Cast(F(widgetName));
		if (!pv) return;
		pv.SetItem(e);
		if (e)
		{
			pv.SetView(e.GetViewIndex());
			pv.SetModelOrientation(orient);
		}
	}

	void ClearPreviews()
	{
		for (int i = 0; i < m_TileEnt.Count(); i++)
		{
			SetPreview("ITile" + i.ToString() + "_pv", null, vector.Zero);
			KillEnt(m_TileEnt[i]);
			m_TileEnt[i] = null;
			m_TileCls[i] = "";
		}
		for (int j = 0; j < m_CarEnt.Count(); j++)
		{
			SetPreview("VTile" + j.ToString() + "_pv", null, vector.Zero);
			KillEnt(m_CarEnt[j]);
		}
		m_CarEnt.Clear();
		SetPreview("IDetail_pv", null, vector.Zero);
		SetPreview("VDetail_pv", null, vector.Zero);
		KillEnt(m_DetailEnt);
		KillEnt(m_CarDetailEnt);
		m_DetailEnt = null;
		m_CarDetailEnt = null;
	}

	string Short(string s, int len)
	{
		if (s.Length() <= len) return s;
		return s.Substring(0, len - 2) + "..";
	}

	AGA_PlayerInfo FindInfo(string id)
	{
		foreach (AGA_PlayerInfo pi : AGA_Client.Get().Players)
		{
			if (pi.SteamId == id) return pi;
		}
		return null;
	}

	// ------------------------------------------------------------ strony
	void ShowPage(int idx)
	{
		s_Page = idx;
		for (int i = 0; i < 6; i++)
		{
			ShowW("Page" + i.ToString(), i == idx);
			ShowW("Nav" + i.ToString() + "_bar", i == idx);
			if (i == idx)
				SetBg("Nav" + i.ToString(), 0xFF171A20);
			else
				ResetBg("Nav" + i.ToString());
		}
		if (m_Markers)
			m_Markers.Show(idx == 0);
		if (idx == 2)
		{
			m_LastSearch = "-";
			RefreshItems();
		}
		if (idx == 3)
			RefreshCars();
	}

	// ------------------------------------------------------------ gracze
	void RefreshPlayers()
	{
		array<ref AGA_PlayerInfo> list = AGA_Client.Get().Players;
		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());

		int mPages = (list.Count() + 13) / 14;
		if (mPages < 1) mPages = 1;
		if (m_MPage >= mPages) m_MPage = mPages - 1;
		SetTxt("MPage", (m_MPage + 1).ToString() + " / " + mPages.ToString());
		for (int i = 0; i < 14; i++)
		{
			int idx = m_MPage * 14 + i;
			string rn = "MRow" + i.ToString();
			if (idx >= list.Count())
			{
				ShowW(rn, false);
				continue;
			}
			AGA_PlayerInfo info = list[idx];
			ShowW(rn, true);
			SetTxt(rn + "_txt", Short(info.Name, 18));
			string dist = "";
			if (me)
				dist = Math.Round(vector.Distance(me.GetPosition(), info.Pos)).ToString() + " m";
			SetTxt(rn + "_r", dist);
			ShowW(rn + "_bar", info.SteamId == m_SelId);
		}

		int pPages = (list.Count() + 14) / 15;
		if (pPages < 1) pPages = 1;
		if (m_PPage >= pPages) m_PPage = pPages - 1;
		SetTxt("PPage", (m_PPage + 1).ToString() + " / " + pPages.ToString());
		for (int k = 0; k < 15; k++)
		{
			int pidx = m_PPage * 15 + k;
			string pn = "PRow" + k.ToString();
			if (pidx >= list.Count())
			{
				ShowW(pn, false);
				continue;
			}
			AGA_PlayerInfo pinfo = list[pidx];
			ShowW(pn, true);
			SetTxt(pn + "_txt", Short(pinfo.Name, 22));
			SetTxt(pn + "_r", "HP " + Math.Round(pinfo.Health).ToString());
			ShowW(pn + "_bar", pinfo.SteamId == m_SelId);
		}
		RefreshCard();
	}

	void RefreshCard()
	{
		AGA_PlayerInfo info = FindInfo(m_SelId);
		if (!info)
		{
			SetTxt("CName", "Wybierz gracza z listy");
			SetTxt("CSteam", "");
			SetTxt("CHpV", "");
			SetTxt("CBlV", "");
			SetTxt("CPos", "");
			SetTxt("CGod", "");
			return;
		}
		SetTxt("CName", info.Name);
		SetTxt("CSteam", "STEAM ID: " + info.SteamId);
		float hp = Math.Clamp(info.Health / 100.0, 0, 1);
		float bl = Math.Clamp(info.Blood / 5000.0, 0, 1);
		SetTxt("CHpV", Math.Round(info.Health).ToString() + " / 100");
		SetTxt("CBlV", Math.Round(info.Blood).ToString() + " / 5000");
		Widget hpw = F("CHp");
		if (hpw) hpw.SetSize(hp, 1);
		Widget blw = F("CBl");
		if (blw) blw.SetSize(bl, 1);
		SetTxt("CPos", "POZYCJA: " + Math.Round(info.Pos[0]).ToString() + " / " + Math.Round(info.Pos[2]).ToString());
		if (info.God)
			SetTxt("CGod", "GOD MODE AKTYWNY");
		else
			SetTxt("CGod", "");
	}

	void UpdateMarkers()
	{
		if (!m_Map || !m_Markers || s_Page != 0) return;
		array<ref AGA_PlayerInfo> list = AGA_Client.Get().Players;
		while (m_MarkerW.Count() < list.Count())
		{
			Widget nw = GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_marker.layout", m_Markers);
			if (!nw) return;
			m_MarkerW.Insert(nw);
		}
		float mx, my, mw, mh;
		m_Map.GetScreenPos(mx, my);
		m_Map.GetScreenSize(mw, mh);
		string myId = "";
		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (me && me.GetIdentity())
			myId = me.GetIdentity().GetPlainId();
		for (int i = 0; i < m_MarkerW.Count(); i++)
		{
			Widget w = m_MarkerW[i];
			if (i >= list.Count())
			{
				w.Show(false);
				continue;
			}
			AGA_PlayerInfo info = list[i];
			vector sp = m_Map.MapToScreen(info.Pos);
			bool inside = sp[0] > mx && sp[0] < mx + mw - 10 && sp[1] > my && sp[1] < my + mh - 10;
			w.Show(inside);
			if (!inside) continue;
			w.SetPos(sp[0] - 6, sp[1] - 13);
			TextWidget t = TextWidget.Cast(w.FindAnyWidget("Text"));
			if (t) t.SetText(Short(info.Name, 20));
			Widget dot = w.FindAnyWidget("Dot");
			if (dot)
			{
				if (info.SteamId == m_SelId)
					dot.SetColor(0xFFFFC52E);
				else if (info.SteamId == myId)
					dot.SetColor(0xFF3DD16A);
				else
					dot.SetColor(COL_RED);
			}
		}
	}

	// ------------------------------------------------------------ przedmioty
	void RefreshItems()
	{
		if (!m_Search) return;
		string q = m_Search.GetText();
		q.ToLower();
		m_Filtered.Clear();
		array<ref AGA_ItemDef> all = AGA_ItemDb.Get();
		foreach (AGA_ItemDef d : all)
		{
			if (m_Cat != 0 && d.Cat != m_Cat) continue;
			if (q != "" && !d.Low.Contains(q)) continue;
			m_Filtered.Insert(d);
		}
		for (int c = 0; c < 8; c++)
		{
			ShowW("ICat" + c.ToString() + "_bar", false);
			if (c == m_Cat)
				SetBg("ICat" + c.ToString(), COL_RED);
			else
				ResetBg("ICat" + c.ToString());
		}
		SetTxt("ICount", "WYNIKI: " + m_Filtered.Count().ToString());
		FillTiles();
	}

	void FillTiles()
	{
		int pages = (m_Filtered.Count() + 17) / 18;
		if (pages < 1) pages = 1;
		if (m_IPage >= pages) m_IPage = pages - 1;
		if (m_IPage < 0) m_IPage = 0;
		SetTxt("IPage", (m_IPage + 1).ToString() + " / " + pages.ToString());
		for (int i = 0; i < 18; i++)
		{
			int idx = m_IPage * 18 + i;
			string tn = "ITile" + i.ToString();
			string cls = "";
			if (idx < m_Filtered.Count())
				cls = m_Filtered[idx].Cls;
			ShowW(tn, cls != "");
			if (cls == "")
			{
				SetPreview(tn + "_pv", null, vector.Zero);
				KillEnt(m_TileEnt[i]);
				m_TileEnt[i] = null;
				m_TileCls[i] = "";
				continue;
			}
			SetTxt(tn + "_nm", Short(m_Filtered[idx].Name, 24));
			ShowW(tn + "_bar", cls == m_SelItem);
			if (m_TileCls[i] != cls)
			{
				SetPreview(tn + "_pv", null, vector.Zero);
				KillEnt(m_TileEnt[i]);
				m_TileEnt[i] = MakeLocal(cls);
				m_TileCls[i] = cls;
				SetPreview(tn + "_pv", m_TileEnt[i], Vector(-10, 35, 0));
			}
		}
	}

	void SelectItem(string cls)
	{
		m_SelItem = cls;
		SetPreview("IDetail_pv", null, vector.Zero);
		KillEnt(m_DetailEnt);
		m_DetailEnt = MakeLocal(cls);
		SetPreview("IDetail_pv", m_DetailEnt, Vector(-12, m_Rot, 0));
		string dn = cls;
		foreach (AGA_ItemDef d : m_Filtered)
		{
			if (d.Cls == cls)
				dn = d.Name;
		}
		SetTxt("IDName", Short(dn, 30));
		SetTxt("IDClass", cls);
		for (int i = 0; i < 18; i++)
			ShowW("ITile" + i.ToString() + "_bar", m_TileCls[i] == cls);
	}

	int Qty()
	{
		if (!m_Qty) return 1;
		int q = m_Qty.GetText().ToInt();
		if (q < 1) q = 1;
		if (q > 50) q = 50;
		return q;
	}

	void SpawnSelected(int mode)
	{
		if (m_SelItem == "") return;
		AGA_Client.Get().Action(AGA_ACT_SPAWN, "", m_SelItem, m_Pick, Qty(), mode);
	}

	// ------------------------------------------------------------ pojazdy
	void BuildCars()
	{
		for (int i = 0; i < 6; i++)
			SetTxt("VTile" + i.ToString() + "_nm", m_CarName[i]);
	}

	void DressCar(EntityAI car, int idx)
	{
		if (!car) return;
		array<string> wheels = {"HatchbackWheel", "CivSedanWheel", "Hatchback_02_Wheel", "Sedan_02_Wheel", "Offroad_02_Wheel", "Truck_01_Wheel"};
		for (int i = 0; i < 4; i++)
			car.GetInventory().CreateInInventory(wheels[idx]);
		if (idx == 5)
		{
			for (int k = 0; k < 4; k++)
				car.GetInventory().CreateInInventory("Truck_01_WheelDouble");
		}
	}

	void RefreshCars()
	{
		if (m_CarEnt.Count() == 0)
		{
			for (int i = 0; i < 6; i++)
			{
				EntityAI e = MakeLocal(m_CarCls[i]);
				DressCar(e, i);
				m_CarEnt.Insert(e);
				SetPreview("VTile" + i.ToString() + "_pv", e, Vector(-8, 40, 0));
			}
		}
		for (int j = 0; j < 6; j++)
			ShowW("VTile" + j.ToString() + "_bar", j == m_SelCar);
	}

	void SelectCar(int idx)
	{
		m_SelCar = idx;
		SetPreview("VDetail_pv", null, vector.Zero);
		KillEnt(m_CarDetailEnt);
		m_CarDetailEnt = MakeLocal(m_CarCls[idx]);
		DressCar(m_CarDetailEnt, idx);
		SetPreview("VDetail_pv", m_CarDetailEnt, Vector(-8, m_Rot, 0));
		SetTxt("VDName", m_CarName[idx]);
		RefreshCars();
	}

	// ------------------------------------------------------------ odświeżanie
	override void Update(float timeslice)
	{
		super.Update(timeslice);
		AGA_Client c = AGA_Client.Get();

		if (s_Page == 2 && m_Search)
		{
			string s = m_Search.GetText();
			if (s != m_LastSearch)
			{
				m_LastSearch = s;
				m_IPage = 0;
				RefreshItems();
			}
		}

		m_Rot = m_Rot + timeslice * 25;
		if (m_Rot > 360) m_Rot = m_Rot - 360;
		ItemPreviewWidget dp = ItemPreviewWidget.Cast(F("IDetail_pv"));
		if (dp && m_DetailEnt && s_Page == 2)
			dp.SetModelOrientation(Vector(-12, m_Rot, 0));
		ItemPreviewWidget vp = ItemPreviewWidget.Cast(F("VDetail_pv"));
		if (vp && m_CarDetailEnt && s_Page == 3)
			vp.SetModelOrientation(Vector(-8, m_Rot, 0));

		m_Tick -= timeslice;
		if (m_Tick <= 0)
		{
			m_Tick = 0.5;
			RefreshPlayers();
			RefreshStats();
		}
		UpdateMarkers();
	}

	void RefreshStats()
	{
		AGA_Client c = AGA_Client.Get();
		int y, mo, d, h, mi;
		GetGame().GetWorld().GetDate(y, mo, d, h, mi);
		string fly = "NIE";
		if (c.IsFlying()) fly = "TAK";
		string god = "NIE";
		if (c.God) god = "TAK";
		SetTxt("Stats", "ONLINE: " + c.Players.Count().ToString() + "      CZAS: " + h.ToStringLen(2) + ":" + mi.ToStringLen(2) + "      GOD: " + god);
		SetTxt("SFly_st", "klawisz END");
		SetTxt("SGod_st", god);
		if (m_Pick != vector.Zero)
			SetTxt("MPick", "PUNKT: " + Math.Round(m_Pick[0]).ToString() + " / " + Math.Round(m_Pick[2]).ToString());
	}

	// ------------------------------------------------------------ zdarzenia
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (ButtonWidget.Cast(w) && m_BaseColor.Contains(w.GetName()))
		{
			Widget bar = F(w.GetName() + "_bar");
			if (!bar || !bar.IsVisible())
				SetBg(w.GetName(), COL_HOVER);
		}
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (ButtonWidget.Cast(w) && m_BaseColor.Contains(w.GetName()))
		{
			string n = w.GetName();
			if (n.IndexOf("Nav") == 0 && n.Substring(3, 1).ToInt() == s_Page)
				SetBg(n, 0xFF171A20);
			else if (n.IndexOf("ICat") == 0 && n.Substring(4, 1).ToInt() == m_Cat)
				SetBg(n, COL_RED);
			else
				ResetBg(n);
		}
		return false;
	}

	override bool OnMouseWheel(Widget w, int x, int y, int wheel)
	{
		if (!w) return false;
		string n = w.GetName();
		int dir = 1;
		if (wheel > 0) dir = -1;
		if (n.IndexOf("ITile") == 0 && s_Page == 2)
		{
			m_IPage = m_IPage + dir;
			FillTiles();
			return true;
		}
		if (n.IndexOf("PRow") == 0)
		{
			m_PPage = m_PPage + dir;
			if (m_PPage < 0) m_PPage = 0;
			RefreshPlayers();
			return true;
		}
		return false;
	}

	override bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		if (m_Map && w == m_Map)
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
				PickPlayerAt(x, y);
			}
		}
		return false;
	}

	void PickPlayerAt(int x, int y)
	{
		foreach (AGA_PlayerInfo info : AGA_Client.Get().Players)
		{
			vector sp = m_Map.MapToScreen(info.Pos);
			if (Math.AbsFloat(sp[0] - x) < 14 && Math.AbsFloat(sp[1] - y) < 14)
			{
				m_SelId = info.SteamId;
				RefreshPlayers();
				return;
			}
		}
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!w) return false;
		string n = w.GetName();
		AGA_Client c = AGA_Client.Get();

		if (n.IndexOf("Nav") == 0) { ShowPage(n.Substring(3, 1).ToInt()); return true; }
		if (n.IndexOf("MRow") == 0)
		{
			int mi = m_MPage * 14 + n.Substring(4, n.Length() - 4).ToInt();
			if (mi < c.Players.Count())
			{
				m_SelId = c.Players[mi].SteamId;
				m_Map.SetMapPos(c.Players[mi].Pos);
			}
			RefreshPlayers();
			return true;
		}
		if (n.IndexOf("PRow") == 0)
		{
			int pi = m_PPage * 15 + n.Substring(4, n.Length() - 4).ToInt();
			if (pi < c.Players.Count())
				m_SelId = c.Players[pi].SteamId;
			RefreshPlayers();
			return true;
		}
		if (n.IndexOf("ICat") == 0)
		{
			m_Cat = n.Substring(4, 1).ToInt();
			m_IPage = 0;
			RefreshItems();
			return true;
		}
		if (n.IndexOf("ITile") == 0)
		{
			int ti = n.Substring(5, n.Length() - 5).ToInt();
			if (ti >= 0 && ti < 18 && m_TileCls[ti] != "")
				SelectItem(m_TileCls[ti]);
			return true;
		}
		if (n.IndexOf("VTile") == 0)
		{
			SelectCar(n.Substring(5, 1).ToInt());
			return true;
		}
		if (n.IndexOf("THour") == 0)
		{
			array<int> hours = {0, 3, 6, 9, 12, 15, 18, 21};
			c.Action(AGA_ACT_TIME, "", "", "0 0 0", hours[n.Substring(5, 1).ToInt()]);
			return true;
		}
		if (n.IndexOf("WSet") == 0)
		{
			c.Action(AGA_ACT_WEATHER, "", "", "0 0 0", n.Substring(4, 1).ToInt());
			return true;
		}

		switch (n)
		{
			case "Close": Close(); return true;
			case "MPrev": if (m_MPage > 0) m_MPage = m_MPage - 1; RefreshPlayers(); return true;
			case "MNext": m_MPage = m_MPage + 1; RefreshPlayers(); return true;
			case "PPrev": if (m_PPage > 0) m_PPage = m_PPage - 1; RefreshPlayers(); return true;
			case "PNext": m_PPage = m_PPage + 1; RefreshPlayers(); return true;
			case "IPrev": m_IPage = m_IPage - 1; FillTiles(); return true;
			case "INext": m_IPage = m_IPage + 1; FillTiles(); return true;
			case "MTpTo": c.Action(AGA_ACT_TP_TO, m_SelId); return true;
			case "ATpTo": c.Action(AGA_ACT_TP_TO, m_SelId); return true;
			case "MBring": c.Action(AGA_ACT_BRING, m_SelId); return true;
			case "ABring": c.Action(AGA_ACT_BRING, m_SelId); return true;
			case "AHeal": c.Action(AGA_ACT_HEAL, m_SelId); return true;
			case "AKill": c.Action(AGA_ACT_KILL, m_SelId); return true;
			case "AKick": c.Action(AGA_ACT_KICK, m_SelId, m_Reason.GetText()); return true;
			case "ABan": c.Action(AGA_ACT_BAN, m_SelId, m_Reason.GetText()); return true;
			case "AGod":
			{
				int gp = 1;
				AGA_PlayerInfo gi = FindInfo(m_SelId);
				if (gi && gi.God) gp = 0;
				c.Action(AGA_ACT_GOD, m_SelId, "", "0 0 0", gp);
				return true;
			}
			case "ALocate":
			{
				AGA_PlayerInfo li = FindInfo(m_SelId);
				if (li && m_Map)
				{
					m_Map.SetScale(0.08);
					m_Map.SetMapPos(li.Pos);
					ShowPage(0);
				}
				return true;
			}
			case "MTpPick":
			{
				if (m_Pick != vector.Zero) c.Action(AGA_ACT_TP_POS, "", "", m_Pick);
				return true;
			}
			case "STpPick":
			{
				if (m_Pick != vector.Zero) c.Action(AGA_ACT_TP_POS, "", "", m_Pick);
				return true;
			}
			case "IQtyMinus": { int qm = Qty() - 1; if (qm < 1) qm = 1; m_Qty.SetText(qm.ToString()); return true; }
			case "IQtyPlus": { int qp = Qty() + 1; if (qp > 50) qp = 50; m_Qty.SetText(qp.ToString()); return true; }
			case "ISpawnInv": SpawnSelected(AGA_SPAWN_INV); return true;
			case "ISpawnGround": SpawnSelected(AGA_SPAWN_GROUND); return true;
			case "ISpawnPoint": SpawnSelected(AGA_SPAWN_CURSOR); return true;
			case "VSpawn":
			{
				if (m_SelCar >= 0) c.Action(AGA_ACT_CAR, "", m_CarCls[m_SelCar]);
				return true;
			}
			case "MsgSend":
			{
				c.Action(AGA_ACT_MESSAGE, "", m_Msg.GetText());
				m_Msg.SetText("");
				return true;
			}
			case "SFly": Close(); c.StartFly(); return true;
			case "SGod":
			{
				c.God = !c.God;
				int g = 0;
				if (c.God) g = 1;
				c.Action(AGA_ACT_GOD, "", "", "0 0 0", g);
				return true;
			}
			case "SHeal": c.Action(AGA_ACT_HEAL); return true;
			case "CoordGo":
			{
				vector tp = Vector(m_CoordX.GetText().ToFloat(), 0, m_CoordZ.GetText().ToFloat());
				if (tp[0] > 0 && tp[2] > 0) c.Action(AGA_ACT_TP_POS, "", "", tp);
				return true;
			}
		}
		return false;
	}
}
