// ARMA-GEDON – funkcje serwera (panel admina: VPP Admin Tools)
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		AGA_Server.Get();
#ifdef VPPADMINTOOLS
		AGA_Server.Get().Log("VPP sync: aktywny");
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_SyncVPP, 20000, true);
#endif
	}

#ifdef VPPADMINTOOLS
	// aktualne eventy jako lokacje w Teleport Managerze VPP ([EVENT] ...)
	protected string m_AGA_VPPSig;

	void AGA_SyncVPP()
	{
		string sig = "";
		foreach (AGA_Event ev : AGA_Events.s_Active)
			sig = sig + ev.Kind + ev.Tp.ToString() + ";";
		if (sig == m_AGA_VPPSig) return;
		TeleportManager tm = TeleportManager.Cast(GetPluginManager().GetPluginByType(TeleportManager));
		if (!tm)
		{
			AGA_Server.Get().Log("VPP sync: brak TeleportManager");
			return;
		}
		array<ref VPPTeleportLocation> locs = tm.GetTeleportLocations();
		if (!locs)
		{
			AGA_Server.Get().Log("VPP sync: brak listy lokacji");
			return;
		}
		for (int i = locs.Count() - 1; i >= 0; i--)
		{
			if (locs[i] && locs[i].GetName().IndexOf("[EVENT]") == 0)
				locs.Remove(i);
		}
		map<string, int> cnt = new map<string, int>;
		foreach (AGA_Event e : AGA_Events.s_Active)
		{
			int n = cnt.Get(e.Kind) + 1;
			cnt.Set(e.Kind, n);
			tm.AddLocation(AGA_Events.Label(e, n), e.Tp);
		}
		// VPP wczytuje listę z pliku przy każdym otwarciu menu – zapisujemy
		tm.Save();
		m_AGA_VPPSig = sig;
		AGA_Server.Get().Log("VPP sync: zapisano " + AGA_Events.s_Active.Count().ToString() + " eventow, lokacji " + locs.Count().ToString());
	}
#endif

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
