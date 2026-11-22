// ARMA-GEDON – magazyny bazy: skrzynie (ulepszane), gun rack, gun wall, gear stand, workbench, metalowe panele
// Wymagają własnego terytorium; ulepszenia odblokowuje poziom flagi.

class AGA_UpStep
{
	string From;
	string To;
	int Flag;
	ref array<string> Mat;
	ref array<int> Qty;
	void AGA_UpStep(string f, string t, int flag)
	{
		From = f;
		To = t;
		Flag = flag;
		Mat = new array<string>;
		Qty = new array<int>;
	}

}

class AGA_Build
{
	static const int MAX_RACKS = 2;
	static const int MAX_WALLS = 2;
	static const int MAX_STANDS = 2;
	static const int MAX_BENCHES = 1;
	static ref array<ref AGA_UpStep> s_Steps;

	static void Add(string f, string t, int flag, string spec)
	{
		AGA_UpStep st = new AGA_UpStep(f, t, flag);
		array<string> parts = new array<string>;
		spec.Split(",", parts);
		foreach (string p : parts)
		{
			array<string> kv = new array<string>;
			p.Split(":", kv);
			if (kv.Count() != 2) continue;
			st.Mat.Insert(kv[0]);
			st.Qty.Insert(kv[1].ToInt());
		}
		s_Steps.Insert(st);
	}

	static array<ref AGA_UpStep> Steps()
	{
		if (s_Steps) return s_Steps;
		s_Steps = new array<ref AGA_UpStep>;
		Add("WoodenCrate", "AGA_Crate_T2", 2, "WoodenPlank:10,Nail:18,MetalWire:1");
		Add("AGA_Crate_T2", "AGA_Crate_T3", 3, "WoodenPlank:14,Nail:22,AGA_MetalPanel:1");
		Add("AGA_Crate_T3", "AGA_Crate_T4", 3, "WoodenPlank:16,Nail:28,AGA_MetalPanel:1");
		Add("AGA_GunRack_T1", "AGA_GunRack_T2", 2, "WoodenPlank:8,Nail:40,BurlapSack:2,AGA_MetalPanel:2");
		Add("AGA_GunRack_T2", "AGA_GunRack_T3", 3, "WoodenPlank:16,Nail:64,MetalWire:1,Rope:1,AGA_MetalPanel:2");
		Add("AGA_GunWall_T1", "AGA_GunWall_T2", 2, "WoodenPlank:20,Nail:80,Rope:1,BurlapSack:4,MetalWire:1,AGA_MetalPanel:6");
		Add("AGA_GearStand_T1", "AGA_GearStand_T2", 2, "Rope:1,Nail:50,TannedLeather:10");
		Add("AGA_Workbench_T1", "AGA_Workbench_T2", 2, "WoodenPlank:12,BurlapSack:2,Nail:40,AGA_MetalPanel:2");
		Add("AGA_Workbench_T2", "AGA_Workbench_T3", 2, "WoodenPlank:20,BurlapSack:4,Nail:80,AGA_MetalPanel:6");
		return s_Steps;
	}

	static AGA_UpStep StepFor(EntityAI e)
	{
		if (!e) return null;
		foreach (AGA_UpStep s : Steps())
		{
			if (e.GetType() == s.From) return s;
		}
		return null;
	}

	static float Amount(ItemBase ib)
	{
		if (ib.HasQuantity()) return ib.GetQuantity();
		return 1;
	}

	static int Have(PlayerBase p, string type)
	{
		array<EntityAI> items = new array<EntityAI>;
		p.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		float n = 0;
		foreach (EntityAI e : items)
		{
			ItemBase ib = ItemBase.Cast(e);
			if (ib && ib.IsKindOf(type) && !ib.IsRuined()) n += Amount(ib);
		}
		return n;
	}

	static string Missing(PlayerBase p, AGA_UpStep s)
	{
		string out_txt = "";
		for (int i = 0; i < s.Mat.Count(); i++)
		{
			int have = Have(p, s.Mat[i]);
			if (have < s.Qty[i])
			{
				if (out_txt != "") out_txt += ", ";
				out_txt += (s.Qty[i] - have).ToString() + "x " + Label(s.Mat[i]);
			}
		}
		return out_txt;
	}

	static string Label(string type)
	{
		if (type == "WoodenPlank") return "planks";
		if (type == "Nail") return "nails";
		if (type == "MetalWire") return "metal wire";
		if (type == "AGA_MetalPanel") return "metal panels";
		if (type == "TannedLeather") return "tanned leather";
		if (type == "BurlapSack") return "burlap sacks";
		if (type == "Rope") return "rope";
		return type;
	}

	static void Consume(PlayerBase p, AGA_UpStep s)
	{
		array<EntityAI> items = new array<EntityAI>;
		p.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		for (int i = 0; i < s.Mat.Count(); i++)
		{
			float need = s.Qty[i];
			foreach (EntityAI e : items)
			{
				if (need <= 0) break;
				ItemBase ib = ItemBase.Cast(e);
				if (!ib || ib.IsRuined() || !ib.IsKindOf(s.Mat[i]) || ib.IsSetForDeletion()) continue;
				if (ib.HasQuantity())
				{
					float take = Math.Min(need, ib.GetQuantity());
					ib.AddQuantity(-take, true);
					need -= take;
				}
				else
				{
					need -= 1;
					GetGame().ObjectDelete(ib);
				}
			}
		}
	}

	// gracz we własnym terytorium o wymaganym poziomie flagi
	static TerritoryFlag OwnFlag(PlayerBase p, vector pos)
	{
		TerritoryFlag f = AGA_Territory.FlagAt(pos);
		if (!f || !p) return null;
		if (!p.AGA_IsStaff() && f.AGA_Gid() != p.AGA_Gid()) return null;
		return f;
	}

	static void Upgrade(PlayerBase p, EntityAI target)
	{
		AGA_UpStep s = StepFor(target);
		if (!s || !p) return;
		TerritoryFlag f = OwnFlag(p, target.GetPosition());
		if (!f || f.AGA_Level() < s.Flag) return;
		if (Missing(p, s) != "") return;
		Consume(p, s);
		vector pos = target.GetPosition();
		vector ori = target.GetOrientation();
		EntityAI nw = EntityAI.Cast(GetGame().CreateObjectEx(s.To, pos, ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
		if (!nw) return;
		nw.SetPosition(pos);
		nw.SetOrientation(ori);
		array<EntityAI> moved = new array<EntityAI>;
		target.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, moved);
		foreach (EntityAI it : moved)
		{
			if (!it || it == target || it.GetHierarchyParent() != target) continue;
			if (!nw.ServerTakeEntityToInventory(FindInventoryLocationType.ATTACHMENT | FindInventoryLocationType.CARGO, it))
				target.GetInventory().DropEntity(InventoryMode.SERVER, target, it);
		}
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().ObjectDelete, 500, false, target);
		AGA_Territory.Tell(p, "Upgraded: " + nw.GetDisplayName());
		AGA_Server.Get().Log("[BAZA] " + p.GetIdentity().GetName() + " ulepszyl " + s.From + " -> " + s.To + " @ " + pos.ToString());
	}

	static bool InOwnTerritory(PlayerBase p)
	{
		if (!p) return false;
		return OwnFlag(p, p.GetPosition()) != null;
	}

	static bool NearBench(vector pos, typename t, float r)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(pos, r, objs, null);
		foreach (Object o : objs)
		{
			if (o && o.IsInherited(t)) return true;
		}
		return false;
	}
}

// ==================================================================== skrzynie
class AGA_Crate_T2 : WoodenCrate
{
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool IsDeployable() { return false; }
	override bool IsTakeable() { return false; }
}
class AGA_Crate_T3 : AGA_Crate_T2 {}
class AGA_Crate_T4 : AGA_Crate_T2 {}

// ==================================================================== baza magazynów
class AGA_StorageBase : DeployableContainer_Base
{
	int AGA_Tier() { return 1; }

	// dozwolone typy w cargo i ich limity (puste = brak cargo)
	void AGA_Rules(array<string> types, array<int> limits) {}

	override bool CanPutInCargo(EntityAI parent)
	{
		if (AGA_Tier() > 1) return false;
		return super.CanPutInCargo(parent) && AGA_IsEmpty();
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		if (AGA_Tier() > 1) return false;
		return super.CanPutIntoHands(parent) && AGA_IsEmpty();
	}

	override bool IsTakeable()
	{
		return AGA_Tier() == 1 && super.IsTakeable();
	}

	override bool IsDeployable() { return AGA_Tier() == 1; }

	bool AGA_IsEmpty()
	{
		if (!GetInventory()) return true;
		if (GetInventory().AttachmentCount() > 0) return false;
		CargoBase c = GetInventory().GetCargo();
		return !c || c.GetItemCount() == 0;
	}

	override bool CanReceiveItemIntoCargo(EntityAI item)
	{
		if (!super.CanReceiveItemIntoCargo(item)) return false;
		array<string> types = new array<string>;
		array<int> limits = new array<int>;
		AGA_Rules(types, limits);
		CargoBase c = GetInventory().GetCargo();
		for (int i = 0; i < types.Count(); i++)
		{
			if (!item.IsKindOf(types[i])) continue;
			int n = 0;
			if (c)
			{
				for (int k = 0; k < c.GetItemCount(); k++)
				{
					EntityAI e = c.GetItem(k);
					if (e && e != item && e.IsKindOf(types[i])) n++;
				}
			}
			return n < limits[i];
		}
		return false;
	}

	override bool CanSwapItemInCargo(EntityAI child_entity, EntityAI new_entity)
	{
		return CanReceiveItemIntoCargo(new_entity) && super.CanSwapItemInCargo(child_entity, new_entity);
	}
}

class AGA_GunRackBase : AGA_StorageBase {}
class AGA_GunRack_T1 : AGA_GunRackBase {}
class AGA_GunRack_T2 : AGA_GunRackBase
{
	override int AGA_Tier() { return 2; }
	override void AGA_Rules(array<string> types, array<int> limits) { types.Insert("AmmoBox"); limits.Insert(2); }
}
class AGA_GunRack_T3 : AGA_GunRackBase
{
	override int AGA_Tier() { return 3; }
	override void AGA_Rules(array<string> types, array<int> limits) { types.Insert("AmmoBox"); limits.Insert(3); types.Insert("Pistol_Base"); limits.Insert(3); }
}

class AGA_GunWallBase : AGA_StorageBase {}
class AGA_GunWall_T1 : AGA_GunWallBase {}
class AGA_GunWall_T2 : AGA_GunWallBase
{
	override int AGA_Tier() { return 2; }
	override void AGA_Rules(array<string> types, array<int> limits) { types.Insert("Grenade_Base"); limits.Insert(8); }
}

class AGA_GearStandBase : AGA_StorageBase {}
class AGA_GearStand_T1 : AGA_GearStandBase {}
class AGA_GearStand_T2 : AGA_GearStandBase
{
	override int AGA_Tier() { return 2; }
}

class AGA_WorkbenchBase : AGA_StorageBase {}
class AGA_Workbench_T1 : AGA_WorkbenchBase
{
	override void AGA_Rules(array<string> types, array<int> limits)
	{
		types.Insert("Shovel"); limits.Insert(1);
		types.Insert("FieldShovel"); limits.Insert(1);
		types.Insert("WoodAxe"); limits.Insert(1);
		types.Insert("FirefighterAxe"); limits.Insert(1);
		types.Insert("Pickaxe"); limits.Insert(1);
		types.Insert("Sledgehammer"); limits.Insert(1);
		types.Insert("Hammer"); limits.Insert(1);
		types.Insert("Nail"); limits.Insert(2);
		types.Insert("WoodenPlank"); limits.Insert(2);
	}
}
class AGA_Workbench_T2 : AGA_WorkbenchBase
{
	override int AGA_Tier() { return 2; }
	override void AGA_Rules(array<string> types, array<int> limits)
	{
		types.Insert("Hatchet"); limits.Insert(1);
		types.Insert("Shovel"); limits.Insert(1);
		types.Insert("FieldShovel"); limits.Insert(1);
		types.Insert("WoodAxe"); limits.Insert(1);
		types.Insert("FirefighterAxe"); limits.Insert(1);
		types.Insert("Pickaxe"); limits.Insert(1);
		types.Insert("HandSaw"); limits.Insert(1);
		types.Insert("Hacksaw"); limits.Insert(1);
		types.Insert("Pliers"); limits.Insert(1);
		types.Insert("Screwdriver"); limits.Insert(1);
		types.Insert("Sledgehammer"); limits.Insert(1);
		types.Insert("Nail"); limits.Insert(3);
		types.Insert("WoodenPlank"); limits.Insert(3);
	}
}
class AGA_Workbench_T3 : AGA_Workbench_T2
{
	override int AGA_Tier() { return 3; }
}

// naprawa do stanu idealnego przy workbenchu poziomu 3
modded class ItemBase
{
	override bool CanBeRepairedToPristine()
	{
		if (super.CanBeRepairedToPristine()) return true;
		vector pos = GetPosition();
		EntityAI root = GetHierarchyRoot();
		if (root) pos = root.GetPosition();
		return AGA_Build.NearBench(pos, AGA_Workbench_T3, 4);
	}
}

// ==================================================================== metalowe panele
class AGA_MetalPanel : ItemBase {}

// ==================================================================== akcja ulepszenia (młotek w rękach)
class AGA_ActionUpgradeCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(8);
	}
}

class AGA_ActionUpgrade : ActionContinuousBase
{
	void AGA_ActionUpgrade()
	{
		m_CallbackClass = AGA_ActionUpgradeCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_ASSEMBLE;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "Upgrade";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!item || !item.IsKindOf("Hammer") || !target) return false;
		EntityAI e = EntityAI.Cast(target.GetObject());
		if (!e || e.GetHierarchyParent()) return false;
		AGA_UpStep s = AGA_Build.StepFor(e);
		if (!s) return false;
		TerritoryFlag f = AGA_Build.OwnFlag(player, e.GetPosition());
		if (!f) return false;
		if (f.AGA_Level() < s.Flag)
		{
			AGA_Territory.Msg("This upgrade requires a level " + s.Flag.ToString() + " flag pole.");
			return false;
		}
		if (!GetGame().IsDedicatedServer())
		{
			string miss = AGA_Build.Missing(player, s);
			if (miss != "")
			{
				AGA_Territory.Msg("Upgrade needs: " + miss + " (in your inventory).");
				return false;
			}
		}
		m_Text = "Upgrade to " + GetGame().ConfigGetTextOut("CfgVehicles " + s.To + " displayName");
		return true;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		EntityAI e = EntityAI.Cast(action_data.m_Target.GetObject());
		AGA_Build.Upgrade(action_data.m_Player, e);
		MiscGameplayFunctions.DealAbsoluteDmg(action_data.m_MainItem, 2);
	}
}

modded class Hammer
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionUpgrade);
	}
}

// ==================================================================== receptury
class AGA_RecipeSimple extends RecipeBase
{
	void Two(string a, int qa, string b, int qb, string result)
	{
		m_IsInstaRecipe = false;
		m_AnimationLength = 3;
		m_Specialty = -0.02;
		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = 3;
		m_MinQuantityIngredient[0] = qa;
		m_MaxQuantityIngredient[0] = -1;
		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = qb;
		m_MaxQuantityIngredient[1] = -1;
		InsertIngredient(0, a);
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = -qa;
		m_IngredientDestroy[0] = false;
		m_IngredientUseSoftSkills[0] = false;
		InsertIngredient(1, b);
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = -qb;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;
		AddResult(result);
		m_ResultSetFullQuantity[0] = false;
		m_ResultSetQuantity[0] = -1;
		m_ResultSetHealth[0] = -1;
		m_ResultInheritsHealth[0] = -1;
		m_ResultInheritsColor[0] = -1;
		m_ResultToInventory[0] = -2;
		m_ResultUseSoftSkills[0] = false;
		m_ResultReplacesIngredient[0] = -1;
	}
}

// meble bazy: tylko we własnym terytorium
class AGA_RecipeBaseItem extends AGA_RecipeSimple
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		return AGA_Build.InOwnTerritory(player);
	}
}

class AGA_CraftGunRack extends AGA_RecipeBaseItem
{
	override void Init() { m_Name = "Craft gun rack"; Two("WoodenPlank", 5, "Nail", 15, "AGA_GunRack_T1"); }
}
class AGA_CraftGunWall extends AGA_RecipeBaseItem
{
	override void Init() { m_Name = "Craft gun wall"; Two("WoodenPlank", 5, "Nail", 15, "AGA_GunWall_T1"); }
}
class AGA_CraftGearStand extends AGA_RecipeBaseItem
{
	override void Init() { m_Name = "Craft gear stand"; Two("WoodenPlank", 5, "Nail", 20, "AGA_GearStand_T1"); }
}
class AGA_CraftWorkbench extends AGA_RecipeBaseItem
{
	override void Init() { m_Name = "Craft workbench"; Two("WoodenPlank", 10, "Nail", 20, "AGA_Workbench_T1"); }
}

class AGA_CraftMetalPanels extends AGA_RecipeSimple
{
	override void Init()
	{
		m_Name = "Cut into metal panels";
		Two("MetalPlate", -1, "Hacksaw", -1, "AGA_MetalPanel");
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = true;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientAddHealth[1] = -4;
		m_ResultSetQuantity[0] = 4;
	}
}

class AGA_CraftMetalSheet extends AGA_RecipeSimple
{
	override void Init()
	{
		m_Name = "Weld into metal sheet";
		Two("AGA_MetalPanel", 6, "Blowtorch", -1, "MetalPlate");
		m_IngredientAddQuantity[1] = 0;
	}
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new AGA_CraftGunRack);
		RegisterRecipe(new AGA_CraftGunWall);
		RegisterRecipe(new AGA_CraftGearStand);
		RegisterRecipe(new AGA_CraftWorkbench);
		RegisterRecipe(new AGA_CraftMetalPanels);
		RegisterRecipe(new AGA_CraftMetalSheet);
	}
}
