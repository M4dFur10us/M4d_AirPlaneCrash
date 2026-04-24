// =====================================================================================
// M4D_PlaneCrashSpawner.c - VERSÃO DEFINITIVA (WorldState Consolidado + RPC Áudio)
// Responsabilidade: Lógica de criação do evento, sorteio de locais, contentores e assets.
// =====================================================================================

// ---------------------------------------------------------------------------------
// CONFIGURAÇÃO ESTRITA DE ASSETS
// ---------------------------------------------------------------------------------
class M4D_SpawnerAssetConfig
{
	string AssetType;
	vector LocalOffset;
	vector Orientation;
	float Scale;
	bool AlignToTerrain;

	void M4D_SpawnerAssetConfig(string type = "", vector offset = "0 0 0", vector orient = "0 0 0", float scale = 1.0, bool align = true)
	{
		AssetType = type;
		LocalOffset = offset;
		Orientation = orient;
		Scale = scale;
		AlignToTerrain = align;
	}
}

class M4D_PlaneCrashSpawner
{
	// Dicionário Estático de Assets do Acidente
	static ref array<ref M4D_SpawnerAssetConfig> GetHardcodedAssets()
	{
		ref array<ref M4D_SpawnerAssetConfig> assets = new array<ref M4D_SpawnerAssetConfig>();
		
		assets.Insert(new M4D_SpawnerAssetConfig("CraterLong", "-0.5940539836883545 -0.4000000059604645 10.352170944213868", "0.0 0.0 0.0", 1.0, true));
		assets.Insert(new M4D_SpawnerAssetConfig("CraterLong", "-0.48886001110076907 -0.4000000059604645 15.957640647888184", "0.35199999809265139 0.0 0.0", 1.0, true));
		assets.Insert(new M4D_SpawnerAssetConfig("StaticObj_ammoboxes_big", "4.570658206939697 0.0 13.319387435913086", "12.247900009155274 0.0 0.0", 1.0, true));
		
		assets.Insert(new M4D_SpawnerAssetConfig("M4DCrashStorage_AmmoBox", "-3.600000 0.0 12.989584", "0.0 0.0 0.0", 1.0, true));
		assets.Insert(new M4D_SpawnerAssetConfig("M4DCrashStorage_AmmoBox", "-3.500000 0.0 15.989584", "138.00 0.0 0.0", 1.0, true));
		assets.Insert(new M4D_SpawnerAssetConfig("M4DCrashStorage_AmmoBox", "-5.000000 0.0 18.489584", "127.60 0.0 0.0", 1.0, true));
		assets.Insert(new M4D_SpawnerAssetConfig("M4DCrashStorage_ToolBox", "4.060220 0.0 20.550507", "12.2479 0.0 0.0", 1.0, true));
		assets.Insert(new M4D_SpawnerAssetConfig("M4DCrashStorage_ToolBox", "4.525254 0.0 17.980886", "129.2479 0.0 0.0", 1.0, true));
		
		return assets;
	}

	// =====================================================================================
	// NOTIFICAÇÃO GLOBAL
	// =====================================================================================
	static void SendCrashNotification(vector pos, string customMsg = "")
	{
		string msg = customMsg;
		if (msg == "") {
			msg = M4DLocationFinder.BuildPlaneCrashMessage(pos);
		}

		if (msg == "" || msg == "Plane crash spotted") {
			msg = string.Format("Um Avião de Suprimentos foi abatido perto de x(%1) z(%2)", Math.Round(pos[0]), Math.Round(pos[2]));
		}
		
		array<Man> players = new array<Man>(); 
		GetGame().GetPlayers(players);
		
		for (int i = 0; i < players.Count(); i++) {
			Man p = players.Get(i);
			PlayerBase pb = PlayerBase.Cast(p);
			if (pb && pb.IsAlive() && pb.GetIdentity()) {
				#ifdef EXPANSIONMOD
					ExpansionNotification("Avião Abatido", msg, "set:dayz_inventory image:explosive", COLOR_EXPANSION_NOTIFICATION_MISSION, 15.0).Create(pb.GetIdentity());
				#else
					GetGame().RPCSingleParam(pb, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>(msg), true, pb.GetIdentity());
				#endif
			}
		}
	}

	// =====================================================================================
	// SORTEIO DE LOCAIS E TELEMETRIA (Com Raio Dinâmico e Proteção CrashBase)
	// =====================================================================================
	static vector FindValidCrashSite(out M4D_PlaneCrashCustomSite outChosenSite) 
	{
		ref M4D_PlaneCrashSettings settings = M4D_PlaneCrashSettings.Get();
		if (settings) {
			settings.ValidateSettings();
		}

		ref M4D_PlaneCrashSites s_sites = M4D_PlaneCrashSites.Get();
		if (!s_sites) {
			M4D_PlaneCrashLogger.Info("ABORTADO: O gerenciador de locais (PlaneCrashSites) não está carregado.");
			return vector.Zero;
		}
		
		outChosenSite = null;

		array<Man> players = new array<Man>();
		GetGame().GetPlayers(players);
		
		if (s_sites.CustomCrashSites) {
			if (s_sites.CustomCrashSites.Count() > 0) {
				ref array<ref M4D_PlaneCrashCustomSite> availableSites = new array<ref M4D_PlaneCrashCustomSite>();
				
				int rejectedByPlayer = 0;
				int rejectedByOccupied = 0;

				for (int csIdx = 0; csIdx < s_sites.CustomCrashSites.Count(); csIdx++) {
					M4D_PlaneCrashCustomSite cs = s_sites.CustomCrashSites.Get(csIdx);
					if (cs) {
						vector testPos = Vector(cs.Position[0], GetGame().SurfaceY(cs.Position[0], cs.Position[2]), cs.Position[2]);
						
						bool playerTooClose = false;
						for (int pIdx = 0; pIdx < players.Count(); pIdx++) {
							Man p = players.Get(pIdx);
							if (p) {
								if (settings && vector.Distance(testPos, p.GetPosition()) <= settings.SafeRadius) {
									playerTooClose = true;
									break;
								}
							}
						}
						
						if (playerTooClose) {
							rejectedByPlayer++;
							continue;
						}
						
						array<Object> nearby = new array<Object>();
						if (settings) {
							GetGame().GetObjectsAtPosition(testPos, settings.DistanceRadius, nearby, null);
						} else {
							GetGame().GetObjectsAtPosition(testPos, 1000.0, nearby, null);
						}
						
						bool occupied = false;
						for (int nIdx = 0; nIdx < nearby.Count(); nIdx++) {
							Object obj = nearby.Get(nIdx);
							if (obj) {
								if (obj.IsInherited(CrashBase)) {
									occupied = true;
									break;
								}
								if (obj.GetType().Contains("M4D_WreckContainer")) {
									occupied = true;
									break;
								}
							}
						}
						
						if (!occupied) {
							availableSites.Insert(cs);
						} else {
							rejectedByOccupied++;
						}
					}
				}
				
				if (availableSites.Count() > 0) {
					outChosenSite = availableSites.GetRandomElement();
					if (outChosenSite) {
						return Vector(outChosenSite.Position[0], GetGame().SurfaceY(outChosenSite.Position[0], outChosenSite.Position[2]), outChosenSite.Position[2]);
					}
				} else {
					M4D_PlaneCrashLogger.Info(string.Format("ABORTADO: Nenhum local valido encontrado. Total: %1 | Rej. Player: %2 | Rej. Ocupado: %3", s_sites.CustomCrashSites.Count(), rejectedByPlayer, rejectedByOccupied));
				}
			} else {
				M4D_PlaneCrashLogger.Info("ABORTADO: A lista de 'CustomCrashSites' esta vazia no JSON.");
			}
		} else {
			M4D_PlaneCrashLogger.Info("ABORTADO: Configuracao 'CustomCrashSites' ausente ou corrompida.");
		}
		
		return vector.Zero;
	}

	// =====================================================================================
	// SELEÇÃO INTELIGENTE DE CONTENTORES
	// =====================================================================================
	static string PickEnabledCrashContainerType() 
	{
		ref M4D_PlaneCrashSettings s = M4D_PlaneCrashSettings.Get();
		if (!s) return "M4D_WreckContainerBlue";

		string pref = s.PreferredContainer;
		
		if (pref != "" && pref != "Random") {
			string prefLower = pref;
			prefLower.ToLower();

			if (prefLower == "red" && s.EnableContainerRed == 1) return "M4D_WreckContainerRed";
			if (prefLower == "blue" && s.EnableContainerBlue == 1) return "M4D_WreckContainerBlue";
			if (prefLower == "yellow" && s.EnableContainerYellow == 1) return "M4D_WreckContainerYellow";
			if (prefLower == "orange" && s.EnableContainerOrange == 1) return "M4D_WreckContainerOrange";
		}

		ref array<string> c = new array<string>();
		if (s.EnableContainerBlue == 1) c.Insert("M4D_WreckContainerBlue");
		if (s.EnableContainerRed == 1) c.Insert("M4D_WreckContainerRed");
		if (s.EnableContainerYellow == 1) c.Insert("M4D_WreckContainerYellow");
		if (s.EnableContainerOrange == 1) c.Insert("M4D_WreckContainerOrange");
		
		if (c.Count() > 0) return c.GetRandomElement();
		
		return "M4D_WreckContainerBlue";
	}

	// =====================================================================================
	// MATEMÁTICA E INJEÇÃO DE ASSETS COM OWNERSHIP / AUTO-HEALING
	// =====================================================================================
	static void BuildWreckMatrix(vector pos, vector fwdOriginal, out vector mat[3]) 
	{
		vector normal = GetGame().SurfaceGetNormal(pos[0], pos[2]);
		vector fwd = vector.Direction(pos, pos + "1 0 0"); 
		if (fwdOriginal != "0 0 0") fwd = fwdOriginal;

		fwd.Normalize();
		vector right = fwd * normal; 
		right.Normalize();
		fwd = normal * right; 
		fwd.Normalize();
		
		mat[0] = fwd; 
		mat[1] = normal; 
		mat[2] = right;
	}

	static void SpawnDetailAssets(M4d_AirPlaneCrash wreck, vector wreckMatrix[3], int tier, bool isReload)
	{
		ref array<ref M4D_SpawnerAssetConfig> assetsList = GetHardcodedAssets();

		vector dir = wreck.GetDirection(); dir[1] = 0; dir.Normalize();
		vector right = dir * "0 1 0"; right.Normalize();
		vector wPos = wreck.GetPosition();

		if (isReload) {
			M4D_PlaneCrashLogger.Warn(string.Format("[EventID: %1] AUTO-HEAL INICIADO. Tier=%2", wreck.GetEventID(), tier));
		}

		for (int x = 0; x < assetsList.Count(); x++) {
			M4D_SpawnerAssetConfig cfg = assetsList.Get(x);
			
			vector bPos = wPos + (right * cfg.LocalOffset[0]) + (dir * cfg.LocalOffset[2]);
			float groundY = GetGame().SurfaceY(bPos[0], bPos[2]);
			float finalY;
			
			if (cfg.AlignToTerrain) {
				if (cfg.LocalOffset[1] > 0) finalY = groundY + cfg.LocalOffset[1];
				else finalY = groundY;
			} else {
				finalY = wPos[1] + cfg.LocalOffset[1];
			}
			
			vector finalPos = Vector(bPos[0], finalY, bPos[2]);
			
			if (isReload) {
				array<Object> objs = new array<Object>(); 
				GetGame().GetObjectsAtPosition(finalPos, 1.5, objs, null);
				bool exists = false;
				
				for (int y = 0; y < objs.Count(); y++) {
					Object o = objs.Get(y);
					if (o && o.GetType() == cfg.AssetType) { exists = true; break; }
				}
				
				if (exists) {
					M4D_PlaneCrashLogger.Warn(string.Format("[EventID: %1] AUTO-HEAL SKIP: %2 ja existe em %3", wreck.GetEventID(), cfg.AssetType, finalPos.ToString()));
					continue;
				}
			}

			EntityAI asset = EntityAI.Cast(GetGame().CreateObject(cfg.AssetType, finalPos));
			if (asset) {
				if (isReload) {
					M4D_PlaneCrashLogger.Warn(string.Format("[EventID: %1] AUTO-HEAL CREATE: %2 em %3", wreck.GetEventID(), cfg.AssetType, finalPos.ToString()));
				}

				vector finalOri = wreck.GetOrientation() + cfg.Orientation;
				asset.SetPosition(finalPos);
				asset.SetOrientation(finalOri);
				
				if (Math.AbsFloat(cfg.Scale - 1.0) > 0.0001) asset.SetScale(cfg.Scale);
				
				if (cfg.AssetType == "StaticObj_ammoboxes_big" || cfg.AssetType == "M4DCrashStorage_AmmoBox" || cfg.AssetType == "M4DCrashStorage_ToolBox") {
					asset.PlaceOnSurface();
				}

				M4d_AirPlaneCrash.M4D_UpdateNavmesh(asset);
				
				if (cfg.AssetType.Contains("M4DCrashStorage")) {
					GetGame().GameScript.CallFunctionParams(asset, "SetOwnerEventID", null, new Param1<int>(wreck.GetEventID()));
				}
				
				wreck.AddDetailAsset(asset);
				
				if (!isReload) {
					if (asset.GetType() == "M4DCrashStorage_AmmoBox") M4D_PlaneCrashLootManager.FillAmmoBoxLoot(asset, tier);
					else if (asset.GetType() == "M4DCrashStorage_ToolBox") M4D_PlaneCrashLootManager.FillToolBoxLoot(asset, tier);
				}
			} else {
				if (isReload) {
					M4D_PlaneCrashLogger.Error(string.Format("[EventID: %1] AUTO-HEAL FAIL: %2 nao criado em %3 (Falha silenciosa do Enforce)", wreck.GetEventID(), cfg.AssetType, finalPos.ToString()));
				}
			}
		}
	}

	// =====================================================================================
	// A FUNÇÃO MESTRA DE MONTAGEM E DELEGAÇÃO DE SNAPSHOT
	// =====================================================================================
	static void SpawnSite()
	{
		M4D_PlaneCrashCustomSite chosenSite;
		vector pos = FindValidCrashSite(chosenSite);
		
		if (pos == vector.Zero || !chosenSite) return;

		int tier = chosenSite.Tier; 
		
		ref M4D_PlaneCrashSettings currentSettings = M4D_PlaneCrashSettings.Get();
		bool isGasSpawning = false;
		if (chosenSite.GasZone && chosenSite.GasZone.Enabled == 1) {
			isGasSpawning = true;
		}

		M4d_AirPlaneCrash wreck = M4d_AirPlaneCrash.Cast(GetGame().CreateObject("M4d_AirPlaneCrash", pos));
		if (!wreck) return;

		// =====================================================================================
		// ETAPA 1: O Nascimento Administrativo Limpo (ANTES de iniciar o Wreck)
		// =====================================================================================
		M4D_PlaneCrashWorldState.OpenEvent(wreck.GetEventID(), pos, tier, isGasSpawning);

		// O Wreck acorda e injeta os seus dados internos e arrays associados
		wreck.SetupEventState(tier, isGasSpawning);
		wreck.SetupThreatsData(chosenSite.ZombieTypes, chosenSite.WolfSpawn, chosenSite.BearSpawn);

		M4D_PlaneCrashLogger.Info("========== INICIO DE NOVO EVENTO ==========");
		M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Local Sorteado: %2 | Tier: %3", wreck.GetEventID(), pos.ToString(), tier));

		wreck.PlaceOnSurface();
		
		vector wPos = wreck.GetPosition();
		vector wMat[3]; 
		BuildWreckMatrix(wPos, "0 0 0", wMat);
		wreck.SetOrientation(Math3D.MatrixToAngles(wMat));
		wreck.SetPosition(wPos); 

		SpawnDetailAssets(wreck, wMat, tier, false);

		if (currentSettings && currentSettings.EnableWreckPathgraphUpdate == 1) {
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(M4d_AirPlaneCrash.M4D_UpdateNavmesh, 500, false, wreck);
		}

		string cType = PickEnabledCrashContainerType();
		EntityAI container = EntityAI.Cast(GetGame().CreateObject(cType, wPos + (wreck.GetDirection() * 25.0)));
		if (container) {
			M4D_PlaneCrashLogger.Info(string.Format("[EventID: %1] Container Gerado: %2", wreck.GetEventID(), cType));
			container.PlaceOnSurface();
			
			vector cPos = container.GetPosition();
			vector cNorm = GetGame().SurfaceGetNormal(cPos[0], cPos[2]);
			vector cFwd = vector.Direction(cPos, cPos + "1 0 0");
			vector cRight = cFwd * cNorm; cRight.Normalize();
			cFwd = cNorm * cRight; cFwd.Normalize();
			vector cMat[3]; cMat[0] = cFwd; cMat[1] = cNorm; cMat[2] = cRight;
			
			container.SetOrientation(Math3D.MatrixToAngles(cMat));
			container.SetPosition(cPos);

			GetGame().GameScript.CallFunctionParams(container, "SetOwnerEventID", null, new Param1<int>(wreck.GetEventID()));
			wreck.SetMainContainer(container);

			if (currentSettings && currentSettings.EnableContainerPathgraphUpdate == 1) {
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(M4d_AirPlaneCrash.M4D_UpdateNavmesh, 500, false, container);
			}

			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(M4D_PlaneCrashLootManager.FillContainerWithRandomLoot, 2000, false, container, tier);
		}

		// =====================================================================================
		// ETAPA 2: Atualização Parcial no Nascimento
		// Informa o que já se sabe: Contentor foi criado e assets visuais carregados
		// =====================================================================================
		M4D_PlaneCrashWorldState.UpdateCounts(wreck.GetEventID(), wreck.GetDetailAssetsCount(), 0, 0, 0);

		string key = "ShippingContainerKeys_Blue";
		if (cType.Contains("Red")) key = "ShippingContainerKeys_Red";
		else if (cType.Contains("Yellow")) key = "ShippingContainerKeys_Yellow";
		else if (cType.Contains("Orange")) key = "ShippingContainerKeys_Orange";
		
		// Início da Injeção de Ameaças
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(M4D_PlaneCrashThreats.SpawnThreatsInstance, 1000, false, wreck, isGasSpawning);
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(M4D_PlaneCrashThreats.M4D_SpawnZombiesWithKey, 3000, false, wreck, pos, key);
		
		// =====================================================================================
		// ETAPA 4: Atualização Consolidada Agendada (Orquestrada pelo Spawner)
		// Ocorre 5 segundos depois, dando tempo para as ameaças terminarem o Spawn.
		// =====================================================================================
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(M4D_PlaneCrashSpawner.ConsolidateEventState, 5000, false, wreck);

		if (currentSettings && currentSettings.EnableCrashNotification == 1) {
			string customMsg = chosenSite.NotificationMessage; 
			SendCrashNotification(wPos, customMsg);
		}

		// =====================================================================================
		// DISPARO DE SOM DIRECIONADO VIA RPC (Cálculo Otimizado no Servidor)
		// =====================================================================================
		const int M4D_RPC_CRASH_SOUND = 4857392; 
		array<Man> playersForSound = new array<Man>(); 
		GetGame().GetPlayers(playersForSound);
		
		for (int pIdx = 0; pIdx < playersForSound.Count(); pIdx++) {
			PlayerBase playerToHear = PlayerBase.Cast(playersForSound.Get(pIdx));
			if (playerToHear && playerToHear.GetIdentity()) {
				
				// Filtro: Servidor calcula a distância individual
				float distance = vector.Distance(playerToHear.GetPosition(), wPos);
				
				// Ignora todos os jogadores acima de 3500m
				if (distance <= 3500.0) {
					float speedOfSound = 343.0;
					float delaySeconds = distance / speedOfSound;
					int delayMilliseconds = Math.Round(delaySeconds * 1000);
					
					// Envio de Metadados (Coordenada + Atraso)
					GetGame().RPCSingleParam(playerToHear, M4D_RPC_CRASH_SOUND, new Param2<vector, int>(wPos, delayMilliseconds), true, playerToHear.GetIdentity());
				}
			}
		}
	}

	// =====================================================================================
	// Função auxiliar da ETAPA 4 para Consolidação do Estado no WorldState
	// =====================================================================================
	static void ConsolidateEventState(M4d_AirPlaneCrash wreck) 
	{
		if (!wreck || !GetGame() || !GetGame().IsServer()) return;
		
		// O Spawner recolhe do Wreck os arrays totalmente instanciados após a poeira baixar
		M4D_PlaneCrashWorldState.UpdateCounts(wreck.GetEventID(), wreck.GetDetailAssetsCount(), wreck.GetZombiesCount(), wreck.GetAnimalsCount(), wreck.GetGasAreasCount());
		M4D_PlaneCrashLogger.Debug(string.Format("[Spawner] Estado consolidado com sucesso para EventID %1", wreck.GetEventID()));
	}
}