// ARMA-GEDON – ładunek C4 z zapalnikiem czasowym
// Ładunek + budzik w slocie + zestaw do naprawy elektroniki (50%) = ładunek czasowy.
// Odliczanie 10 s z tykaniem, rozbrojenie szczypcami w trakcie odliczania 6 s.
class AGA_C4Rules
{
	static const float COUNTDOWN      = 10.0;
	static const float DEFUSE         = 6.0;
	static const float KIT_COST       = 50.0;
	static const string TICK_SOUND    = "AGA_TimerTick_SoundSet";
	static const string SLOT_CLOCK    = "TriggerAlarmClock";
	static const string DEFAULT_CLOCK = "AlarmClock_Red";
}

modded class Plastic_Explosive
{
	protected bool m_AGA_Count;
	protected int m_AGA_CountMs;
	protected ref Timer m_AGA_SrvTimer;
	protected ref Timer m_AGA_TickStop;
	protected EffectSound m_AGA_Tick;
	protected bool m_AGA_WasCount;

	void Plastic_Explosive()
	{
		RegisterNetSyncVariableBool("m_AGA_Count");
		RegisterNetSyncVariableInt("m_AGA_CountMs", 0, 3600000);
	}

	override void EEDelete(EntityAI parent)
	{
		AGA_StopAll();
		super.EEDelete(parent);
	}

	override void EEKilled(Object killer)
	{
		AGA_Cancel();
		super.EEKilled(killer);
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		if (newLevel == GameConstants.STATE_RUINED) AGA_Cancel();
		super.EEHealthLevelChanged(oldLevel, newLevel, zone);
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (m_AGA_Count && !m_AGA_WasCount) AGA_StartTick();
		else if (!m_AGA_Count && m_AGA_WasCount) AGA_StopTick();
		m_AGA_WasCount = m_AGA_Count;
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (slotId == InventorySlots.GetSlotIdFromString(AGA_C4Rules.SLOT_CLOCK))
			return attachment && attachment.IsInherited(AlarmClock_ColorBase) && !GetArmed() && !IsRuined();
		return super.CanReceiveAttachment(attachment, slotId);
	}

	override bool CanDisplayAttachmentSlot(int slot_id)
	{
		if (InventorySlots.GetSlotName(slot_id) == AGA_C4Rules.SLOT_CLOCK) return true;
		return super.CanDisplayAttachmentSlot(slot_id);
	}

	bool AGA_HasClock()
	{
		EntityAI a = FindAttachmentBySlotName(AGA_C4Rules.SLOT_CLOCK);
		return a && a.IsInherited(AlarmClock_ColorBase);
	}

	bool AGA_Counting() { return m_AGA_Count; }

	bool AGA_CanDefuseWith(ItemBase item)
	{
		return item && item.IsInherited(Pliers) && m_AGA_Count && GetArmed() && !IsRuined();
	}

	bool AGA_CanVanillaDisarm(ItemBase item)
	{
		if (m_AGA_Count) return false;
		if (!item || !item.IsInherited(Pliers)) return false;
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(this);
		if (t) return t.AGA_CanDismantle();
		return true;
	}

	void AGA_StartCountdown()
	{
		if (!GetGame().IsServer() || m_AGA_Count) return;
		m_AGA_CountMs = Math.Round(AGA_C4Rules.COUNTDOWN * 1000.0);
		m_AGA_Count = true;
		SetSynchDirty();
		if (!m_AGA_SrvTimer) m_AGA_SrvTimer = new Timer(CALL_CATEGORY_SYSTEM);
		m_AGA_SrvTimer.Run(AGA_C4Rules.COUNTDOWN, this, "AGA_OnCountdownEnd");
		if (!GetGame().IsDedicatedServer()) AGA_StartTick();
	}

	void AGA_Cancel()
	{
		if (m_AGA_SrvTimer) m_AGA_SrvTimer.Stop();
		m_AGA_Count = false;
		m_AGA_CountMs = 0;
		SetSynchDirty();
		AGA_StopTick();
	}

	void AGA_OnCountdownEnd()
	{
		if (!GetGame().IsServer()) return;
		AGA_StopAll();
		m_AGA_Count = false;
		SetSynchDirty();
		AGA_Detonate();
	}

	void AGA_Detonate() {}

	override void OnDisarmed(bool pWithTool)
	{
		AGA_Cancel();
		super.OnDisarmed(pWithTool);
	}

	protected void AGA_StopAll()
	{
		if (m_AGA_SrvTimer) m_AGA_SrvTimer.Stop();
		AGA_StopTick();
	}

	protected void AGA_StartTick()
	{
		if (GetGame().IsDedicatedServer() || m_AGA_CountMs <= 0) return;
		AGA_StopTick();
		m_AGA_Tick = SEffectManager.PlaySound(AGA_C4Rules.TICK_SOUND, GetPosition(), 0, 0, true);
		if (!m_AGA_TickStop) m_AGA_TickStop = new Timer(CALL_CATEGORY_SYSTEM);
		m_AGA_TickStop.Run(m_AGA_CountMs / 1000.0, this, "AGA_StopTick");
	}

	void AGA_StopTick()
	{
		if (m_AGA_TickStop) m_AGA_TickStop.Stop();
		if (m_AGA_Tick)
		{
			m_AGA_Tick.SoundStop();
			SEffectManager.DestroySound(m_AGA_Tick);
			m_AGA_Tick = null;
		}
	}
}

class AGA_TimedExplosive : Plastic_Explosive
{
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (!GetGame().IsServer()) return;
		if (AGA_OnGround())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_ResetAfterRestart, 0, false);
		else
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_EnsureClock, 500, false);
	}

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_EnsureClock, 500, false);
	}

	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionArmTimed);
	}

	override bool IsDeployable()
	{
		return !IsRuined() && super.IsDeployable();
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (slotId == InventorySlots.GetSlotIdFromString(AGA_C4Rules.SLOT_CLOCK))
			return attachment && attachment.IsInherited(AlarmClock_ColorBase) && !FindAttachmentBySlotName(AGA_C4Rules.SLOT_CLOCK);
		return false;
	}

	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (attachment && attachment.IsInherited(AlarmClock_ColorBase) && GetArmed()) return false;
		return super.CanReleaseAttachment(attachment);
	}

	override bool CanDisplayAttachmentSlot(int slot_id)
	{
		return InventorySlots.GetSlotName(slot_id) == AGA_C4Rules.SLOT_CLOCK;
	}

	// w trakcie odliczania ładunku nie da się zestrzelić
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef)) return false;
		return !AGA_BlockShot(damageType);
	}

	bool AGA_BlockShot(int damageType)
	{
		return damageType == DamageType.FIRE_ARM && GetArmed() && AGA_Counting();
	}

	bool AGA_CanDismantle()
	{
		return !GetHierarchyParent() && !IsRuined() && !GetArmed() && !AGA_Counting();
	}

	bool AGA_CanArm()
	{
		return !GetHierarchyParent() && !IsRuined() && !GetArmed() && !AGA_Counting();
	}

	void AGA_Arm()
	{
		if (!GetGame().IsServer() || !AGA_CanArm()) return;
		Arm();
		AGA_StartCountdown();
	}

	void AGA_Dismantle()
	{
		if (!GetGame().IsServer() || !AGA_CanDismantle()) return;
		AGA_ReplaceWithPlain();
	}

	void AGA_Defuse()
	{
		if (!GetGame().IsServer() || IsRuined()) return;
		AGA_Cancel();
		OnBeforeDisarm();
		Disarm(true);
		AGA_ReplaceWithPlain();
	}

	void AGA_ResetAfterRestart()
	{
		if (!GetGame().IsServer() || IsRuined() || !AGA_OnGround()) return;
		AGA_Cancel();
		if (GetArmed())
		{
			OnBeforeDisarm();
			Disarm(false);
		}
		AGA_EnsureClock();
	}

	override void AGA_Detonate()
	{
		if (!GetGame().IsServer() || IsRuined()) return;
		OnActivatedByItem(this);
	}

	protected bool AGA_OnGround()
	{
		InventoryLocation il = new InventoryLocation();
		if (!GetInventory().GetCurrentInventoryLocation(il)) return false;
		return il.GetType() == InventoryLocationType.GROUND;
	}

	void AGA_EnsureClock()
	{
		if (!GetGame().IsServer()) return;
		ItemBase clock = ItemBase.Cast(FindAttachmentBySlotName(AGA_C4Rules.SLOT_CLOCK));
		if (!clock) clock = ItemBase.Cast(GetInventory().CreateAttachment(AGA_C4Rules.DEFAULT_CLOCK));
		if (clock)
		{
			clock.LockToParent();
			clock.SetSynchDirty();
			SetSynchDirty();
		}
	}

	protected void AGA_ReplaceWithPlain()
	{
		vector pos = GetPosition();
		vector ori = GetOrientation();
		float hp = GetHealth("", "");
		string clockType = "";
		ItemBase clock = ItemBase.Cast(FindAttachmentBySlotName(AGA_C4Rules.SLOT_CLOCK));
		if (clock)
		{
			clockType = clock.GetType();
			clock.UnlockFromParent();
			GetGame().ObjectDelete(clock);
		}
		Plastic_Explosive plain = Plastic_Explosive.Cast(GetGame().CreateObjectEx("Plastic_Explosive", pos, ECE_PLACE_ON_SURFACE));
		if (plain)
		{
			plain.SetOrientation(ori);
			plain.SetPosition(pos);
			plain.SetHealth("", "", hp);
			plain.SetSynchDirty();
		}
		if (clockType != "") GetGame().CreateObjectEx(clockType, pos, ECE_PLACE_ON_SURFACE);
		GetGame().ObjectDelete(this);
	}
}

modded class AlarmClock_ColorBase
{
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef)) return false;
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(GetHierarchyParent());
		if (!t) return true;
		return !t.AGA_BlockShot(damageType);
	}
}

// --- receptura ---
class AGA_CraftTimedExplosive extends RecipeBase
{
	override void Init()
	{
		m_Name = "Craft Timed Plastic Explosive";
		m_IsInstaRecipe = false;
		m_AnimationLength = 2;
		m_Specialty = -0.02;

		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = -1;
		m_MaxQuantityIngredient[0] = -1;
		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = -1;
		m_MaxQuantityIngredient[1] = -1;

		InsertIngredient(0, "Plastic_Explosive");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = false;
		m_IngredientUseSoftSkills[0] = false;

		InsertIngredient(1, "ElectronicRepairKit");
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;

		m_ResultSetFullQuantity[0] = false;
		m_ResultSetQuantity[0] = -1;
		m_ResultSetHealth[0] = -1;
		m_ResultInheritsHealth[0] = 0;
		m_ResultInheritsColor[0] = -1;
		m_ResultToInventory[0] = -2;
		m_ResultUseSoftSkills[0] = false;
		m_ResultReplacesIngredient[0] = -1;
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		Plastic_Explosive pe = Plastic_Explosive.Cast(ingredients[0]);
		if (!pe || pe.IsInherited(AGA_TimedExplosive) || !pe.AGA_HasClock()) return false;
		if (!player || (player.GetItemInHands() != pe && !player.GetInventory().HasEntityInInventory(pe))) return false;
		InventoryLocation il = new InventoryLocation();
		if (!pe.GetInventory().GetCurrentInventoryLocation(il) || il.GetType() == InventoryLocationType.GROUND) return false;
		ItemBase kit = ItemBase.Cast(ingredients[1]);
		if (!kit || !kit.IsInherited(ElectronicRepairKit)) return false;
		return kit.GetQuantity() >= AGA_C4Rules.KIT_COST;
	}

	override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
	{
		Plastic_Explosive pe = Plastic_Explosive.Cast(ingredients[0]);
		if (!pe || !player) return;
		ItemBase clock = ItemBase.Cast(pe.FindAttachmentBySlotName(AGA_C4Rules.SLOT_CLOCK));
		if (!clock || !clock.IsInherited(AlarmClock_ColorBase)) return;
		clock.UnlockFromParent();
		ItemBase kit = ItemBase.Cast(ingredients[1]);
		if (!kit) return;
		kit.AddQuantity(-AGA_C4Rules.KIT_COST, true);

		AGA_TimedExplosiveLambda lambda = new AGA_TimedExplosiveLambda(pe, "AGA_TimedExplosive", player);
		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			if (player.GetItemInHands() == pe) player.ServerReplaceItemInHandsWithNew(lambda);
			else player.ServerReplaceItemElsewhereWithNewInHands(lambda);
		}
		else
		{
			if (player.GetItemInHands() == pe) player.LocalReplaceItemInHandsWithNew(lambda);
			else player.LocalReplaceItemElsewhereWithNewInHands(lambda);
		}
	}
}

class AGA_TimedExplosiveLambda : TurnItemIntoItemLambda
{
	void AGA_TimedExplosiveLambda(EntityAI old_item, string new_item_type, PlayerBase player)
	{
		SetTransferParams(true, true, true, true, -1);
	}

	override void CopyOldPropertiesToNew(notnull EntityAI old_item, EntityAI new_item)
	{
		super.CopyOldPropertiesToNew(old_item, new_item);
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(new_item);
		if (!t) return;
		ItemBase clock = ItemBase.Cast(t.FindAttachmentBySlotName(AGA_C4Rules.SLOT_CLOCK));
		if (clock)
		{
			clock.LockToParent();
			clock.SetSynchDirty();
		}
		t.SetSynchDirty();
	}
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new AGA_CraftTimedExplosive);
	}
}

// --- akcje ---
class AGA_ActionArmTimed : ActionInteractBase
{
	void AGA_ActionArmTimed()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_PICKUP_HANDS;
		m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_PICKUP_HANDS;
		m_Text = "#STR_ArmExplosive";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINone();
		m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
	}

	override bool UseMainItem() { return false; }
	override bool MainItemAlwaysInHands() { return false; }
	override bool HasProneException() { return true; }
	override bool HasProgress() { return false; }
	override typename GetInputType() { return ContinuousInteractActionInput; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!player || !player.IsAlive() || !target || target.GetParent()) return false;
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(target.GetObject());
		return t && t.AGA_CanArm();
	}

	override void OnExecuteServer(ActionData action_data)
	{
		super.OnExecuteServer(action_data);
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(action_data.m_Target.GetObject());
		if (t) t.AGA_Arm();
	}
}

class AGA_ActionDefuseTimedCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(AGA_C4Rules.DEFUSE);
	}
}

class AGA_ActionDefuseTimed : ActionContinuousBase
{
	void AGA_ActionDefuseTimed()
	{
		m_CallbackClass = AGA_ActionDefuseTimedCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH;
		m_FullBody = true;
		m_Text = "#disarm";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTCursor();
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!target) return false;
		Plastic_Explosive pe = Plastic_Explosive.Cast(target.GetObject());
		return pe && pe.AGA_CanDefuseWith(item);
	}

	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);
		Plastic_Explosive pe = Plastic_Explosive.Cast(action_data.m_Target.GetObject());
		if (pe && pe.GetDisarmSoundset() != string.Empty) pe.StartItemSoundServer(SoundConstants.ITEM_EXPLOSIVE_DISARM);
	}

	override void OnEndServer(ActionData action_data)
	{
		super.OnEndServer(action_data);
		Plastic_Explosive pe = Plastic_Explosive.Cast(action_data.m_Target.GetObject());
		if (pe && pe.GetDisarmSoundset() != string.Empty) pe.StopItemSoundServer(SoundConstants.ITEM_EXPLOSIVE_DISARM);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		Plastic_Explosive pe = Plastic_Explosive.Cast(action_data.m_Target.GetObject());
		if (!pe) return;
		MiscGameplayFunctions.DealAbsoluteDmg(action_data.m_MainItem, UADamageApplied.DEFUSE_TOOLS);
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(pe);
		if (t)
		{
			t.AGA_Defuse();
			return;
		}
		pe.AGA_Cancel();
		pe.OnBeforeDisarm();
		pe.Disarm(true);
	}
}

modded class ActionDisarmExplosive
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!target) return false;
		Plastic_Explosive pe = Plastic_Explosive.Cast(target.GetObject());
		if (!pe) return super.ActionCondition(player, target, item);
		if (AGA_TimedExplosive.Cast(pe)) return pe.AGA_CanVanillaDisarm(item);
		if (!super.ActionCondition(player, target, item)) return false;
		return pe.AGA_CanVanillaDisarm(item);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		AGA_TimedExplosive t = AGA_TimedExplosive.Cast(action_data.m_Target.GetObject());
		if (t)
		{
			MiscGameplayFunctions.DealAbsoluteDmg(action_data.m_MainItem, UADamageApplied.DEFUSE_TOOLS);
			t.AGA_Dismantle();
			return;
		}
		super.OnFinishProgressServer(action_data);
	}
}

modded class Pliers
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionDefuseTimed);
	}
}
