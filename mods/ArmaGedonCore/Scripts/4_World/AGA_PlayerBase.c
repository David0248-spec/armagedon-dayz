// ARMA-GEDON Admin – RPC, leczenie, log walki
modded class PlayerBase
{
	// walka: strzał albo trafienie przez gracza w ostatnich 60 s
	protected int m_AGA_CombatUntil;
	// walka z graczem: 10 min, strzal / walka z zombie: 1 min (dluzszy licznik nie jest skracany)
	void AGA_MarkCombat(int ms = 600000) { int t = GetGame().GetTime() + ms; if (t > m_AGA_CombatUntil) m_AGA_CombatUntil = t; }
	bool AGA_InCombat() { return GetGame().GetTime() < m_AGA_CombatUntil; }


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
				case AGA_RPC_LOGS:
					AGA_Server.Get().OnLogs(this, sender);
					break;
				case AGA_RPC_CAM:
				{
					float camDist;
					if (ctx.Read(camDist) && !AGA_Server.Get().IsStaff(sender))
						AGA_Server.Get().OnCamReport(sender, camDist);
					break;
				}
				case AGA_RPC_CLOG:
				{
					string msg;
					if (ctx.Read(msg) && AGA_Server.Get().IsAdmin(sender))
						AGA_Server.Get().Log("[KLIENT " + sender.GetName() + "] " + msg);
					break;
				}
			}
			return;
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

	protected PlayerBase AGA_RootPlayer(Object o)
	{
		EntityAI e = EntityAI.Cast(o);
		if (!e) return null;
		PlayerBase pb = PlayerBase.Cast(e);
		if (pb) return pb;
		return PlayerBase.Cast(e.GetHierarchyRootPlayer());
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer())
			return;
		PlayerBase shooter = AGA_RootPlayer(source);
		if (shooter && shooter != this)
		{
			AGA_MarkCombat();
			shooter.AGA_MarkCombat();
		}
		if (!shooter || shooter == this)
			return;
		float dmg = 0;
		if (damageResult)
			dmg = damageResult.GetDamage(dmgZone, "Health");
		string weapon = "?";
		if (source)
			weapon = source.GetDisplayName();
		float dist = vector.Distance(shooter.GetPosition(), GetPosition());
		AGA_Server.Get().OnHitMarker(ModelToWorld(modelPos), AGA_Server.Get().NameOf(this));
		AGA_Server.Get().CombatLog("TRAFIENIE " + AGA_Server.Get().NameOf(shooter) + " -> " + AGA_Server.Get().NameOf(this) + " | " + weapon + " | " + dmgZone + " | " + Math.Round(dist).ToString() + " m | dmg " + Math.Round(dmg).ToString());
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		if (!GetGame().IsServer())
			return;
		PlayerBase k = AGA_RootPlayer(killer);
		string how = "?";
		if (killer)
			how = killer.GetDisplayName();
		string why = "You died";
		if (k && k != this)
			why = "Killed by " + AGA_Server.Get().NameOf(k) + " | " + how + " | " + Math.Round(vector.Distance(k.GetPosition(), GetPosition())).ToString() + " m";
		else if (killer && killer != this)
			why = "Cause of death: " + how;
		if (GetIdentity())
			NotificationSystem.SendNotificationToPlayerIdentityExtended(GetIdentity(), 12, "YOU DIED", why, "set:dayz_gui image:icon_info");
		if (k && k != this)
		{
			float dist = vector.Distance(k.GetPosition(), GetPosition());
			AGA_Server.Get().CombatLog("ZABOJSTWO " + AGA_Server.Get().NameOf(k) + " zabil " + AGA_Server.Get().NameOf(this) + " | " + how + " | " + Math.Round(dist).ToString() + " m");
		}
		else
		{
			AGA_Server.Get().CombatLog("SMIERC " + AGA_Server.Get().NameOf(this) + " | " + how);
		}
	}
}

modded class Weapon_Base
{
	override void EEFired(int muzzleType, int mode, string ammoType)
	{
		super.EEFired(muzzleType, mode, ammoType);
		if (!GetGame().IsDedicatedServer())
			return;
		PlayerBase pb = PlayerBase.Cast(GetHierarchyRootPlayer());
		if (!pb)
			return;
		pb.AGA_MarkCombat(60000);
	}
}
