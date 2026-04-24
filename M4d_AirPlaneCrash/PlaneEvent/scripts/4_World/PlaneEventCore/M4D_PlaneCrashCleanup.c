class M4D_PlaneCrashCleanup
{
	protected float m_WorldSize = 0;
	protected float m_Step = 2000.0;
	protected float m_Radius = 1500.0;
	protected int m_Tiles = 0;
	protected int m_X = 0;
	protected int m_Z = 0;

	void Init(float step)
	{
		if (!GetGame() || !GetGame().GetWorld())
			return;

		m_WorldSize = GetGame().GetWorld().GetWorldSize();
		if (m_WorldSize <= 0)
			return;

		if (step < 500.0)
			step = 500.0;

		m_Step = step;
		m_Radius = m_Step * 0.75;

		m_Tiles = (int)Math.Ceil(m_WorldSize / m_Step);
		if (m_Tiles < 1)
			m_Tiles = 1;

		m_X = 0;
		m_Z = 0;
	}

	protected bool ShouldDelete(Object o)
	{
		if (!o)
			return false;

		string t = o.GetType();
		if (!t)
			return false;

		if (o.IsKindOf("M4d_AirPlaneCrash"))
			return true;

		if (t.Contains("M4d_AirPlaneCrash"))
			return true;

		if (t.Contains("M4D_WreckContainer"))
			return true;

		return false;
	}

	protected bool DoTileStep(out int deletedThisStep, out int doneTiles, out int totalTiles)
	{
		deletedThisStep = 0;

		if (m_Tiles <= 0 || m_WorldSize <= 0)
			Init(m_Step);

		totalTiles = m_Tiles * m_Tiles;

		if (m_Z >= m_Tiles)
		{
			doneTiles = totalTiles;
			return true;
		}

		float cx = (m_X + 0.5) * m_Step;
		float cz = (m_Z + 0.5) * m_Step;

		vector center = Vector(cx, 0, cz);
		center[1] = GetGame().SurfaceY(cx, cz);

		array<Object> objects = new array<Object>();
		array<CargoBase> proxy = new array<CargoBase>();

		GetGame().GetObjectsAtPosition(center, m_Radius, objects, proxy);

		for (int i = 0; i < objects.Count(); i++)
		{
			Object o = objects.Get(i);
			if (!ShouldDelete(o))
				continue;

			GetGame().ObjectDelete(o);
			deletedThisStep++;
		}

		m_X++;
		if (m_X >= m_Tiles)
		{
			m_X = 0;
			m_Z++;
		}

		doneTiles = (m_Z * m_Tiles) + m_X;
		if (doneTiles > totalTiles)
			doneTiles = totalTiles;

		if (m_Z >= m_Tiles)
			return true;

		return false;
	}

	bool Step(out int deletedThisStep, out int doneTiles, out int totalTiles)
	{
		return this.DoTileStep(deletedThisStep, doneTiles, totalTiles);
	}

	int CleanupAllFast(float step)
	{
		Init(step);

		int deleted = 0;
		int d, done, total;

		while (true)
		{
			bool finished = this.DoTileStep(d, done, total);
			deleted = deleted + d;
			if (finished)
				break;
		}

		m_X = 0;
		m_Z = 0;

		return deleted;
	}
}
