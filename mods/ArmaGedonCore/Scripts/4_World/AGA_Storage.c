// ARMA-GEDON – meble bazy (skrzynie, stojaki na broń i ubrania, warsztat, schowki) – zasady serwera.
//  - ulepszanie mebli wymaga poziomu własnej flagi: pierwszy stopień poziom 2, kolejne poziom 3
//  - rozbieranie mebli tylko we własnym terytorium i tylko gdy są puste
//  - skrzynie eventów (aga_fixed) nie do podniesienia ani rozebrania

class AGA_Storage
{
	static bool Fixed(EntityAI e)
	{
		return e && e.ConfigGetBool("aga_fixed");
	}

	static TerritoryFlag OwnFlag(PlayerBase p)
	{
		if (!p) return null;
		TerritoryFlag f = AGA_Territory.FlagAt(p.GetPosition());
		if (!f || f.AGA_Gid() == 0 || f.AGA_Gid() != p.AGA_Gid()) return null;
		return f;
	}

	static bool InOwnTerritory(PlayerBase p)
	{
		return OwnFlag(p) != null;
	}

	static bool FlagLevel(PlayerBase p, int need)
	{
		TerritoryFlag f = OwnFlag(p);
		return f && f.AGA_Level() >= need;
	}

	static bool IsEmpty(EntityAI e)
	{
		if (!e || !e.GetInventory()) return true;
		CargoBase cargo = e.GetInventory().GetCargo();
		if (cargo && cargo.GetItemCount() > 0) return false;
		return true;
	}

	// receptury rozbierania mebli
	static bool RecipeOk(ItemBase ingredients[], PlayerBase player)
	{
		if (!InOwnTerritory(player)) return false;
		for (int i = 0; i < 2; i++)
		{
			ItemBase ib = ingredients[i];
			if (!ib) continue;
			if (Fixed(ib)) return false;
			if (ib.IsInherited(Container_Base) && !IsEmpty(ib)) return false;
		}
		return true;
	}
}

// ==================================================================== skrzynie eventów
modded class AGAStorage_openable_noLock_base
{
	override bool CanPutInCargo(EntityAI parent)
	{
		if (AGA_Storage.Fixed(this)) return false;
		return super.CanPutInCargo(parent);
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		if (AGA_Storage.Fixed(this)) return false;
		return super.CanPutIntoHands(parent);
	}

	override bool IsTakeable()
	{
		if (AGA_Storage.Fixed(this)) return false;
		return super.IsTakeable();
	}
}

// ==================================================================== ulepszenia (poziom flagi)
modded class ActionUpgradeSmallToMedium
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 2);
	}
}
modded class ActionUpgradeMediumToLarge
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 3);
	}
}
modded class ActionUpgradeLargeToLvl4
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 3);
	}
}
modded class ActionUpgradeGunrack_1to2
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 2);
	}
}
modded class ActionUpgradeGunrack_2to3
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 3);
	}
}
modded class ActionUpgradeWorkbench_1to2
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 2);
	}
}
modded class ActionUpgradeWorkbench_2to3
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return super.ActionCondition(player, target, item) && AGA_Storage.FlagLevel(player, 3);
	}
}

// ==================================================================== rozbieranie (właściciel, puste)
modded class ActionDismantleMetal_GunRack
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!super.ActionCondition(player, target, item)) return false;
		EntityAI e = EntityAI.Cast(target.GetObject());
		return AGA_Storage.InOwnTerritory(player) && !AGA_Storage.Fixed(e) && AGA_Storage.IsEmpty(e);
	}
}

modded class AGAStorage_DisassembleLogHolder { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleMetalCabinet { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassemblePlanterLvl1 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassemblePlanterLvl2 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassemblePlanterLvl3 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWaterCollector { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenCrateLarge { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenCrateLvl1 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenCrateLvl4 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenCrateMedium { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenGunrack { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenGunrackLvl2 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWoodenGunrackLvl3 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWorkbenchLvl1 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWorkbenchLvl2 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }
modded class AGAStorage_DisassembleWorkbenchLvl3 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_Storage.RecipeOk(ingredients, player); } }

// ==================================================================== vanillowe budowanie wyłączone (budujemy tylko z zestawów bazy)
modded class CraftFenceKit
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; }
}
modded class CraftWatchtowerKit
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; }
}
modded class FenceKit
{
	override bool CanBePlaced(Man player, vector position) { return false; }
}
modded class WatchtowerKit
{
	override bool CanBePlaced(Man player, vector position) { return false; }
}
