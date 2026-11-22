// Barykady okien/drzwi (Building Fortifications) stawia się w otworach budynków:
// dla ich kitów hologram nie sprawdza podparcia, kąta, kolizji ze ścianą i wysokości.
modded class Hologram
{
	bool AGA_IsBarricade()
	{
		if (!m_Parent) return false;
		string t = m_Parent.GetType();
		t.ToLower();
		return t.IndexOf("barricade") >= 0 && t.IndexOf("kit") >= 0;
	}

	override bool IsBaseViable()
	{
		if (AGA_IsBarricade()) return true;
		return super.IsBaseViable();
	}

	override bool IsCollidingAngle()
	{
		if (AGA_IsBarricade()) return false;
		return super.IsCollidingAngle();
	}

	override bool IsCollidingBBox(ItemBase action_item = null)
	{
		if (AGA_IsBarricade()) return false;
		return super.IsCollidingBBox(action_item);
	}

	override bool IsCollidingGeometryProxy(ItemBase action_item = null)
	{
		if (AGA_IsBarricade()) return false;
		return super.IsCollidingGeometryProxy(action_item);
	}

	override bool IsClippingRoof()
	{
		if (AGA_IsBarricade()) return false;
		return super.IsClippingRoof();
	}

	override bool HeightPlacementCheck()
	{
		if (AGA_IsBarricade()) return true;
		return super.HeightPlacementCheck();
	}

	override bool IsFloating()
	{
		if (AGA_IsBarricade()) return false;
		return super.IsFloating();
	}

	override bool IsInTerrain()
	{
		if (AGA_IsBarricade()) return false;
		return super.IsInTerrain();
	}
}
