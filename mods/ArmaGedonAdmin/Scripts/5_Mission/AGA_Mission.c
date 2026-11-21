// ARMA-GEDON Admin – podpięcie pod misję
modded class MissionGameplay
{
	protected bool m_AGA_HomeDown;

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		AGA_Client.Get().Update(timeslice);
		bool down = KeyState(KeyCode.KC_HOME) != 0;
		if (down && !m_AGA_HomeDown)
			AGA_Toggle();
		m_AGA_HomeDown = down;
	}

	void AGA_Toggle()
	{
		UIScriptedMenu cur = GetGame().GetUIManager().GetMenu();
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
		GetGame().GetUIManager().ShowScriptedMenu(new AGA_Menu(), null);
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
			AGA_Server.Get().Log("Zbanowany próbował wejść: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Server.Get().DoKick, 2000, false, identity.GetPlainId());
		}
	}
}
