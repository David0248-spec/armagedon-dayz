// Jednorazowe odświeżenie lootu: gdy istnieje $profile:ArmaGedonCore/loot_reset.flag,
// po starcie usuwa luźne przedmioty leżące na mapie poza terytoriami baz (CE spawnuje loot od nowa).
// Nie rusza: przedmiotów w ekwipunku/pojemnikach, baz, skrzyń, namiotów, beczek, pojazdów, pułapek.
class AGA_LootReset
{
	static bool s_Pending;

	static void Check()
	{
		string f = "$profile:ArmaGedonCore/loot_reset.flag";
		if (!FileExist(f)) return;
		DeleteFile(f);
		s_Pending = true;
	}

	static bool Keep(ItemBase ib)
	{
		if (ib.GetHierarchyParent()) return true;
		if (ib.IsInherited(Container_Base) || ib.IsInherited(TentBase) || ib.IsInherited(BaseBuildingBase)) return true;
		if (ib.IsInherited(TrapBase) || ib.IsInherited(ExplosivesBase) || ib.IsInherited(FireplaceBase)) return true;
		if (ib.IsInherited(TerritoryFlagKit) || ib.IsInherited(KitBase)) return true;
		string t = ib.GetType();
		if (t.IndexOf("mmg_") == 0 || t.IndexOf("AGA_") == 0 || t.IndexOf("BF_") == 0) return true;
		if (AGA_Territory.FlagAt(ib.GetPosition(), 1.2)) return true;
		return false;
	}

	static void Run()
	{
		if (!s_Pending) return;
		s_Pending = false;
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition3D(Vector(7680, 0, 7680), 11000, objs, null);
		int n = 0;
		foreach (Object o : objs)
		{
			ItemBase ib = ItemBase.Cast(o);
			if (!ib || Keep(ib)) continue;
			GetGame().ObjectDelete(ib);
			n++;
		}
		AGA_Server.Get().Log("[LOOT] reset: usunieto " + n.ToString() + " przedmiotow z mapy, CE spawnuje loot od nowa");
	}
}
