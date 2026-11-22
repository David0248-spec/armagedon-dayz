class CfgPatches
{
	class ArmaGedonCore
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods
{
	class ArmaGedonCore
	{
		dir = "ArmaGedonCore";
		name = "ARMA-GEDON Core";
		author = "ARMA-GEDON";
		version = "1.0";
		type = "mod";
		dependencies[] = {"Game", "World", "Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"ArmaGedonCore/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"ArmaGedonCore/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"ArmaGedonCore/Scripts/5_Mission"};
			};
		};
	};
};

class CfgVehicles
{
	class Wreck_UH1Y;
	class WoodenCrate;
	class AGA_SmokeColumn: Wreck_UH1Y
	{
		scope = 1;
		model = "\DZ\data\lightpoint.p3d";
	};
	class AGA_LootCrate: WoodenCrate
	{
		scope = 2;
		displayName = "Supply crate";
		descriptionShort = "A heavy crate left behind at the site. Search it, it is too heavy to carry.";
		weight = 50000;
	};
};
