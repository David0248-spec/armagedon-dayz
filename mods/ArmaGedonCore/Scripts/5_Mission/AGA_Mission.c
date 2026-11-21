// ARMA-GEDON – funkcje serwera (panel admina: VPP Admin Tools)
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
		if (identity)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Server.Get().WelcomeInfo, 25000, false, identity.GetPlainId());
		if (identity && AGA_Server.Get().IsBanned(identity.GetPlainId()))
		{
			AGA_Server.Get().Log("Zbanowany probowal wejsc: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Server.Get().DoKick, 2000, false, identity.GetPlainId());
		}
	}
}
