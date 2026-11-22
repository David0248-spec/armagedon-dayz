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

// regulamin serwera (okno po wejściu)
const string AGA_RULES_TITLE = "ARMA-GEDON - SERVER RULES";
const string AGA_RULES_TEXT = "GENERAL\n- No cheats, exploits, glitching, duping, macros or stretched resolution.\n- No toxicity, racism or harassment (voice and text). No stream sniping.\n- AFK players may be kicked. No loot cycling. No restart abuse.\n\nCOMBAT\n- No logging out within 10 minutes of combat (shooting, getting shot, voice contact, trapped in a raided base).\n- Disconnected by accident? Rejoin fast and open a ticket.\n- No second Steam account to continue a fight, loot your body or join a raid. Ban evasion = permanent ban on both accounts.\n\nGROUPS & BASES\n- Max 8 players per group, 1 base per group. No alliances to raid (max 3 team-ups per week).\n- Max 5 gates/layers to reach all loot. No fake loot rooms or fake entrances.\n- No building on roads or blocking military, medical, police areas or castles. No stacked watchtowers.\n- No logging out in someone else's base. Max 2 vehicles per base (only vehicle items next to them).\n\nSTORAGE (per base)\n- 6 crates, 2 gun walls, 2 gun racks, 2 gear stands, 1 workbench, 1 fridge, 1 fireplace. Barrels and sea chests unlimited. 1 backup stash.";

// --- terytorium / grupa: widok dla klienta (okno P) ---
class AGA_GroupView
{
	int Gid;
	bool IsOwner;
	int Level;
	bool Raised;
	bool Invite;
	int Limit;
	ref array<string> Ids;
	ref array<string> Names;
	ref array<int> Online;
	void AGA_GroupView()
	{
		Ids = new array<string>;
		Names = new array<string>;
		Online = new array<int>;
	}
}

class AGA_ZoneDef
{
	float X;
	float Z;
	float R;
	string K;
}

class AGA_ZoneDb
{
	ref array<ref AGA_ZoneDef> Zones;
	void AGA_ZoneDb() { Zones = new array<ref AGA_ZoneDef>; }
}
