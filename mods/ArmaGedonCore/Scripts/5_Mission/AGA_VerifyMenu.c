// ARMA-GEDON – weryfikacja anty-cheat (okno po regulaminie)
class AGA_VerifyMenu extends UIScriptedMenu
{
	static ref AGA_VerifyMenu s_Open;
	static bool s_Pending;
	static string s_Code;
	static int s_Tries;

	protected EditBoxWidget m_Answer;
	protected TextWidget m_Status;
	protected bool m_Passed;
	protected bool m_Closed;

	bool IsClosed() { return m_Closed; }

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonCore/GUI/layouts/aga_verify.layout");
		m_Answer = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("Answer"));
		m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));
		Refresh(false);
		return layoutRoot;
	}

	void Refresh(bool wrong)
	{
		if (!layoutRoot) return;
		// cyfry: kazda lekko obrocona i przesunieta, w tle kilka linii
		for (int i = 0; i < 3; i++)
		{
			TextWidget d = TextWidget.Cast(layoutRoot.FindAnyWidget("D" + i.ToString()));
			if (!d) continue;
			string ch = "";
			if (s_Code.Length() > i) ch = s_Code.Get(i);
			d.SetText(ch);
			d.SetRotation(0, 0, Math.RandomFloatInclusive(-14, 14));
			d.SetPos(0.08 + i * 0.29 + Math.RandomFloatInclusive(-0.03, 0.03), Math.RandomFloatInclusive(-0.06, 0.06));
		}
		for (int n = 0; n < 4; n++)
		{
			Widget l = layoutRoot.FindAnyWidget("N" + n.ToString());
			if (!l) continue;
			l.SetPos(Math.RandomFloatInclusive(0.02, 0.2), Math.RandomFloatInclusive(0.15, 0.85));
			l.SetRotation(0, 0, Math.RandomFloatInclusive(-12, 12));
		}
		for (int t = 0; t < 3; t++)
		{
			Widget a = layoutRoot.FindAnyWidget("T" + t.ToString());
			if (!a) continue;
			if (t < s_Tries) a.SetColor(ARGB(255, 196, 52, 38));
			else a.SetColor(ARGB(255, 60, 60, 64));
		}
		if (m_Answer) m_Answer.SetText("");
		if (m_Status)
		{
			if (wrong) m_Status.SetText("Wrong answer - try again (" + s_Tries.ToString() + " left)");
			else m_Status.SetText("Type the number shown in the frame");
		}
	}

	void Passed()
	{
		m_Passed = true;
		if (!m_Closed) Close();
	}

	protected void Send(string v)
	{
		GetRPCManager().SendRPC("AGA", "AGA_VerifyAnswer", new Param1<string>(v), true);
	}

	override void OnShow()
	{
		super.OnShow();
		GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetUIManager().ShowUICursor(true);
		if (m_Answer) SetFocus(m_Answer);
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetMission().PlayerControlEnable(false);
		m_Closed = true;
	}

	override bool OnClick(Widget w, int x, int y, int button)
	{
		if (!w) return false;
		string n = w.GetName();
		if (n == "Submit" || n == "Submit_txt")
		{
			if (m_Answer) Send(m_Answer.GetText());
			return true;
		}
		if (n == "Exit" || n == "Exit_txt")
		{
			Send("#exit");
			return true;
		}
		return false;
	}

	override bool OnChange(Widget w, int x, int y, bool finished)
	{
		if (w == m_Answer && finished)
		{
			Send(m_Answer.GetText());
			return true;
		}
		return false;
	}
}
