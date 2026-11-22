// Jednorazowa sonda: nazwy klas obiektów do eventów + rodzaje nawierzchni (wykrywanie dróg)
class AGA_Probe
{
	static void Run()
	{
		string path = "$profile:ArmaGedonCore/probe_classes.txt";
		if (FileExist(path)) return;
		FileHandle f = OpenFile(path, FileMode.WRITE);
		if (!f) return;
		array<string> keys = {"wreck", "c130", "caravan", "camper", "excavator", "bulldozer", "dozer", "crane", "container", "pipe", "pallet", "brick", "cement", "toilet", "camonet", "barrel", "woodpile", "firewood", "lumber", "plank", "tank_", "trailer", "ambulance", "v3s", "ural", "tractor", "bagfence", "barricade", "scaffold", "construction", "smoke", "crash", "boxwooden", "crate"};
		int n = GetGame().ConfigGetChildrenCount("CfgVehicles");
		for (int i = 0; i < n; i++)
		{
			string cls;
			GetGame().ConfigGetChildName("CfgVehicles", i, cls);
			string low = cls;
			low.ToLower();
			foreach (string k : keys)
			{
				if (low.Contains(k))
				{
					int scope = GetGame().ConfigGetInt("CfgVehicles " + cls + " scope");
					FPrintln(f, cls + " scope=" + scope.ToString());
					break;
				}
			}
		}
		// nawierzchnie
		map<string, int> surf = new map<string, int>;
		map<string, string> sample = new map<string, string>;
		for (int s = 0; s < 4000; s++)
		{
			float x = Math.RandomFloat(1000, 14500);
			float z = Math.RandomFloat(1500, 14500);
			float y = GetGame().SurfaceY(x, z);
			string t;
			GetGame().SurfaceGetType3D(x, y + 1.5, z, t);
			surf.Set(t, surf.Get(t) + 1);
			if (!sample.Contains(t)) sample.Set(t, x.ToString() + " " + z.ToString());
		}
		FPrintln(f, "=== SURFACES ===");
		for (int j = 0; j < surf.Count(); j++)
			FPrintln(f, surf.GetKey(j) + " " + surf.GetElement(j).ToString() + " @ " + sample.Get(surf.GetKey(j)));
		CloseFile(f);
	}
}
