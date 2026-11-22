// ARMA-GEDON: automatyczne laczenie amunicji tego samego typu po podniesieniu do ekwipunku
modded class Ammunition_Base
{
	override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
	{
		super.EEItemLocationChanged(oldLoc, newLoc);
		if (!GetGame().IsServer()) return;
		PlayerBase pNew = PlayerBase.Cast(GetHierarchyRootPlayer());
		if (!pNew) return;
		EntityAI oldParent = oldLoc.GetParent();
		if (oldParent && oldParent.GetHierarchyRootPlayer() == pNew) return; // przekladanie we wlasnym ekwipunku
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AGA_StackInto, 150, false, pNew);
	}

	void AGA_StackInto(PlayerBase p)
	{
		if (!p || GetHierarchyRootPlayer() != p || IsRuined()) return;
		array<EntityAI> items = new array<EntityAI>;
		p.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		foreach (EntityAI it : items)
		{
			if (GetAmmoCount() <= 0) break;
			Ammunition_Base t = Ammunition_Base.Cast(it);
			if (!t || t == this || t.GetType() != GetType() || t.IsRuined()) continue;
			int space = t.GetAmmoMax() - t.GetAmmoCount();
			if (space <= 0) continue;
			int move = Math.Min(space, GetAmmoCount());
			t.ServerSetAmmoCount(t.GetAmmoCount() + move);
			ServerSetAmmoCount(GetAmmoCount() - move);
		}
		if (GetAmmoCount() <= 0) GetGame().ObjectDelete(this);
	}
}
