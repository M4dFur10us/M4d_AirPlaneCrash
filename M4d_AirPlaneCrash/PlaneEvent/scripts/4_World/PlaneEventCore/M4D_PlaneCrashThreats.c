// =====================================================================================
// M4D_PlaneCrashThreats.c - VERSÃO DEFINITIVA (Integração WorldState & Anti-Farming)
// Responsabilidade: Gestão e Spawn de Zumbis, Lobos, Ursos e Gás baseado no Snapshot.
// =====================================================================================

class M4D_PlaneCrashThreats
{
	// =====================================================================================
	// ZUMBIS E INJEÇÃO DA CHAVE DO CONTENTOR
	// =====================================================================================
	static void M4D_SpawnZombiesWithKey(M4d_AirPlaneCrash wreck, vector center, string keyName)
	{
		if (!wreck) return;

		ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
		if (!settings) return;

		// 1. Radar passivo de Gás para decidir a máscara/roupa de proteção dos Zumbis
		bool isGasActive = false;
		array<Object> envObjs = new array<Object>();
		GetGame().GetObjectsAtPosition(center, 60.0, envObjs, null);
		
		for (int e = 0; e < envObjs.Count(); e++) {
			if (envObjs.Get(e) && envObjs.Get(e).GetType() == "ContaminatedArea_Static") {
				isGasActive = true;
				break;
			}
		}

		// 2. Conta os zumbis já existentes na área para não sobrecarregar a performance do servidor
		int currentZombies = 0;
		array<Object> objs = new array<Object>();
		GetGame().GetObjectsAtPosition(center, 45.0, objs, null);
		
		for (int i = 0; i < objs.Count(); i++) {
			if (DayZInfected.Cast(objs.Get(i))) {
				currentZombies++;
			}
		}

		// Define a cota de zumbis (Ex: máximo de 12 em volta do contentor)
		int targetZombies = Math.RandomIntInclusive(8, 12);
		int toSpawn = targetZombies - currentZombies;
		
		if (toSpawn <= 0 && wreck.HasKeyDropped()) return; // Área cheia e chave já entregue
		if (toSpawn <= 0 && !wreck.HasKeyDropped()) toSpawn = 1; // Força 1 zumbi para carregar a chave

		array<string> zTypes = wreck.GetZombieTypes();
		if (!zTypes || zTypes.Count() == 0) {
			zTypes = new array<string>();
			zTypes.Insert("ZmbM_PatrolNormal_PautRev");
			zTypes.Insert("ZmbM_SoldierNormal");
		}

		array<DayZInfected> spawnedZeds = new array<DayZInfected>();

		// 3. Spawna os Zumbis
		for (int s = 0; s < toSpawn; s++) 
		{
			string zType = zTypes.GetRandomElement();
			
			// Se o avião caiu numa zona de gás, força o spawn de zumbis com traje NBC
			if (isGasActive && !zType.Contains("NBC")) {
				zType = "ZmbM_NBC_Yellow"; 
			}
			
			vector zPos = center + Vector(Math.RandomFloat(-15.0, 15.0), 0, Math.RandomFloat(-15.0, 15.0));
			zPos[1] = GetGame().SurfaceY(zPos[0], zPos[2]);
			
			DayZInfected zed = DayZInfected.Cast(GetGame().CreateObject(zType, zPos, false, true));
			if (zed) {
				wreck.AddZombie(zed);
				spawnedZeds.Insert(zed);
			}
		}

		// 4. A Roleta da Chave (Proteção Anti-Farming)
		// Verifica se a chave já não foi dropada antes neste evento
		if (spawnedZeds.Count() > 0 && !wreck.HasKeyDropped()) 
		{
			DayZInfected luckyZed = spawnedZeds.GetRandomElement();
			if (luckyZed && luckyZed.GetInventory()) 
			{
				EntityAI keyItem = luckyZed.GetInventory().CreateInInventory(keyName);
				if (keyItem) 
				{
					// Blinda o evento para não dropar mais chaves no futuro
					wreck.SetKeyDropped();
					
					// INTEGRAÇÃO WORLDSTATE: Comunica ao Painel Vivo que o evento forneceu a chave
					M4D_PlaneCrashWorldState.MarkKeyDropped(wreck.GetEventID());
					
					M4D_PlaneCrashLogger.Debug(string.Format("[EventID: %1] Proteção Anti-Farming: Chave '%2' gerada e bloqueada para futuros spawns.", wreck.GetEventID(), keyName));
				}
			}
		}
	}

	// =====================================================================================
	// LOBOS, URSOS E ZONAS DE GÁS
	// =====================================================================================
	static void SpawnThreatsInstance(M4d_AirPlaneCrash wreck, bool isGasSpawning)
	{
		if (!wreck) return;
		vector center = wreck.GetPosition();
		
		// 1. Instanciação do Gás
		if (isGasSpawning) {
			EffectArea gas = EffectArea.Cast(GetGame().CreateObject("ContaminatedArea_Static", center));
			if (gas) {
				gas.PlaceOnSurface();
				wreck.AddGasArea(gas);
				M4D_PlaneCrashLogger.Debug(string.Format("[EventID: %1] Zona de Gás instanciada com sucesso.", wreck.GetEventID()));
			}
		}

		int wolfCount = 0;
		int bearCount = 0;

		// 2. Spawn de Lobos (Carga baseada nas definições do JSON do Local)
		PlaneCrashAnimalSpawn wolf = wreck.GetWolfSpawn();
		if (wolf && wolf.Enabled == 1 && wolf.Types && wolf.Types.Count() > 0) {
			for (int i = 0; i < wolf.Count; i++) {
				string wolfType = wolf.Types.GetRandomElement();
				vector wPos = Vector(center[0] + Math.RandomFloat(-wolf.Radius, wolf.Radius), 0, center[2] + Math.RandomFloat(-wolf.Radius, wolf.Radius));
				wPos[1] = GetGame().SurfaceY(wPos[0], wPos[2]);
				
				AnimalBase wAnimal = AnimalBase.Cast(GetGame().CreateObject(wolfType, wPos, false, true));
				if (wAnimal) {
					// ENTREGA O RECIBO AO WRECK
					wreck.AddAnimal(wAnimal);
					wolfCount++;
				}
			}
		}
		
		// 3. Spawn de Ursos com blindagem anti-null no array de Types
		PlaneCrashAnimalSpawn bear = wreck.GetBearSpawn();
		if (bear && bear.Enabled == 1 && bear.Types && bear.Types.Count() > 0) {
			for (int j = 0; j < bear.Count; j++) {
				string bearType = bear.Types.GetRandomElement();
				vector bPos = Vector(center[0] + Math.RandomFloat(-bear.Radius, bear.Radius), 0, center[2] + Math.RandomFloat(-bear.Radius, bear.Radius));
				bPos[1] = GetGame().SurfaceY(bPos[0], bPos[2]);
				
				AnimalBase bAnimal = AnimalBase.Cast(GetGame().CreateObject(bearType, bPos, false, true));
				if (bAnimal) {
					// ENTREGA O RECIBO AO WRECK
					wreck.AddAnimal(bAnimal);
					bearCount++;
				}
			}
		}
		
		if (wolfCount > 0 || bearCount > 0) {
			M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Ameaças Animais geradas: %2 Lobos, %3 Ursos.", wreck.GetEventID(), wolfCount, bearCount));
		}
	}
}