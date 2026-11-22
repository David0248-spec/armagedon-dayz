// ARMA-GEDON: wyposazenie startowe po respawnie (zastepuje vanillowe drobiazgi, ubrania zostaja)
modded class MissionServer
{
	override void EquipCharacter(MenuDefaultCharacterData char_data)
	{
		super.EquipCharacter(char_data);
		if (!m_player) return;
		// usun drobiazgi z vanilli (wszystko w kieszeniach ubran)
		array<EntityAI> items = new array<EntityAI>;
		m_player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		foreach (EntityAI it : items)
		{
			if (!it || it == m_player || it.IsClothing()) continue;
			if (it.GetHierarchyParent() && it.GetHierarchyParent().IsClothing()) GetGame().ObjectDelete(it);
		}
		array<string> food = {"TunaCan", "BakedBeansCan", "SardinesCan", "SpaghettiCan", "PeachesCan"};
		array<string> drink = {"SodaCan_Cola", "SodaCan_Pipsi", "SodaCan_Spite", "SodaCan_Kvass"};
		m_player.GetInventory().CreateInInventory("BandageDressing");
		m_player.GetInventory().CreateInInventory("Chemlight_White");
		m_player.GetInventory().CreateInInventory(food.GetRandomElement());
		m_player.GetInventory().CreateInInventory(drink.GetRandomElement());
		EntityAI k = m_player.GetInventory().CreateInInventory("BoneKnife");
		if (k) k.SetHealth01("", "", 1.0);
		m_player.GetInventory().CreateInInventory("ChernarusMap");
	}
}
