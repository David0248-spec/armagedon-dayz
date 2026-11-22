// ARMA-GEDON: palisada z bali (model sciany z gry), stawiana z kitu przez hologram
class CfgPatches
{
	class AGA_Palisade
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data","DZ_Gear_Camping"};
	};
};
class CfgVehicles
{
	class FenceKit;
	class Inventory_Base;
	class AGA_PalisadeKit: FenceKit
	{
		scope = 2;
		displayName = "Palisade Kit";
		descriptionShort = "A bundle of planks and rope. Place it to build a wooden palisade wall (own territory only).";
		attachments[] = {};
		itemSize[] = {2,3};
		weight = 6000;
	};
	class AGA_PalisadeKitPlacing: AGA_PalisadeKit
	{
		scope = 1;
		model = "\DZ\structures\Walls\Wall_Wood4_4.p3d";
		storageCategory = 10;
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		hiddenSelectionsMaterials[] = {};
	};
	class AGA_Palisade: Inventory_Base
	{
		scope = 2;
		displayName = "Palisade";
		descriptionShort = "Wooden palisade wall. Cannot be destroyed - enter bases only through gates and doors.";
		model = "\DZ\structures\Walls\Wall_Wood4_4.p3d";
		bounding = "BSphere";
		overrideDrawArea = "8.0";
		forceFarBubble = "true";
		handheld = "false";
		carveNavmesh = 1;
		canBeDigged = 0;
		heavyItem = 1;
		weight = 60000;
		itemSize[] = {10,10};
		itemBehaviour = 0;
		physLayer = "item_large";
		storageCategory = 1;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 1000;
				};
			};
		};
	};
};
