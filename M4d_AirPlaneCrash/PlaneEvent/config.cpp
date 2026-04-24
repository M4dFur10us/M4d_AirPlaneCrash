class CfgPatches
{
    class M4D_PlaneEvent
    {
        units[] = {};
        weapons[] = {};
        requiredAddons[] = 
		{
			"DZ_Data",
			"DZ_Sounds_Effects",
			"DZ_Gear_Camping" // NOVO: Necessário para a engine reconhecer o SeaChest
		};
    };
};

class CfgMods
{
    class M4D_PlaneEvent
    {
        dir = "M4d_AirPlaneCrash";
        name = "M4D AirPlane Crash Event";
		author="xXM4dFur10usXx";
        picture = "";
        action = "";
        hideName = 1;
        hidePicture = 1;
        version = "2.0";
        type = "mod";
        dependencies[] = 
		{
			"Game",
			"World",
			"Mission"
		};
        class defs
        {
			class engineScriptModule
			{
				files[]=
				{
					"M4d_AirPlaneCrash/PlaneEvent/scripts/Common"
				};
			};
			class gameLibScriptModule
			{
				files[]=
				{
					"M4d_AirPlaneCrash/PlaneEvent/scripts/Common"
				};
			};
            class gameScriptModule
            {
                files[] = 
				{
					"M4d_AirPlaneCrash/PlaneEvent/scripts/Common"
				};
            };
            class worldScriptModule
            {
                files[] = 
				{
					"M4d_AirPlaneCrash/PlaneEvent/scripts/Common",
					"M4d_AirPlaneCrash/PlaneEvent/scripts/4_World"
				};
            };
			class missionScriptModule
            {
                files[] = 
				{
					"M4d_AirPlaneCrash/PlaneEvent/scripts/Common",
					"M4d_AirPlaneCrash/PlaneEvent/scripts/5_Mission"
				};
            };
        };
    };
};

class CfgVehicles
{
	class Land_Wreck_C130J_Cargo;
	class Land_ContainerLocked_Red_DE;
	class Land_ContainerLocked_Blue_DE;
	class Land_ContainerLocked_Yellow_DE;
	class Land_ContainerLocked_Orange_DE;
	class SeaChest; // MODIFICADO: Importa a classe base nativa da Bohemia para persistência

	class M4d_AirPlaneCrash: Land_Wreck_C130J_Cargo
	{
		scope=2;
		forceNavMesh=1;
		storageCategory=4;
	};

	class M4D_WreckContainerRed: Land_ContainerLocked_Red_DE
	{
		scope=2;
		forceNavMesh=1;
		attachments[]=
		{
		    "Truck_01_WoodenCrate1"
		};
		class GUIInventoryAttachmentsProps
		{
			class Cargo
			{
				name="Cargo";
				attachmentSlots[]=
				{
					"Truck_01_WoodenCrate1"
				};
			};
			
		};
	};
	class M4D_WreckContainerBlue: Land_ContainerLocked_Blue_DE
	{
		scope=2;
		forceNavMesh=1;
		attachments[]=
		{
		    "Truck_01_WoodenCrate1"
		};
		class GUIInventoryAttachmentsProps
		{
			class Cargo
			{
				name="Cargo";
				attachmentSlots[]=
				{
					"Truck_01_WoodenCrate1"
				};
			};
			
		};
	};
	class M4D_WreckContainerYellow: Land_ContainerLocked_Yellow_DE
	{
		scope=2;
		forceNavMesh=1;
		attachments[]=
		{
		    "Truck_01_WoodenCrate1"
		};
		class GUIInventoryAttachmentsProps
		{
			class Cargo
			{
				name="Cargo";
				attachmentSlots[]=
				{
					"Truck_01_WoodenCrate1"
				};
			};
			
		};
	};
	class M4D_WreckContainerOrange: Land_ContainerLocked_Orange_DE
	{
		scope=2;
		forceNavMesh=1;
		attachments[]=
		{
		    "Truck_01_WoodenCrate1"
		};
		class GUIInventoryAttachmentsProps
		{
			class Cargo
			{
				name="Cargo";
				attachmentSlots[]=
				{
					"Truck_01_WoodenCrate1"
				};
			};
			
		};
	};

	// =====================================================================
	// CLASSES DE STORAGE (Atualizadas para herdar do SeaChest)
	// =====================================================================
	class M4DCrashStorage: SeaChest
	{
		scope=1;
		model="DZ\structures\Military\Misc\Misc_SupplyBox2.p3d";
		displayName="";
		descriptionShort="";
		inventorySlot[]=
		{
			"Truck_01_WoodenCrate1"
		};
		class Cargo
		{
			itemsCargoSize[]={10,150};
			openable=0;
			allowOwnedCargoManipulation=1;
		};
		weight = 5000;
		itemBehaviour = 0;
		itemSize[] = {6,6};
		physLayer = "item_large";
	};
	class M4D_BoatCrashLoot: SeaChest
	{
		scope=1;
		model="DZ\structures\Military\Misc\Misc_SupplyBox2.p3d";
		displayName="";
		descriptionShort="";
		canBeHeldInHands=0;
		canBePlacedInCargo=0;
		inventorySlot[]=
		{
			"Truck_01_WoodenCrate1"
		};
		class Cargo
		{
			itemsCargoSize[]={10,150};
			openable=0;
			allowOwnedCargoManipulation=1;
		};
		weight = 5000;
		itemBehaviour = 0;
		itemSize[] = {6,6};
		physLayer = "item_large";
	};
	class M4D_IndustrialCrashLoot: SeaChest
	{
		scope=1;
		model="DZ\structures\military\misc\misc_supplybox3.p3d";
		displayName="";
		descriptionShort="";
		canBeHeldInHands=0;
		canBePlacedInCargo=0;
		inventorySlot[]=
		{
			"Truck_01_WoodenCrate1"
		};
		class Cargo
		{
			itemsCargoSize[]={10,150};
			openable=0;
			allowOwnedCargoManipulation=1;
		};
		weight = 5000;
		itemBehaviour = 0;
		itemSize[] = {6,6};
		physLayer = "item_large";
	};
	class M4DCrashStorage_AmmoBox: SeaChest
    {
        scope = 2;
        model = "DZ\structures_bliss\Underground\Storage\proxy\ammoboxes_single.p3d";
        displayName = "Caixa de Munição";
        descriptionShort = "Caixa militar fixa. Apenas retirada permitida.";
        canBeHeldInHands = 0;
        canBePlacedInCargo = 0;
        inventorySlot[] = {}; 
        class Cargo
        {
            itemsCargoSize[] = {10,100};
            openable = 0;
            allowOwnedCargoManipulation = 1;
        };
        weight = 500000;
        itemBehaviour = 0;
        itemSize[] = {10,10};
        physLayer = "item_large";
    };
	class M4DCrashStorage_ToolBox: SeaChest
    {
        scope = 2;
        model = "DZ\structures_bliss\Underground\Storage\proxy\ammoboxes_single.p3d";
        displayName = "Caixa de Ferramentas";
        descriptionShort = "Caixa militar fixa. Apenas retirada permitida.";
        canBeHeldInHands = 0;
        canBePlacedInCargo = 0;
        inventorySlot[] = {}; 
        class Cargo
        {
            itemsCargoSize[] = {10,100};
            openable = 0;
            allowOwnedCargoManipulation = 1;
        };
        weight = 500000;
        itemBehaviour = 0;
        itemSize[] = {10,10};
        physLayer = "item_large";
    };
};

class CfgSoundshaders
{
	class HeliCrash_Distant_SoundShader;
	class PlaneCrash_SoundShader: HeliCrash_Distant_SoundShader
	{
		samples[]=
		{
			{
				"\M4d_AirPlaneCrash\PlaneEvent\Sound\PlaneCrash",
				1
			}
		};
		volume=1.5;
		range=3500;
	};
	class PlaneCrash_UnlockAlarm_SoundShader: HeliCrash_Distant_SoundShader
	{
		samples[]=
		{
			{
				"\M4d_AirPlaneCrash\PlaneEvent\Sound\Open_Alarm",
				1
			}
		};
		volume=1.0;
		range=500;
	};
};

class CfgSoundsets
{
	class HeliCrash_Distant_Base_SoundSet;
	class PlaneCrash_SoundSet: HeliCrash_Distant_Base_SoundSet
	{
		soundShaders[]=
		{
			"PlaneCrash_SoundShader"
		};
		sound3DProcessingType="ThunderNear3DProcessingType";
	};
	class PlaneCrash_UnlockAlarm_SoundSet: HeliCrash_Distant_Base_SoundSet
	{
		soundShaders[]=
		{
			"PlaneCrash_UnlockAlarm_SoundShader"
		};
		sound3DProcessingType="ThunderNear3DProcessingType";
	};
};