// ARMA-GEDON Admin – stałe, identyfikatory RPC i struktury danych
const int AGA_RPC_AUTH        = 0x41470101; // klient -> serwer: czy jestem adminem
const int AGA_RPC_AUTH_RESULT = 0x41470102; // serwer -> klient: bool
const int AGA_RPC_PLAYERS     = 0x41470103; // klient -> serwer: prośba o listę graczy
const int AGA_RPC_PLAYERS_RES = 0x41470104; // serwer -> klient: lista graczy
const int AGA_RPC_ACTION      = 0x41470105; // klient -> serwer: akcja admina
const int AGA_RPC_NOTIFY      = 0x41470106; // zarezerwowane
const int AGA_RPC_CLOG        = 0x41470107; // klient -> serwer: diagnostyka
const int AGA_RPC_INVIS_SYNC  = 0x41470108; // serwer -> wszyscy: lista niewidzialnych (net id)
const int AGA_RPC_SHOT        = 0x41470109; // serwer -> admini: strzał (skąd, dokąd, kto)
const int AGA_RPC_LOGS        = 0x4147010A; // klient -> serwer: prośba o logi
const int AGA_RPC_LOGS_RES    = 0x4147010B; // serwer -> klient: logi
const int AGA_RPC_FREEZE      = 0x4147010C;
const int AGA_RPC_CAM = 0x41470110;    // gracz -> serwer: raport kamery

// typy akcji
const int AGA_ACT_TP_POS      = 1;
const int AGA_ACT_TP_TO       = 2;
const int AGA_ACT_BRING       = 3;
const int AGA_ACT_HEAL        = 4;
const int AGA_ACT_KILL        = 5;
const int AGA_ACT_KICK        = 6;
const int AGA_ACT_BAN         = 7;
const int AGA_ACT_GOD         = 8;
const int AGA_ACT_SPAWN       = 9;
const int AGA_ACT_CAR         = 10;
const int AGA_ACT_TIME        = 11;
const int AGA_ACT_WEATHER     = 12;
const int AGA_ACT_MESSAGE     = 13;
const int AGA_ACT_INVIS       = 14;
const int AGA_ACT_FREEZE      = 15;
const int AGA_ACT_STRIP       = 16;
const int AGA_ACT_KIT         = 17;
const int AGA_ACT_REPAIR_CAR  = 18;
const int AGA_ACT_DELETE_OBJ  = 19;
const int AGA_ACT_SPAWN_AI    = 20;
const int AGA_ACT_PM          = 21;
const int AGA_ACT_HEAL_ALL    = 22;
const int AGA_ACT_CLEAR_AI    = 23;
const int AGA_ACT_BOT         = 24; // num 1 = stwórz bota testowego, 0 = usuń boty

// tryby spawnu (val); +10 = z wyposażeniem (OnDebugSpawn)
const int AGA_SPAWN_INV       = 0;
const int AGA_SPAWN_GROUND    = 1;
const int AGA_SPAWN_CURSOR    = 2;

class AGA_PlayerInfo
{
	string Name;
	string SteamId;
	vector Pos;
	float Health;
	float Blood;
	bool God;
	int NetLow;
	int NetHigh;
	string InHands;
	float Water;
	float Energy;
	bool Invis;
	bool InVehicle;
}

class AGA_Config
{
	ref array<string> Admins;
	void AGA_Config()
	{
		Admins = new array<string>;
	}
}

class AGA_Bans
{
	ref array<string> Banned;
	void AGA_Bans()
	{
		Banned = new array<string>;
	}
}
