// ARMA-GEDON – nadlatujaca maszyna przed katastrofa: dzwiek silnika przesuwa sie od kierunku nadlotu nad miejsce upadku
class AGA_FlyBy
{
	static ref array<ref AGA_FlyBy> s_All = new array<ref AGA_FlyBy>;

	vector m_From;
	vector m_To;
	float m_Dur;
	float m_T;
	ref array<EffectSound> m_Fx = new array<EffectSound>;

	static void Start(vector from, vector to, int ms, int type)
	{
		AGA_FlyBy f = new AGA_FlyBy();
		f.m_From = from;
		f.m_To = to;
		f.m_Dur = ms / 1000.0;
		array<string> sets = new array<string>;
		if (type == 1) { sets.Insert("Expansion_An2_Engine_Ext_SoundSet"); sets.Insert("Expansion_Merlin_Engine2_Full_Ext_SoundSet"); }
		else { sets.Insert("Expansion_Uh1h_Rotor_Full_Ext_SoundSet"); sets.Insert("Expansion_Uh1h_Engine_Full_Ext_SoundSet"); }
		foreach (string ss : sets)
		{
			EffectSound e = SEffectManager.PlaySound(ss, from, 1.5, 0, true);
			if (e) f.m_Fx.Insert(e);
		}
		s_All.Insert(f);
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).CallLater(f.Step, 50, true);
	}

	void Step()
	{
		m_T += 0.05;
		float k = Math.Clamp(m_T / m_Dur, 0, 1);
		// koncowka lotu opada stromo w dol
		vector p = m_From + (m_To - m_From) * k;
		p[1] = m_From[1] + (m_To[1] - m_From[1]) * (k * k);
		foreach (EffectSound e : m_Fx) if (e) e.SetPosition(p);
		if (k >= 1) Stop();
	}

	void Stop()
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GUI).Remove(Step);
		foreach (EffectSound e : m_Fx)
		{
			if (!e) continue;
			e.SoundStop();
			SEffectManager.DestroyEffect(e);
		}
		m_Fx.Clear();
		s_All.RemoveItem(this);
	}
}
