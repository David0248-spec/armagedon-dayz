class CfgPatches
{
	// zgodnosc paczki broni (zgoda autorow) - zastepcze nazwy wymaganych dodatkow
	class AG_Sounds { units[] = {}; weapons[] = {}; requiredVersion = 0.1; requiredAddons[] = {"DZ_Data"}; };
	class AG_Ammo_Balancer { units[] = {}; weapons[] = {}; requiredVersion = 0.1; requiredAddons[] = {"DZ_Data"}; };
	class ArmaGedonCore
	{
		units[] = {"AGA_TimedExplosive", "AGA_EvWeapons", "AGA_EvArmy", "AGA_EvMedic"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data", "DZ_Scripts", "DZ_Characters", "DZ_Weapons_Projectiles", "DZ_Weapons_Magazines", "DZ_Weapons_Firearms", "DZ_Weapons_Shotguns", "DZ_Pistols", "DZ_Weapons_Archery_Crossbow", "DZ_Weapons_Explosives", "DZ_Gear_Tools", "DZ_Sounds_Effects", "JM_CF_Scripts", "GDZ_Codelock", "DZ_Gear_Camping", "VBB_Weapons", "AG_Dependence", "AG_Fortification", "AG_Modules", "AG_Stockade", "AG_TreeHouse", "AGAStorage", "AGA_Furniture_redefine"};
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
	class Container_Base;
	class WoodenCrate: Container_Base {};
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
		hiddenSelections[] = {};
		hiddenSelectionsTextures[] = {};
		weight = 50000;
		itemsCargoSize[] = {10,6};
		attachments[] = {};
		class GUIInventoryAttachmentsProps {};
	};
	class AGA_LootCrateMil: AGA_LootCrate
	{
		displayName = "Military crate";
	};
	class AGA_LootCrateMed: AGA_LootCrate
	{
		displayName = "Medical case";
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
	class Nail: Inventory_Base
	{
		inventorySlot[] += {"aga_up_nails","aga_up_nails40","aga_up_nails70"};
	};
	class WoodenPlank: Inventory_Base
	{
		inventorySlot[] += {"aga_up_planks","aga_up_planks2","aga_up_planks10","aga_up_planks20","aga_up_planks22"};
	};
	class MetalWire: Inventory_Base
	{
		inventorySlot[] += {"aga_up_wire"};
	};
	class Rope: Inventory_Base
	{
		inventorySlot[] += {"aga_up_rope"};
	};
	class TerritoryFlagKit: Inventory_Base
	{
		attachments[] = {};
		descriptionShort = "Flag pole kit. Place it to claim a territory (30 m). Upgrade it at the flag: level 2 - 1 log and 1 rope, level 3 - 6 logs, 60 nails and 16 stones.";
	};
	// ================================================================ skrzynie eventów (nie do podniesienia)
	class AGA_Storage_WoodenCrate_Large;
	class AGA_Storage_WoodenCrate_Medium;
	class AGA_Storage_WoodenCrate_Lvl_1;
	class AGA_EvWeapons: AGA_Storage_WoodenCrate_Large
	{
		scope = 2;
		aga_fixed = 1;
		displayName = "Military Weapon Crate";
		descriptionShort = "A heavy military crate. Open it and search it, it is too heavy to carry.";
		weight = 90000;
	};
	class AGA_EvArmy: AGA_Storage_WoodenCrate_Medium
	{
		scope = 2;
		aga_fixed = 1;
		displayName = "Military Supply Box";
		descriptionShort = "A heavy military box. Open it and search it, it is too heavy to carry.";
		weight = 90000;
	};
	class AGA_EvMedic: AGA_Storage_WoodenCrate_Lvl_1
	{
		scope = 2;
		aga_fixed = 1;
		displayName = "Medical Supply Box";
		descriptionShort = "A heavy medical box. Open it and search it, it is too heavy to carry.";
		weight = 90000;
	};
	class MetalPlate: Inventory_Base
	{
		inventorySlot[] += {"aga_up_metal","aga_up_metal2","aga_up_metal3","aga_up_metal4"};
	};
	class ExplosivesBase;
	// Plastik zostaje vanillowy (raidy: improwizowany ladunek / zapalnik zdalny) - bez slotu na budzik
	class Plastic_Explosive: ExplosivesBase {};
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
	class Slot_aga_up_planks10
	{
		name = "aga_up_planks10";
		displayName = "Planks";
		ghostIcon = "set:dayz_inventory image:planks";
		stackMax = 10;
	};
	class Slot_aga_up_planks20
	{
		name = "aga_up_planks20";
		displayName = "Planks";
		ghostIcon = "set:dayz_inventory image:planks";
		stackMax = 20;
	};
	class Slot_aga_up_planks22
	{
		name = "aga_up_planks22";
		displayName = "Planks";
		ghostIcon = "set:dayz_inventory image:planks";
		stackMax = 22;
	};
	class Slot_aga_up_nails40
	{
		name = "aga_up_nails40";
		displayName = "Nails";
		ghostIcon = "set:dayz_inventory image:nails";
		stackMax = 40;
	};
	class Slot_aga_up_nails70
	{
		name = "aga_up_nails70";
		displayName = "Nails";
		ghostIcon = "set:dayz_inventory image:nails";
		stackMax = 70;
	};
	class Slot_aga_up_metal2
	{
		name = "aga_up_metal2";
		displayName = "Metal Sheets";
		ghostIcon = "set:dayz_inventory image:metalplate";
		stackMax = 2;
	};
	class Slot_aga_up_metal3
	{
		name = "aga_up_metal3";
		displayName = "Metal Sheets";
		ghostIcon = "set:dayz_inventory image:metalplate";
		stackMax = 3;
	};
	class Slot_aga_up_metal4
	{
		name = "aga_up_metal4";
		displayName = "Metal Sheets";
		ghostIcon = "set:dayz_inventory image:metalplate";
		stackMax = 4;
	};
	class Slot_aga_up_nails
	{
		name = "aga_up_nails";
		displayName = "Nails";
		ghostIcon = "set:dayz_inventory image:nails";
		stackMax = 99;
	};
	class Slot_aga_up_planks
	{
		name = "aga_up_planks";
		displayName = "Planks";
		ghostIcon = "set:dayz_inventory image:planks";
		stackMax = 99;
	};
	class Slot_aga_up_planks2
	{
		name = "aga_up_planks2";
		displayName = "Planks";
		ghostIcon = "set:dayz_inventory image:planks";
		stackMax = 99;
	};
	class Slot_aga_up_wire
	{
		name = "aga_up_wire";
		displayName = "Metal Wire";
		ghostIcon = "set:dayz_inventory image:metalwire";
	};
	class Slot_aga_up_rope
	{
		name = "aga_up_rope";
		displayName = "Rope";
		ghostIcon = "set:dayz_inventory image:rope";
	};
	class Slot_aga_up_metal
	{
		name = "aga_up_metal";
		displayName = "Metal Sheets";
		ghostIcon = "set:dayz_inventory image:metalplate";
		stackMax = 99;
	};
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
	// katastrofy eventow slyszalne z ~4 km
	class HeliCrash_Distant_SoundShader;
	class AGA_PlaneCrash_Far_SoundShader: HeliCrash_Distant_SoundShader
	{
		range = 4200;
		volume = 1.6;
	};
	class AGA_HeliCrash_Far_SoundShader: HeliCrash_Distant_SoundShader
	{
		range = 3200;
		volume = 1.3;
	};
};

class CfgSoundCurves
{
	class AGA_CrashFarCurve
	{
		points[] = {{0,1},{800,1},{1600,0.8},{2400,0.55},{3200,0.3},{4200,0}};
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
	class HeliCrash_Distant_Base_SoundSet;
	class AGA_PlaneCrash_Far_SoundSet: HeliCrash_Distant_Base_SoundSet
	{
		soundShaders[] = {"AGA_PlaneCrash_Far_SoundShader"};
		sound3DProcessingType = "ThunderNear3DProcessingType";
		volumeCurve = "AGA_CrashFarCurve";
	};
	class AGA_HeliCrash_Far_SoundSet: HeliCrash_Distant_Base_SoundSet
	{
		soundShaders[] = {"AGA_HeliCrash_Far_SoundShader"};
		sound3DProcessingType = "ThunderNear3DProcessingType";
		volumeCurve = "AGA_CrashFarCurve";
	};
};
