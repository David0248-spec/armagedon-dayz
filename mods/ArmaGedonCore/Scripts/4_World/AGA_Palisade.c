// ARMA-GEDON: palisada (kit -> hologram -> sciana z bali), raid wg naszej tabeli, rozbiorka siekiera
class AGA_PalisadeKit extends KitBase
{
	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);
		if (!GetGame().IsServer()) return;
		EntityAI w = EntityAI.Cast(GetGame().CreateObjectEx("AGA_Palisade", position, ECE_PLACE_ON_SURFACE));
		if (w)
		{
			w.SetPosition(position);
			w.SetOrientation(orientation);
		}
		HideAllSelections();
		PlayerBase pb = PlayerBase.Cast(player);
		if (pb && pb.GetIdentity())
			AGA_Server.Get().Log("[BAZA] palisada postawiona przez " + pb.GetIdentity().GetName() + " @ " + position.ToString());
	}

	override bool DoPlacingHeightCheck() { return false; }

	override void DisassembleKit(ItemBase item)
	{
		if (IsHologram()) return;
		ItemBase pl = ItemBase.Cast(GetGame().CreateObjectEx("WoodenPlank", GetPosition(), ECE_PLACE_ON_SURFACE));
		if (pl) pl.SetQuantity(10);
	}

	override string GetDeploySoundset() { return "putDown_FenceKit_SoundSet"; }
	override string GetLoopDeploySoundset() { return "BarbedWire_Deploy_loop_SoundSet"; }
}

class AGA_PalisadeKitPlacing extends AGA_PalisadeKit {}

class AGA_Palisade extends ItemBase
{
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool IsTakeable() { return false; }
	override bool CanDisplayCargo() { return false; }

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!GetGame().IsServer()) return false;
		if (AGA_Raid.Block(this, source)) return false;
		if (damageType == DamageType.CLOSE_COMBAT) return false;
		int n = AGA_RaidDmg.Hits(ammo, false);
		if (n <= 0) return false;
		DecreaseHealth("", "Health", GetMaxHealth("", "Health") / n);
		if (GetHealth("", "Health") <= 0) GetGame().ObjectDelete(this);
		return false;
	}

	override void SetActions()
	{
		super.SetActions();
	}
}

// rozbiorka palisady siekiera we wlasnym terytorium (zwraca 10 desek)
class AGA_ActionDismantlePalisadeCB : ActionContinuousBaseCB
{
	override void CreateActionComponent() { m_ActionData.m_ActionComponent = new CAContinuousTime(10); }
}

class AGA_ActionDismantlePalisade : ActionContinuousBase
{
	void AGA_ActionDismantlePalisade()
	{
		m_CallbackClass = AGA_ActionDismantlePalisadeCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_ASSEMBLE;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
		m_Text = "Dismantle palisade";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTCursor(UAMaxDistances.BASEBUILDING);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!target) return false;
		AGA_Palisade w = AGA_Palisade.Cast(target.GetObject());
		if (!w) return false;
		if (!GetGame().IsServer()) return true;
		string msg;
		return AGA_Territory.CanBuildAt(player, w.GetPosition(), msg);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		AGA_Palisade w = AGA_Palisade.Cast(action_data.m_Target.GetObject());
		if (!w) return;
		ItemBase pl = ItemBase.Cast(GetGame().CreateObjectEx("WoodenPlank", w.GetPosition(), ECE_PLACE_ON_SURFACE));
		if (pl) pl.SetQuantity(10);
		GetGame().ObjectDelete(w);
		MiscGameplayFunctions.DealAbsoluteDmg(action_data.m_MainItem, 4);
	}
}

modded class Hatchet
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionDismantlePalisade);
	}
}

// kit palisady: 20 desek + 30 gwozdzi
class AGA_CraftPalisadeKit extends AGA_RecipeSimple
{
	override void Init() { m_Name = "Craft palisade kit"; Two("WoodenPlank", 20, "Nail", 30, "AGA_PalisadeKit"); }
}
