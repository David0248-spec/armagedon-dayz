// ARMA-GEDON – funkcje serwera (panel admina: VPP Admin Tools)
modded class MissionServer
{
	// weryfikacja startuje juz przy laczeniu (przed odliczaniem do wejscia)
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
		AGA_Verify.Begin(identity);
	}

	override void OnInit()
	{
		super.OnInit();
		AGA_Server.Get();
		AGA_Server.Get().Log("VPP sync: aktywny");
		AGA_Recipes.Apply(AGA_Recipes.Cfg().Disabled);
		AGA_Territory.Init();
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
		if (identity && player) AGA_Survival.SaveStam(player, identity.GetPlainId());
		if (identity) AGA_Verify.Forget(identity.GetPlainId());
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
		AGA_Survival.Start();
		AGA_Verify.OnSpawn(identity);
		if (identity) AGA_Survival.LoadStam(player, identity.GetPlainId());
		AGA_Territory.AGA_SyncPl(player);
		if (identity)
			GetRPCManager().SendRPC("AGA", "AGA_Zones", new Param1<ref AGA_ZoneDb>(AGA_Territory.Zones()), true, identity);
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

	protected bool m_AGA_PDown;
	protected ref AGA_GroupMenu m_AGA_GroupMenu;
	protected ref AGA_RulesMenu m_AGA_RulesMenu;

	void MissionGameplay()
	{
		GetRPCManager().AddRPC("AGA", "AGA_OnRecipes", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("AGA", "AGA_Zones", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("AGA", "AGA_OnGroupView", this, SingleplayerExecutionType.Client);
		GetRPCManager().AddRPC("AGA", "AGA_OnFlyBy", this, SingleplayerExecutionType.Client);
	}

	void AGA_Zones(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref AGA_ZoneDb> data;
		if (ctx.Read(data) && data.param1) AGA_Territory.s_Zones = data.param1;
	}

	void AGA_OnFlyBy(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param4<vector, vector, int, int> d;
		if (!ctx.Read(d)) return;
		AGA_FlyBy.Start(d.param1, d.param2, d.param3, d.param4);
	}

	void AGA_OnGroupView(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<ref AGA_GroupView> data;
		if (!ctx.Read(data) || !data.param1) return;
		AGA_GroupMenu.s_View = data.param1;
		if (AGA_GroupMenu.s_Open) AGA_GroupMenu.s_Open.Fill();
	}

	void AGA_ToggleGroup()
	{
		UIScriptedMenu cur = GetGame().GetUIManager().GetMenu();
		if (cur && AGA_GroupMenu.Cast(cur))
		{
			cur.Close();
			return;
		}
		if (cur) return;
		m_AGA_GroupMenu = new AGA_GroupMenu();
		GetGame().GetUIManager().ShowScriptedMenu(m_AGA_GroupMenu, null);
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
		UAInput gIn = GetUApi().GetInputByName("UAAGAGroup");
		bool pd = gIn && gIn.LocalPress();
		if ((pd && !m_AGA_PDown) || AGA_Territory.s_OpenMenu)
		{
			AGA_Territory.s_OpenMenu = false;
			AGA_ToggleGroup();
		}
		m_AGA_PDown = pd;
		// regulamin dopiero po weryfikacji anty-cheat
		if (AGA_VerifyMenu.s_Pending) return;
		if (m_AGA_RulesShown) return;
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (!p || !p.IsAlive()) return;
		m_AGA_RulesTimer += timeslice;
		if (m_AGA_RulesTimer < 1.5 || GetGame().GetUIManager().GetMenu()) return;
		m_AGA_RulesShown = true;
		m_AGA_RulesMenu = new AGA_RulesMenu();
		GetGame().GetUIManager().ShowScriptedMenu(m_AGA_RulesMenu, null);
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
