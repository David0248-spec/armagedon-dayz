// ARMA-GEDON Admin – RPC i pełne leczenie
modded class PlayerBase
{
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (GetGame().IsServer())
		{
			if (!sender || !GetIdentity() || sender.GetPlainId() != GetIdentity().GetPlainId())
				return;
			switch (rpc_type)
			{
				case AGA_RPC_AUTH:
					AGA_Server.Get().OnAuth(this, sender);
					break;
				case AGA_RPC_PLAYERS:
					AGA_Server.Get().OnPlayers(this, sender);
					break;
				case AGA_RPC_ACTION:
					AGA_Server.Get().OnAction(this, sender, ctx);
					break;
				case AGA_RPC_CLOG:
				{
					string msg;
					if (ctx.Read(msg))
						AGA_Server.Get().Log("[KLIENT " + sender.GetName() + "] " + msg);
					break;
				}
			}
		}
		else
		{
			switch (rpc_type)
			{
				case AGA_RPC_AUTH_RESULT:
				{
					bool ok;
					if (ctx.Read(ok))
						AGA_Client.Get().SetAdmin(ok);
					break;
				}
				case AGA_RPC_PLAYERS_RES:
				{
					AGA_Client.Get().ReadPlayers(ctx);
					break;
				}
			}
		}
	}

	void AGA_FullHeal()
	{
		DamageSystem.ResetAllZones(this);
		if (GetModifiersManager())
			GetModifiersManager().ResetAll();
		if (GetBleedingManagerServer())
			GetBleedingManagerServer().RemoveAllSources();
		RemoveAllAgents();
		if (GetStatWater())
			GetStatWater().Set(GetStatWater().GetMax());
		if (GetStatEnergy())
			GetStatEnergy().Set(GetStatEnergy().GetMax());
		if (GetStomach())
			GetStomach().ClearContents();
		if (IsUnconscious())
			DayZPlayerSyncJunctures.SendPlayerUnconsciousness(this, false);
		SetBloodyHands(false);
		SetHealthMax("", "");
		SetHealthMax("", "Blood");
		SetHealthMax("", "Shock");
	}
}
