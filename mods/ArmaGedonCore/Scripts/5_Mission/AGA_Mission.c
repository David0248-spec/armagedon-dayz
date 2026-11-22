// ARMA-GEDON – funkcje serwera (panel admina: VPP Admin Tools)
modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		AGA_Server.Get();
		AGA_Server.Get().Log("VPP sync: aktywny");
		AGA_Recipes.Apply(AGA_Recipes.Cfg().Disabled);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_SyncVPP, 20000, true);
	}

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

	// wylogowanie w trakcie walki: dłuższy licznik i wpis do logu
	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
		if (identity) AGA_Link.Forget(identity.GetPlainId());
		if (!authFailed && player && player.IsAlive() && player.AGA_InCombat())
		{
			if (logoutTime < 60) logoutTime = 60;
			if (identity)
				AGA_Server.Get().Log("[WALKA] wylogowanie w trakcie walki: " + identity.GetName() + " (" + identity.GetPlainId() + ") @ " + player.GetPosition().ToString());
		}
		super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		if (identity && AGA_Recipes.Cfg().Disabled.Count() > 0)
			GetRPCManager().SendRPC("AGA", "AGA_OnRecipes", new Param1<array<string>>(AGA_Recipes.Cfg().Disabled), true, identity);
		if (identity)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Server.Get().WelcomeInfo, 25000, false, identity.GetPlainId());
		if (identity && AGA_Server.Get().IsBanned(identity.GetPlainId()))
		{
			AGA_Server.Get().Log("Zbanowany probowal wejsc: " + identity.GetName() + " (" + identity.GetPlainId() + ")");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Server.Get().DoKick, 2000, false, identity.GetPlainId());
		}
	}
}

modded class MissionGameplay
{
	protected bool m_AGA_RulesShown;
	protected float m_AGA_RulesTimer;
	protected bool m_AGA_RecipesDone;

	void MissionGameplay()
	{
		GetRPCManager().AddRPC("AGA", "AGA_OnRecipes", this, SingleplayerExecutionType.Client);
	}

	void AGA_OnRecipes(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (m_AGA_RecipesDone) return;
		Param1<array<string>> data;
		if (!ctx.Read(data) || !data.param1) return;
		m_AGA_RecipesDone = true;
		AGA_Recipes.Apply(data.param1);
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (m_AGA_RulesShown) return;
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (!p || !p.IsAlive()) return;
		m_AGA_RulesTimer += timeslice;
		if (m_AGA_RulesTimer < 20 || GetGame().GetUIManager().GetMenu()) return;
		m_AGA_RulesShown = true;
		GetGame().GetUIManager().ShowDialog(AGA_RULES_TITLE, AGA_RULES_TEXT, 7701, DBT_OK, DBB_OK, DMT_INFO, null);
	}
}

// ekran wylogowania: ostrzeżenie w trakcie walki i krótka blokada przycisku "wyjdź teraz"
modded class LogoutMenu
{
	protected float m_AGA_LockTime;
	protected bool m_AGA_Locked;

	override void SetTime(int time)
	{
		super.SetTime(time);
		if (time <= 50) return;
		TextWidget desc = TextWidget.Cast(layoutRoot.FindAnyWidget("txtDescription"));
		if (desc)
		{
			desc.SetText("You are still in combat. Logout may result in a ban!");
			desc.SetColor(ARGB(255, 255, 64, 64));
		}
		ButtonWidget now = ButtonWidget.Cast(layoutRoot.FindAnyWidget("bLogoutNow"));
		if (now)
		{
			m_AGA_Locked = true;
			m_AGA_LockTime = 1;
			now.Enable(false);
		}
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (!m_AGA_Locked) return;
		m_AGA_LockTime -= timeslice;
		if (m_AGA_LockTime > 0) return;
		ButtonWidget now = ButtonWidget.Cast(layoutRoot.FindAnyWidget("bLogoutNow"));
		if (now) now.Enable(true);
		m_AGA_Locked = false;
	}
}
