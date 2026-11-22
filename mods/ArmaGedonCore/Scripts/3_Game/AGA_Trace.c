// ARMA-GEDON – znacznik stanowiska gracza (wykrywanie multikont / omijania bana)
// Klient przechowuje podpisany przez serwer wpis i odsyła go przy każdym wejściu.
#ifndef SERVER
class AGA_TraceStore
{
	protected static const string VAR       = "AG_UI_STATE";
	protected static const string DIR_A     = "$saves:UiCache";
	protected static const string FILE_A    = "$saves:UiCache/state.dat";
	protected static const string DIR_B     = "$profile:UiCache";
	protected static const string FILE_B    = "$profile:UiCache/state.dat";
	static const int MAX_LEN = 2048;

	static void ReadAll(array<string> out)
	{
		string v;
		if (GetGame().GetProfileString(VAR, v)) Add(out, v);
		Add(out, ReadFile(FILE_A));
		Add(out, ReadFile(FILE_B));
	}

	static void WriteAll(string blob)
	{
		GetGame().SetProfileString(VAR, blob);
		GetGame().SaveProfile();
		MakeDirectory(DIR_A);
		WriteFile(FILE_A, blob);
		MakeDirectory(DIR_B);
		WriteFile(FILE_B, blob);
	}

	protected static void Add(array<string> out, string v)
	{
		if (v == "" || v.Length() > MAX_LEN) return;
		if (out.Find(v) < 0) out.Insert(v);
	}

	protected static string ReadFile(string path)
	{
		string r = "";
		FileSerializer f = new FileSerializer();
		if (!f.Open(path, FileMode.READ)) return r;
		if (!f.Read(r) || r.Length() > MAX_LEN) r = "";
		f.Close();
		return r;
	}

	protected static void WriteFile(string path, string blob)
	{
		FileSerializer f = new FileSerializer();
		if (!f.Open(path, FileMode.WRITE)) return;
		f.Write(blob);
		f.Close();
	}
}

modded class DayZGame
{
	protected bool m_AGA_TraceSent;

	override void DeferredInit()
	{
		super.DeferredInit();
		GetRPCManager().AddRPC("AGA", "AGA_Store", this, SingleplayerExecutionType.Client);
	}

	void AGA_Store(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<string> data;
		if (!ctx.Read(data)) return;
		string blob = data.param1;
		if (blob == "" || blob.Length() > AGA_TraceStore.MAX_LEN) return;
		AGA_TraceStore.WriteAll(blob);
	}

	override void OnEvent(EventType eventTypeId, Param params)
	{
		super.OnEvent(eventTypeId, params);
		if (eventTypeId == MPSessionStartEventTypeID)
		{
			m_AGA_TraceSent = false;
			return;
		}
		if (eventTypeId != MPSessionPlayerReadyEventTypeID || m_AGA_TraceSent) return;
		m_AGA_TraceSent = true;
		array<string> blobs = new array<string>;
		AGA_TraceStore.ReadAll(blobs);
		GetRPCManager().SendRPC("AGA", "AGA_Report", new Param1<array<string>>(blobs), true);
	}
}
#endif
