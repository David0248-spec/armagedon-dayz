// ARMA-GEDON Admin – podpięcie pod misję
modded class MissionGameplay
{
	protected bool m_AGA_HomeDown;
	protected bool m_AGA_EndDown;
	protected ref AGA_Menu m_AGA_Menu;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		AGA_Client.Get().Update(timeslice);

		bool home = KeyState(KeyCode.KC_HOME) != 0 || KeyState(KeyCode.KC_INSERT) != 0;
		UAInput panelIn = GetUApi().GetInputByName("UAAGAPanel");
		if (panelIn && panelIn.LocalPress())
			home = true;
		if (home && !m_AGA_HomeDown)
			AGA_TogglePanel();
		m_AGA_HomeDown = home;

		bool end = KeyState(KeyCode.KC_END) != 0;
		UAInput flyIn = GetUApi().GetInputByName("UAAGAFly");
		if (flyIn && flyIn.LocalPress())
			end = true;
		if (end && !m_AGA_EndDown)
			AGA_ToggleFly();
		m_AGA_EndDown = end;
	}

	void AGA_TogglePanel()
	{
		UIScriptedMenu cur = GetGame().GetUIManager().GetMenu();
		string curName = "brak";
		if (cur) curName = cur.ClassName();
		AGA_Client.Get().CLog("wcisnieto HOME/INSERT: admin=" + AGA_Client.Get().IsAdmin().ToString() + ", otwarte menu=" + curName);
		if (cur && AGA_Menu.Cast(cur))
		{
			cur.Close();
			return;
		}
		if (cur)
			return;
		if (!AGA_Client.Get().IsAdmin())
		{
			AGA_Client.Get().RequestAuth();
			return;
		}
		if (AGA_Client.Get().IsFlying())
			AGA_Client.Get().StopFly(false);
		m_AGA_Menu = new AGA_Menu();
		UIScriptedMenu shown = GetGame().GetUIManager().ShowScriptedMenu(m_AGA_Menu, null);
		AGA_Client.Get().CLog("ShowScriptedMenu zwrocil: " + (shown != null).ToString());
	}

	void AGA_ToggleFly()
	{
		if (GetGame().GetUIManager().GetMenu())
			return;
		if (!AGA_Client.Get().IsAdmin())
			return;
		AGA_Client.Get().ToggleFly();
	}
}

modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		AGA_Server.Get();
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		if (identity && AGA_Server.Get().IsBanned(identity.GetPlainId()))
		{
			AGA_Server.Get().Log("Zbanowany probowal wejsc: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Server.Get().DoKick, 2000, false, identity.GetPlainId());
		}
	}
}
