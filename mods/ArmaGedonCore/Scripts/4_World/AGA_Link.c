// ARMA-GEDON – powiązania kont (ten sam komputer) i blokada omijania bana
class AGA_LinkData
{
	ref map<string, ref array<string>> Links;
	void AGA_LinkData() { Links = new map<string, ref array<string>>; }
}

class AGA_Link
{
	protected static const string DIR = "$profile:ArmaGedonCore";
	protected static string s_Secret;
	protected static ref AGA_LinkData s_Data;
	protected static ref map<string, bool> s_Done;
	static const int MAX_IDS = 30;

	static string Secret()
	{
		if (s_Secret != "") return s_Secret;
		string path = DIR + "/link_secret.txt";
		FileHandle fh = OpenFile(path, FileMode.READ);
		if (fh)
		{
			FGets(fh, s_Secret);
			CloseFile(fh);
			s_Secret.Trim();
		}
		if (s_Secret.Length() < 24)
		{
			int y, mo, d, h, mi, se;
			GetYearMonthDay(y, mo, d);
			GetHourMinuteSecond(h, mi, se);
			s_Secret = "";
			for (int i = 0; i < 6; i++)
				s_Secret += Math.RandomInt(100000, 999999).ToString();
			s_Secret += y.ToString() + mo.ToString() + d.ToString() + h.ToString() + mi.ToString() + se.ToString();
			fh = OpenFile(path, FileMode.WRITE);
			if (fh)
			{
				FPrintln(fh, s_Secret);
				CloseFile(fh);
			}
		}
		return s_Secret;
	}

	static string Sign(string payload)
	{
		string k = Secret();
		string sig = "";
		for (int i = 0; i < 4; i++)
		{
			string src = k + "#" + i.ToString() + "#" + payload + "#" + k;
			sig += src.Hash().ToString() + ".";
		}
		return sig;
	}

	static string Pack(array<string> ids)
	{
		string list = "";
		foreach (string id : ids)
		{
			if (list != "") list += ",";
			list += id;
		}
		string payload = "2|" + list;
		return payload + "|" + Sign(payload);
	}

	// zwraca listę id z poprawnie podpisanego wpisu
	static bool Unpack(string blob, array<string> dst)
	{
		int cut = blob.LastIndexOf("|");
		if (cut <= 0) return false;
		string payload = blob.Substring(0, cut);
		string sig = blob.Substring(cut + 1, blob.Length() - cut - 1);
		if (sig != Sign(payload)) return false;
		array<string> parts = new array<string>;
		payload.Split("|", parts);
		if (parts.Count() < 2 || parts[0] != "2") return false;
		array<string> ids = new array<string>;
		parts[1].Split(",", ids);
		foreach (string id : ids)
		{
			if (id.Length() >= 5 && dst.Find(id) < 0) dst.Insert(id);
		}
		return true;
	}

	static AGA_LinkData Data()
	{
		if (!s_Data)
		{
			s_Data = new AGA_LinkData();
			string p = DIR + "/account_links.json";
			if (FileExist(p)) JsonFileLoader<AGA_LinkData>.JsonLoadFile(p, s_Data);
		}
		return s_Data;
	}

	static void Remember(string a, string b)
	{
		array<string> l = Data().Links.Get(a);
		if (!l)
		{
			l = new array<string>;
			Data().Links.Set(a, l);
		}
		if (l.Find(b) < 0) l.Insert(b);
	}

	static bool IsBannedAnywhere(string id)
	{
		if (AGA_Server.Get().IsBanned(id)) return true;
		return FileContains("$profile:VPPAdminTools/Permissions/BanList.json", id);
	}

	protected static bool FileContains(string path, string needle)
	{
		if (!FileExist(path)) return false;
		FileHandle fh = OpenFile(path, FileMode.READ);
		if (!fh) return false;
		string line;
		bool found = false;
		while (FGets(fh, line) >= 0)
		{
			if (line.IndexOf(needle) >= 0)
			{
				found = true;
				break;
			}
		}
		CloseFile(fh);
		return found;
	}

	static void OnReport(ParamsReadContext ctx, PlayerIdentity sender)
	{
		if (!sender) return;
		string me = sender.GetPlainId();
		if (!s_Done) s_Done = new map<string, bool>;
		if (s_Done.Contains(me)) return;
		s_Done.Set(me, true);

		Param1<array<string>> data;
		if (!ctx.Read(data) || !data.param1) return;

		array<string> ids = new array<string>;
		int forged = 0;
		int n = 0;
		foreach (string blob : data.param1)
		{
			if (++n > 6) break;
			if (blob.Length() > 2048) continue;
			if (!Unpack(blob, ids)) forged++;
		}

		AGA_Server srv = AGA_Server.Get();
		if (forged > 0)
			srv.Log("[MULTIKONTO] niepoprawny wpis stanowiska (" + forged.ToString() + "): " + sender.GetName() + " (" + me + ")");

		array<string> others = new array<string>;
		foreach (string o : ids)
		{
			if (o != me) others.Insert(o);
		}

		bool evading = false;
		string list = "";
		foreach (string other : others)
		{
			Remember(me, other);
			Remember(other, me);
			if (list != "") list += ", ";
			list += other;
			if (IsBannedAnywhere(other))
			{
				evading = true;
				list += " (BAN)";
			}
		}

		if (others.Count() > 0)
		{
			JsonFileLoader<AGA_LinkData>.JsonSaveFile(DIR + "/account_links.json", Data());
			string msg = "[MULTIKONTO] " + sender.GetName() + " (" + me + ") gra z tego samego komputera co: " + list;
			srv.Log(msg);
			StaffAlert(msg);
		}

		// zapis nowego wpisu: bieżące konto na początku, limit długości
		array<string> keep = new array<string>;
		keep.Insert(me);
		foreach (string k : others)
		{
			if (keep.Count() >= MAX_IDS) break;
			keep.Insert(k);
		}
		GetRPCManager().SendRPC("AGA", "AGA_Store", new Param1<string>(Pack(keep)), true, sender);

		if (evading)
		{
			srv.Log("[MULTIKONTO] omijanie bana – wyrzucono: " + sender.GetName() + " (" + me + ")");
			NotificationSystem.SendNotificationToPlayerIdentityExtended(sender, 10, "ACCESS DENIED", "This computer is linked to a banned account. Contact the admins on Discord.", "set:dayz_gui image:icon_info");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(srv.DoKick, 3000, false, me);
		}
	}

	static void Forget(string id)
	{
		if (s_Done) s_Done.Remove(id);
	}

	static void StaffAlert(string msg)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			if (m && m.GetIdentity() && AGA_Server.Get().IsStaff(m.GetIdentity()))
				NotificationSystem.SendNotificationToPlayerIdentityExtended(m.GetIdentity(), 10, "ANTYCHEAT", msg, "set:dayz_gui image:icon_info");
		}
	}
}
