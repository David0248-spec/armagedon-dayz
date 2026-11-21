class CfgPatches
{
	class ArmaGedonAdmin
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts"};
	};
};

class CfgMods
{
	class ArmaGedonAdmin
	{
		dir = "ArmaGedonAdmin";
		name = "ARMA-GEDON Admin";
		author = "ARMA-GEDON";
		version = "1.0";
		type = "mod";
		dependencies[] = {"Game", "World", "Mission"};
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"ArmaGedonAdmin/Scripts/3_Game"};
			};
			class worldScriptModule
			{
				value = "";
				files[] = {"ArmaGedonAdmin/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"ArmaGedonAdmin/Scripts/5_Mission"};
			};
		};
	};
};
