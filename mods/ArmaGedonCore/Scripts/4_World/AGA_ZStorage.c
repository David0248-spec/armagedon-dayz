// ARMA-GEDON – meble bazy na modelach MMG Base Storage, ulepszane tierami.
// Ulepszenie: materiały wkłada się w sloty "Upgrade Materials" obiektu, potem siekiera w ręce -> "Upgrade".
// Zawartość przechodzi do nowego obiektu. Poziom ulepszenia wymaga poziomu flagi.
//
//  Skrzynia:   WoodenCrate (50) -> T2 mmg_supplycrate_level1 (100) -> T3 level2 (150) -> T4 level3 (200)
//  Gun rack:   mmg_craftable_gun_rack (2 bronie) -> mmg_gun_rack_tan (4 + 2 ammo) -> mmg_gun_rack_green (4 + 3 ammo + 3 pistolety)
//  Gun wall:   mmg_gun_wall_wooden (4 bronie) -> mmg_gun_wall_metal (8 broni + 8 granatów)
//  Gear stand: mmg_gear_stand (hełm + kamizelka) -> AGA_GearStand2 (pełny strój)
//  Workbench:  mmg_old_table -> mmg_toolwagon_red (naprawa do pristine)

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
		Add("WoodenCrate", "mmg_supplycrate_level1", 2, "WoodenPlank:10,Nail:40,MetalWire:1");
		Add("mmg_supplycrate_level1", "mmg_supplycrate_level2", 3, "WoodenPlank:20,Nail:70,MetalWire:1,MetalPlate:2");
		Add("mmg_supplycrate_level2", "mmg_supplycrate_level3", 3, "WoodenPlank:22,Nail:70,MetalWire:1,MetalPlate:4");
		Add("mmg_craftable_gun_rack", "mmg_gun_rack_tan", 2, "Nail:40,MetalPlate:2,MetalWire:1");
		Add("mmg_gun_rack_tan", "mmg_gun_rack_green", 3, "Nail:70,MetalPlate:4,MetalWire:1");
		Add("mmg_gun_wall_wooden", "mmg_gun_wall_metal", 2, "WoodenPlank:10,Nail:70,MetalPlate:4,MetalWire:1");
		Add("mmg_gear_stand", "AGA_GearStand2", 2, "WoodenPlank:10,Nail:40,Rope:1");
		Add("mmg_old_table", "mmg_toolwagon_red", 2, "WoodenPlank:10,Nail:70,MetalPlate:3,MetalWire:1");
		return s_Steps;
	}

	static AGA_UpStep StepFor(EntityAI e)
	{
		if (!e) return null;
		string t = e.GetType();
		foreach (AGA_UpStep s : Steps())
		{
			if (t == s.From) return s;
		}
		return null;
	}

	static string Label(string type)
	{
		if (type == "WoodenPlank") return "planks";
		if (type == "Nail") return "nails";
		if (type == "MetalPlate") return "metal sheets";
		if (type == "MetalWire") return "metal wire";
		if (type == "Rope") return "rope";
		return type;
	}

	static float Amount(EntityAI e)
	{
		ItemBase ib = ItemBase.Cast(e);
		if (ib && ib.HasQuantity()) return ib.GetQuantity();
		return 1;
	}

	static bool InUpSlot(EntityAI a)
	{
		InventoryLocation loc = new InventoryLocation;
		if (!a.GetInventory().GetCurrentInventoryLocation(loc)) return false;
		if (loc.GetType() != InventoryLocationType.ATTACHMENT) return false;
		string sn = InventorySlots.GetSlotName(loc.GetSlot());
		sn.ToLower();
		return sn.IndexOf("aga_up_") == 0;
	}

	// ilość materiału w slotach ulepszenia obiektu
	static float InSlots(EntityAI box, string type)
	{
		float n = 0;
		if (!box || !box.GetInventory()) return 0;
		for (int i = 0; i < box.GetInventory().AttachmentCount(); i++)
		{
			EntityAI a = box.GetInventory().GetAttachmentFromIndex(i);
			if (a && !a.IsRuined() && a.IsKindOf(type) && InUpSlot(a)) n += Amount(a);
		}
		return n;
	}

	static string Missing(EntityAI box, AGA_UpStep s)
	{
		string txt = "";
		for (int i = 0; i < s.Mat.Count(); i++)
		{
			int have = InSlots(box, s.Mat[i]);
			if (have < s.Qty[i])
			{
				if (txt != "") txt += ", ";
				txt += (s.Qty[i] - have).ToString() + "x " + Label(s.Mat[i]);
			}
		}
		return txt;
	}

	static void Consume(EntityAI box, AGA_UpStep s)
	{
		for (int i = 0; i < s.Mat.Count(); i++)
		{
			float need = s.Qty[i];
			for (int k = box.GetInventory().AttachmentCount() - 1; k >= 0 && need > 0; k--)
			{
				EntityAI a = box.GetInventory().GetAttachmentFromIndex(k);
				if (!a || a.IsRuined() || !a.IsKindOf(s.Mat[i]) || !InUpSlot(a)) continue;
				ItemBase ib = ItemBase.Cast(a);
				if (ib && ib.HasQuantity() && ib.GetQuantity() > need)
				{
					ib.AddQuantity(-need, false);
					need = 0;
				}
				else
				{
					need -= Amount(a);
					GetGame().ObjectDelete(a);
				}
			}
		}
	}

	// gracz we własnym terytorium
	static TerritoryFlag OwnFlag(PlayerBase p, vector pos)
	{
		TerritoryFlag f = AGA_Territory.FlagAt(pos);
		if (!f || !p) return null;
		if (f.AGA_Gid() == 0 || f.AGA_Gid() != p.AGA_Gid()) return null;
		return f;
	}

	static bool InOwnTerritory(PlayerBase p)
	{
		if (!p) return false;
		return OwnFlag(p, p.GetPosition()) != null;
	}

	// podmiana obiektu na inny typ w tym samym miejscu z przeniesieniem zawartości
	static EntityAI Swap(EntityAI old, string to)
	{
		if (!old || to == "") return null;
		vector pos = old.GetPosition();
		vector ori = old.GetOrientation();
		EntityAI nw = EntityAI.Cast(GetGame().CreateObjectEx(to, pos, ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
		if (!nw) return null;
		nw.SetPosition(pos);
		nw.SetOrientation(ori);
		nw.Update();
		ItemBase oib = ItemBase.Cast(old);
		ItemBase nib = ItemBase.Cast(nw);
		bool wasClosed = false;
		if (oib && oib.IsOpen() == false && oib.ConfigGetBool("itemIsOpenable")) { wasClosed = true; oib.Open(); }
		if (nib && nib.ConfigGetBool("itemIsOpenable")) nib.Open();

		array<EntityAI> items = new array<EntityAI>;
		old.GetInventory().EnumerateInventory(InventoryTraversalType.LEVELORDER, items);
		foreach (EntityAI it : items)
		{
			if (!it || it == old || it.GetHierarchyParent() != old) continue;
			if (!nw.ServerTakeEntityToInventory(FindInventoryLocationType.ATTACHMENT | FindInventoryLocationType.CARGO, it))
				old.GetInventory().DropEntity(InventoryMode.SERVER, old, it);
		}
		if (nib && wasClosed) GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(nib.Close, 1500, false);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(GetGame().ObjectDelete, 500, false, old);
		return nw;
	}

	static void Upgrade(PlayerBase p, EntityAI target)
	{
		AGA_UpStep s = StepFor(target);
		if (!s || !p || target.IsSetForDeletion()) return;
		string why = Blocker(p, target, s);
		if (why != "")
		{
			AGA_Territory.Tell(p, "Upgrade needs: " + why + ".");
			return;
		}
		Consume(target, s);
		EntityAI nw = Swap(target, s.To);
		if (!nw) return;
		AGA_Territory.Tell(p, "Upgraded: " + nw.GetDisplayName());
		AGA_Server.Get().Log("[BAZA] " + p.GetIdentity().GetName() + " ulepszyl " + s.From + " -> " + s.To + " @ " + target.GetPosition().ToString());
	}

	// ---------------------------------------------------------------- zasady cargo z configu (aga_cargoTypes / aga_cargoLimits)
	static bool CargoOk(EntityAI box, EntityAI item)
	{
		string base = "CfgVehicles " + box.GetType();
		if (!GetGame().ConfigIsExisting(base + " aga_cargoTypes"))
		{
			// brak listy = cargo wolne, chyba że obiekt nie ma cargo
			return true;
		}
		TStringArray types = new TStringArray;
		TIntArray limits = new TIntArray;
		GetGame().ConfigGetTextArray(base + " aga_cargoTypes", types);
		GetGame().ConfigGetIntArray(base + " aga_cargoLimits", limits);
		CargoBase c = box.GetInventory().GetCargo();
		for (int i = 0; i < types.Count() && i < limits.Count(); i++)
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

	static int Tier(EntityAI e)
	{
		if (!e) return 0;
		return e.ConfigGetInt("aga_tier");
	}

	static bool Fixed(EntityAI e)
	{
		return e && e.ConfigGetInt("aga_fixed") == 1;
	}

	// skrzyń eventów nie da się podnieść; meble bazy – tylko puste
	static bool Locked(EntityAI e)
	{
		return Fixed(e);
	}

	static bool IsEmptyBox(EntityAI e)
	{
		if (!e || !e.GetInventory()) return true;
		if (e.GetInventory().AttachmentCount() > 0) return false;
		CargoBase c = e.GetInventory().GetCargo();
		return !c || c.GetItemCount() == 0;
	}

	// cel akcji: sam obiekt, nawet gdy celownik trafia w przedmiot leżący na nim
	static EntityAI Furniture(ActionTarget target)
	{
		if (!target) return null;
		EntityAI e = EntityAI.Cast(target.GetObject());
		if (e && StepFor(e)) return e;
		EntityAI par = EntityAI.Cast(target.GetParent());
		if (par && StepFor(par)) return par;
		if (e && e.GetHierarchyParent())
		{
			EntityAI root = e.GetHierarchyRoot();
			if (root && StepFor(root)) return root;
		}
		return null;
	}

	// najbliższy mebel do ulepszenia przy graczu (nie zależy od celownika – cienkie modele jak gun wall)
	static EntityAI Nearest(PlayerBase p, ActionTarget target)
	{
		EntityAI t = Furniture(target);
		if (t) return t;
		if (!p) return null;
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(p.GetPosition(), 3.0, objs, null);
		vector dir = p.GetDirection();
		EntityAI best = null;
		float bestScore = 1000;
		foreach (Object o : objs)
		{
			EntityAI e = EntityAI.Cast(o);
			if (!e || e.GetHierarchyParent() || e.IsSetForDeletion() || !StepFor(e)) continue;
			vector to = e.GetPosition() - p.GetPosition();
			to[1] = 0;
			float dist = to.Length();
			if (dist > 0.1) to.Normalize();
			float facing = vector.Dot(dir, to);
			if (dist > 0.8 && facing < 0.2) continue;
			float score = dist - facing;
			if (score < bestScore) { bestScore = score; best = e; }
		}
		return best;
	}

	// co blokuje ulepszenie ("" = można)
	static string Blocker(PlayerBase p, EntityAI e, AGA_UpStep s)
	{
		TerritoryFlag f = OwnFlag(p, e.GetPosition());
		if (!f) return "your group's flag pole within 30 m";
		if (f.AGA_Level() < s.Flag) return "flag pole level " + s.Flag.ToString();
		string miss = Missing(e, s);
		if (miss != "") return miss + " in Upgrade Materials";
		return "";
	}

	static bool NearBench(vector pos, float r)
	{
		array<Object> objs = new array<Object>;
		GetGame().GetObjectsAtPosition(pos, r, objs, null);
		foreach (Object o : objs)
		{
			if (o && o.IsKindOf("mmg_toolwagon_base")) return true;
		}
		return false;
	}

	// stare meble ARMA-GEDON -> nowe modele (zawartość zostaje)
	static string LegacyTarget(string t)
	{
		if (t == "AGA_Crate_T2") return "mmg_supplycrate_level1";
		if (t == "AGA_Crate_T3") return "mmg_supplycrate_level2";
		if (t == "AGA_Crate_T4") return "mmg_supplycrate_level3";
		if (t == "AGA_GunRack_T1") return "mmg_craftable_gun_rack";
		if (t == "AGA_GunRack_T2") return "mmg_gun_rack_tan";
		if (t == "AGA_GunRack_T3") return "mmg_gun_rack_green";
		if (t == "AGA_GunWall_T1") return "mmg_gun_wall_wooden";
		if (t == "AGA_GunWall_T2") return "mmg_gun_wall_metal";
		if (t == "AGA_GearStand_T1") return "mmg_gear_stand";
		if (t == "AGA_GearStand_T2") return "AGA_GearStand2";
		if (t == "AGA_Workbench_T1") return "mmg_old_table";
		if (t == "AGA_Workbench_T2") return "mmg_toolwagon_red";
		if (t == "AGA_Workbench_T3") return "mmg_toolwagon_red";
		return "";
	}

	static void ConvertLegacy(EntityAI e)
	{
		if (!e || e.IsSetForDeletion() || e.GetHierarchyParent()) return;
		string to = LegacyTarget(e.GetType());
		if (to == "") return;
		Swap(e, to);
		AGA_Server.Get().Log("[BAZA] konwersja " + e.GetType() + " -> " + to + " @ " + e.GetPosition().ToString());
	}
}

// ==================================================================== stare klasy (konwertowane przy wczytaniu)
class AGA_Crate_T2 : WoodenCrate
{
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool IsTakeable() { return false; }
	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		if (GetGame().IsServer()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Build.ConvertLegacy, 3000, false, this);
	}
}
class AGA_Crate_T3 : AGA_Crate_T2 {}
class AGA_Crate_T4 : AGA_Crate_T2 {}

class AGA_StorageBase : DeployableContainer_Base
{
	override bool CanPutInCargo(EntityAI parent) { return false; }
	override bool CanPutIntoHands(EntityAI parent) { return false; }
	override bool IsTakeable() { return false; }
	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(AGA_Build.ConvertLegacy, 3000, false, this);
	}
}
class AGA_GunRackBase : AGA_StorageBase {}
class AGA_GunRack_T1 : AGA_GunRackBase {}
class AGA_GunRack_T2 : AGA_GunRackBase {}
class AGA_GunRack_T3 : AGA_GunRackBase {}
class AGA_GunWallBase : AGA_StorageBase {}
class AGA_GunWall_T1 : AGA_GunWallBase {}
class AGA_GunWall_T2 : AGA_GunWallBase {}
class AGA_GearStandBase : AGA_StorageBase {}
class AGA_GearStand_T1 : AGA_GearStandBase {}
class AGA_GearStand_T2 : AGA_GearStandBase {}
class AGA_WorkbenchBase : AGA_StorageBase {}
class AGA_Workbench_T1 : AGA_WorkbenchBase {}
class AGA_Workbench_T2 : AGA_WorkbenchBase {}
class AGA_Workbench_T3 : AGA_WorkbenchBase {}
class AGA_MetalPanel : ItemBase {}

// ==================================================================== zachowanie mebli MMG
modded class mmg_storage_placeable_base
{
	override bool CanPutInCargo(EntityAI parent)
	{
		if (AGA_Build.Locked(this) || !AGA_Build.IsEmptyBox(this)) return false;
		return super.CanPutInCargo(parent);
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		if (AGA_Build.Locked(this) || !AGA_Build.IsEmptyBox(this)) return false;
		return super.CanPutIntoHands(parent);
	}

	override bool IsTakeable()
	{
		if (AGA_Build.Locked(this)) return false;
		return super.IsTakeable();
	}

	override bool CanReceiveItemIntoCargo(EntityAI item)
	{
		if (!super.CanReceiveItemIntoCargo(item)) return false;
		return AGA_Build.CargoOk(this, item);
	}

	override bool CanSwapItemInCargo(EntityAI child_entity, EntityAI new_entity)
	{
		if (!super.CanSwapItemInCargo(child_entity, new_entity)) return false;
		return AGA_Build.CargoOk(this, new_entity);
	}
}

modded class mmg_storage_openable_noLock_base
{
	override bool CanPutInCargo(EntityAI parent)
	{
		if (AGA_Build.Locked(this) || !AGA_Build.IsEmptyBox(this)) return false;
		return super.CanPutInCargo(parent);
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		if (AGA_Build.Locked(this) || !AGA_Build.IsEmptyBox(this)) return false;
		return super.CanPutIntoHands(parent);
	}

	override bool IsTakeable()
	{
		if (AGA_Build.Locked(this)) return false;
		return super.IsTakeable();
	}
}

// drewniana skrzynia – przenoszenie tylko pustej
modded class WoodenCrate
{
	override bool CanPutIntoHands(EntityAI parent)
	{
		if (!AGA_Build.IsEmptyBox(this)) return false;
		return super.CanPutIntoHands(parent);
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		if (!AGA_Build.IsEmptyBox(this)) return false;
		return super.CanPutInCargo(parent);
	}
}

// gun wall (MMG blokuje podnoszenie) – pusty można przenieść
modded class mmg_gun_wall_base
{
	override bool CanPutIntoHands(EntityAI parent)
	{
		return AGA_Build.IsEmptyBox(this);
	}

	override bool IsTakeable()
	{
		return AGA_Build.IsEmptyBox(this);
	}
}

// natywne ulepszenie MMG (młotek, tylko pusta skrzynia, bez flagi) – zastąpione naszym
modded class ActionUpgradeSupplyCrate_1to2
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item) { return false; }
}
modded class ActionUpgradeSupplyCrate_2to3
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item) { return false; }
}

// naprawa do stanu idealnego przy workbenchu tier 2 (wózek narzędziowy)
modded class ItemBase
{
	override bool CanBeRepairedToPristine()
	{
		if (super.CanBeRepairedToPristine()) return true;
		vector pos = GetPosition();
		EntityAI root = GetHierarchyRoot();
		if (root) pos = root.GetPosition();
		return AGA_Build.NearBench(pos, 4);
	}
}

// ==================================================================== akcja ulepszenia (siekiera w rękach)
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
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
		m_Text = "Upgrade";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined();
		m_ConditionTarget = new CCTNone();
	}

	override bool HasTarget() { return false; }

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!item || player.IsPlacingLocal()) return false;
		EntityAI e = AGA_Build.Nearest(player, target);
		if (!e) return false;
		AGA_UpStep s = AGA_Build.StepFor(e);
		if (!s) return false;
		string to = GetGame().ConfigGetTextOut("CfgVehicles " + s.To + " displayName");
		string why = AGA_Build.Blocker(player, e, s);
		if (why == "") m_Text = "Upgrade to " + to;
		else m_Text = "Upgrade to " + to + " (needs: " + why + ")";
		return true;
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		EntityAI e = AGA_Build.Nearest(action_data.m_Player, action_data.m_Target);
		if (!e) return;
		AGA_UpStep s = AGA_Build.StepFor(e);
		bool ok = false;
		if (s && AGA_Build.Blocker(action_data.m_Player, e, s) == "") ok = true;
		AGA_Server.Get().Log("[BAZA] proba ulepszenia (siekiera) " + e.GetType() + " ok=" + ok.ToString());
		AGA_Build.Upgrade(action_data.m_Player, e);
		if (ok) MiscGameplayFunctions.DealAbsoluteDmg(action_data.m_MainItem, 4);
	}
}

modded class Hatchet
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

// gun rack bez półproduktu "gun rack planks"
class AGA_CraftGunRack extends AGA_RecipeSimple
{
	override void Init() { m_Name = "Craft gun rack"; Two("WoodenPlank", 10, "Nail", 30, "mmg_craftable_gun_rack"); }
}

// receptury MMG spoza budowania ARMA-GEDON – wyłączone
modded class craft_mmg_craftable_tent_kit { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; } }
modded class craft_mmg_wall_shelf { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; } }
modded class craft_mmg_gun_wall_kit_metal { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; } }
modded class craft_mmg_supplycrate_level1 { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; } }
modded class craft_mmg_gun_rack_planks { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; } }
modded class craft_mmg_craftable_gun_rack { override bool CanDo(ItemBase ingredients[], PlayerBase player) { return false; } }

// przepisy MMG "zniszcz / rozbierz": bez siekiery (siekiera = ulepszanie), tylko puste i we własnym terytorium
class AGA_RecipeGuard
{
	static bool Ok(ItemBase ingredients[], PlayerBase player)
	{
		if (!AGA_Build.InOwnTerritory(player)) return false;
		for (int i = 0; i < 2; i++)
		{
			ItemBase ib = ingredients[i];
			if (ib && AGA_Territory.BaseCat(ib, false) != "" && !AGA_Build.IsEmptyBox(ib)) return false;
		}
		return true;
	}
}

modded class destroy_mmgsupplycrate_lvl1
{
	override void Init() { super.Init(); RemoveIngredient(1, "Hatchet"); }
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class destroy_mmgsupplycrate_lvl2
{
	override void Init() { super.Init(); RemoveIngredient(1, "Hatchet"); }
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class destroy_mmgsupplycrate_lvl3
{
	override void Init() { super.Init(); RemoveIngredient(1, "Hatchet"); }
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_mmg_craftable_gun_rack
{
	override void Init() { super.Init(); RemoveIngredient(1, "Hatchet"); InsertIngredient(1, "Screwdriver"); }
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_mmg_gun_rack_tan
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_mmg_gun_rack_green
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_mmg_gun_wall_wooden
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_mmg_gun_wall_metal
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_gear_stand
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}
modded class dismantle_mmg_toolwagon_red
{
	override bool CanDo(ItemBase ingredients[], PlayerBase player) { return super.CanDo(ingredients, player) && AGA_RecipeGuard.Ok(ingredients, player); }
}

modded class PluginRecipesManagerBase
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new AGA_CraftGunRack);
	}
}

// ==================================================================== ulepszenie pod F (interakcja z obiektem, toporek w ręku)
class AGA_ActionUpgradeF : ActionInteractBase
{
	void AGA_ActionUpgradeF()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_Text = "Upgrade";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCIDummy();
		m_ConditionTarget = new CCTNone();
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (!item || !item.IsKindOf("Hatchet") || item.IsRuined()) return false;
		EntityAI e = AGA_Build.Nearest(player, target);
		if (!e) return false;
		AGA_UpStep s = AGA_Build.StepFor(e);
		if (!s) return false;
		string to = GetGame().ConfigGetTextOut("CfgVehicles " + s.To + " displayName");
		string why = AGA_Build.Blocker(player, e, s);
		if (why == "") m_Text = "Upgrade to " + to;
		else m_Text = "Upgrade to " + to + " (needs: " + why + ")";
		return true;
	}

	override void OnExecuteServer(ActionData action_data)
	{
		EntityAI e = AGA_Build.Nearest(action_data.m_Player, action_data.m_Target);
		if (!e) return;
		AGA_UpStep s = AGA_Build.StepFor(e);
		if (!s) return;
		string why = AGA_Build.Blocker(action_data.m_Player, e, s);
		AGA_Server.Get().Log("[BAZA] proba ulepszenia " + e.GetType() + " przez " + action_data.m_Player.GetIdentity().GetName() + " brak: '" + why + "'");
		AGA_Build.Upgrade(action_data.m_Player, e);
		if (why == "" && action_data.m_MainItem) MiscGameplayFunctions.DealAbsoluteDmg(action_data.m_MainItem, 4);
	}
}

modded class WoodenCrate
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionUpgradeF);
	}
}

modded class mmg_storage_placeable_base
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionUpgradeF);
	}
}

modded class mmg_storage_openable_noLock_base
{
	override void SetActions()
	{
		super.SetActions();
		AddAction(AGA_ActionUpgradeF);
	}
}
