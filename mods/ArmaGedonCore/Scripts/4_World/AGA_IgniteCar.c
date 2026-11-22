// ARMA-GEDON: podpalenie pojazdu zapalniczką / zapałkami przy wlewie paliwa

class AGA_ActionIgniteCarCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(0.5);
	}
}

class AGA_ActionIgniteCar : ActionContinuousBase
{
	void AGA_ActionIgniteCar()
	{
		m_CallbackClass = AGA_ActionIgniteCarCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_STARTFIRE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_FullBody = true;
		m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
		m_LockTargetOnUse = false;
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINotRuinedAndEmpty;
		m_ConditionTarget = new CCTNone;
	}

	override string GetText()
	{
		return "#ignite";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!target || !item) return false;
		CarScript car = CarScript.Cast(target.GetObject());
		if (!car || car.IsDamageDestroyed() || car.AGA_IsIgnited()) return false;
		if (!item.CanIgniteItem(car)) return false;
		// tylko przy wlewie paliwa
		array<string> sel = new array<string>;
		car.GetActionComponentNameList(target.GetComponentIndex(), sel);
		if (sel.Find(car.GetActionCompNameFuel()) < 0) return false;
		float d = car.GetActionDistanceFuel();
		return vector.DistanceSq(car.GetRefillPointPosWS(), player.GetPosition()) < d * d * d;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		CarScript car = CarScript.Cast(action_data.m_Target.GetObject());
		if (!car || car.AGA_IsIgnited()) return;
		action_data.m_MainItem.OnIgnitedTarget(car);
		car.AGA_Ignite();
		car.SetHealth("Engine", "Health", 0);
		if (action_data.m_Player && action_data.m_Player.GetIdentity())
			AGA_Server.Get().Log("Podpalenie pojazdu " + car.GetType() + " przez " + action_data.m_Player.GetIdentity().GetName() + " (" + action_data.m_Player.GetIdentity().GetPlainId() + ") @ " + car.GetPosition().ToString());
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Burn, 7000, false, car);
	}

	protected void Burn(CarScript car)
	{
		if (!car) return;
		car.AGA_SetBurned();
		car.SetHealth("", "", 0);
		array<EntityAI> children = new array<EntityAI>;
		car.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, children);
		foreach (EntityAI child : children)
			if (child && child != car) child.SetHealth("", "", 0);
		for (int i = 0; i < car.GetInventory().AttachmentCount(); i++)
		{
			ItemBase att = ItemBase.Cast(car.GetInventory().GetAttachmentFromIndex(i));
			if (!att) continue;
			att.SetHealth("", "", 0);
			if (att.IsInherited(CarWheel))
			{
				ReplaceWheelLambda lambda = new ReplaceWheelLambda(att, att.GetType() + "_Ruined", null);
				lambda.SetTransferParams(true, true, true);
				car.GetInventory().ReplaceItemWithNew(InventoryMode.SERVER, lambda);
			}
		}
		MiscGameplayFunctions.ThrowAllItemsInInventory(car, 0);
		Object boom = GetGame().CreateObject("AGA_IgniteExplosion", car.GetPosition(), false);
		if (boom) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().ObjectDelete, 5000, false, boom);
	}
}

class AGA_IgniteExplosion extends House
{
	void AGA_IgniteExplosion()
	{
		if (GetGame().IsServer())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Boom, 10, false);
	}

	void Boom()
	{
		Explode(DT_EXPLOSION, "AGA_IgniteCar_Ammo");
	}
}

modded class CarScript
{
	protected bool m_AGA_Ignited;
	protected bool m_AGA_Burned;
	protected Particle m_AGA_Smoke;

	void CarScript()
	{
		RegisterNetSyncVariableBool("m_AGA_Burned");
	}

	bool AGA_IsIgnited() { return m_AGA_Ignited || m_AGA_Burned; }
	void AGA_Ignite() { m_AGA_Ignited = true; }

	void AGA_SetBurned()
	{
		m_AGA_Burned = true;
		SetSynchDirty();
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
		if (m_AGA_Burned && !m_AGA_Smoke && !GetGame().IsDedicatedServer())
			m_AGA_Smoke = ParticleManager.GetInstance().PlayOnObject(ParticleList.SMOKING_HELI_WRECK, this, "0 1 0");
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);
		if (m_AGA_Smoke) m_AGA_Smoke.Stop();
	}
}

modded class Matchbox
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionIgniteCar);
	}
}

modded class PetrolLighter
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionIgniteCar);
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(AGA_ActionIgniteCar);
		actions.Insert(AGA_ActionArmTimed);
		actions.Insert(AGA_ActionDefuseTimed);
		actions.Insert(AGA_ActionFlagInviteOn);
		actions.Insert(AGA_ActionFlagInviteOff);
		actions.Insert(AGA_ActionFlagJoin);
		actions.Insert(AGA_ActionFlagLeave);
		actions.Insert(AGA_ActionFlagGroup);
		actions.Insert(AGA_ActionUpgrade);
	}
}
