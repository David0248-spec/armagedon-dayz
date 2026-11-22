// ARMA-GEDON: balans rozgrywki

// choroby wyłączone: zapalenie płuc i zatrucie metalami ciężkimi
modded class PneumoniaMdfr
{
	override protected bool ActivateCondition(PlayerBase player)
	{
		return false;
	}
}

modded class HeavyMetalPhase1Mdfr
{
	override protected bool ActivateCondition(PlayerBase player)
	{
		return false;
	}
}

modded class HeavyMetalPhase2Mdfr
{
	override protected bool ActivateCondition(PlayerBase player)
	{
		return false;
	}
}

modded class HeavyMetalPhase3Mdfr
{
	override protected bool ActivateCondition(PlayerBase player)
	{
		return false;
	}
}

// helikoptery z ekonomii pojawiają się z pełnym olejem
modded class ExpansionHelicopterScript
{
	override void EEOnCECreate()
	{
		super.EEOnCECreate();
		Fill(CarFluid.OIL, GetFluidCapacity(CarFluid.OIL));
	}
}

// bez przykrywania pojazdów plandeką i bez tankowania heli w zawisie
modded class ExpansionActionCoverVehicle
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return false;
	}
}

modded class ExpansionActionHelicopterHoverRefill
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		return false;
	}
}
