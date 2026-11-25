// ARMA-GEDON: strzalki przy pasku kondycji (tylko klient)
// Zielona strzalka w gore = kondycja rosnie, czerwona w dol = spada. Znika po 6 s.
// Odpowiednik DisplayStaminaArrows z KK_StaminaSoftSkill: wartosc przychodzi z serwera
// (m_AGA_StaminaSkill), a strzalki doklejamy do panelu gracza w HUD obok paska.

class AGA_StaminaArrowsUI
{
	static ImageWidget s_Up;
	static ImageWidget s_Down;
	static float s_Last;
	static bool s_HasLast;

	static void Attach(Widget hudPanel)
	{
		s_Up = null;
		s_Down = null;
		s_HasLast = false;
		if (!hudPanel) return;
		// StaminaBar to pasek kondycji z IngameHud (5_Mission/GUI/IngameHud.c w grze);
		// strzalki doklejamy do jego rodzica, wiec stoja tuz przy pasku
		Widget parent = null;
		Widget bar = hudPanel.FindAnyWidget("StaminaBar");
		if (bar) parent = bar.GetParent();
		if (!parent) parent = hudPanel.FindAnyWidget("PlayerPanel");
		if (!parent) parent = hudPanel;
		Widget root = GetGame().GetWorkspace().CreateWidgets("ArmaGedonCore/GUI/layouts/aga_stamina_arrows.layout", parent);
		if (!root) return;
		s_Up = ImageWidget.Cast(root.FindAnyWidget("AGA_StaminaArrowUp"));
		s_Down = ImageWidget.Cast(root.FindAnyWidget("AGA_StaminaArrowDown"));
	}

	static void Hide()
	{
		if (s_Up) s_Up.Show(false);
		if (s_Down) s_Down.Show(false);
	}

	static void Update(float value)
	{
		if (!s_Up || !s_Down) return;
		if (!s_HasLast)
		{
			// pierwsza wartosc po wejsciu na serwer - bez strzalki, tylko zapamietujemy
			s_HasLast = true;
			s_Last = value;
			return;
		}
		if (value > s_Last)
		{
			s_Up.Show(true);
			s_Down.Show(false);
		}
		else if (value < s_Last)
		{
			s_Up.Show(false);
			s_Down.Show(true);
		}
		else
		{
			return;
		}
		s_Last = value;
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Hide);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(Hide, 6000, false);
	}
}

modded class IngameHud
{
	override void Init(Widget hud_panel_widget)
	{
		super.Init(hud_panel_widget);
		AGA_StaminaArrowsUI.Attach(m_HudPanelWidget);
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);
		if (!AGA_Stamina.s_ClientDirty) return;
		AGA_Stamina.s_ClientDirty = false;
		AGA_StaminaArrowsUI.Update(AGA_Stamina.s_ClientValue);
	}
}
