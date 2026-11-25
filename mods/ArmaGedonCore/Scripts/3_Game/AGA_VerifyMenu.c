// ARMA-GEDON – okno weryfikacji anty-cheat (otwierane dopiero po wejsciu postaci do swiata gry)
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
	protected bool m_Wrong;
	protected float m_Tick;
	protected float m_OpenedAt;

	bool IsClosed() { return m_Closed; }

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonCore/GUI/layouts/aga_verify.layout");
		m_Answer = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("Answer"));
		m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));
		m_OpenedAt = GetGame().GetTickTime();
		Refresh(false);
		return layoutRoot;
	}

	void Refresh(bool wrong)
	{
		if (!layoutRoot) return;
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
		if (m_Answer)
		{
			m_Answer.SetText("");
			SetFocus(m_Answer);
		}
		m_Wrong = wrong;
		if (m_Status)
		{
			if (wrong) m_Status.SetText("Wrong answer - try again (" + s_Tries.ToString() + " left)");
			else m_Status.SetText("Type the number shown in the frame");
		}
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (m_Answer && GetFocus() != m_Answer) SetFocus(m_Answer);
		m_Tick += timeslice;
		if (m_Tick < 0.25 || !m_Status) return;
		m_Tick = 0;
		int left = 180 - (int)(GetGame().GetTickTime() - m_OpenedAt);
		if (left < 0) left = 0;
		string msg = "Type the number shown in the frame";
		if (m_Wrong) msg = "Wrong answer - try again (" + s_Tries.ToString() + " left)";
		m_Status.SetText(msg + "   |   " + left.ToString() + "s");
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
		if (GetGame().GetMission()) GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
		GetGame().GetUIManager().ShowUICursor(true);
		if (m_Answer) SetFocus(m_Answer);
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetUIManager().ShowUICursor(false);
		if (GetGame().GetMission()) GetGame().GetMission().PlayerControlEnable(false);
		m_Closed = true;
		if (s_Open == this) s_Open = null;
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

class AGA_VerifyClient
{
	static ref AGA_VerifyClient s_Inst;
	static bool s_Verified;
	static bool s_HelloSent;
	static float s_HelloAt;
	static bool s_ReadyToShow;

	static void Init()
	{
		if (s_Inst) return;
		s_Inst = new AGA_VerifyClient();
		GetRPCManager().AddRPC("AGA", "AGA_VerifyAsk", s_Inst, SingleplayerExecutionType.Client);
	}

	static void Reset()
	{
		s_Verified = false;
		s_HelloSent = false;
		s_HelloAt = 0;
		s_ReadyToShow = false;
		AGA_VerifyMenu.s_Pending = false;
		AGA_VerifyMenu.s_Code = "";
		AGA_VerifyMenu.s_Tries = 3;
		if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed())
			AGA_VerifyMenu.s_Open.Close();
		AGA_VerifyMenu.s_Open = null;
	}

	static void Hello()
	{
		Init();
		if (s_Verified) return;
		// Prośbę o kod ponawiamy co 3 s, dopóki serwer nie odpowie. Wcześniej leciała tylko raz na
		// uruchomienie gry: po powrocie do menu i ponownym wejściu okno nie dostawało kodu i trzeba było
		// restartować grę.
		if (AGA_VerifyMenu.s_Pending) return;
		float now = GetGame().GetTickTime();
		if (s_HelloSent && now - s_HelloAt < 3.0) return;
		s_HelloSent = true;
		s_HelloAt = now;
		GetRPCManager().SendRPC("AGA", "AGA_VerifyHello", new Param1<int>(0), true);
	}

	void AGA_VerifyAsk(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param2<string, int> data;
		if (!ctx.Read(data)) return;

		if (data.param2 < 0)
		{
			AGA_VerifyMenu.s_Pending = false;
			s_Verified = true;
			if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed())
				AGA_VerifyMenu.s_Open.Passed();
			return;
		}

		bool wasPending = AGA_VerifyMenu.s_Pending;
		s_Verified = false;
		AGA_VerifyMenu.s_Code = data.param1;
		AGA_VerifyMenu.s_Tries = data.param2;
		AGA_VerifyMenu.s_Pending = true;
		if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed())
			AGA_VerifyMenu.s_Open.Refresh(wasPending);
	}

	static void Show()
	{
		if (s_Verified || !s_ReadyToShow || !AGA_VerifyMenu.s_Pending) return;
		if (GetGame().IsLoading()) return;
		if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed()) return;
		UIScriptedMenu cur = GetGame().GetUIManager().GetMenu();
		if (cur) return;
		AGA_VerifyMenu.s_Open = new AGA_VerifyMenu();
		GetGame().GetUIManager().ShowScriptedMenu(AGA_VerifyMenu.s_Open, null);
	}
}
