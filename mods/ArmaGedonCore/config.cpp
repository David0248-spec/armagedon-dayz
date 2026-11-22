class CfgPatches
{
	class ArmaGedonCore
	{
		units[] = {"AGA_TimedExplosive", "AGA_MetalPanel", "AGA_GunRack_T1", "AGA_GunWall_T1", "AGA_GearStand_T1", "AGA_Workbench_T1"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Characters", "DZ_Weapons_Projectiles", "DZ_Weapons_Magazines", "DZ_Weapons_Firearms", "DZ_Weapons_Shotguns", "DZ_Pistols", "DZ_Weapons_Archery_Crossbow", "DZ_Weapons_Explosives", "DZ_Gear_Tools", "DZ_Sounds_Effects", "JM_CF_Scripts", "GDZ_Codelock", "DZ_Gear_Camping"};
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
		inputs = "ArmaGedonCore/Data/Inputs.xml";
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

// --- balans broni, amunicji i mapy ARMA-GEDON ---
class Mode_Safe;
class Mode_Single;
class Mode_SemiAuto;
class Mode_FullAuto;
class Mode_Burst;
class Mode_Double;
class CfgAmmo
{
	class Plastic_Explosive_Ammo;
	class AGA_IgniteCar_Ammo: Plastic_Explosive_Ammo
	{
		indirectHit = 1;
		indirectHitRange = 1;
		indirectHitRangeMultiplier = 13;
		class DamageApplied
		{
			type = "FragGrenade";
			class Health
			{
				damage = 60;
			};
			class Blood
			{
				damage = 0;
			};
			class Shock
			{
				damage = 100;
			};
		};
	};
	class Bullet_Base;
	class Bullet_12GaugeSlug: Bullet_Base
	{
		airFriction = -0.0025;
		unconRefillModifier = 2.5;
	};
	class Bullet_12GaugeRubberSlug: Bullet_12GaugeSlug
	{
		initSpeed = 380;
		typicalSpeed = 420;
		airFriction = -0.01;
	};
	class Bullet_556x45: Bullet_Base
	{
		airFriction = -0.000875;
		unconRefillModifier = 1.5;
		damageBarrel = 250;
		damageBarrelDestroyed = 250;
		class DamageApplied
		{
			defaultDamageOverride[] = {{1.1,1}};
			class Health
			{
				damage = 120;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 105;
			};
		};
	};
	class Bullet_545x39: Bullet_Base
	{
		airFriction = -0.000875;
		unconRefillModifier = 1.5;
		damageBarrel = 215;
		damageBarrelDestroyed = 215;
		class DamageApplied
		{
			defaultDamageOverride[] = {{1.2,1}};
			class Health
			{
				damage = 120;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 75;
			};
		};
	};
	class Bullet_762x54: Bullet_Base
	{
		airFriction = -0.0002;
		unconRefillModifier = 1.5;
		impactBehaviour = 0;
		class DamageApplied
		{
			defaultDamageOverride[] = {{0.99,1}};
			class Health
			{
				damage = 170;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 105;
			};
		};
	};
	class Bullet_308Win: Bullet_Base
	{
		airFriction = -0.0002;
		unconRefillModifier = 1.5;
		impactBehaviour = 0;
		class DamageApplied
		{
			defaultDamageOverride[] = {{1.05,1}};
			class Health
			{
				damage = 182;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 110;
			};
		};
	};
	class Bullet_762x39: Bullet_Base
	{
		airFriction = -0.001125;
		unconRefillModifier = 1.5;
		damageBarrel = 500;
		damageBarrelDestroyed = 500;
		class DamageApplied
		{
			defaultDamageOverride[] = {{1.2,1}};
			class Health
			{
				damage = 130;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 75;
			};
		};
	};
	class Bullet_45ACP: Bullet_Base
	{
		initSpeed = 290;
		airFriction = -0.0007;
		unconRefillModifier = 4.5;
		class DamageApplied
		{
			class Health
			{
				damage = 45;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 45;
				damageOverride[] = {{0.9,1}};
			};
		};
	};
	class Bullet_9x19: Bullet_Base
	{
		initSpeed = 380;
		airFriction = -0.00161;
		unconRefillModifier = 4.5;
	};
	class Bullet_9x39AP: Bullet_Base
	{
		typicalSpeed = 420;
		initSpeed = 420;
		unconRefillModifier = 1.5;
		class DamageApplied
		{
			defaultDamageOverride[] = {{0.95,1}};
			class Health
			{
				damage = 75;
				armorDamage = 2.5;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 65;
			};
		};
	};
	class Bullet_9x39: Bullet_Base
	{
		typicalSpeed = 380;
		initSpeed = 380;
		unconRefillModifier = 1.5;
		class DamageApplied
		{
			class Health
			{
				damage = 75;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 65;
				damageOverride[] = {{0.9,1}};
			};
		};
	};
	class Bolt_Base;
	class Bullet_HuntingBolt: Bolt_Base
	{
		unconRefillModifier = 1.5;
		airFriction = -0.000975;
		class DamageApplied
		{
			defaultDamageOverride[] = {{0.98,0.96},{0.961,0.96},{0.935,0.925},{0.905,0.71},{0.87,0.56}};
			class Health
			{
				damage = 149;
			};
			class Blood
			{
				damage = 100;
			};
			class Shock
			{
				damage = 95;
			};
		};
	};
};
class CfgWeapons
{
	class Archery_Base;
	class Crossbow_Base: Archery_Base
	{
		itemSize[] = {7,4};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 100;
				};
			};
		};
	};
	class RifleCore;
	class Rifle_Base: RifleCore
	{
		class Reliability
		{
			ChanceToJam[] = {0,0,0.0075,0.0375,1};
		};
	};
	class AK74_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.825;
	};
	class AKS74U: AK74_Base
	{
		initSpeedMultiplier = 0.775;
	};
	class AK101_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.925;
	};
	class AK101: AK101_Base
	{

	};
	class AK101_Black: AK101
	{

	};
	class AKM_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.95;
		recoilModifier[] = {1.05,1.05,1.05};
	};
	class AKM: AKM_Base
	{

	};
	class Aug_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.925;
	};
	class Aug: Aug_Base
	{

	};
	class AugShort: Aug_Base
	{
		initSpeedMultiplier = 0.9;
	};
	class B95_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.025;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
		class Double: Mode_Double
		{
			dispersion = 0.0075;
		};
	};
	class CZ61_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.15;
	};
	class BoltActionRifle_ExternalMagazine_Base;
	class CZ527_Base: BoltActionRifle_ExternalMagazine_Base
	{
		initSpeedMultiplier = 1.3;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
	};
	class CZ527;
	class CZ527_Black: CZ527
	{

	};
	class CZ550_Base: BoltActionRifle_ExternalMagazine_Base
	{
		initSpeedMultiplier = 1.125;
		recoilModifier[] = {0.9,0.9,0.9};
		class Single: Mode_Single
		{
			dispersion = 0;
		};
	};
	class CZ550: CZ550_Base
	{

	};
	class FAL_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.075;
		class SemiAuto: Mode_SemiAuto
		{
			dispersion = 0;
		};
		class FullAuto: Mode_FullAuto
		{
			dispersion = 0;
		};
	};
	class FAL: FAL_Base
	{

	};
	class SCARH_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.125;
		class SemiAuto: Mode_SemiAuto
		{
			dispersion = 0;
		};
		class FullAuto: Mode_FullAuto
		{
			dispersion = 0;
		};
	};
	class SCARH: SCARH_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class SCARH_Black: SCARH
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class M79;
	class Famas_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.9;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 250;
				};
			};
		};
	};
	class SawedoffFAMAS: Famas_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 250;
				};
			};
		};
	};
	class Izh18_Base;
	class Izh18: Izh18_Base
	{
		initSpeedMultiplier = 1.05;
	};
	class Shotgun_Base;
	class Izh18Shotgun_Base: Shotgun_Base
	{
		initSpeedMultiplier = 1.25;
		class Single: Mode_Single
		{
			dispersion = 0.0007;
		};
	};
	class Izh43Shotgun_Base: Shotgun_Base
	{
		initSpeedMultiplier = 1.25;
	};
	class Mp133Shotgun_Base: Shotgun_Base
	{
		initSpeedMultiplier = 1.25;
	};
	class Saiga_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.15;
	};
	class Saiga: Saiga_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 312;
				};
			};
		};
	};
	class M4A1_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.9;
		recoilModifier[] = {1.1,1.1,1.1};
	};
	class M4A1: M4A1_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 250;
				};
			};
		};
	};
	class M14_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.125;
		class SemiAuto: Mode_SemiAuto
		{
			dispersion = 0;
		};
	};
	class M14: M14_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class M16A2_Base: Rifle_Base
	{
		initSpeedMultiplier = 0.95;
	};
	class M16A2;
	class BoltActionRifle_InnerMagazine_Base;
	class Mosin9130_Base: BoltActionRifle_InnerMagazine_Base
	{
		initSpeedMultiplier = 1.025;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class Mosin9130;
	class MP5K_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.05;
	};
	class Repeater_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.3;
	};
	class Scout_Base: BoltActionRifle_ExternalMagazine_Base
	{
		initSpeedMultiplier = 1.15;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
	};
	class SKS_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.05;
	};
	class SKS: SKS_Base
	{

	};
	class SSG82_Base: BoltActionRifle_ExternalMagazine_Base
	{
		initSpeedMultiplier = 1.2;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
	};
	class SVD_Base: Rifle_Base
	{
		initSpeedMultiplier = 1.025;
		class SemiAuto: Mode_SemiAuto
		{
			dispersion = 0;
		};
	};
	class SVD: SVD_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class SVD_Wooden: SVD_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class Winchester70_Base: BoltActionRifle_InnerMagazine_Base
	{
		initSpeedMultiplier = 1.125;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
	};
	class Winchester70: Winchester70_Base
	{
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class Winchester70_Black: Winchester70
	{

	};
	class SV98_Base: BoltActionRifle_ExternalMagazine_Base
	{
		initSpeedMultiplier = 1.175;
		class Single: Mode_Single
		{
			dispersion = 0;
		};
	};
	class SV98: SV98_Base
	{
		weight = 4800;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 300;
				};
			};
		};
	};
	class VSS_Base: Rifle_Base
	{

	};
	class ASVAL: VSS_Base
	{

	};
	class PistolCore;
	class Pistol_Base: PistolCore
	{
		class Reliability
		{
			ChanceToJam[] = {0,0,0.0075,0.0375,1};
		};
	};
	class LongHorn_Base: Pistol_Base
	{
		initSpeedMultiplier = 1.025;
	};
	class Colt1911_Base: Pistol_Base
	{
		initSpeedMultiplier = 1;
	};
	class CZ75_Base: Pistol_Base
	{
		initSpeedMultiplier = 1;
	};
	class Deagle_Base: Pistol_Base
	{
		initSpeedMultiplier = 0.9;
	};
	class FNX45_Base: Pistol_Base
	{
		initSpeedMultiplier = 1;
	};
	class Glock19_Base: Pistol_Base
	{
		initSpeedMultiplier = 1;
	};
	class Magnum_Base: Pistol_Base
	{
		initSpeedMultiplier = 0.9;
	};
	class Derringer_Base: Pistol_Base
	{
		initSpeedMultiplier = 0.8;
	};
	class P1_Base: Pistol_Base
	{
		initSpeedMultiplier = 0.9;
	};
};
class CfgMagazines
{
	class DefaultMagazine;
	class Magazine_Base: DefaultMagazine
	{
		class Reliability
		{
			ChanceToJam[] = {0,0,0.0075,0.0375,1};
		};
	};
};
class CfgWorlds
{
	class CAWorld;
	class Chernarusplus: CAWorld
	{
		volFogOffset = -200;
	};
};
class CfgVehicles
{
	class ExplosionTest;
	class AGA_IgniteExplosion: ExplosionTest
	{
	};
	class ZmbM_eastSoldier_normal_Navy;
	class DamageSystem;
	class AG_ZmbM_Spetsnaz: ZmbM_eastSoldier_normal_Navy
	{
		class DamageSystem: DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 230;
				};
			};
		};
	};
	class CombinationLock;
	class CodeLock: CombinationLock
	{
		itemSize[] = {2,2};
	};
	class ExpansionHelicopterScript;
	class SimulationModule;
	class ExpansionMerlin: ExpansionHelicopterScript
	{
		fuelConsumption = 150;
		class DamageSystem
		{
			class GlobalArmor
			{
				class Projectile
				{
					class Health
					{
						damage = 1.35;
					};
				};
			};
		};
		class SimulationModule: SimulationModule
		{
			altitudeFullForce = 800;
			altitudeNoForce = 800;
		};
		class Cargo
		{
			itemsCargoSize[] = {10,30};
			allowOwnedCargoManipulation = 1;
			openable = 0;
		};
	};
	class ExpansionUh1h: ExpansionHelicopterScript
	{
		fuelConsumption = 150;
		class SimulationModule: SimulationModule
		{
			altitudeFullForce = 800;
			altitudeNoForce = 800;
		};
		class Cargo
		{
			itemsCargoSize[] = {10,20};
			allowOwnedCargoManipulation = 1;
			openable = 0;
		};
	};
	class ExpansionMh6: ExpansionHelicopterScript
	{
		fuelConsumption = 150;
		class DamageSystem
		{
			class GlobalArmor
			{
				class Projectile
				{
					class Health
					{
						damage = 1.35;
					};
				};
			};
		};
		class SimulationModule: SimulationModule
		{
			altitudeFullForce = 800;
			altitudeNoForce = 800;
		};
		class Cargo
		{
			itemsCargoSize[] = {10,10};
			allowOwnedCargoManipulation = 1;
			openable = 0;
		};
	};
	class CarWheel;
	class HatchbackDoors_Driver;
	class ExpansionMerlinFrontWheel: CarWheel
	{
		physLayer = "item_small";
	};
	class ExpansionMerlinBackWheel: CarWheel
	{
		physLayer = "item_small";
	};
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
	class BaseBuildingBase;
	class TerritoryFlag: BaseBuildingBase
	{
		class GUIInventoryAttachmentsProps
		{
			class Support
			{
				attachmentSlots[] = {"Material_FPole_WoodenLog", "Material_FPole_Rope"};
			};
			class Pole
			{
				attachmentSlots[] = {"Material_FPole_Stones", "Material_FPole_WoodenLog2", "Material_FPole_Nails"};
			};
		};
		class Construction
		{
			class totem
			{
				class base
				{
					class Materials
					{
						delete Material1;
					};
				};
				class support
				{
					class Materials
					{
						class Material1
						{
							type = "WoodenLog";
							slot_name = "Material_FPole_WoodenLog";
							quantity = 1;
							lockable = 0;
						};
						class Material2
						{
							type = "Rope";
							slot_name = "Material_FPole_Rope";
							quantity = 0;
							lockable = 0;
						};
					};
				};
				class pole
				{
					class Materials
					{
						class Material1
						{
							type = "Stone";
							slot_name = "Material_FPole_Stones";
							quantity = 16;
							lockable = 0;
						};
						class Material2
						{
							type = "WoodenLog";
							slot_name = "Material_FPole_WoodenLog2";
							quantity = 6;
							lockable = 0;
						};
						class Material3
						{
							type = "Nail";
							slot_name = "Material_FPole_Nails";
							quantity = 60;
							lockable = 0;
						};
						delete Material4;
					};
				};
			};
		};
	};
	class Inventory_Base;
	class TerritoryFlagKit: Inventory_Base
	{
		attachments[] = {};
		descriptionShort = "Flag pole kit. Place it to claim a territory (30 m). Upgrade it at the flag: level 2 - 1 log and 1 rope, level 3 - 6 logs, 60 nails and 16 stones.";
	};
	class AGA_Crate_T2: WoodenCrate
	{
		scope = 2;
		displayName = "Wooden Crate (Tier 2)";
		descriptionShort = "Upgraded storage crate. 100 slots.";
		model = "\DZ\structures_bliss\residential\Misc\Misc_WoodenCrate.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		itemsCargoSize[] = {10,10};
	};
	class AGA_Crate_T3: WoodenCrate
	{
		scope = 2;
		displayName = "Wooden Crate (Tier 3)";
		descriptionShort = "Upgraded storage crate. 150 slots.";
		model = "\DZ\structures_bliss\residential\Misc\Misc_WoodenCrate_3x.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		itemsCargoSize[] = {10,15};
	};
	class AGA_Crate_T4: WoodenCrate
	{
		scope = 2;
		displayName = "Wooden Crate (Tier 4)";
		descriptionShort = "Fully upgraded storage crate. 200 slots.";
		model = "\DZ\structures_bliss\residential\Misc\Misc_WoodenCrate_5x.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		itemsCargoSize[] = {10,20};
	};
	class AGA_GunRack_T1: WoodenCrate
	{
		scope = 2;
		displayName = "Gun Rack (Tier 1)";
		descriptionShort = "Holds 2 rifles. Upgrade with a hammer.";
		model = "\DayZExpansion\Objects\Basebuilding\Misc\gunrack\gunrack.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Shoulder1","Shoulder2"};
		itemsCargoSize[] = {0,0};
		weight = 10000;
	};
	class AGA_GunRack_T2: AGA_GunRack_T1
	{
		scope = 2;
		displayName = "Gun Rack (Tier 2)";
		descriptionShort = "Holds 4 rifles and 2 ammo boxes.";
		model = "\DayZExpansion\Objects\Basebuilding\Misc\gunrack\gunrack.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Shoulder1","Shoulder2","Shoulder3","Shoulder4"};
		itemsCargoSize[] = {6,4};
	};
	class AGA_GunRack_T3: AGA_GunRack_T1
	{
		scope = 2;
		displayName = "Gun Rack (Tier 3)";
		descriptionShort = "Holds 4 rifles, 3 ammo boxes and 3 pistols.";
		model = "\DayZExpansion\Objects\Basebuilding\Misc\gunrack\gunrack.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Shoulder1","Shoulder2","Shoulder3","Shoulder4"};
		itemsCargoSize[] = {8,6};
	};
	class AGA_GunWall_T1: WoodenCrate
	{
		scope = 2;
		displayName = "Gun Wall (Tier 1)";
		descriptionShort = "Holds 4 rifles. Upgrade with a hammer.";
		model = "\DZ\structures\military\Misc\Misc_Gunrack.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Shoulder1","Shoulder2","Shoulder3","Shoulder4"};
		itemsCargoSize[] = {0,0};
		weight = 12000;
	};
	class AGA_GunWall_T2: AGA_GunWall_T1
	{
		scope = 2;
		displayName = "Gun Wall (Tier 2)";
		descriptionShort = "Holds 8 rifles and 8 grenades.";
		model = "\DZ\structures\military\Misc\Misc_Gunrack.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Shoulder1","Shoulder2","Shoulder3","Shoulder4","Shoulder5","Shoulder6","Shoulder7","Shoulder8"};
		itemsCargoSize[] = {8,4};
	};
	class AGA_GearStand_T1: WoodenCrate
	{
		scope = 2;
		displayName = "Gear Stand (Tier 1)";
		descriptionShort = "Holds headgear and a vest. Upgrade with a hammer.";
		model = "\DZ\structures\furniture\Various\rack_DZ.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Headgear","Vest"};
		itemsCargoSize[] = {0,0};
		weight = 8000;
	};
	class AGA_GearStand_T2: AGA_GearStand_T1
	{
		scope = 2;
		displayName = "Gear Stand (Tier 2)";
		descriptionShort = "Holds a full outfit.";
		model = "\DZ\structures\furniture\Various\rack_DZ.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		attachments[] = {"Headgear","Vest","Eyewear","Gloves","Body","Back","Hips","Legs","Feet"};
		itemsCargoSize[] = {0,0};
	};
	class AGA_Workbench_T1: WoodenCrate
	{
		scope = 2;
		displayName = "Workbench (Tier 1)";
		descriptionShort = "Tool storage: shovel, axe, pickaxe, sledgehammer, hammer, nails and planks.";
		model = "\DZ\structures\furniture\Various\workbench_DZ.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		itemsCargoSize[] = {10,10};
		weight = 15000;
	};
	class AGA_Workbench_T2: AGA_Workbench_T1
	{
		scope = 2;
		displayName = "Workbench (Tier 2)";
		descriptionShort = "Tool storage: hatchet, shovel, axe, pickaxe, saw, pliers, screwdriver, sledgehammer, nails and planks.";
		model = "\DZ\structures\furniture\Various\workbench_DZ.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		itemsCargoSize[] = {10,10};
	};
	class AGA_Workbench_T3: AGA_Workbench_T1
	{
		scope = 2;
		displayName = "Workbench (Tier 3)";
		descriptionShort = "All tier 2 storage. Items repaired near it can reach pristine condition.";
		model = "\DZ\structures\furniture\Various\workbench_DZ.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		itemsCargoSize[] = {10,10};
	};
	class MetalPlate;
	class AGA_MetalPanel: MetalPlate
	{
		scope = 2;
		displayName = "Metal Panel";
		descriptionShort = "Smaller metal panel used to upgrade base storage. Fits in a backpack. Six panels and a blowtorch make a metal sheet.";
		itemSize[] = {2,3};
		weight = 1500;
		canBeSplit = 1;
		varQuantityInit = 1;
		varQuantityMin = 0;
		varQuantityMax = 10;
		varQuantityDestroyOnMin = 1;
	};
	class ExplosivesBase;
	class Plastic_Explosive: ExplosivesBase
	{
		attachments[] = {"TriggerRemoteDetonator_Receiver", "TriggerAlarmClock"};
		class GUIInventoryAttachmentsProps
		{
			class Trigger
			{
				name = "#STR_CfgVehicles_AlarmClock0";
				description = "";
				attachmentSlots[] = {"TriggerAlarmClock"};
				icon = "cat_bb_attachments";
				view_index = 1;
			};
		};
	};
	class AGA_TimedExplosive: Plastic_Explosive
	{
		scope = 2;
		displayName = "Timed Plastic Explosive";
		model = "\\DayZExpansion\\Objects\\Weapons\\Explosives\\explosive.p3d";
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		descriptionShort = "Plastic explosive wired to an alarm clock. Once armed it detonates after a short countdown. Can be defused with pliers.";
		attachments[] = {"TriggerAlarmClock"};
	};
};

class CfgSlots
{
	class Slot_TriggerAlarmClock
	{
		name = "TriggerAlarmClock";
		displayName = "#STR_CfgVehicles_AlarmClock0";
		ghostIcon = "set:dayz_inventory image:cat_bb_attachments";
	};
};

class CfgSoundShaders
{
	class baseCharacter_SoundShader;
	class AGA_TimerTick_SoundShader: baseCharacter_SoundShader
	{
		samples[] = {{"DZ\sounds\Characters\actions\items\kitchentimer\Kitchen_Timer_Ticking_Loop_01", 1}};
		volume = 4;
		range = 70;
	};
};

class CfgSound3DProcessors
{
	class AGA_TimerTick_3DProcessingType
	{
		type = "panner";
		innerRange = 35;
		range = 70;
		rangeCurve = "Smooth1Curve";
	};
};

class CfgSoundSets
{
	class baseCharacter_SoundSet;
	class AGA_TimerTick_SoundSet: baseCharacter_SoundSet
	{
		soundShaders[] = {"AGA_TimerTick_SoundShader"};
		sound3DProcessingType = "AGA_TimerTick_3DProcessingType";
		volumeCurve = "Smooth1Curve";
	};
};
