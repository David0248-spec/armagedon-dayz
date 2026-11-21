// ARMA-GEDON Admin – logika serwera
class AGA_Server
{
	static ref AGA_Server s_Instance;

	protected string m_Dir = "$profile:ArmaGedonAdmin";
	protected ref AGA_Config m_Config;
	protected ref AGA_Bans m_Bans;
	protected ref map<string, bool> m_God;

	static AGA_Server Get()
	{
		if (!s_Instance)
			s_Instance = new AGA_Server();
		return s_Instance;
	}

	void AGA_Server()
	{
		m_God = new map<string, bool>;
		if (!FileExist(m_Dir))
			MakeDirectory(m_Dir);
		LoadConfig();
	}

	void LoadConfig()
	{
		string cfgPath = m_Dir + "/admins.json";
		m_Config = new AGA_Config();
		if (FileExist(cfgPath))
		{
			JsonFileLoader<AGA_Config>.JsonLoadFile(cfgPath, m_Config);
		}
		else
		{
			m_Config.Admins.Insert("76561199562774488");
			JsonFileLoader<AGA_Config>.JsonSaveFile(cfgPath, m_Config);
		}

		string banPath = m_Dir + "/bans.json";
		m_Bans = new AGA_Bans();
		if (FileExist(banPath))
			JsonFileLoader<AGA_Bans>.JsonLoadFile(banPath, m_Bans);
		Log("Załadowano adminów: " + m_Config.Admins.Count().ToString() + ", banów: " + m_Bans.Banned.Count().ToString());
	}

	bool IsAdmin(PlayerIdentity id)
	{
		if (!id || !m_Config)
			return false;
		return m_Config.Admins.Find(id.GetPlainId()) != -1;
	}

	bool IsBanned(string steamId)
	{
		return m_Bans.Banned.Find(steamId) != -1;
	}

	void Log(string line)
	{
		int y, mo, d, h, mi, s;
		GetYearMonthDay(y, mo, d);
		GetHourMinuteSecond(h, mi, s);
		string stamp = string.Format("%1-%2-%3 %4:%5:%6", y, mo.ToStringLen(2), d.ToStringLen(2), h.ToStringLen(2), mi.ToStringLen(2), s.ToStringLen(2));
		FileHandle f = OpenFile(m_Dir + "/admin_log.txt", FileMode.APPEND);
		if (f != 0)
		{
			FPrintln(f, stamp + " | " + line);
			CloseFile(f);
		}
		Print("[ArmaGedonAdmin] " + line);
	}

	PlayerBase FindPlayer(string steamId)
	{
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		foreach (Man m : players)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (pb && pb.GetIdentity() && pb.GetIdentity().GetPlainId() == steamId)
				return pb;
		}
		return null;
	}

	protected void Notify(PlayerIdentity id, string text)
	{
		NotificationSystem.SendNotificationToPlayerIdentityExtended(id, 5, "ARMA-GEDON ADMIN", text, "set:dayz_gui image:icon_info");
	}

	protected string Who(PlayerBase p)
	{
		if (p && p.GetIdentity())
			return p.GetIdentity().GetName() + " (" + p.GetIdentity().GetPlainId() + ")";
		return "?";
	}

	// ---------- obsługa RPC ----------
	void OnAuth(PlayerBase sender, PlayerIdentity id)
	{
		bool ok = IsAdmin(id);
		ScriptRPC rpc = new ScriptRPC();
		rpc.Write(ok);
		rpc.Send(sender, AGA_RPC_AUTH_RESULT, true, id);
		if (ok)
			Log("Autoryzacja admina: " + Who(sender));
	}

	void OnPlayers(PlayerBase sender, PlayerIdentity id)
	{
		if (!IsAdmin(id))
			return;
		array<Man> players = new array<Man>;
		GetGame().GetPlayers(players);
		ScriptRPC rpc = new ScriptRPC();
		int count = 0;
		foreach (Man c : players)
		{
			if (PlayerBase.Cast(c) && c.GetIdentity())
				count++;
		}
		rpc.Write(count);
		foreach (Man m : players)
		{
			PlayerBase pb = PlayerBase.Cast(m);
			if (!pb || !pb.GetIdentity())
				continue;
			string sid = pb.GetIdentity().GetPlainId();
			rpc.Write(pb.GetIdentity().GetName());
			rpc.Write(sid);
			rpc.Write(pb.GetPosition());
			rpc.Write(pb.GetHealth("", ""));
			rpc.Write(pb.GetHealth("", "Blood"));
			rpc.Write(m_God.Contains(sid));
		}
		rpc.Send(sender, AGA_RPC_PLAYERS_RES, true, id);
	}

	void OnAction(PlayerBase sender, PlayerIdentity id, ParamsReadContext ctx)
	{
		if (!IsAdmin(id))
		{
			Log("ODRZUCONO akcję od nie-admina: " + Who(sender));
			return;
		}
		int act;
		string target;
		string text;
		vector pos;
		int num;
		float val;
		if (!ctx.Read(act) || !ctx.Read(target) || !ctx.Read(text) || !ctx.Read(pos) || !ctx.Read(num) || !ctx.Read(val))
			return;

		PlayerBase t;
		if (target != "")
			t = FindPlayer(target);

		switch (act)
		{
			case AGA_ACT_TP_POS:
			{
				if (pos[1] <= 0)
					pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
				TeleportTo(sender, pos);
				Log(Who(sender) + " teleport na " + pos.ToString());
				break;
			}
			case AGA_ACT_TP_TO:
			{
				if (!t) { Notify(id, "Gracz offline"); return; }
				TeleportTo(sender, t.GetPosition() + Vector(1, 0, 1));
				Log(Who(sender) + " teleport do " + Who(t));
				break;
			}
			case AGA_ACT_BRING:
			{
				if (!t) { Notify(id, "Gracz offline"); return; }
				TeleportTo(t, sender.GetPosition() + Vector(1, 0, 1));
				Notify(id, "Przyciągnięto " + t.GetIdentity().GetName());
				Log(Who(sender) + " przyciągnął " + Who(t));
				break;
			}
			case AGA_ACT_HEAL:
			{
				if (!t) t = sender;
				t.AGA_FullHeal();
				Notify(id, "Uleczono " + t.GetIdentity().GetName());
				Log(Who(sender) + " uleczył " + Who(t));
				break;
			}
			case AGA_ACT_KILL:
			{
				if (!t) return;
				t.SetAllowDamage(true);
				m_God.Remove(t.GetIdentity().GetPlainId());
				t.SetHealth("", "", 0);
				Log(Who(sender) + " zabił " + Who(t));
				break;
			}
			case AGA_ACT_KICK:
			{
				if (!t) return;
				Log(Who(sender) + " wyrzucił " + Who(t) + " | powód: " + text);
				Notify(t.GetIdentity(), "Zostałeś wyrzucony: " + text);
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DoKick, 1500, false, t.GetIdentity().GetPlainId());
				break;
			}
			case AGA_ACT_BAN:
			{
				if (target == "") return;
				if (m_Bans.Banned.Find(target) == -1)
				{
					m_Bans.Banned.Insert(target);
					JsonFileLoader<AGA_Bans>.JsonSaveFile(m_Dir + "/bans.json", m_Bans);
				}
				Log(Who(sender) + " zbanował " + target + " | powód: " + text);
				if (t)
					GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(DoKick, 500, false, target);
				Notify(id, "Ban nadany: " + target);
				break;
			}
			case AGA_ACT_GOD:
			{
				if (!t) t = sender;
				string gid = t.GetIdentity().GetPlainId();
				bool on = (num == 1);
				t.SetAllowDamage(!on);
				if (on)
				{
					m_God.Set(gid, true);
					t.AGA_FullHeal();
				}
				else
				{
					m_God.Remove(gid);
				}
				Notify(id, "God mode: " + on.ToString());
				Log(Who(sender) + " god mode " + on.ToString() + " dla " + Who(t));
				break;
			}
			case AGA_ACT_SPAWN:
			{
				SpawnItem(sender, id, text, num, val, pos);
				break;
			}
			case AGA_ACT_CAR:
			{
				SpawnCar(sender, id, text);
				break;
			}
			case AGA_ACT_TIME:
			{
				int yy, mm, dd, hh, mn;
				GetGame().GetWorld().GetDate(yy, mm, dd, hh, mn);
				GetGame().GetWorld().SetDate(yy, mm, dd, num, 0);
				Notify(id, "Godzina: " + num.ToString() + ":00");
				Log(Who(sender) + " ustawił godzinę " + num.ToString());
				break;
			}
			case AGA_ACT_WEATHER:
			{
				SetWeather(num);
				Notify(id, "Pogoda zmieniona");
				Log(Who(sender) + " ustawił pogodę " + num.ToString());
				break;
			}
			case AGA_ACT_MESSAGE:
			{
				if (text == "") return;
				NotificationSystem.SendNotificationToPlayerIdentityExtended(null, 12, "ARMA-GEDON", text, "set:dayz_gui image:icon_info");
				Log(Who(sender) + " ogłoszenie: " + text);
				break;
			}
		}
	}

	void DoKick(string steamId)
	{
		PlayerBase p = FindPlayer(steamId);
		if (p && p.GetIdentity())
			GetGame().DisconnectPlayer(p.GetIdentity(), steamId);
	}

	void TeleportTo(PlayerBase p, vector pos)
	{
		if (!p)
			return;
		float ground = GetGame().SurfaceY(pos[0], pos[2]);
		if (pos[1] < ground)
			pos[1] = ground;
		p.SetPosition(pos);
	}

	void SpawnItem(PlayerBase p, PlayerIdentity id, string cls, int count, float mode, vector cursor)
	{
		if (cls == "" || !IsValidClass(cls))
		{
			Notify(id, "Nieznana klasa: " + cls);
			return;
		}
		if (count < 1) count = 1;
		if (count > 50) count = 50;
		int m = mode;
		int made = 0;
		for (int i = 0; i < count; i++)
		{
			EntityAI e;
			if (m == AGA_SPAWN_INV)
				e = p.GetInventory().CreateInInventory(cls);
			if (!e)
			{
				vector at = p.GetPosition() + p.GetDirection() * 1.5;
				if (m == AGA_SPAWN_CURSOR && cursor != vector.Zero)
					at = cursor;
				e = EntityAI.Cast(GetGame().CreateObjectEx(cls, at, ECE_PLACE_ON_SURFACE));
			}
			if (e)
			{
				made++;
				ItemBase ib = ItemBase.Cast(e);
				if (ib && ib.HasQuantity())
					ib.SetQuantityMax();
			}
		}
		Notify(id, "Utworzono " + made.ToString() + "x " + cls);
		Log(Who(p) + " spawn " + made.ToString() + "x " + cls);
	}

	bool IsValidClass(string cls)
	{
		return GetGame().ConfigIsExisting("CfgVehicles " + cls) || GetGame().ConfigIsExisting("CfgWeapons " + cls) || GetGame().ConfigIsExisting("CfgMagazines " + cls);
	}

	void SpawnCar(PlayerBase p, PlayerIdentity id, string preset)
	{
		array<string> parts = new array<string>;
		string body = preset;
		switch (preset)
		{
			case "OffroadHatchback":
				parts = {"HatchbackWheel","HatchbackWheel","HatchbackWheel","HatchbackWheel","HatchbackWheel","CarBattery","CarRadiator","SparkPlug","HeadlightH7","HeadlightH7","HatchbackHood","HatchbackTrunk","HatchbackDoors_Driver","HatchbackDoors_CoDriver"};
				break;
			case "CivilianSedan":
				parts = {"CivSedanWheel","CivSedanWheel","CivSedanWheel","CivSedanWheel","CarBattery","CarRadiator","SparkPlug","HeadlightH7","HeadlightH7","CivSedanHood","CivSedanTrunk","CivSedanDoors_Driver","CivSedanDoors_CoDriver","CivSedanDoors_BackLeft","CivSedanDoors_BackRight"};
				break;
			case "Hatchback_02":
				parts = {"Hatchback_02_Wheel","Hatchback_02_Wheel","Hatchback_02_Wheel","Hatchback_02_Wheel","CarBattery","CarRadiator","SparkPlug","HeadlightH7","HeadlightH7","Hatchback_02_Hood","Hatchback_02_Trunk","Hatchback_02_Door_1_1","Hatchback_02_Door_1_2","Hatchback_02_Door_2_1","Hatchback_02_Door_2_2"};
				break;
			case "Sedan_02":
				parts = {"Sedan_02_Wheel","Sedan_02_Wheel","Sedan_02_Wheel","Sedan_02_Wheel","CarBattery","CarRadiator","SparkPlug","HeadlightH7","HeadlightH7","Sedan_02_Hood","Sedan_02_Trunk","Sedan_02_Door_1_1","Sedan_02_Door_1_2","Sedan_02_Door_2_1","Sedan_02_Door_2_2"};
				break;
			case "Offroad_02":
				parts = {"Offroad_02_Wheel","Offroad_02_Wheel","Offroad_02_Wheel","Offroad_02_Wheel","Offroad_02_Wheel","CarBattery","GlowPlug","HeadlightH7","HeadlightH7","Offroad_02_Hood","Offroad_02_Trunk","Offroad_02_Door_1_1","Offroad_02_Door_1_2","Offroad_02_Door_2_1","Offroad_02_Door_2_2"};
				break;
			case "Truck_01_Covered":
				parts = {"Truck_01_Wheel","Truck_01_Wheel","Truck_01_WheelDouble","Truck_01_WheelDouble","Truck_01_WheelDouble","Truck_01_WheelDouble","TruckBattery","HeadlightH7","HeadlightH7","Truck_01_Hood","Truck_01_Door_1_1","Truck_01_Door_2_1"};
				break;
			default:
				Notify(id, "Nieznany pojazd: " + preset);
				return;
		}
		vector at = p.GetPosition() + p.GetDirection() * 6;
		at[1] = GetGame().SurfaceY(at[0], at[2]) + 0.5;
		CarScript car = CarScript.Cast(GetGame().CreateObjectEx(body, at, ECE_PLACE_ON_SURFACE));
		if (!car)
		{
			Notify(id, "Nie udało się stworzyć: " + body);
			return;
		}
		car.SetOrientation(p.GetOrientation());
		foreach (string part : parts)
			car.GetInventory().CreateInInventory(part);
		car.Fill(CarFluid.FUEL, car.GetFluidCapacity(CarFluid.FUEL));
		car.Fill(CarFluid.OIL, car.GetFluidCapacity(CarFluid.OIL));
		car.Fill(CarFluid.BRAKE, car.GetFluidCapacity(CarFluid.BRAKE));
		car.Fill(CarFluid.COOLANT, car.GetFluidCapacity(CarFluid.COOLANT));
		Notify(id, "Pojazd gotowy: " + body);
		Log(Who(p) + " spawn pojazdu " + body);
	}

	void SetWeather(int preset)
	{
		Weather w = GetGame().GetWeather();
		w.SetWeatherUpdateFreeze(false);
		float hold = 3600;
		switch (preset)
		{
			case 0: // słońce
				w.GetOvercast().Set(0.05, 30, hold);
				w.GetRain().Set(0, 10, hold);
				w.GetFog().Set(0, 30, hold);
				break;
			case 1: // pochmurno
				w.GetOvercast().Set(0.6, 30, hold);
				w.GetRain().Set(0, 10, hold);
				w.GetFog().Set(0.05, 30, hold);
				break;
			case 2: // deszcz
				w.GetOvercast().Set(0.85, 30, hold);
				w.GetRain().Set(0.7, 30, hold);
				w.GetFog().Set(0.1, 30, hold);
				break;
			case 3: // burza
				w.GetOvercast().Set(1, 30, hold);
				w.GetRain().Set(1, 30, hold);
				w.GetFog().Set(0.15, 30, hold);
				w.SetStorm(1, 0.7, 20);
				break;
			case 4: // mgła
				w.GetOvercast().Set(0.5, 30, hold);
				w.GetRain().Set(0, 10, hold);
				w.GetFog().Set(0.6, 30, hold);
				break;
		}
	}
}
