// =====================================================================================
// CLASSES DE ESTRUTURA DE DADOS (JSON)
// =====================================================================================
class PlaneCrashAnimalSpawn
{
	int Enabled;
	int Count;
	float Radius;
	ref array<string> Types;

	void PlaneCrashAnimalSpawn()
	{
		Enabled = 0; Count = 0; Radius = 0.0;
		Types = new array<string>();
	}
}

class PlaneCrashGasZone
{
	int Enabled;

	void PlaneCrashGasZone()
	{
		Enabled = 0;
	}
}

class M4D_PlaneCrashCustomSite
{
	vector Position;
	string NotificationMessage;
	int Tier;

	ref array<string>         ZombieTypes;
	ref PlaneCrashAnimalSpawn WolfSpawn;
	ref PlaneCrashAnimalSpawn BearSpawn;
	ref PlaneCrashGasZone     GasZone;
}

class M4D_PlaneCrashLootEntry
{
	string Item;
	float  Chance; 
	
	ref array<string> Attachments;
	ref array<string> CargoItems;

	void M4D_PlaneCrashLootEntry(string item = "", float chance = 1.0)
	{
		Item = item;
		Chance = chance;
		Attachments = new array<string>();
		CargoItems = new array<string>();
	}
}

// =====================================================================================
// GERENCIADOR 1: LOCAIS (PlaneCrashSites.json)
// =====================================================================================
class M4D_PlaneCrashSites
{
	static const string SITES_PATH = "$profile:M4D_AirPlaneCrash/PlaneCrashSites.json";
	ref array<ref M4D_PlaneCrashCustomSite> CustomCrashSites;

	private static ref M4D_PlaneCrashSites s_Instance;

	static M4D_PlaneCrashSites Get()
	{
		if (!s_Instance) {
			s_Instance = new M4D_PlaneCrashSites();
			s_Instance.Load();
		}
		return s_Instance;
	}

	void Load()
	{
		if (FileExist(SITES_PATH)) {
			string errorMessage;
			if (!JsonFileLoader<M4D_PlaneCrashSites>.LoadFile(SITES_PATH, this, errorMessage)) {
				Print("[M4D_PlaneCrash] ERROR: Falha ao ler PlaneCrashSites.json! Restaurando backup nativo...");
				BuildDefault();
				Save();
			}
		} else {
			BuildDefault();
			Save();
		}
	}

	void Save() { JsonFileLoader<M4D_PlaneCrashSites>.JsonSaveFile(SITES_PATH, this); }

	void AddSite(vector pos, string msg, int tier)
	{
		M4D_PlaneCrashCustomSite s = new M4D_PlaneCrashCustomSite();
		s.Position = pos;
		s.NotificationMessage = msg;
		s.Tier = tier;
		
		s.ZombieTypes = new array<string>();
		s.ZombieTypes.InsertAll({
			"ZmbM_NBC_Grey",
			"ZmbM_PatrolNormal_Autumn",
			"ZmbM_PatrolNormal_Flat",
			"ZmbM_PatrolNormal_PautRev",
			"ZmbM_PatrolNormal_Summer",
			"ZmbM_SoldierNormal",
			"ZmbM_usSoldier_Heavy_Woodland",
			"ZmbM_usSoldier_Officer_Desert",
			"ZmbM_usSoldier_normal_Desert",
			"ZmbM_usSoldier_normal_Woodland"
		});
		
		s.WolfSpawn = new PlaneCrashAnimalSpawn();
		s.WolfSpawn.Enabled = 0;
		s.WolfSpawn.Count = 0;
		s.WolfSpawn.Radius = 0.0;
		s.WolfSpawn.Types.Insert("Animal_CanisLupus_Grey");
		s.WolfSpawn.Types.Insert("Animal_CanisLupus_White");
		
		s.BearSpawn = new PlaneCrashAnimalSpawn();
		s.BearSpawn.Enabled = 0;
		s.BearSpawn.Count = 0;
		s.BearSpawn.Radius = 0.0;
		s.BearSpawn.Types.Insert("Animal_UrsusArctos");
		
		s.GasZone = new PlaneCrashGasZone();
		s.GasZone.Enabled = 0;
		
		CustomCrashSites.Insert(s);
	}

	void BuildDefault()
	{
		CustomCrashSites = new array<ref M4D_PlaneCrashCustomSite>();
		
		AddSite(Vector(4748.660156, 0.0, 9732.080078), "Um avião foi abatido próximo a Airfield!", 3);
		AddSite(Vector(1269.11, 0.0, 5860.32), "Um avião foi abatido próximo a Plotina Tishina!", 3);
		AddSite(Vector(2731.03, 0.0, 3215.99), "Um avião foi abatido próximo a Maryanka!", 3);
		AddSite(Vector(5750.89, 0.0, 7614.38), "Um avião foi abatido próximo a Stary Sobor!", 3);
		AddSite(Vector(4600.42, 0.0, 12718.83), "Um avião foi abatido próximo a Zaprudnoye!", 3);
		AddSite(Vector(12312.50, 0.0, 12438.87), "Um avião foi abatido próximo a Airfield Krasno!", 1);
		AddSite(Vector(13999.56, 0.0, 14959.98), "Um avião foi abatido próximo a Belaya Polyana!", 1);
		AddSite(Vector(11399.70, 0.0, 7258.60), "Um avião foi abatido próximo a Youth Pioneer!", 1);
		AddSite(Vector(9141.54, 0.0, 3512.16), "Um avião foi abatido próximo a Pusta!", 1);
		AddSite(Vector(5127.78, 0.0, 2314.06), "Um avião foi abatido próximo a Airfield Balota!", 1);
		AddSite(Vector(9200.35, 0.0, 11807.50), "Um avião foi abatido próximo a Novy Lug!", 2);
		AddSite(Vector(4152.77, 0.0, 4807.96), "Um avião foi abatido próximo a Kozlovka!", 2);
		AddSite(Vector(4499.81, 0.0, 5960.86), "Um avião foi abatido próximo a Green Mountain!", 2);
		AddSite(Vector(6386.63, 0.0, 6712.77), "Um avião foi abatido próximo a Pop Ivan!", 2);
		AddSite(Vector(7753.39, 0.0, 8821.72), "Um avião foi abatido próximo a Gnomovzamok!", 2);
	}
}

// =====================================================================================
// GERENCIADOR 2: LOOT (PlaneCrashLoot.json)
// =====================================================================================
class M4D_PlaneCrashLoot
{
	static const string LOOT_PATH = "$profile:M4D_AirPlaneCrash/PlaneCrashLoot.json";

	ref array<ref M4D_PlaneCrashLootEntry> Tier1_MainLoot;
	ref array<ref M4D_PlaneCrashLootEntry> Tier2_MainLoot;
	ref array<ref M4D_PlaneCrashLootEntry> Tier3_MainLoot;
	
	ref array<ref M4D_PlaneCrashLootEntry> Tier1_AmmoBoxLoot;
	ref array<ref M4D_PlaneCrashLootEntry> Tier2_AmmoBoxLoot;
	ref array<ref M4D_PlaneCrashLootEntry> Tier3_AmmoBoxLoot;

	ref array<ref M4D_PlaneCrashLootEntry> Tier1_ToolBoxLoot;
	ref array<ref M4D_PlaneCrashLootEntry> Tier2_ToolBoxLoot;
	ref array<ref M4D_PlaneCrashLootEntry> Tier3_ToolBoxLoot;
	
	ref array<ref M4D_PlaneCrashLootEntry> DefaultLootItems;
	ref array<ref M4D_PlaneCrashLootEntry> DefaultAmmoBoxLoot;
	ref array<ref M4D_PlaneCrashLootEntry> DefaultToolBoxLoot;

	private static ref M4D_PlaneCrashLoot s_Instance;

	static M4D_PlaneCrashLoot Get()
	{
		if (!s_Instance) {
			s_Instance = new M4D_PlaneCrashLoot();
			s_Instance.Load();
		}
		return s_Instance;
	}

	void Load()
	{
		if (FileExist(LOOT_PATH)) {
			string errorMessage;
			if (!JsonFileLoader<M4D_PlaneCrashLoot>.LoadFile(LOOT_PATH, this, errorMessage)) {
				Print("[M4D_PlaneCrash] ERROR: Falha ao ler PlaneCrashLoot.json! Restaurando backup nativo...");
				BuildDefault();
				Save();
			}
		} else {
			BuildDefault();
			Save();
		}
	}

	void Save() { JsonFileLoader<M4D_PlaneCrashLoot>.JsonSaveFile(LOOT_PATH, this); }

	void BuildDefault()
	{
		Tier1_MainLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier2_MainLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier3_MainLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier1_AmmoBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier2_AmmoBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier3_AmmoBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier1_ToolBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier2_ToolBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		Tier3_ToolBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		DefaultLootItems = new array<ref M4D_PlaneCrashLootEntry>();
		DefaultAmmoBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();
		DefaultToolBoxLoot = new array<ref M4D_PlaneCrashLootEntry>();

		M4D_PlaneCrashLootEntry e;

		// --- TIER 1 MAIN ---
		e = new M4D_PlaneCrashLootEntry("MP5K", 1.0);
		e.Attachments.InsertAll({"MP5_PlasticHndgrd", "MP5k_StockBttstck", "Mag_MP5_15Rnd"});
		Tier1_MainLoot.Insert(e);

		// --- TIER 2 MAIN ---
		e = new M4D_PlaneCrashLootEntry("AliceBag_Green", 1.0);
		e.CargoItems.InsertAll({"VitaminBottle", "CombatKnife", "Glock19"});
		Tier2_MainLoot.Insert(e);

		// --- TIER 3 MAIN ---
		e = new M4D_PlaneCrashLootEntry("AK101", 1.0);
		e.Attachments.InsertAll({"AK_PlasticBttstck", "AK_PlasticHndgrd", "Mag_AK101_30Rnd"});
		Tier3_MainLoot.Insert(e);

		// --- AMMOBOXES ---
		Tier1_AmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_9x19_25rnd", 1.0));
		Tier1_AmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_45ACP_25rnd", 1.0));

		Tier2_AmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_545x39_20Rnd", 1.0));
		Tier2_AmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_556x45_20Rnd", 1.0));

		Tier3_AmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_762x39_20Rnd", 1.0));
		Tier3_AmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_762x54_20Rnd", 1.0));

		// --- TOOLBOXES ---
		Tier1_ToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("Pliers", 1.0));
		Tier1_ToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("NailBox", 1.0));

		Tier2_ToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("Pickaxe", 1.0));
		Tier2_ToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("Whetstone", 1.0));

		Tier3_ToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("MetalPlate", 1.0));
		Tier3_ToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("MetalWire", 1.0));

		// --- DEFAULTS ---
		e = new M4D_PlaneCrashLootEntry("AK101", 0.5);
		e.Attachments.InsertAll({"AK_PlasticBttstck", "Mag_AK101_30Rnd"});
		DefaultLootItems.Insert(e);
		DefaultLootItems.Insert(new M4D_PlaneCrashLootEntry("PlateCarrierVest", 1.0));
		DefaultLootItems.Insert(new M4D_PlaneCrashLootEntry("NVGoggles", 1.0));

		DefaultAmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_45ACP_25rnd", 1.0));
		DefaultAmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_9x19_25rnd", 1.0));
		DefaultAmmoBoxLoot.Insert(new M4D_PlaneCrashLootEntry("AmmoBox_357_20Rnd", 1.0));

		DefaultToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("MetalPlate", 1.0));
		DefaultToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("Pliers", 1.0));
		DefaultToolBoxLoot.Insert(new M4D_PlaneCrashLootEntry("Shovel", 1.0));
	}
}

// =====================================================================================
// GERENCIADOR 3: CONFIGURAÇÕES GERAIS (PlaneCrashSettings.json)
// =====================================================================================
class M4D_PlaneCrashSettings
{
	static const string SETTINGS_PATH = "$profile:M4D_AirPlaneCrash/PlaneCrashSettings.json";
	static const string FOLDER_PATH = "$profile:M4D_AirPlaneCrash";

	int EnableMod;
	int MaxActivePlaneEvents;
	int SafeRadius;
	int DistanceRadius;
	int CleanupRadius;
	int EnableCrashNotification;
	int EnableFileLogging;
	int EnableDebugLogging;
	int AlsoLogToRPT;
	int EnableWreckPathgraphUpdate;
	int EnableContainerPathgraphUpdate;
	int EnableZombiePathgraphUpdate;
	int EnableContainerBlue;
	int EnableContainerRed;
	int EnableContainerYellow;
	int EnableContainerOrange;
	string PreferredContainer;
	int ZombieCount;
	int EnableZombieKeyDrop;
	int MinLootItems;
	int MaxLootItems;
	int MinAmmoBoxItems;
	int MaxAmmoBoxItems;
	int MinToolBoxItems;
	int MaxToolBoxItems;
	int PristineLoot;
	int EnableOpeningAlarm;
	int EnableDefaultLootItems;
	int EnableDefaultAmmoBoxLoot;
	int EnableDefaultToolBoxLoot;

	private static ref M4D_PlaneCrashSettings s_Instance;

	static M4D_PlaneCrashSettings Get()
	{
		if (!s_Instance)
		{
			s_Instance = new M4D_PlaneCrashSettings();
			s_Instance.Load();

			// IMPORTANTE: Força a leitura dos outros dois ficheiros ao iniciar o servidor!
			M4D_PlaneCrashSites.Get();
			M4D_PlaneCrashLoot.Get();
		}
		return s_Instance;
	}

	void Load()
	{
		if (!FileExist(FOLDER_PATH)) { MakeDirectory(FOLDER_PATH); }

		if (FileExist(SETTINGS_PATH)) {
			string errorMessage;
			if (!JsonFileLoader<M4D_PlaneCrashSettings>.LoadFile(SETTINGS_PATH, this, errorMessage)) {
				Print("[M4D_PlaneCrash] ERROR: Falha ao ler PlaneCrashSettings.json! Restaurando backup nativo...");
				BuildDefaultSettings();
				Save();
			}
		} else {
			BuildDefaultSettings();
			Save();
		}
		
		ValidateSettings();
	}

	void Save() { JsonFileLoader<M4D_PlaneCrashSettings>.JsonSaveFile(SETTINGS_PATH, this); }

	void ValidateSettings()
	{
		if (MaxActivePlaneEvents > 10) { 
			MaxActivePlaneEvents = 3; 
			M4D_PlaneCrashLogger.Warn("[Settings] MaxActivePlaneEvents excedeu o limite de 10. Restaurado para o padrao: 3.");
		}
		
		if (SafeRadius <= 200) { SafeRadius = 1000; }
		if (DistanceRadius <= 200) { DistanceRadius = 1000; }
		if (CleanupRadius <= 200) { CleanupRadius = 1000; }
	}

	void BuildDefaultSettings()
	{
		EnableMod = 1;
		MaxActivePlaneEvents = 3;
		SafeRadius = 1000;
		DistanceRadius = 1000;
		CleanupRadius = 1000;
		EnableCrashNotification = 1;
		EnableFileLogging = 1;
		EnableDebugLogging = 0;
		AlsoLogToRPT = 1;
		EnableWreckPathgraphUpdate = 1;
		EnableContainerPathgraphUpdate = 1;
		EnableZombiePathgraphUpdate = 1;
		EnableContainerBlue = 1;
		EnableContainerRed = 1;
		EnableContainerYellow = 1;
		EnableContainerOrange = 1;
		PreferredContainer = "Random";
		ZombieCount = 15;
		EnableZombieKeyDrop = 1;
		MinLootItems = 2;
		MaxLootItems = 5;
		MinAmmoBoxItems = 2;
		MaxAmmoBoxItems = 5;
		MinToolBoxItems = 2;
		MaxToolBoxItems = 5;
		PristineLoot = 1;
		EnableOpeningAlarm = 1;
		EnableDefaultLootItems = 0;
		EnableDefaultAmmoBoxLoot = 0;
		EnableDefaultToolBoxLoot = 0;
	}
}