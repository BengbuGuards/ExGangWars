#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <utility>
#include <tgmath.h>
#include <algorithm>

// #include "GTASA_STRUCTS.h"

MYMOD(com.xbeatless.exgangwars, ExGangWars, 1.0, SilentPL & RusJJ & BengbuGuards)

enum {III=1,VC,SA}nGame;
enum {Netflex=1,Rockstar}nGameType;

const int NUM_GANGS = 10;

// SA classes
class CRGBA
{
public:
	uint8_t red, green, blue, alpha;

	inline CRGBA() {}

	inline CRGBA(const CRGBA& in)
		: red(in.red), green(in.green), blue(in.blue), alpha(in.alpha)
	{}

	inline CRGBA(const CRGBA& in, uint8_t alpha)
		: red(in.red), green(in.green), blue(in.blue), alpha(in.alpha)
	{}


	inline CRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
		: red(red), green(green), blue(blue), alpha(alpha)
	{}
};

class CZone
{
public:
	char					Name[8];
	char					TranslatedName[8];
	int16_t					x1, y1, z1;
	int16_t					x2, y2, z2;
	uint16_t				nZoneInfoIndex;
	uint8_t					bType;
	uint8_t					bTownID;
};

class CZoneInfo
{
public:
	uint8_t					GangDensity[NUM_GANGS];
	uint8_t					DrugDealerCounter;
	CRGBA					ZoneColor;
	bool					unk1 : 1;
	bool					unk2 : 1;
	bool					unk3 : 1;
	bool					unk4 : 1;
	bool					unk5 : 1;
	bool					bUseColour : 1;
	bool					bInGangWar : 1;
	bool					bNoCops : 1;
	uint8_t					flags;
};

struct tGangInfo
{
    bool     bCanFightWith : 1;
    bool     bShowOnMap : 1;
    uint8_t  bRed, bGreen, bBlue;
    uint32_t nBlipIndex;
};

// Savings
uintptr_t pGTASA;
void* hGTASA;
Config* cfg;
tGangInfo CustomGangInfo[NUM_GANGS];
rgba_t DefaultGangTurfColors[NUM_GANGS] =
{
    {0xC8, 0x00, 0xC8, 0x00}, // 0xC800C8
    {0x46, 0xC8, 0x00, 0x00}, // 0x46C800
    {0xFF, 0xC8, 0x00, 0x00}, // 0xFFC800
    {0x00, 0x00, 0xC8, 0x00}, // 0x0000C8
    {0xFF, 0x77, 0x00, 0x00}, // 0xFF7700
    {0xC8, 0xC8, 0xC8, 0x00}, // 0xC8C8C8
    {0xFF, 0x22, 0x22, 0x00}, // 0xFF2222
    {0x00, 0xC8, 0xFF, 0x00}, // 0x00C8FF
    {0x00, 0x00, 0x00, 0x00}, // 0x000000
    {0x00, 0x00, 0x00, 0x00}, // 0x000000
};
int DefaultGangAttackBlips[NUM_GANGS] = 
{
    59, 19, 60, 61, 19, 19, 19, 58, 19, 19
};

// Vars
uint16_t *TotalNumberOfNavigationZones;
uint16_t *TotalNumberOfZoneInfos;
CZone *NavigationZoneArray;
CZoneInfo *ZoneInfoArray;
float *TerritoryUnderControlPercentage;
int32_t *GangRatings;
int32_t *GangRatingStrength;
int32_t *GangWarsGang1;

// Funcs
bool (*CanPlayerStartAGangWarHere)(CZoneInfo*);
float (*GetStatValue)(uint16_t);
void (*SetStatValue)(uint16_t, float);

// Patches
/* uintptr_t StartOffensiveGangWar_BackTo1, StartOffensiveGangWar_BackTo2;
extern "C" uintptr_t StartOffensiveGangWar_Patch()
{
    return (*GangWarsGang1 == 1) ? StartOffensiveGangWar_BackTo2 : StartOffensiveGangWar_BackTo1;
}
__attribute__((optnone)) __attribute__((naked)) void StartOffensiveGangWar_Inject(void)
{
    asm volatile(
        "PUSH {R2}\n"
        "BL StartOffensiveGangWar_Patch\n"
        "POP {R2}\n");
    asm volatile(
        "BX R0");
}

uintptr_t AddKillToProvocation_BackTo1, AddKillToProvocation_BackTo2;
extern "C" uintptr_t AddKillToProvocation_Patch(int32_t nPedType)
{
    return (nPedType < 7 || nPedType > 16 || CustomGangInfo[nPedType - 7].bCanFightWith == false) ? AddKillToProvocation_BackTo2 : AddKillToProvocation_BackTo1;
}
__attribute__((optnone)) __attribute__((naked)) void AddKillToProvocation_Inject(void)
{
    asm volatile(
        "MOV R0, R4\n"
        "BL AddKillToProvocation_Patch\n");
    asm volatile(
        "BX R0");
}

extern "C" int32_t GetRivalGangsTotalDensity(uint32_t nZoneExtraInfoID)
{
    // We need to get a total strength of all gangs player can fight with
    // so the game can decide whether to start a defensive gang war there
    int32_t nTotalStrength = 0;
    
    for ( int32_t i = 0; i < NUM_GANGS; i++ )
    {
        if ( CustomGangInfo[i].bCanFightWith )
            nTotalStrength += ZoneInfoArray[nZoneExtraInfoID].GangDensity[i];
    }
    return nTotalStrength;
}
 */
// Hooks
DECL_HOOKv(FillZonesWithGangColours, bool bDontColour)
{
    for ( int16_t i = 0; i < *TotalNumberOfZoneInfos; i++ )
    {
        uint32_t nTotalDensity = 0;
        uint32_t bRedToPick = 0, bGreenToPick = 0, bBlueToPick = 0;

        for ( int j = 0; j < NUM_GANGS; j++ )
        {
            if ( CustomGangInfo[j].bShowOnMap )
            {
                nTotalDensity += ZoneInfoArray[i].GangDensity[j];
                bRedToPick += CustomGangInfo[j].bRed * ZoneInfoArray[i].GangDensity[j];
                bGreenToPick += CustomGangInfo[j].bGreen * ZoneInfoArray[i].GangDensity[j];
                bBlueToPick += CustomGangInfo[j].bBlue * ZoneInfoArray[i].GangDensity[j];
            }
        }

        ZoneInfoArray[i].bUseColour = nTotalDensity != 0 && !bDontColour && CanPlayerStartAGangWarHere(&ZoneInfoArray[i]);
        ZoneInfoArray[i].bInGangWar = false;

        ZoneInfoArray[i].ZoneColor.alpha = fmin(120, 3 * nTotalDensity);

        if ( nTotalDensity != 0 )
            ZoneInfoArray[i].ZoneColor.alpha = fmax(55, ZoneInfoArray[i].ZoneColor.alpha);
        else
            nTotalDensity = 1;

        // The result is a simple weighted arithmetic average
        // each gang's RGB having the weight of gang's density in this area
        ZoneInfoArray[i].ZoneColor.red = (uint8_t)(bRedToPick / nTotalDensity);
        ZoneInfoArray[i].ZoneColor.green = (uint8_t)(bGreenToPick / nTotalDensity);
        ZoneInfoArray[i].ZoneColor.blue = (uint8_t)(bBlueToPick / nTotalDensity);
    }
}
/* DECL_HOOKv(UpdateTerritoryUnderControlPercentage)
{
    std::pair<uint8_t,int32_t> vecZonesForGang[NUM_GANGS];
    int32_t                    nTotalTerritories = 0;

    // Initialise the array
    {
        uint8_t index = 0;
        for ( auto& it : vecZonesForGang )
        {
            it.first = index++;
        }
    }

    // Count the turfs belonging to each gang
    int zonesC = *TotalNumberOfNavigationZones;
    for ( int32_t i = 0; i < zonesC; i++ )
    {
        const uint32_t nZoneInfoIndex = NavigationZoneArray[i].nZoneInfoIndex;
        if ( nZoneInfoIndex != 0 )
        {
            // Should we even count this territory?
            bool bCountMe = false;

            for ( uint32_t j = 0; j < NUM_GANGS; j++ )
            {
                if ( (j == 1 || CustomGangInfo[j].bCanFightWith) && ZoneInfoArray[nZoneInfoIndex].GangDensity[j] != 0 )
                {
                    bCountMe = true;
                    break;
                }
            }

            if ( bCountMe )
            {
                // Instantiate a very temporary array to find which fightable gang has the most influence in this area
                uint8_t vecGangPopularity[NUM_GANGS];

                for ( uint8_t j = 0; j < NUM_GANGS; j++ )
                {
                    vecGangPopularity[j] = j == 1 || CustomGangInfo[j].bCanFightWith ? ZoneInfoArray[nZoneInfoIndex].GangDensity[j] : 0;
                }

                auto it = std::max_element( std::begin(vecGangPopularity), std::end(vecGangPopularity) );
                //auto it = std::max_element( &vecGangPopularity[0], &vecGangPopularity[NUM_GANGS-1] );

                // Add to gang's territory counter
                vecZonesForGang[std::distance(std::begin(vecGangPopularity), it)].second++;
                nTotalTerritories++;
            }
        }
    }

    // Update the stats
    SetStatValue(236, (float)(vecZonesForGang[1].second));                          // NUMBER_TERRITORIES_HELD
    SetStatValue(237, fmax((float)(vecZonesForGang[1].second), GetStatValue(237))); // HIGHEST_NUMBER_TERRITORIES_HELD

    if ( nTotalTerritories != 0 )
    {
        *TerritoryUnderControlPercentage = (float)(vecZonesForGang[1].second) / nTotalTerritories;

        // Sort the array to find top 3 gangs
        std::partial_sort(std::begin(vecZonesForGang), std::begin(vecZonesForGang) + 3, std::end(vecZonesForGang), [] (const auto& Left, const auto& Right)
        { 
            if ( Right.second < Left.second ) return true;
            if ( Left.second < Right.second ) return false;

            // In case of a tie, game favours GSF, then Ballas, then Vagos
            // So we sort by gang ID, with the exception that ID 1 (GSF) always comes first
            if ( Left.first == 1 ) return true;
            if ( Right.first == 1 ) return false;

            return Left.first < Right.first;
        });

        GangRatings[0] = vecZonesForGang[0].first;
        GangRatings[1] = vecZonesForGang[1].first;
        GangRatings[2] = vecZonesForGang[2].first;

        GangRatingStrength[0] = vecZonesForGang[0].second;
        GangRatingStrength[1] = vecZonesForGang[1].second;
        GangRatingStrength[2] = vecZonesForGang[2].second;
    }
    else
        *TerritoryUnderControlPercentage = 0.0f;
} */

// int main!
extern "C" void OnModLoad()
{
    logger->SetTag("ExGangWars");
    const char* szGame = aml->GetCurrentGame();
	logger->Info("Game %s", szGame);
	#define _stricmp strcasecmp
	if(!_stricmp(szGame, "com.netflix.NGP.GTASanAndreasDefinitiveEdition"))nGame=SA, nGameType=Netflex;
	else if(!_stricmp(szGame, "com.rockstargames.gtasa.de"))nGame=SA, nGameType=Rockstar;
	
	if(nGame != SA)return;

	pGTASA = aml->GetLib("libUE4.so");
    hGTASA = aml->GetLibHandle("libUE4.so");

    cfg = new Config("ExGangWars");
    cfg->Bind("Author", "", "About")->SetString("Silent, RusJJ, BengbuGuards");
        
    /* // Allow defensive gang wars in entire state
    aml->PlaceB(pGTASA + 0x30B044 + 0x1, pGTASA + 0x30B07C + 0x1);

    // CGangWars::StartOffensiveGangWar patches
    StartOffensiveGangWar_BackTo1 = pGTASA + 0x30A7E4 + 0x1;
    StartOffensiveGangWar_BackTo2 = pGTASA + 0x30A996 + 0x1;
    aml->Redirect(pGTASA + 0x30A7D2 + 0x1, (uintptr_t)StartOffensiveGangWar_Inject);
    //aml->PlaceNOP4(pGTASA + 0x30A7DA + 0x1, 1);
    //aml->Write8(pGTASA + 0x30A7DE, 0x01);

    // CGangWars::AddKillToProvocation patches
    AddKillToProvocation_BackTo1 = pGTASA + 0x30DFE8 + 0x1;
    AddKillToProvocation_BackTo2 = pGTASA + 0x30E046 + 0x1;
    aml->Redirect(pGTASA + 0x30DFE0 + 0x1, (uintptr_t)AddKillToProvocation_Inject);

    // CGangWars::PickZoneToAttack patches
    //PickZoneToAttack_BackTo = pGTASA + 0x30E046 + 0x1;
    //aml->Redirect(pGTASA + 0x30DFE0 + 0x1, (uintptr_t)PickZoneToAttack_Inject);
 */
    // Hooks
    HOOK(FillZonesWithGangColours, aml->GetSym(hGTASA, "_ZN9CTheZones24FillZonesWithGangColoursEb"));
    //HOOKPLT(UpdateTerritoryUnderControlPercentage, pGTASA + 0x67031C);

    // Getters!
    SET_TO(TotalNumberOfNavigationZones, aml->GetSym(hGTASA, "_ZN9CTheZones28TotalNumberOfNavigationZonesE"));
    SET_TO(TotalNumberOfZoneInfos, aml->GetSym(hGTASA, "_ZN9CTheZones22TotalNumberOfZoneInfosE"));
    SET_TO(NavigationZoneArray, aml->GetSym(hGTASA, "_ZN9CTheZones19NavigationZoneArrayE"));
    SET_TO(ZoneInfoArray, aml->GetSym(hGTASA, "_ZN9CTheZones13ZoneInfoArrayE"));
    SET_TO(TerritoryUnderControlPercentage, aml->GetSym(hGTASA, "_ZN9CGangWars31TerritoryUnderControlPercentageE"));
    SET_TO(GangRatings, aml->GetSym(hGTASA, "_ZN9CGangWars11GangRatingsE"));
    SET_TO(GangRatingStrength, aml->GetSym(hGTASA, "_ZN9CGangWars18GangRatingStrengthE"));
    SET_TO(GangWarsGang1, aml->GetSym(hGTASA, "_ZN9CGangWars5Gang1E"));

    SET_TO(CanPlayerStartAGangWarHere, aml->GetSym(hGTASA, "_ZN9CGangWars26CanPlayerStartAGangWarHereEP9CZoneInfo"));
    SET_TO(GetStatValue, aml->GetSym(hGTASA, "_ZN6CStats12GetStatValueEt"));
    SET_TO(SetStatValue, aml->GetSym(hGTASA, "_ZN6CStats12SetStatValueEtf"));

    // Config
    char sectionName[32];
    for(int i = 0; i < NUM_GANGS; ++i)
    {
        sprintf(sectionName, "Gang%d", i + 1);
        if(i == 1)
        {
            // For Groves, you can't fight with them, no matter what the player sets
            CustomGangInfo[i].bCanFightWith = false;
        }
        else
        {
            CustomGangInfo[i].bCanFightWith = true; //cfg->GetBool("CanFightWith", true, sectionName);
        }
        rgba_t clr = cfg->GetColor("TurfColour", DefaultGangTurfColors[i], sectionName);
        if(clr.r == 0 && clr.g == 0 && clr.b == 0)
        {
            CustomGangInfo[i].bShowOnMap = false;
        }
        else
        {
            CustomGangInfo[i].bShowOnMap = true;
            CustomGangInfo[i].bRed = clr.r;
            CustomGangInfo[i].bGreen = clr.g;
            CustomGangInfo[i].bBlue = clr.b;
        }
        CustomGangInfo[i].nBlipIndex = DefaultGangAttackBlips[i]; // cfg->GetInt("AttackBlip", DefaultGangAttackBlips[i], sectionName);
    }
}