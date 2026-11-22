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
	class HouseNoDestruct;
	class AGA_SmokeColumn: HouseNoDestruct
	{
		scope = 1;
		model = "\DZ\data\lightpoint.p3d";
	};
};
