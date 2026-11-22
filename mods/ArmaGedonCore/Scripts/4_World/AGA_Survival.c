// ARMA-GEDON: przetrwanie – zlamana reka (zapisywana), kondycja (stamina jako umiejetnosc), walka z zombie, zamki

// ======================================================================== stale
class AGA_SurvCfg
{
	// zlamana reka
	static const float ARM_CHANCE_BULLET = 0.35;
	static const float ARM_CHANCE_MELEE  = 0.10;
	static const float ARM_SWAY_BROKEN   = 3.5;
	static const float ARM_SWAY_SPLINT   = 1.8;
	static const int   ARM_HEAL_MS       = 300000;   // 5 min po zalozeniu szyny

	// kondycja: zakres i zmiany co 20 s
	static const float STAM_MIN = -15.0;
	static const float STAM_MAX = 15.0;
	static const float STAM_GAIN_SPRINT = 0.04;
	static const float STAM_GAIN_MOVE   = 0.02;
	static const float STAM_LOSS_IDLE   = -0.03;
	static const float STAM_LOSS_BASE   = -0.035;
	static const float STAM_RED_BONUS   = 1.15;
	static const float STAM_DRAIN_FROM  = -7.5;
	static const float STAM_DRAIN_MULT  = 1.5;

}

// ======================================================================== gracz
modded class PlayerBase
{
	int   m_AGAArm;       // 0 zdrowa, 1 zlamana, 2 na szynie
	float m_AGAStam;      // umiejetnosc kondycji (-15..15)
	float m_AGAStamTick;

	override void Init()
	{
		super.Init();
		RegisterNetSyncVariableInt("m_AGAArm", 0, 2);
		RegisterNetSyncVariableFloat("m_AGAStam", AGA_SurvCfg.STAM_MIN, AGA_SurvCfg.STAM_MAX, 2);
	}

	int AGA_Arm() { return m_AGAArm; }
	float AGA_Stam() { return m_AGAStam; }

	void AGA_SetArm(int v)
	{
		if (m_AGAArm == v) return;
		m_AGAArm = v;
		SetSynchDirty();
		if (v == 1 && GetIdentity())
			NotificationSystem.SendNotificationToPlayerIdentityExtended(GetIdentity(), 8, "Broken arm", "Your aim is shaking. Use a splint to stabilise the arm.", "set:dayz_gui image:icon_fracture");
		if (v == 2)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_HealArm, AGA_SurvCfg.ARM_HEAL_MS, false);
		if (v == 0 && GetIdentity())
			NotificationSystem.SendNotificationToPlayerIdentityExtended(GetIdentity(), 6, "Arm healed", "Your arm is fine again.", "set:dayz_gui image:icon_info");
	}

	void AGA_HealArm() { if (m_AGAArm == 2) AGA_SetArm(0); }

	void AGA_SetStam(float v)
	{
		v = Math.Clamp(v, AGA_SurvCfg.STAM_MIN, AGA_SurvCfg.STAM_MAX);
		if (Math.AbsFloat(v - m_AGAStam) < 0.0001) return;
		m_AGAStam = v;
		SetSynchDirty();
		if (GetStaminaHandler()) GetStaminaHandler().RecalculateStaminaCap();
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer() || !IsAlive() || m_AGAArm != 0) return;
		string z = dmgZone;
		z.ToLower();
		if (z.IndexOf("arm") < 0 && z.IndexOf("hand") < 0) return;
		float ch = 0;
		if (damageType == DamageType.FIRE_ARM) ch = AGA_SurvCfg.ARM_CHANCE_BULLET;
		else if (damageType == DamageType.CLOSE_COMBAT) ch = AGA_SurvCfg.ARM_CHANCE_MELEE;
		if (ch > 0 && Math.RandomFloat01() < ch) AGA_SetArm(1);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (GetGame().IsDedicatedServer() || this != GetGame().GetPlayer()) return;
		AGA_ShowArmBadge();
		if (GetStaminaHandler()) GetStaminaHandler().RecalculateStaminaCap();
	}

	void AGA_ShowArmBadge()
	{
		if (!GetVirtualHud()) return;
		DisplayElementBadge b = DisplayElementBadge.Cast(GetVirtualHud().GetElement(eDisplayElements.DELM_BADGE_FRACTURE));
		if (!b) return;
		if (m_AGAArm > 0) b.SetLevel(eBadgeLevel.FIRST);
		else if (GetBrokenLegs() == eBrokenLegs.NO_BROKEN_LEGS) b.SetLevel(eBadgeLevel.NONE);
	}
}

// ikonka zlamania nie znika, gdy noga zdrowa, a reka zlamana
modded class FracturedLegNotfr
{
	override void HideBadge()
	{
		PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
		if (p && p.AGA_Arm() > 0) return;
		super.HideBadge();
	}
}

// celowanie ze zlamana reka
modded class DayZPlayerImplementAiming
{
	override protected float CalculateSwayMultiplier()
	{
		float r = super.CalculateSwayMultiplier();
		if (m_PlayerPb)
		{
			if (m_PlayerPb.AGA_Arm() == 1) r *= AGA_SurvCfg.ARM_SWAY_BROKEN;
			else if (m_PlayerPb.AGA_Arm() == 2) r *= AGA_SurvCfg.ARM_SWAY_SPLINT;
		}
		return r;
	}
}

// zwykla szyna z gry leczy tez reke (najpierw noga, jesli obie zlamane)
modded class ActionSplintSelf
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (super.ActionCondition(player, target, item)) return true;
		return player && player.AGA_Arm() == 1 && player.GetBrokenLegs() != eBrokenLegs.BROKEN_LEGS;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		if (action_data.m_Player.GetBrokenLegs() == eBrokenLegs.BROKEN_LEGS)
		{
			super.OnFinishProgressServer(action_data);
			return;
		}
		if (action_data.m_Player.AGA_Arm() == 1)
		{
			action_data.m_MainItem.TransferModifiers(action_data.m_Player);
			action_data.m_Player.AGA_SetArm(2);
			action_data.m_MainItem.Delete();
		}
	}
}

// ======================================================================== kondycja
modded class StaminaHandler
{
	override void RecalculateStaminaCap()
	{
		super.RecalculateStaminaCap();
		if (!m_Player) return;
		m_StaminaCap = Math.Max(m_StaminaCap + m_Player.AGA_Stam(), CfgGameplayHandler.GetStaminaMinCap());
	}

	override void DepleteStaminaEx(EStaminaModifiers modifier, float dT = -1, float coef = 1.0)
	{
		if (m_Player && m_Player.AGA_Stam() <= AGA_SurvCfg.STAM_DRAIN_FROM) coef *= AGA_SurvCfg.STAM_DRAIN_MULT;
		super.DepleteStaminaEx(modifier, dT, coef);
	}

	override void Update(float deltaT, int pCurrentCommandID)
	{
		super.Update(deltaT, pCurrentCommandID);
		if (!m_Player || !GetGame().IsServer()) return;
		m_Player.m_AGAStamTick += deltaT;
		if (m_Player.m_AGAStamTick < 20) return;
		m_Player.m_AGAStamTick = 0;
		AGA_StamStep();
	}

	protected void AGA_StamStep()
	{
		float cur = m_Player.AGA_Stam();
		float d = 0;
		if (AGA_Territory.FlagAt(m_Player.GetPosition()))
		{
			d = AGA_SurvCfg.STAM_LOSS_BASE;                 // siedzenie w bazie oslabia kondycje
		}
		else if (m_Player.GetParent() && (m_Player.GetParent().IsInherited(Car) || m_Player.GetParent().IsInherited(Boat)))
		{
			return;                                         // w pojezdzie bez zmian
		}
		else
		{
			HumanMovementState ms = new HumanMovementState();
			m_Player.GetMovementState(ms);
			if (m_Stamina < m_StaminaCap * 0.85) d = AGA_SurvCfg.STAM_GAIN_SPRINT;       // wysilek
			else if (ms.m_iMovement < DayZPlayerConstants.MOVEMENTIDX_WALK) d = AGA_SurvCfg.STAM_LOSS_IDLE; // stanie
			else d = AGA_SurvCfg.STAM_GAIN_MOVE;                                          // marsz / trucht
			if (d > 0 && cur < 0) d *= AGA_SurvCfg.STAM_RED_BONUS;
		}
		if (d < 0 && cur <= AGA_SurvCfg.STAM_MIN) return;
		if (d > 0 && cur >= AGA_SurvCfg.STAM_MAX) return;
		m_Player.AGA_SetStam(cur + d);
	}
}

class AGA_StamSave
{
	float Value;
	int Arm;
}

class AGA_Survival
{
	static bool s_Started;
	static string Dir() { return "$profile:ArmaGedonCore/stamina"; }

	static void Start()
	{
		if (s_Started || !GetGame().IsServer()) return;
		s_Started = true;
		if (!FileExist("$profile:ArmaGedonCore")) MakeDirectory("$profile:ArmaGedonCore");
		if (!FileExist(Dir())) MakeDirectory(Dir());
	}

	static void LoadStam(PlayerBase p, string uid)
	{
		if (!p || uid == "") return;
		string f = Dir() + "/" + uid + ".json";
		if (!FileExist(f)) return;
		AGA_StamSave s = new AGA_StamSave();
		JsonFileLoader<AGA_StamSave>.JsonLoadFile(f, s);
		p.AGA_SetStam(s.Value);
		if (s.Arm > 0) p.AGA_SetArm(s.Arm);
	}

	static void SaveStam(PlayerBase p, string uid)
	{
		if (!p || uid == "") return;
		AGA_StamSave s = new AGA_StamSave();
		s.Value = p.AGA_Stam();
		s.Arm = p.AGA_Arm();
		JsonFileLoader<AGA_StamSave>.JsonSaveFile(Dir() + "/" + uid + ".json", s);
	}

}

// ======================================================================== walka z zombie = 1 min
modded class ZombieBase
{
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
		if (!GetGame().IsServer() || !source) return;
		PlayerBase pb = PlayerBase.Cast(source.GetHierarchyRootPlayer());
		if (pb) pb.AGA_MarkCombat(60000);
	}
}

// ======================================================================== zamek z gry -> nasz CodeLock
// Zamkow z gry nie da sie zakladac. Luzne zamieniaja sie od razu w CodeLock, a zalozone na bramie
// zamieniaja sie, gdy tylko wlasciciel je otworzy (zamkniety zamek zostaje, zeby nie otworzyc nikomu bazy).
modded class CombinationLock
{
	// CodeLock z naszego moda dziedziczy po CombinationLock – dotyczy to TYLKO zamkow z gry
	bool AGA_IsVanillaLock()
	{
		string t = GetType();
		return t == "CombinationLock" || t == "CombinationLock4";
	}

	override bool CanPutAsAttachment(EntityAI parent)
	{
		if (AGA_IsVanillaLock()) return false;
		return super.CanPutAsAttachment(parent);
	}

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer() && AGA_IsVanillaLock())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Swap, 3000, false);
	}

	void AGA_Swap()
	{
		if (IsPendingDeletion() || !AGA_IsVanillaLock()) return;
		EntityAI parent = GetHierarchyParent();
		bool onBase = parent && (parent.IsInherited(BaseBuildingBase) || parent.IsInherited(TentBase));
		if (onBase && IsLocked())
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Swap, 60000, false);
			return;
		}
		vector pos = GetPosition();
		if (parent) pos = parent.GetPosition();
		GetGame().ObjectDelete(this);
		EntityAI cl;
		if (parent && !onBase) cl = parent.GetInventory().CreateInInventory("CodeLock");
		if (!cl) GetGame().CreateObjectEx("CodeLock", pos, ECE_PLACE_ON_SURFACE);
		if (onBase) AGA_Server.Get().Log("[ZAMEK] zamek z gry zdjety z otwartej bramy i zamieniony na CodeLock @ " + pos.ToString());
	}
}
