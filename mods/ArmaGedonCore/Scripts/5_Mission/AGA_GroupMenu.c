// ARMA-GEDON – okno grupy (klawisz P)
class AGA_GroupMenu extends UIScriptedMenu
{
	static ref AGA_GroupView s_View;
	static AGA_GroupMenu s_Open;

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonCore/GUI/layouts/aga_group.layout");
		s_Open = this;
		Fill();
		GetRPCManager().SendRPC("AGA", "AGA_GroupCmd", new Param2<string, string>("view", ""), true);
		return layoutRoot;
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetMission().PlayerControlEnable(false);
		if (s_Open == this) s_Open = null;
	}

	void SetT(string n, string t)
	{
		TextWidget w = TextWidget.Cast(layoutRoot.FindAnyWidget(n));
		if (w) w.SetText(t);
	}

	void Fill()
	{
		if (!layoutRoot) return;
		AGA_GroupView v = s_View;
		bool has = v && v.Gid != 0;
		if (!has)
		{
			SetT("Title", "GROUP");
			SetT("Info", "You are not part of a territory.");
			SetT("Info2", "Craft a flag pole kit (sharpened stick + rag) or join a group at its flag pole.");
		}
		else
		{
			SetT("Title", "GROUP #" + v.Gid.ToString() + "   " + v.Ids.Count().ToString() + "/" + v.Limit.ToString());
			string fl = "lowered";
			if (v.Raised) fl = "RAISED";
			string inv = "off";
			if (v.Invite) inv = "ON";
			SetT("Info", "Flag pole level " + v.Level.ToString() + " / 3     Flag: " + fl + "     Invites: " + inv);
			SetT("Info2", "Lvl 1: 5 days   Lvl 2: 7 days   Lvl 3 + raised flag: base does not decay");
		}
		for (int i = 0; i < 8; i++)
		{
			Widget row = layoutRoot.FindAnyWidget("Row" + i.ToString());
			bool show = has && i < v.Ids.Count();
			if (row) row.Show(show);
			if (!show) continue;
			SetT("Name" + i.ToString(), v.Names[i]);
			Widget dot = layoutRoot.FindAnyWidget("Dot" + i.ToString());
			if (dot)
			{
				if (v.Online[i] == 1) dot.SetColor(ARGB(255, 70, 200, 90));
				else dot.SetColor(ARGB(255, 80, 80, 80));
			}
			Widget k = layoutRoot.FindAnyWidget("Kick" + i.ToString());
			if (k) k.Show(v.IsOwner && v.Names[i].IndexOf("(owner)") < 0);
		}
		Widget lv = layoutRoot.FindAnyWidget("Leave");
		if (lv) lv.Show(has && !v.IsOwner);
		if (has && v.IsOwner) SetT("Hint", "Manage invites at your flag pole. Members join there.");
		else SetT("Hint", "");
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		string n = w.GetName();
		if (n == "Close")
		{
			Close();
			return true;
		}
		if (n == "Leave")
		{
			GetRPCManager().SendRPC("AGA", "AGA_GroupCmd", new Param2<string, string>("leave", ""), true);
			return true;
		}
		if (n.IndexOf("Kick") == 0 && s_View)
		{
			int i = n.Substring(4, n.Length() - 4).ToInt();
			if (i < s_View.Ids.Count())
				GetRPCManager().SendRPC("AGA", "AGA_GroupCmd", new Param2<string, string>("kick", s_View.Ids[i]), true);
			return true;
		}
		return false;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		UAInput back = GetUApi().GetInputByName("UAUIBack");
		if (back && back.LocalPress()) Close();
	}
}
