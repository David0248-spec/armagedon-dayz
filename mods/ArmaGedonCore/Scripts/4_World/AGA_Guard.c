// ARMA-GEDON – zabezpieczenia baz: zamki szyfrowe nie dają się zdjąć ani przenieść, gdy są zamknięte
modded class CodeLock
{
	protected bool m_AGA_Releasing;

	override void NewUnlockServer(EntityAI player, EntityAI parent)
	{
		m_AGA_Releasing = true;
		super.NewUnlockServer(player, parent);
		m_AGA_Releasing = false;
	}

	bool AGA_CanRelease()
	{
		if (m_AGA_Releasing) return true;
		if (GetLockState()) return false;
		if (GetGame().IsServer() && m_PassCode != string.Empty) return false;
		return true;
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		if (!super.CanPutIntoHands(parent)) return false;
		return AGA_CanRelease();
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		if (!super.CanPutInCargo(parent)) return false;
		return AGA_CanRelease();
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (!GetGame().IsServer()) return;
		AGA_Relock();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Relock, 3000, false);
	}

	override void OnItemLocationChanged(EntityAI old_owner, EntityAI new_owner)
	{
		super.OnItemLocationChanged(old_owner, new_owner);
		if (GetGame().IsServer() && new_owner)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Relock, 1000, false);
	}

	void AGA_Relock()
	{
		if (!GetGame().IsServer() || !GetHierarchyParent()) return;
		if (m_PassCode == string.Empty && !m_IsLocked) return;
		m_IsLocked = true;
		LockToParent();
		SetTakeable(false);
		Synchronize();
	}
}

class AGA_LockGuard
{
	static bool CanRelease(EntityAI attachment)
	{
		CodeLock cl = CodeLock.Cast(attachment);
		return !cl || cl.AGA_CanRelease();
	}

	static void RelockAll(EntityAI owner)
	{
		if (!GetGame().IsServer() || !owner || !owner.GetInventory()) return;
		for (int i = 0; i < owner.GetInventory().AttachmentCount(); i++)
		{
			CodeLock cl = CodeLock.Cast(owner.GetInventory().GetAttachmentFromIndex(i));
			if (cl) cl.AGA_Relock();
		}
	}
}

modded class BaseBuildingBase
{
	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (!super.CanReleaseAttachment(attachment)) return false;
		return AGA_LockGuard.CanRelease(attachment);
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		AGA_LockGuard.RelockAll(this);
	}
}

modded class TentBase
{
	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (!super.CanReleaseAttachment(attachment)) return false;
		return AGA_LockGuard.CanRelease(attachment);
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		AGA_LockGuard.RelockAll(this);
	}
}

// po jednorazowej akcji na zamku zwalniamy blokadę akcji (zawieszone akcje = exploit)
modded class ActionSingleUseBase
{
	override protected void OnExecute(ActionData action_data)
	{
		super.OnExecute(action_data);
		if (GetGame().IsServer() && GetGame().IsMultiplayer() && CodeLock.Cast(action_data.m_MainItem))
			ClearActionJuncture(action_data);
	}
}

// receptury wyłączone z poziomu serwera (lista w $profile:ArmaGedonCore/recipes_disabled.json)
class AGA_RecipeCfg
{
	ref array<string> Disabled;
	void AGA_RecipeCfg() { Disabled = new array<string>; }
}

class AGA_Recipes
{
	static ref AGA_RecipeCfg s_Cfg;

	static AGA_RecipeCfg Cfg()
	{
		if (s_Cfg) return s_Cfg;
		s_Cfg = new AGA_RecipeCfg();
		string p = "$profile:ArmaGedonCore/recipes_disabled.json";
		if (FileExist(p)) JsonFileLoader<AGA_RecipeCfg>.JsonLoadFile(p, s_Cfg);
		else JsonFileLoader<AGA_RecipeCfg>.JsonSaveFile(p, s_Cfg);
		return s_Cfg;
	}

	static void Apply(array<string> list)
	{
		if (!list || list.Count() == 0) return;
		PluginRecipesManager pm = PluginRecipesManager.Cast(GetPlugin(PluginRecipesManager));
		if (pm) pm.AGA_Unregister(list);
	}
}

modded class PluginRecipesManager
{
	void AGA_Unregister(array<string> list)
	{
		foreach (string r : list) UnregisterRecipe(r);
		GenerateRecipeCache();
	}
}
