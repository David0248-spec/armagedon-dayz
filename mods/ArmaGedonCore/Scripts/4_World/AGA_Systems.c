// ARMA-GEDON: systemy rozgrywki – kondycja (soft skill), limit wysokości wieży, kontrola kamery

// ================= KONDYCJA =================
// Bonus do maksymalnej wytrzymałości w zakresie -15..+15, liczony co 20 s po stronie serwera.
class AGA_Stamina
{
	static const float MAX = 15.0;
	static const float TICK = 20.0;
	static const float GAIN_EFFORT = 0.04;    // wysiłek (wytrzymałość < 85% maksimum)
	static const float GAIN_MOVING = 0.02;    // ruch bez wysiłku
	static const float LOSS_IDLE = -0.04;     // stanie w miejscu (jak KK lean_config)
	static const float LOSS_BASE = -0.06;     // przebywanie na terenie bazy (jak KK lean_config)
	static const float NEGATIVE_BOOST = 1.15; // szybsza odbudowa przy ujemnym bonusie
	static const float MAX_PENALTY = 0.5;     // maks. dodatkowe zużycie jedzenia/wody
	static const float DRAIN_THRESHOLD = -7.5;
	static const float DRAIN_MULT = 1.5;
	static const float TERRITORY_RADIUS = 60;

	static ref map<string, float> s_Saved;
	static string s_Path = "$profile:ArmaGedonCore/stamina.json";

	// klient: ostatnia wartosc z serwera dla strzalek w HUD (5_Mission/AGA_StaminaArrows.c)
	static float s_ClientValue;
	static bool s_ClientDirty;

	static void Load()
	{
		if (s_Saved) return;
		s_Saved = new map<string, float>;
		if (FileExist(s_Path)) JsonFileLoader<map<string, float>>.JsonLoadFile(s_Path, s_Saved);
	}

	static void Save()
	{
		if (!s_Saved) return;
		JsonFileLoader<map<string, float>>.JsonSaveFile(s_Path, s_Saved);
	}

	static float Get(string id)
	{
		Load();
		if (s_Saved.Contains(id)) return s_Saved.Get(id);
		return 0;
	}

	static void Set(string id, float v)
	{
		Load();
		s_Saved.Set(id, v);
	}

	static bool InTerritory(vector pos)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(pos, TERRITORY_RADIUS, objs, null);
		foreach (Object o : objs)
			if (TerritoryFlag.Cast(o)) return true;
		return false;
	}
}

modded class StaminaHandler
{
	// Cap liczony OD ZERA (waga + bonus), tak jak w KK_StaminaSoftSkill.
	// Wariant "super() + dodaj bonus" dublowal wartosc, bo AGA_Survival.c robil to samo
	// i oba override sie wykonywaly - stad pasek skakal i znikal co 20 s.
	override void RecalculateStaminaCap()
	{
		if (!m_Player)
		{
			super.RecalculateStaminaCap();
			return;
		}
		float skill = m_Player.AGA_GetStaminaSkill();
		if (m_PlayerLoad >= CfgGameplayHandler.GetStaminaWeightLimitThreshold())
			m_StaminaCap = Math.Max(CfgGameplayHandler.GetStaminaMax() - (((m_PlayerLoad - CfgGameplayHandler.GetStaminaWeightLimitThreshold()) / GameConstants.STAMINA_KG_TO_GRAMS) * CfgGameplayHandler.GetStaminaKgToStaminaPercentPenalty()), CfgGameplayHandler.GetStaminaMinCap());
		else
			m_StaminaCap = CfgGameplayHandler.GetStaminaMax();
		m_StaminaCap = Math.Max(m_StaminaCap + skill, CfgGameplayHandler.GetStaminaMinCap());
		m_StaminaCapSynced = m_StaminaCap;
	}

	override void DepleteStaminaEx(EStaminaModifiers modifier, float dT = -1, float coef = 1.0)
	{
		if (m_Player && m_Player.AGA_GetStaminaSkill() <= AGA_Stamina.DRAIN_THRESHOLD)
			coef = coef * AGA_Stamina.DRAIN_MULT;
		super.DepleteStaminaEx(modifier, dT, coef);
	}

	override protected void ProcessMovementState()
	{
		super.ProcessMovementState();
		if (m_StaminaDelta < 0 && m_Player && m_Player.AGA_GetStaminaSkill() <= AGA_Stamina.DRAIN_THRESHOLD)
			m_StaminaDelta = m_StaminaDelta * AGA_Stamina.DRAIN_MULT;
	}

	float AGA_GetStamina() { return m_Stamina; }
	float AGA_GetCap() { return m_StaminaCap; }
}

modded class PlayerBase
{
	protected float m_AGA_StaminaSkill;
	protected float m_AGA_StaminaTimer;
	protected bool m_AGA_StaminaLoaded;
	protected float m_AGA_CamTimer;

	void PlayerBase()
	{
		RegisterNetSyncVariableFloat("m_AGA_StaminaSkill", -AGA_Stamina.MAX, AGA_Stamina.MAX, 2);
	}

	float AGA_GetStaminaSkill()
	{
		return m_AGA_StaminaSkill;
	}

	void AGA_SetStaminaSkill(float v)
	{
		m_AGA_StaminaSkill = Math.Clamp(v, -AGA_Stamina.MAX, AGA_Stamina.MAX);
		SetSynchDirty();
		if (GetIdentity()) AGA_Stamina.Set(GetIdentity().GetPlainId(), m_AGA_StaminaSkill);
		StaminaHandler sh = GetStaminaHandler();
		if (sh) sh.RecalculateStaminaCap();
	}

	// Klient: nowa wartosc z serwera -> przelicz pasek i pokaz strzalke wzrostu/spadku (jak KK DisplayStaminaArrows)
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (GetGame().IsDedicatedServer() || this != GetGame().GetPlayer()) return;
		StaminaHandler sh = GetStaminaHandler();
		if (sh) sh.RecalculateStaminaCap();
		AGA_Stamina.s_ClientValue = m_AGA_StaminaSkill;
		AGA_Stamina.s_ClientDirty = true;
	}

	override void OnScheduledTick(float deltaTime)
	{
		super.OnScheduledTick(deltaTime);
		if (GetGame().IsServer())
			AGA_StaminaServerTick(deltaTime);
		else if (GetGame().GetPlayer() == this)
			AGA_CamCheck(deltaTime);
	}

	protected void AGA_StaminaServerTick(float deltaTime)
	{
		if (!IsAlive() || !GetIdentity()) return;
		if (!m_AGA_StaminaLoaded)
		{
			m_AGA_StaminaLoaded = true;
			m_AGA_StaminaSkill = Math.Clamp(AGA_Stamina.Get(GetIdentity().GetPlainId()), -AGA_Stamina.MAX, AGA_Stamina.MAX);
			SetSynchDirty();
		}
		// ujemny bonus = większe zużycie jedzenia i wody
		if (m_AGA_StaminaSkill < 0)
		{
			float drain = (-m_AGA_StaminaSkill / AGA_Stamina.MAX * AGA_Stamina.MAX_PENALTY) * deltaTime;
			GetStatEnergy().Add(-drain);
			GetStatWater().Add(-drain);
		}
		m_AGA_StaminaTimer += deltaTime;
		if (m_AGA_StaminaTimer <= AGA_Stamina.TICK) return;
		m_AGA_StaminaTimer = 0;

		float v = m_AGA_StaminaSkill;
		StaminaHandler sh = GetStaminaHandler();
		if (AGA_Stamina.InTerritory(GetPosition()))
		{
			if (v > -AGA_Stamina.MAX) v += AGA_Stamina.LOSS_BASE;
		}
		else if (IsInVehicle())
		{
			return;
		}
		else if (sh && sh.AGA_GetStamina() < sh.AGA_GetCap() * 0.85)
		{
			if (v < AGA_Stamina.MAX && v >= 0) v += AGA_Stamina.GAIN_EFFORT;
			else v += AGA_Stamina.GAIN_EFFORT * AGA_Stamina.NEGATIVE_BOOST;
		}
		else
		{
			HumanMovementState ms = new HumanMovementState();
			GetMovementState(ms);
			if (ms.m_iMovement < DayZPlayerConstants.MOVEMENTIDX_WALK)
			{
				if (v > -AGA_Stamina.MAX) v += AGA_Stamina.LOSS_IDLE;
			}
			else
			{
				if (v < AGA_Stamina.MAX && v >= 0) v += AGA_Stamina.GAIN_MOVING;
				else v += AGA_Stamina.GAIN_MOVING * AGA_Stamina.NEGATIVE_BOOST;
			}
		}
		AGA_SetStaminaSkill(v);
	}

	// ================= KONTROLA KAMERY (anty-freecam) =================
	protected void AGA_CamCheck(float deltaTime)
	{
		m_AGA_CamTimer += deltaTime;
		if (m_AGA_CamTimer <= 9) return;
		m_AGA_CamTimer = 0;
		if (!IsAlive() || IsInVehicle() || IsUnconscious()) return;
		float distance = vector.Distance(GetPosition(), GetGame().GetCurrentCameraPosition());
		if (distance > 5 && distance < 1300)
		{
			ScriptRPC rpc = new ScriptRPC();
			rpc.Write(distance);
			rpc.Send(this, AGA_RPC_CAM, true, null);
		}
	}
}

// ================= LIMIT WYSOKOŚCI WIEŻY =================
// Zestaw wieży można postawić najwyżej 1 m nad terenem (sprawdzane też po stronie serwera).
modded class WatchtowerKit
{
	static const float AGA_MAX_HEIGHT = 1.0;

	protected float AGA_HeightAboveTerrain(vector position)
	{
		return position[1] - GetGame().SurfaceY(position[0], position[2]);
	}

	override bool CanBePlaced(Man player, vector position)
	{
		if (!super.CanBePlaced(player, position)) return false;
		return AGA_HeightAboveTerrain(position) <= AGA_MAX_HEIGHT;
	}

	override string CanBePlacedFailMessage(Man player, vector position)
	{
		float h = AGA_HeightAboveTerrain(position);
		if (h > AGA_MAX_HEIGHT)
			return "Watchtower too high: " + h.ToString() + " m above terrain (max 1 m)";
		return super.CanBePlacedFailMessage(player, position);
	}
}
