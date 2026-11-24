// ARMA-GEDON – weryfikacja anty-cheat
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
	protected bool m_BackConsumed;

	bool IsClosed() { return m_Closed; }

	override Widget Init()
	{
		layoutRoot = GetGame().GetWorkspace().CreateWidgets("ArmaGedonCore/GUI/layouts/aga_verify.layout");
		if (!layoutRoot)
			return super.Init();

		m_Answer = EditBoxWidget.Cast(layoutRoot.FindAnyWidget("Answer"));
		m_Status = TextWidget.Cast(layoutRoot.FindAnyWidget("Status"));
		m_OpenedAt = GetGame().GetTickTime();

		Widget notif = layoutRoot.FindAnyWidget("notification_root");
		if (notif) notif.Show(false);

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
		if (m_Answer) m_Answer.SetText("");
		m_Wrong = wrong;
		if (m_Status)
		{
			if (wrong) m_Status.SetText("Wrong answer - try again (" + s_Tries.ToString() + " left)");
			else m_Status.SetText("Type the number shown in the frame");
		}
	}

	// Identyczna obsluga ESC jak w sal_loginscreen.c (blokuje przypadkowe zamkniecie okna)
	override void Update(float timeslice)
	{
		if (GetUApi().GetInputByID(UAUIBack).LocalPress())
		{
			if (!m_BackConsumed)
			{
				m_BackConsumed = true;
				return;
			}
		}
		else
		{
			m_BackConsumed = false;
			super.Update(timeslice);
		}

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
		GetGame().GetSoundScene().SetSoundVolume(0, 1);
		if (m_Answer) SetFocus(m_Answer);
	}

	override void OnHide()
	{
		super.OnHide();
		GetGame().GetUIManager().ShowUICursor(false);
		GetGame().GetSoundScene().SetSoundVolume(1.0, 1.0);
		if (GetGame().GetMission()) GetGame().GetMission().PlayerControlEnable(false);
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
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(GetGame().DisconnectSessionForce);
			return true;
		}
		return super.OnClick(w, x, y, button);
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
		AGA_VerifyMenu.s_Pending = false;
		if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed())
			AGA_VerifyMenu.s_Open.Close();
		AGA_VerifyMenu.s_Open = null;
	}

	static void Hello()
	{
		Init();
		if (s_Verified || s_HelloSent) return;
		s_HelloSent = true;
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
			if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed()) AGA_VerifyMenu.s_Open.Passed();
			return;
		}
		s_Verified = false;
		AGA_VerifyMenu.s_Code = data.param1;
		AGA_VerifyMenu.s_Tries = data.param2;
		AGA_VerifyMenu.s_Pending = true;
		if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed())
			AGA_VerifyMenu.s_Open.Refresh(true);
		else
			Show();
	}

	static void Show()
	{
		if (s_Verified || !AGA_VerifyMenu.s_Pending) return;
		if (AGA_VerifyMenu.s_Open && !AGA_VerifyMenu.s_Open.IsClosed()) return;
		UIScriptedMenu cur = GetGame().GetUIManager().GetMenu();
		if (cur && !LoginTimeBase.Cast(cur) && !LoginScreenBase.Cast(cur)) return;
		AGA_VerifyMenu.s_Open = new AGA_VerifyMenu();
		GetGame().GetUIManager().ShowScriptedMenu(AGA_VerifyMenu.s_Open, cur);
	}
}

// 1:1 struktura z sal_loginscreen.c (LoginQueueBase + LoginTimeBase)
modded class LoginQueueBase
{
	override Widget Init()
	{
		Widget w = super.Init();
		AGA_VerifyClient.Reset();
		AGA_VerifyClient.Hello();
		return w;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		AGA_VerifyClient.Show();
	}
}

modded class LoginTimeBase
{
	override Widget Init()
	{
		Widget w = super.Init();
		AGA_VerifyClient.Hello();
		return w;
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		AGA_VerifyClient.Show();
	}
}

modded class DayZGame
{
	override void OnUpdate(bool doSim, float timeslice)
	{
		super.OnUpdate(doSim, timeslice);
		if (!IsDedicatedServer() && IsMultiplayer())
		{
			AGA_VerifyClient.Init();
			if (AGA_VerifyMenu.s_Pending && !AGA_VerifyClient.s_Verified)
			{
				AGA_VerifyClient.Show();
				if (GetMission()) GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
			}
		}
	}
}
