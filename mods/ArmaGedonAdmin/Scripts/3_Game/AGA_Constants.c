// ARMA-GEDON Admin – stałe i identyfikatory RPC
const int AGA_RPC_AUTH        = 0x41470101; // klient -> serwer: czy jestem adminem
const int AGA_RPC_AUTH_RESULT = 0x41470102; // serwer -> klient: bool
const int AGA_RPC_PLAYERS     = 0x41470103; // klient -> serwer: prośba o listę graczy
const int AGA_RPC_PLAYERS_RES = 0x41470104; // serwer -> klient: lista graczy
const int AGA_RPC_ACTION      = 0x41470105; // klient -> serwer: akcja admina
const int AGA_RPC_NOTIFY      = 0x41470106; // serwer -> klient: powiadomienie

// typy akcji
const int AGA_ACT_TP_POS      = 1;  // teleport siebie na pozycję
const int AGA_ACT_TP_TO       = 2;  // teleport do gracza
const int AGA_ACT_BRING       = 3;  // przyciągnij gracza do siebie
const int AGA_ACT_HEAL        = 4;  // ulecz gracza
const int AGA_ACT_KILL        = 5;  // zabij gracza
const int AGA_ACT_KICK        = 6;  // wyrzuć gracza
const int AGA_ACT_BAN         = 7;  // zbanuj gracza
const int AGA_ACT_GOD         = 8;  // god mode dla siebie (param: 1/0)
const int AGA_ACT_SPAWN       = 9;  // spawn przedmiotu
const int AGA_ACT_CAR         = 10; // spawn gotowego auta
const int AGA_ACT_TIME        = 11; // ustaw godzinę
const int AGA_ACT_WEATHER     = 12; // ustaw pogodę
const int AGA_ACT_MESSAGE     = 13; // ogłoszenie dla wszystkich

// tryby spawnu
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
