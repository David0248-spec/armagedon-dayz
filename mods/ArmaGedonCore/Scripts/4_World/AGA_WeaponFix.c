// ARMA-GEDON: tlumik AK 5.45 pasuje do wszystkich broni 5.45 ze slotem AK (AEK-971, AN-94, AKS-74UN)
modded class AK_Suppressor545
{
	override bool CanPutAsAttachment(EntityAI parent)
	{
		if (super.CanPutAsAttachment(parent)) return true;
		if (!parent) return false;
		if (parent.FindAttachmentBySlotName("suppressorImpro") || parent.FindAttachmentBySlotName("weaponBayonetAK")) return false;
		string t = parent.GetType();
		t.ToLower();
		return (t.Contains("aek971") || t.Contains("an94") || t.Contains("aks74un"));
	}
}

// M249 z gry bazowej nie ma zainicjalizowanego FSM - powoduje bledy i crashe klientow. Usuwamy kazda sztuke (swiat i ekwipunki).
modded class Weapon_Base
{
	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer() && GetType() == "M249")
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_RemoveBroken, 500, false);
	}

	void AGA_RemoveBroken()
	{
		if (!IsPendingDeletion()) GetGame().ObjectDelete(this);
	}
}
