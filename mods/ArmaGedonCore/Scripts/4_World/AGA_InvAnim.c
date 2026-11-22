// ARMA-GEDON: animacja przegladania ekwipunku (postac przeszukuje plecak, gdy gracz ma otwarty ekwipunek)
const int AGA_UDT_INVVIEW = 7351904;

class AGA_InvAnimCallback extends HumanCommandActionCallback {}

class AGA_InvAnim
{
	PlayerBase m_P;
	HumanCommandActionCallback m_Cb;
	bool m_Synced;     // stan znany serwerowi
	bool m_Open;       // stan lokalny (klient)
	float m_Delay;

	void AGA_InvAnim(PlayerBase p) { m_P = p; }

	bool CanPlay(bool running)
	{
		if (!m_Synced || m_P.IsRestrained() || m_P.IsRestrainStarted() || m_P.IsUnconscious() || m_P.IsSwimming() || m_P.IsClimbing()) return false;
		if (!running && m_P.GetCommandModifier_Action()) return false;
		if (running && !AGA_InvAnimCallback.Cast(m_P.GetCommandModifier_Action())) return false;
		ActionManagerBase am = m_P.GetActionManager();
		if (am && am.GetRunningAction()) return false;
		if (m_P.GetDayZPlayerInventory() && m_P.GetDayZPlayerInventory().IsProcessing()) return false;
		if (m_P.GetWeaponManager() && m_P.GetWeaponManager().IsRunning()) return false;
		ItemBase h = m_P.GetItemInHands();
		if (h && h.IsHeavyBehaviour()) return false;
		return true;
	}

	void Update(int cmd, float dt)
	{
		if (!GetGame().IsDedicatedServer() && m_P == GetGame().GetPlayer())
		{
			bool open = GetGame().IsInventoryOpen();
			if (open != m_Open) { m_Open = open; m_Delay = 0; }
			if (m_Open != m_Synced)
			{
				m_Delay += dt;
				if (!m_Open || m_Delay >= 0.3)
				{
					if (GetGame().IsMultiplayer())
					{
						if (!ScriptInputUserData.CanStoreInputUserData()) return;
						ScriptInputUserData ctx = new ScriptInputUserData;
						ctx.Write(AGA_UDT_INVVIEW);
						ctx.Write(m_Open);
						ctx.Send();
					}
					m_Synced = m_Open;
				}
			}
		}
		if (cmd != DayZPlayerConstants.COMMANDID_MOVE) return;
		if (m_Cb)
		{
			if (!CanPlay(true)) { m_Cb.Cancel(); m_Cb = null; }
		}
		else if (CanPlay(false))
		{
			m_Cb = m_P.AddCommandModifier_Action(DayZPlayerConstants.CMD_ACTIONMOD_SEARCHINVENTORY, AGA_InvAnimCallback);
		}
	}
}

modded class PlayerBase
{
	ref AGA_InvAnim m_AGAInvAnim;

	override void OnSelectPlayer()
	{
		super.OnSelectPlayer();
		m_AGAInvAnim = new AGA_InvAnim(this);
	}

	override void CommandHandler(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		super.CommandHandler(pDt, pCurrentCommandID, pCurrentCommandFinished);
		if (m_AGAInvAnim) m_AGAInvAnim.Update(pCurrentCommandID, pDt);
	}

	override bool OnInputUserDataProcess(int userDataType, ParamsReadContext ctx)
	{
		if (super.OnInputUserDataProcess(userDataType, ctx)) return true;
		if (userDataType == AGA_UDT_INVVIEW && m_AGAInvAnim)
		{
			bool v;
			if (!ctx.Read(v)) return false;
			m_AGAInvAnim.m_Synced = v;
			return true;
		}
		return false;
	}
}

// akcje (np. jedzenie, laczenie przedmiotow) dzialaja tez w trakcie animacji przegladania
modded class ActionManagerBase
{
	override bool ActionPossibilityCheck(int pCurrentCommandID)
	{
		if (!AGA_InvAnimCallback.Cast(m_Player.GetCommandModifier_Action())) return super.ActionPossibilityCheck(pCurrentCommandID);
		if (!m_ActionsEnabled || m_Player.IsSprinting() || m_Player.IsUnconscious() || m_Player.GetCommand_Action() || m_Player.IsEmotePlaying()) return false;
		if (m_Player.GetWeaponManager().IsRunning() || m_Player.GetThrowing().IsThrowingAnimationPlaying() || m_Player.GetDayZPlayerInventory().IsProcessing() || m_Player.IsItemsToDelete() || m_Player.IsRolling()) return false;
		return (pCurrentCommandID == DayZPlayerConstants.COMMANDID_ACTION || pCurrentCommandID == DayZPlayerConstants.COMMANDID_MOVE || pCurrentCommandID == DayZPlayerConstants.COMMANDID_SWIM || pCurrentCommandID == DayZPlayerConstants.COMMANDID_LADDER || pCurrentCommandID == DayZPlayerConstants.COMMANDID_VEHICLE);
	}
}

modded class ActionManagerServer
{
	override void StartDeliveredAction()
	{
		if (!m_CurrentActionData || !AGA_InvAnimCallback.Cast(m_Player.GetCommandModifier_Action()))
		{
			super.StartDeliveredAction();
			return;
		}
		m_Interrupted = false;
		bool accepted = false;
		ActionBase pickedAction = m_CurrentActionData.m_Action;
		ActionTarget target = m_CurrentActionData.m_Target;
		ItemBase item = m_CurrentActionData.m_MainItem;
		if (!m_Player.GetCommand_Action() && !m_Player.IsSprinting() && pickedAction && pickedAction.Can(m_Player, target, item))
		{
			if (pickedAction.AddActionJuncture(m_CurrentActionData)) accepted = true;
		}
		if (accepted)
		{
			if (pickedAction.UseAcknowledgment())
			{
				m_CurrentActionData.m_State = UA_AM_PENDING;
				DayZPlayerSyncJunctures.SendActionAcknowledgment(m_Player, m_PendingActionAcknowledgmentID, true);
			}
			else m_CurrentActionData.m_State = UA_AM_ACCEPTED;
		}
		else
		{
			if (pickedAction.UseAcknowledgment())
			{
				m_CurrentActionData.m_State = UA_AM_PENDING;
				DayZPlayerSyncJunctures.SendActionAcknowledgment(m_Player, m_PendingActionAcknowledgmentID, false);
			}
			else RequestInterruptAction();
		}
	}
}
