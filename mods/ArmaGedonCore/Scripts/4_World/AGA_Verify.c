// ARMA-GEDON: weryfikacja anty-cheat po wejsciu na serwer (przepisz liczbe z ramki, 3 proby, limit czasu)
class AGA_VerifyState
{
	string Code;
	int Tries;
	bool Done;
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
	}

	static string NewCode()
	{
		return (Math.RandomIntInclusive(1, 9) * 100 + Math.RandomIntInclusive(0, 99)).ToString();
	}

	static void Begin(PlayerIdentity id)
	{
		Init();
		if (!id || AGA_Server.Get().IsStaff(id)) return;
		string uid = id.GetPlainId();
		AGA_VerifyState st = new AGA_VerifyState();
		st.Code = NewCode();
		st.Tries = TRIES;
		s_St.Set(uid, st);
		Ask(id, st);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Timeout, TIMEOUT_MS, false, uid);
	}

	static void Ask(PlayerIdentity id, AGA_VerifyState st)
	{
		GetRPCManager().SendRPC("AGA", "AGA_VerifyAsk", new Param2<string, int>(st.Code, st.Tries), true, id);
	}

	static void Forget(string uid) { if (s_St) s_St.Remove(uid); }

	static void Kick(string uid, string why)
	{
		PlayerBase p = AGA_Server.Get().FindPlayer(uid);
		AGA_Server.Get().Log("[WERYFIKACJA] kick " + uid + ": " + why);
		Forget(uid);
		if (p && p.GetIdentity()) GetGame().DisconnectPlayer(p.GetIdentity(), uid);
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
