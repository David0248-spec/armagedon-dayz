// ARMA-GEDON: serwerowa weryfikacja anty-cheat (ochrona postaci podczas weryfikacji + usuwanie obiektu przy odrzuceniu -> 0 klonow)
class AGA_VerifyState
{
	string Code;
	int Tries;
	bool Done;
	PlayerIdentity Id;
}

class AGA_Verify
{
	static const int TRIES = 3;
	static const int TIMEOUT_MS = 180000;
	static ref map<string, ref AGA_VerifyState> s_St;
	static ref AGA_Verify s_Inst;

	static void Init()
	{
		if (s_Inst) return;
		s_Inst = new AGA_Verify();
		s_St = new map<string, ref AGA_VerifyState>;
		GetRPCManager().AddRPC("AGA", "AGA_VerifyAnswer", s_Inst, SingleplayerExecutionType.Server);
		GetRPCManager().AddRPC("AGA", "AGA_VerifyHello", s_Inst, SingleplayerExecutionType.Server);
	}

	static string NewCode()
	{
		return (Math.RandomIntInclusive(1, 9) * 100 + Math.RandomIntInclusive(0, 99)).ToString();
	}

	static bool IsVerified(string uid)
	{
		if (!s_St) return false;
		AGA_VerifyState st;
		return s_St.Find(uid, st) && st.Done;
	}

	static void Begin(PlayerIdentity id)
	{
		Init();
		if (!id) return;
		string uid = id.GetPlainId();
		AGA_VerifyState st = new AGA_VerifyState();
		st.Code = NewCode();
		st.Tries = TRIES;
		st.Done = false;
		st.Id = id;
		s_St.Set(uid, st);
		AGA_Server.Get().Log("[WERYFIKACJA] start " + id.GetName() + " (" + uid + ")");
		Ask(id, st);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Timeout, TIMEOUT_MS, false, uid);
	}

	static void EnsureStarted(PlayerIdentity id)
	{
		Init();
		if (!id) return;
		AGA_VerifyState st;
		if (s_St.Find(id.GetPlainId(), st))
		{
			st.Id = id;
			if (st.Done)
				GetRPCManager().SendRPC("AGA", "AGA_VerifyAsk", new Param2<string, int>("", -1), true, id);
			else
				Ask(id, st);
			return;
		}
		Begin(id);
	}

	static void ProtectPlayer(PlayerBase p, string uid)
	{
		if (!p) return;
		if (!IsVerified(uid)) p.SetAllowDamage(false);
	}

	static void UnprotectPlayer(string uid)
	{
		PlayerBase p = AGA_Server.Get().FindPlayer(uid);
		if (p) p.SetAllowDamage(true);
	}

	static void CleanupGhostPlayer(string uid)
	{
		PlayerBase p = AGA_Server.Get().FindPlayer(uid);
		if (p)
		{
			AGA_Server.Get().Log("[WERYFIKACJA] usuwanie niezweryfikowanej postaci " + uid);
			GetGame().ObjectDelete(p);
		}
	}

	void AGA_VerifyHello(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender) return;
		EnsureStarted(sender);
	}

	static void Ask(PlayerIdentity id, AGA_VerifyState st)
	{
		GetRPCManager().SendRPC("AGA", "AGA_VerifyAsk", new Param2<string, int>(st.Code, st.Tries), true, id);
	}

	static void Forget(string uid)
	{
		if (s_St) s_St.Remove(uid);
	}

	static void Kick(string uid, string why)
	{
		AGA_Server.Get().Log("[WERYFIKACJA] kick " + uid + ": " + why);
		PlayerIdentity id;
		AGA_VerifyState st;
		if (s_St && s_St.Find(uid, st)) id = st.Id;
		PlayerBase p = AGA_Server.Get().FindPlayer(uid);
		if (p && p.GetIdentity()) id = p.GetIdentity();
		CleanupGhostPlayer(uid);
		Forget(uid);
		if (id) GetGame().DisconnectPlayer(id, uid);
	}

	static void Timeout(string uid)
	{
		AGA_VerifyState st;
		if (!s_St || !s_St.Find(uid, st) || st.Done) return;
		Kick(uid, "brak weryfikacji w czasie");
	}

	void AGA_VerifyAnswer(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if (type != CallType.Server || !sender) return;
		Param1<string> data;
		if (!ctx.Read(data)) return;
		string uid = sender.GetPlainId();
		AGA_VerifyState st;
		if (!s_St.Find(uid, st) || st.Done) return;
		string a = data.param1;
		a.Trim();
		if (a == "#exit") { Kick(uid, "wyjscie z weryfikacji"); return; }
		if (a == st.Code)
		{
			st.Done = true;
			UnprotectPlayer(uid);
			GetRPCManager().SendRPC("AGA", "AGA_VerifyAsk", new Param2<string, int>("", -1), true, sender);
			AGA_Server.Get().Log("[WERYFIKACJA] ok " + sender.GetName() + " (" + uid + ")");
			return;
		}
		st.Tries--;
		if (st.Tries <= 0) { Kick(uid, "3 bledne odpowiedzi"); return; }
		st.Code = NewCode();
		Ask(sender, st);
	}
}
