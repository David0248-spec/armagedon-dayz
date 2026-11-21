// ARMA-GEDON Admin – okno panelu
class AGA_ItemDef
{
	string Cls;
	string Name;
	string Low;
	int Cat;
}

// baza przedmiotów i pojazdów z konfiguracji gry (budowana raz)
class AGA_ItemDb
{
	static ref array<ref AGA_ItemDef> s_Items;
	static ref array<ref AGA_ItemDef> s_Vehicles;

	static array<ref AGA_ItemDef> Get()
	{
		if (!s_Items)
			Build();
		return s_Items;
	}

	static array<ref AGA_ItemDef> Vehicles()
	{
		if (!s_Vehicles)
			Build();
		return s_Vehicles;
	}

	static void Build()
	{
		s_Items = new array<ref AGA_ItemDef>;
		s_Vehicles = new array<ref AGA_ItemDef>;
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
				string low = cls;
				low.ToLower();
				string dn;
				GetGame().ConfigGetText(path + " displayName", dn);
				if (dn == "")
					dn = cls;
				string dl = dn;
				dl.ToLower();

				TStringArray parents = new TStringArray;
				GetGame().ConfigGetFullPath(path, parents);

				// pojazdy
				if (parents.Find("Transport") != -1)
				{
					if (low.Contains("test"))
						continue;
					int vc = 5;
					if (parents.Find("BoatScript") != -1 || parents.Find("Boat") != -1)
						vc = 3;
					else if (parents.Find("MotorbikeScript") != -1 || parents.Find("Motorbike") != -1)
						vc = 4;
					else if (parents.Find("CarScript") != -1 || parents.Find("Car") != -1)
					{
						vc = 1;
						if (low.Contains("truck") || low.Contains("van_"))
							vc = 2;
					}
					AGA_ItemDef v = new AGA_ItemDef();
					v.Cls = cls;
					v.Name = dn;
					v.Low = low + " " + dl;
					v.Cat = vc;
					s_Vehicles.Insert(v);
					continue;
				}

				int cat = 7;
				if (parents.Find("Weapon_Base") != -1)
					cat = 1;
				else if (parents.Find("Magazine_Base") != -1)
					cat = 2;
				else if (parents.Find("Clothing_Base") != -1 || parents.Find("Clothing") != -1)
				{
					cat = 3;
					TStringArray slots = new TStringArray;
					GetGame().ConfigGetTextArray(path + " inventorySlot", slots);
					string slot;
					GetGame().ConfigGetText(path + " inventorySlot", slot);
					if (slot == "Back" || slots.Find("Back") != -1)
						cat = 4;
				}
				else if (parents.Find("Edible_Base") != -1)
					cat = 5;
				else if (parents.Find("Inventory_Base") == -1 && parents.Find("ItemBase") == -1)
					continue;

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
				AGA_ItemDef d = new AGA_ItemDef();
				d.Cls = cls;
				d.Name = dn;
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

	// przedmioty
	protected int m_Cat;
	protected string m_LastSearch = "-";
	protected int m_IPage;
	protected bool m_Full;
	protected ref array<AGA_ItemDef> m_Filtered = new array<AGA_ItemDef>;
	protected ref array<EntityAI> m_TileEnt = new array<EntityAI>;
	protected ref array<string> m_TileCls = new array<string>;
	protected string m_SelItem;
	protected EntityAI m_DetailEnt;
	protected float m_Rot;

	// pojazdy
	protected int m_VCat;
	protected int m_VPage;
	protected ref array<AGA_ItemDef> m_VFiltered = new array<AGA_ItemDef>;
	protected ref array<EntityAI> m_VEnt = new array<EntityAI>;
	protected ref array<string> m_VCls = new array<string>;
	protected string m_SelVeh;
	protected EntityAI m_VDetailEnt;

	// AI
	protected ref array<string> m_Zombies;
	protected ref array<string> m_Animals;
	protected ref array<int> m_AiCounts;
	protected string m_AiCls;
	protected string m_AiName;
	protected int m_AiCountIdx;

	protected ref array<string> m_Kits;

	protected float m_Tick;
	protected float m_LogTick;
	protected bool m_EscDown = true;

	const int COL_HOVER = 0xFF2A2F38;
	const int COL_RED = 0xFFDB2626;
	const int COL_ACTIVE = 0xFF171A20;

	void AGA_Menu()
	{
		m_Zombies = {"ZmbM_CitizenASkinny_Blue", "ZmbF_CitizenANormal_Blue", "ZmbM_SoldierNormal", "ZmbM_PolicemanFat", "ZmbM_DoctorFat", "ZmbM_PrisonerSkinny", "ZmbM_FirefighterNormal", "ZmbM_ConstrWorkerNormal_Beige"};
		m_Animals = {"Animal_CanisLupus_Grey", "Animal_UrsusArctos", "Animal_CervusElaphus", "Animal_SusScrofa", "Animal_BosTaurus_Brown", "Animal_GallusGallusDomesticus", "Animal_VulpesVulpes", "Animal_OvisAries"};
		m_AiCounts = {1, 3, 5, 10, 20};
		m_Kits = {"SZTURM", "SNAJPER", "MEDYK", "BUDOWA", "SURVIVAL", "UBRANIE"};
	}

	// ------------------------------------------------------------
	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonAdmin/GUI/layouts/aga_menu.layout");
		if (!layoutRoot)
		{
			AGA_Client.Get().CLog("BLAD: layout panelu nie wczytal sie (CreateWidgets = null)");
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
		for (int j = 0; j < 6; j++)
		{
			m_VEnt.Insert(null);
			m_VCls.Insert("");
		}

		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());
		if (m_Map && me)
		{
			m_Map.SetScale(0.25);
			m_Map.SetMapPos(me.GetPosition());
		}
		if (me && me.GetIdentity())
			SetTxt("Who", "ADMIN: " + me.GetIdentity().GetName());

		RefreshToggles();
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

	void Mark(string n, bool on)
	{
		ShowW(n + "_bar", on);
		if (on)
			SetBg(n, COL_ACTIVE);
		else
			ResetBg(n);
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

	string OnOff(bool b)
	{
		if (b) return "WLACZONE";
		return "wylaczone";
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
		for (int j = 0; j < m_VEnt.Count(); j++)
		{
			SetPreview("VTile" + j.ToString() + "_pv", null, vector.Zero);
			KillEnt(m_VEnt[j]);
			m_VEnt[j] = null;
			m_VCls[j] = "";
		}
		SetPreview("IDetail_pv", null, vector.Zero);
		SetPreview("VDetail_pv", null, vector.Zero);
		KillEnt(m_DetailEnt);
		KillEnt(m_VDetailEnt);
		m_DetailEnt = null;
		m_VDetailEnt = null;
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

	int Pages(int count, int per)
	{
		int p = (count + per - 1) / per;
		if (p < 1) p = 1;
		return p;
	}

	// ------------------------------------------------------------ strony
	void ShowPage(int idx)
	{
		s_Page = idx;
		for (int i = 0; i < 9; i++)
		{
			ShowW("Page" + i.ToString(), i == idx);
			Mark("Nav" + i.ToString(), i == idx);
		}
		if (m_Markers)
			m_Markers.Show(idx == 0);
		if (idx == 2)
		{
			m_LastSearch = "-";
			RefreshItems();
		}
		if (idx == 3)
			RefreshVehicles();
		if (idx == 4)
			RefreshAi();
		if (idx == 8)
		{
			AGA_Client.Get().RequestLogs();
			m_LogTick = 0.5;
		}
	}

	// ------------------------------------------------------------ gracze
	void RefreshPlayers()
	{
		array<ref AGA_PlayerInfo> list = AGA_Client.Get().Players;
		PlayerBase me = PlayerBase.Cast(GetGame().GetPlayer());

		int mPages = Pages(list.Count(), 14);
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

		int pPages = Pages(list.Count(), 15);
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
			string flags = "";
			if (pinfo.God) flags = flags + " G";
			if (pinfo.Invis) flags = flags + " N";
			if (pinfo.InVehicle) flags = flags + " P";
			SetTxt(pn + "_txt", Short(pinfo.Name, 20));
			SetTxt(pn + "_r", "HP " + Math.Round(pinfo.Health).ToString() + flags);
			ShowW(pn + "_bar", pinfo.SteamId == m_SelId);
		}
		RefreshCard();
	}

	void SetBar(string n, float frac)
	{
		Widget w = F(n);
		if (w) w.SetSize(Math.Clamp(frac, 0, 1), 1);
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
			SetTxt("CWaV", "");
			SetTxt("CEnV", "");
			SetTxt("CPos", "");
			SetTxt("CHands", "");
			SetTxt("CGod", "");
			return;
		}
		SetTxt("CName", info.Name);
		SetTxt("CSteam", "STEAM ID: " + info.SteamId);
		SetTxt("CHpV", Math.Round(info.Health).ToString() + " / 100");
		SetTxt("CBlV", Math.Round(info.Blood).ToString() + " / 5000");
		SetTxt("CWaV", Math.Round(info.Water).ToString());
		SetTxt("CEnV", Math.Round(info.Energy).ToString());
		SetBar("CHp", info.Health / 100.0);
		SetBar("CBl", info.Blood / 5000.0);
		SetBar("CWa", info.Water / 5000.0);
		SetBar("CEn", info.Energy / 20000.0);
		SetTxt("CPos", "POZYCJA: " + Math.Round(info.Pos[0]).ToString() + " / " + Math.Round(info.Pos[2]).ToString());
		string hands = info.InHands;
		if (hands == "") hands = "nic";
		SetTxt("CHands", "W RECE: " + hands);
		string st = "";
		if (info.God) st = st + "GOD MODE   ";
		if (info.Invis) st = st + "NIEWIDZIALNY   ";
		if (info.InVehicle) st = st + "W POJEZDZIE";
		SetTxt("CGod", st);
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
				else if (info.Invis)
					dot.SetColor(0xFFB46CFF);
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
		int pages = Pages(m_Filtered.Count(), 18);
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
		int m = mode;
		if (m_Full) m = m + 10;
		AGA_Client.Get().Action(AGA_ACT_SPAWN, "", m_SelItem, m_Pick, Qty(), m);
	}

	// ------------------------------------------------------------ pojazdy
	void RefreshVehicles()
	{
		m_VFiltered.Clear();
		array<ref AGA_ItemDef> all = AGA_ItemDb.Vehicles();
		foreach (AGA_ItemDef d : all)
		{
			if (m_VCat != 0 && d.Cat != m_VCat) continue;
			m_VFiltered.Insert(d);
		}
		for (int c = 0; c < 6; c++)
		{
			if (c == m_VCat)
				SetBg("VCat" + c.ToString(), COL_RED);
			else
				ResetBg("VCat" + c.ToString());
		}
		SetTxt("VCount", "POJAZDY: " + m_VFiltered.Count().ToString());
		FillVehTiles();
	}

	void FillVehTiles()
	{
		int pages = Pages(m_VFiltered.Count(), 6);
		if (m_VPage >= pages) m_VPage = pages - 1;
		if (m_VPage < 0) m_VPage = 0;
		SetTxt("VPage", (m_VPage + 1).ToString() + " / " + pages.ToString());
		for (int i = 0; i < 6; i++)
		{
			int idx = m_VPage * 6 + i;
			string tn = "VTile" + i.ToString();
			string cls = "";
			if (idx < m_VFiltered.Count())
				cls = m_VFiltered[idx].Cls;
			ShowW(tn, cls != "");
			if (cls == "")
			{
				SetPreview(tn + "_pv", null, vector.Zero);
				KillEnt(m_VEnt[i]);
				m_VEnt[i] = null;
				m_VCls[i] = "";
				continue;
			}
			SetTxt(tn + "_nm", Short(m_VFiltered[idx].Name, 26));
			ShowW(tn + "_bar", cls == m_SelVeh);
			if (m_VCls[i] != cls)
			{
				SetPreview(tn + "_pv", null, vector.Zero);
				KillEnt(m_VEnt[i]);
				m_VEnt[i] = MakeLocal(cls);
				m_VCls[i] = cls;
				SetPreview(tn + "_pv", m_VEnt[i], Vector(-8, 40, 0));
			}
		}
	}

	void SelectVehicle(string cls)
	{
		m_SelVeh = cls;
		SetPreview("VDetail_pv", null, vector.Zero);
		KillEnt(m_VDetailEnt);
		m_VDetailEnt = MakeLocal(cls);
		SetPreview("VDetail_pv", m_VDetailEnt, Vector(-8, m_Rot, 0));
		string dn = cls;
		foreach (AGA_ItemDef d : m_VFiltered)
		{
			if (d.Cls == cls)
				dn = d.Name;
		}
		SetTxt("VDName", Short(dn, 28));
		SetTxt("VDClass", cls);
		for (int i = 0; i < 6; i++)
			ShowW("VTile" + i.ToString() + "_bar", m_VCls[i] == cls);
	}

	// ------------------------------------------------------------ AI
	void RefreshAi()
	{
		for (int i = 0; i < 8; i++)
		{
			Mark("AiZ" + i.ToString(), m_Zombies[i] == m_AiCls);
			Mark("AiA" + i.ToString(), m_Animals[i] == m_AiCls);
		}
		for (int k = 0; k < 5; k++)
			Mark("AiN" + k.ToString(), k == m_AiCountIdx);
		if (m_AiCls == "")
		{
			SetTxt("ASel", "Wybierz typ AI");
			SetTxt("ASelN", "");
		}
		else
		{
			SetTxt("ASel", m_AiName + "  x" + m_AiCounts[m_AiCountIdx].ToString());
			SetTxt("ASelN", m_AiCls);
		}
	}

	// ------------------------------------------------------------ ESP / przełączniki
	void RefreshToggles()
	{
		AGA_Client c = AGA_Client.Get();
		SetTxt("EOn_st", OnOff(c.EspOn));
		SetTxt("ENames_st", OnOff(c.EspNames));
		SetTxt("EBoxes_st", OnOff(c.EspBoxes));
		SetTxt("ELines_st", OnOff(c.EspLines));
		SetTxt("EShots_st", OnOff(c.EspShots));
		Mark("EOn", c.EspOn);
		Mark("ENames", c.EspNames);
		Mark("EBoxes", c.EspBoxes);
		Mark("ELines", c.EspLines);
		Mark("EShots", c.EspShots);
		array<float> ranges = {300, 1000, 2500, 10000};
		for (int i = 0; i < 4; i++)
			Mark("ERange" + i.ToString(), ranges[i] == c.EspRange);
		SetTxt("SFly_st", "klawisz END");
		SetTxt("SGod_st", OnOff(c.God));
		SetTxt("SInvis_st", OnOff(c.Invis));
		SetTxt("SDel_st", OnOff(c.DeleteMode) + " (DELETE)");
		SetTxt("SHeal_st", "");
		SetTxt("STpPick_st", "");
		SetTxt("SRepair_st", "promien 25 m");
		SetTxt("SLogs_st", "");
		IFullLabel();
	}

	void IFullLabel()
	{
		if (m_Full)
			SetTxt("IFull_txt", "Z WYPOSAZENIEM: TAK");
		else
			SetTxt("IFull_txt", "Z WYPOSAZENIEM: NIE");
	}

	// ------------------------------------------------------------ logi
	void RefreshLogs()
	{
		AGA_Client c = AGA_Client.Get();
		int na = c.LogAdmin.Count();
		int nc = c.LogCombat.Count();
		for (int i = 0; i < 24; i++)
		{
			int ia = na - 24 + i;
			string la = "";
			if (ia >= 0 && ia < na) la = c.LogAdmin[ia];
			SetTxt("LA" + i.ToString(), Short(la, 95));
			int ic = nc - 24 + i;
			string lc = "";
			if (ic >= 0 && ic < nc) lc = c.LogCombat[ic];
			SetTxt("LC" + i.ToString(), Short(lc, 95));
		}
	}

	// ------------------------------------------------------------ odświeżanie
	override void Update(float timeslice)
	{
		super.Update(timeslice);

		bool esc = KeyState(KeyCode.KC_ESCAPE) != 0;
		if (esc && !m_EscDown)
		{
			Close();
			return;
		}
		m_EscDown = esc;

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
		if (vp && m_VDetailEnt && s_Page == 3)
			vp.SetModelOrientation(Vector(-8, m_Rot, 0));

		m_Tick -= timeslice;
		if (m_Tick <= 0)
		{
			m_Tick = 0.5;
			RefreshPlayers();
			RefreshStats();
			if (s_Page == 8)
				RefreshLogs();
		}
		if (s_Page == 8)
		{
			m_LogTick -= timeslice;
			if (m_LogTick <= 0)
			{
				m_LogTick = 3;
				AGA_Client.Get().RequestLogs();
			}
		}
		UpdateMarkers();
	}

	void RefreshStats()
	{
		AGA_Client c = AGA_Client.Get();
		int y, mo, d, h, mi;
		GetGame().GetWorld().GetDate(y, mo, d, h, mi);
		string flags = "";
		if (c.God) flags = flags + "   GOD";
		if (c.Invis) flags = flags + "   NIEWIDZIALNY";
		if (c.EspOn) flags = flags + "   ESP";
		if (c.DeleteMode) flags = flags + "   USUWANIE";
		SetTxt("Stats", "ONLINE: " + c.Players.Count().ToString() + "      CZAS: " + h.ToStringLen(2) + ":" + mi.ToStringLen(2) + flags);
		if (m_Pick != vector.Zero)
			SetTxt("MPick", "PUNKT: " + Math.Round(m_Pick[0]).ToString() + " / " + Math.Round(m_Pick[2]).ToString());
	}

	// ------------------------------------------------------------ zdarzenia
	bool IsActiveButton(string n)
	{
		Widget bar = F(n + "_bar");
		if (bar && bar.IsVisible()) return true;
		if (n.IndexOf("ICat") == 0 && n.Substring(4, 1).ToInt() == m_Cat) return true;
		if (n.IndexOf("VCat") == 0 && n.Substring(4, 1).ToInt() == m_VCat) return true;
		return false;
	}

	override bool OnMouseEnter(Widget w, int x, int y)
	{
		if (ButtonWidget.Cast(w) && m_BaseColor.Contains(w.GetName()) && !IsActiveButton(w.GetName()))
			SetBg(w.GetName(), COL_HOVER);
		return false;
	}

	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		if (ButtonWidget.Cast(w) && m_BaseColor.Contains(w.GetName()) && !IsActiveButton(w.GetName()))
			ResetBg(w.GetName());
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
		if (n.IndexOf("VTile") == 0 && s_Page == 3)
		{
			m_VPage = m_VPage + dir;
			FillVehTiles();
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

	void Spectate()
	{
		if (m_SelId == "") return;
		Close();
		AGA_Client.Get().StartSpectate(m_SelId);
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
		if (n.IndexOf("VCat") == 0)
		{
			m_VCat = n.Substring(4, 1).ToInt();
			m_VPage = 0;
			RefreshVehicles();
			return true;
		}
		if (n.IndexOf("VTile") == 0)
		{
			int vi = n.Substring(5, 1).ToInt();
			if (vi >= 0 && vi < 6 && m_VCls[vi] != "")
				SelectVehicle(m_VCls[vi]);
			return true;
		}
		if (n.IndexOf("AiZ") == 0)
		{
			int zi = n.Substring(3, 1).ToInt();
			m_AiCls = m_Zombies[zi];
			m_AiName = "ZOMBIE";
			RefreshAi();
			return true;
		}
		if (n.IndexOf("AiA") == 0)
		{
			int ai = n.Substring(3, 1).ToInt();
			m_AiCls = m_Animals[ai];
			m_AiName = "ZWIERZE";
			RefreshAi();
			return true;
		}
		if (n.IndexOf("AiN") == 0)
		{
			m_AiCountIdx = n.Substring(3, 1).ToInt();
			RefreshAi();
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
		if (n.IndexOf("ERange") == 0)
		{
			array<float> ranges = {300, 1000, 2500, 10000};
			c.EspRange = ranges[n.Substring(6, 1).ToInt()];
			RefreshToggles();
			return true;
		}
		if (n.IndexOf("PKit") == 0)
		{
			if (m_SelId != "")
				c.Action(AGA_ACT_KIT, m_SelId, m_Kits[n.Substring(4, 1).ToInt()]);
			return true;
		}
		if (n.IndexOf("SKit") == 0)
		{
			c.Action(AGA_ACT_KIT, "", m_Kits[n.Substring(4, 1).ToInt()]);
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
			case "VPrev": m_VPage = m_VPage - 1; FillVehTiles(); return true;
			case "VNext": m_VPage = m_VPage + 1; FillVehTiles(); return true;
			case "MTpTo": c.Action(AGA_ACT_TP_TO, m_SelId); return true;
			case "ATpTo": c.Action(AGA_ACT_TP_TO, m_SelId); return true;
			case "MBring": c.Action(AGA_ACT_BRING, m_SelId); return true;
			case "ABring": c.Action(AGA_ACT_BRING, m_SelId); return true;
			case "MSpec": Spectate(); return true;
			case "ASpec": Spectate(); return true;
			case "AHeal": c.Action(AGA_ACT_HEAL, m_SelId); return true;
			case "AKill": c.Action(AGA_ACT_KILL, m_SelId); return true;
			case "AKick": c.Action(AGA_ACT_KICK, m_SelId, m_Reason.GetText()); return true;
			case "ABan": c.Action(AGA_ACT_BAN, m_SelId, m_Reason.GetText()); return true;
			case "AFreeze": c.Action(AGA_ACT_FREEZE, m_SelId, "", "0 0 0", 1); return true;
			case "AUnfreeze": c.Action(AGA_ACT_FREEZE, m_SelId, "", "0 0 0", 0); return true;
			case "AStrip": c.Action(AGA_ACT_STRIP, m_SelId); return true;
			case "APm": c.Action(AGA_ACT_PM, m_SelId, m_Reason.GetText()); return true;
			case "PHealAll": c.Action(AGA_ACT_HEAL_ALL); return true;
			case "AGod":
			{
				int gp = 1;
				AGA_PlayerInfo gi = FindInfo(m_SelId);
				if (gi && gi.God) gp = 0;
				c.Action(AGA_ACT_GOD, m_SelId, "", "0 0 0", gp);
				return true;
			}
			case "AInvis":
			{
				int ip = 1;
				AGA_PlayerInfo ii = FindInfo(m_SelId);
				if (ii && ii.Invis) ip = 0;
				c.Action(AGA_ACT_INVIS, m_SelId, "", "0 0 0", ip);
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
			case "IFull": m_Full = !m_Full; IFullLabel(); return true;
			case "ISpawnInv": SpawnSelected(AGA_SPAWN_INV); return true;
			case "ISpawnGround": SpawnSelected(AGA_SPAWN_GROUND); return true;
			case "ISpawnPoint": SpawnSelected(AGA_SPAWN_CURSOR); return true;
			case "VSpawn":
			{
				if (m_SelVeh != "") c.Action(AGA_ACT_CAR, "", m_SelVeh);
				return true;
			}
			case "VSpawnPick":
			{
				if (m_SelVeh != "" && m_Pick != vector.Zero) c.Action(AGA_ACT_CAR, "", m_SelVeh, m_Pick);
				return true;
			}
			case "VRepair": c.Action(AGA_ACT_REPAIR_CAR); return true;
			case "SRepair": c.Action(AGA_ACT_REPAIR_CAR); return true;
			case "AiSpawn":
			{
				if (m_AiCls != "") c.Action(AGA_ACT_SPAWN_AI, "", m_AiCls, "0 0 0", m_AiCounts[m_AiCountIdx]);
				return true;
			}
			case "AiSpawnPick":
			{
				if (m_AiCls != "" && m_Pick != vector.Zero) c.Action(AGA_ACT_SPAWN_AI, "", m_AiCls, m_Pick, m_AiCounts[m_AiCountIdx]);
				return true;
			}
			case "AiClear": c.Action(AGA_ACT_CLEAR_AI, "", "", "0 0 0", 150); return true;
			case "MsgSend":
			{
				c.Action(AGA_ACT_MESSAGE, "", m_Msg.GetText());
				m_Msg.SetText("");
				return true;
			}
			case "EOn": c.EspOn = !c.EspOn; RefreshToggles(); return true;
			case "ENames": c.EspNames = !c.EspNames; RefreshToggles(); return true;
			case "EBoxes": c.EspBoxes = !c.EspBoxes; RefreshToggles(); return true;
			case "ELines": c.EspLines = !c.EspLines; RefreshToggles(); return true;
			case "EShots": c.EspShots = !c.EspShots; RefreshToggles(); return true;
			case "SFly": Close(); c.StartFly(); return true;
			case "SGod":
			{
				c.God = !c.God;
				int g = 0;
				if (c.God) g = 1;
				c.Action(AGA_ACT_GOD, "", "", "0 0 0", g);
				RefreshToggles();
				return true;
			}
			case "SInvis":
			{
				c.Invis = !c.Invis;
				int iv = 0;
				if (c.Invis) iv = 1;
				c.Action(AGA_ACT_INVIS, "", "", "0 0 0", iv);
				RefreshToggles();
				return true;
			}
			case "SDel": c.DeleteMode = !c.DeleteMode; RefreshToggles(); return true;
			case "SHeal": c.Action(AGA_ACT_HEAL); return true;
			case "SLogs": ShowPage(8); return true;
			case "LRefresh": c.RequestLogs(); return true;
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
