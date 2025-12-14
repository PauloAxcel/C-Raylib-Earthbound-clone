#include "stringto.h"
#include "magic.h"
#include "skill.h"
#include "grid.h"
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <dirent.h>
#include <time.h>
#include <assert.h>
#include "cJSON.h"
#include "prototypes.h"
#include "music.h"
#include "achievements.h"
#include "minigames.h"
#include "saveload.h"

// Constants for Claw Switch Mechanic
#define CLAW_SWITCH_THRESHOLD 0.8f // How full the bar needs to be (0.0 to 1.0)
#define CLAW_FILL_RATE 1.5f      // Fill amount per second
#define CLAW_DECAY_RATE 2.5f     // Decay amount per second
#define CLAW_RESISTANCE_SEGMENT_INNER_RADIUS 80 // Should be >= main wheel outer radius
#define CLAW_RESISTANCE_SEGMENT_OUTER_RADIUS 95 // Adjust thickness as desired

#define DEFAULT_ENGAGEMENT_RADIUS 4.0f // Radius in tile units
// #define MAX_RTBS_CREATURES 10


// Global variables
// EnemyDatabase enemyDatabase = {0};
// AchievementState achievementState = {0};

const float standardSpeedCost = 1.0f;

// Terrain worldMap[MAP_HEIGHT][MAP_WIDTH];
int mapWidth = 0;
int mapHeight = 0;
Terrain **worldMap = NULL;
bool **explored = NULL; // 2D array to track explored tiles
float moveIncrement = 0.07f; // walking speed of main
float lag = 18.0f; // lag of followers compared to main

RenderTexture2D miniMapRenderTexture;
bool minimapNeedsUpdate = true;
static bool showFullMap = true;


// Creature pendingCreatureToAdd;
Follower followers[MAX_FOLLOWERS]; // Array of followers
int followerCount = 0;             // Number of active followers
FollowerBehavior currentFollowerBehavior = FOLLOWER_BEHAVIOR_FOLLOW;

SwapMenu swapMenu;

static int buttonPressCount = 0;
static float lastCheckTime = 0.0f; // Time in seconds


Menu menu = {0}; // Global variable

GroundPattern groundPattern = {0};
HolePattern holePattern = {0};

TiledMap currentTileMap = {0};

TerrainAnimation terrainAnimations[NUM_TERRAINS];
// note that the size of the terrainfolder compared to the terraintype is different which when iterating though will give NULL pointer to folder error.
TerrainType terrainTypes[NUM_TERRAINS] = {TERRAIN_MOUNTAIN, TERRAIN_WATER, TERRAIN_GRASS, TERRAIN_GROUND, TERRAIN_EMPTY, TERRAIN_BOSS, TERRAIN_USEDGROUND, TERRAIN_CUTGRASS, TERRAIN_MOON, TERRAIN_ICE, TERRAIN_MAGMA};

GameState previousGameState = GAME_STATE_UNAVAILABLE; // Invalid initial state
// static const char* currentMusic = NULL;
// GameState currentGameState = GAME_STATE_OVERWORLD;
GameState currentGameState = GAME_STATE_TITLE_SCREEN;

CurrentMap currentMap = MAP_OVERWORLD;
CurrentMap targetMap = MAP_EMPTY;  // Where we want to go

// Magic magicList[MAX_MAGIC];
// int magicCount = 0;

// static MagicLookupEntry magicLookup[MAX_MAGIC];
// static int magicLookupCount = 0;

// Skill skillList[MAX_SKILLS];
// int skillCount = 0;

// Array to hold our lookup entries
// static SkillLookupEntry skillLookup[MAX_SKILLS];
// static int skillLookupCount = 0;

Item playerInventory[MAX_ITEMS];       // Usable items list
Item keyItemList[MAX_KEY_ITEMS]; // Key items list
int itemCount = 0;              // Count of usable items
int keyItemCount = 0;           // Count of key items
Item masterItemList[MAX_ITEMS + MAX_KEY_ITEMS];
int masterItemCount = 0;

// EnemyDefeatInfo enemyDefeatList[MAX_ENEMY_TYPES];
// int enemyDefeatCount = 0;

// Global instance
Animations animations = {0};

// Global variables for overworld messages
Message overworldMessages[MAX_MESSAGES];
int overworldMessageCount = 0;

// Declare the tool wheel variable
ToolWheel toolWheel = {0};
// FishingMinigameState fishingMinigameState = {0}; // Initializes all members to zero or false
// PickaxeMinigameState pickaxeMinigameState = {0};
// DiggingMinigameState diggingMinigameState = {0};
// CuttingMinigameState cuttingMinigameState = {0};
// EatingState eatingState = {0}; // Initialize with zeros

Creature mainTeam[MAX_PARTY_SIZE] = {0};
Creature backupTeam[MAX_BACKUP_SIZE] = {0};
Creature bank[MAX_BANK_SIZE] = {0};

static int validTargets[MAX_PARTICIPANTS];
static int validCount = 0;
static int currentTargetPosition = 0;

// static int validAllies[MAX_PARTICIPANTS];
// static int validAlliesCount = 0;
// static int currentAlliesPosition = 0;


FrameTracking frameTracking = {0};
Particle particles[MAX_PARTICLES];

Position playerHistory[MAX_HISTORY]; // Circular buffer for player's positions
int historyIndex = 0;                // Index to track the buffer

bool justTransitioned = false;

SoundManager soundManager;
int targetFPS;

// MainMenuState mainMenuState = {0};
Camera2D camera = {0};
// BossBattleData bossBattleData = {0};
BattleState battleState = {0};
float shakeMagnitude = 5.0f; // Maximum shake offset
Vector2 shakeOffset = { 0.0f, 0.0f };

// TypeGridProgress typeGridProgress[NUM_TYPE];  // Assuming you have an enum count

// In main.c, near other global variables
RealTimeBattleState realTimeBattleState = {0};
// Global instance (or part of a larger game state struct)
CommandMenuState commandMenuState = {0};

// New fields for overworld creatures
OverworldCreature creatures[MAX_RTBS_CREATURES];    // Array of active creatures
int creatureCount;                                  // Number of active creatures
float spawnTimer;                                   // Timer for spawning new creatures
float spawnInterval;                                // How often to check for spawning

int main(void) {
    
    // Initialize everything
    InitializeGame();
        
    while (!WindowShouldClose()) {
        // uncoment here to have music!
        // UpdateMusic(&soundManager);
        // if (currentGameState != previousGameState) UpdateGameMusic(&soundManager, currentGameStatGameStateFishingMinigameDrawe);

        // printf("Previous game state %s\n Current game state %s\n", StateToString(previousGameState), StateToString(currentGameState));
        // printf("CMS.previous game state %s\n", StateToString(commandMenuState.previousState));

        HandleGlobalInput();

                        // Update sound and music
        UpdateMusic(&soundManager);
        if (currentGameState != previousGameState) {
            UpdateGameMusic(&soundManager, currentGameState);
        }
        

        previousGameState = currentGameState;


        // Update key states
        bool rightPressed = IsKeyDown(KEY_RIGHT);
        bool leftPressed = IsKeyDown(KEY_LEFT);
        bool downPressed = IsKeyDown(KEY_DOWN);
        bool upPressed = IsKeyDown(KEY_UP);
        bool APressed = IsKeyPressed(KEY_Z); // "Z" key
        bool BPressed = IsKeyDown(KEY_X); // "X" key
        bool LPressed = IsKeyPressed(KEY_A);
        bool RPressed = IsKeyPressed(KEY_S);
        bool YPressed = IsKeyPressed(KEY_C);

        bool keysPressed[] = {
            rightPressed,            // ">"
            leftPressed,             // "<"
            downPressed,             // "v"
            upPressed,               // "^"
            APressed,                // "Z"
            BPressed,                // "X"
            LPressed,                // "A"
            RPressed,                // "S"
            YPressed,                // "C"
        };


// LOGIC
        switch (currentGameState) {
            case GAME_STATE_TITLE_SCREEN: 
                UpdateTitleScreen(&mainMenuState);
                break;
            
            case GAME_STATE_MAIN_MENU: 
                UpdateTitleMainMenu(&mainMenuState);
                break;

            case GAME_STATE_LOAD_SELECTION:
                UpdateLoadSelection();
                break;
            
            case GAME_STATE_SETTINGS: 
                UpdateSettingsMenu(&mainMenuState);
                break;

            case GAME_STATE_QUIT_CONFIRM:
                UpdateQuitConfirm(&mainMenuState);
                break;

            case GAME_STATE_OVERWORLD: {
                // Check if we need to transition to a different map
                if (targetMap != MAP_EMPTY && targetMap != currentMap) {
                    printf("Transitioning from map %d to map %d\n", currentMap, targetMap);
                    
                    // Handle the specific transition
                    switch (targetMap) {
                        case MAP_OVERWORLD_DIGGING:
                            TransitionToTunnelState();
                            break;
                        case MAP_OVERWORLD:
                            TransitionToOverworldState();
                            break;
                        case MAP_OVERWORLD_HOLE:
                            TransitionToHoleState();
                            break;
                        case MAP_INSIDE_HOLE:
                            printf("Inside HOLE :D\n");
                            TransitionToInsideHoleState();
                            DrawTiledMap(camera);
                            break;
                        default: assert(false && "Unhandled enum value!"); break;
                        // Easy to add more cases later
                    }
                    
                    // Update current map and reset target
                    currentMap = targetMap;
                    targetMap = MAP_EMPTY;  // Fixed assignment operator
                }

                // In your code where you transition to RTBS mode
                if (toolWheel.tools[toolWheel.selectedToolIndex].type == TOOL_CLAW) {
                    currentGameState = GAME_STATE_REALTIME_BATTLE;
                    realTimeBattleState.isActive = true;
                    printf("Switching to RealTimeBattle State (Claw equipped)\n");
                    break;
                }
                
                // Then handle the overworld update as usual
                UpdateOverworld(APressed, LPressed, RPressed, &bossBattleData, &battleState);
                break;
            }

            case GAME_STATE_MENU: {
                // check which index we need to get for the submenu
                MenuState cm = menu.currentMenu;
                int *selIndex = &menu.selectedIndex[cm];
                SyncTeamStats();

                switch (cm) {
                    case MENU_MAIN: {
                        const int optionCount = MENU_MAIN_COUNT;

                        if (IsKeyPressed(KEY_DOWN)) {
                            (*selIndex)++;
                            if (*selIndex >= optionCount) *selIndex = 0;
                        }

                        if (IsKeyPressed(KEY_UP)) {
                            (*selIndex)--;
                            if (*selIndex < 0) *selIndex = optionCount - 1;
                        }

                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z)) {
                            switch (*selIndex) {
                                case MENU_MAIN_PARTY: { 
                                    menu.currentMenu = MENU_PARTY;
                                    menu.partyViewMode = PARTY_MAIN_TEAM;
                                    menu.selectedIndex[MENU_PARTY] = 0;
                                    // menu.partyViewMode = 0; // Start with Main Team
                                    break;
                                }
                                case MENU_MAIN_ITEMS: {
                                    menu.currentMenu = MENU_ITEMS;
                                    break;
                                }
                                case MENU_MAIN_GRID: {
                                    menu.currentMenu = MENU_GRID;
                                    menu.selectedCreature = 0;
                                    FriendshipGrid* grid = &menu.gridState;
                                    InitializeGridForCreature(grid, &mainTeam[menu.selectedCreature]);
                                    break;
                                }
                                case MENU_MAIN_ACHIEVEMENTS: {
                                    menu.currentMenu = MENU_ACHIEVEMENTS;
                                    break;
                                }                                
                                case MENU_MAIN_SAVE: {
                                    menu.currentMenu = MENU_SAVE_SELECTION;
                                    InitSaveMenu(&menu);
                                    break;
                                }
                                case MENU_MAIN_SETTINGS: {
                                    // Copy current settings to menu for temporary editing
                                    menu.gameSettings = mainMenuState;
                                    menu.currentMenu = MENU_SETTINGS;
                                    menu.selectedIndex[MENU_SETTINGS] = 0; // Start with first option selected
                                    break;
                                }
                                case MENU_MAIN_QUIT: {
                                    currentGameState = GAME_STATE_QUIT_CONFIRM;
                                    mainMenuState.confirmQuitSelection = false; // Default to "No"
                                    break;
                                }                               
                            }
                        }
                        
                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                            currentGameState = GAME_STATE_OVERWORLD;
                            menu.partyViewMode = PARTY_MAIN_TEAM;
                            menu.selectedIndex[MENU_PARTY] = 0;
                        }
                        break;
                    }

                    case MENU_PARTY: {
                        Creature *currentTeam = NULL;
                        int teamSize = 0;
                        static bool arrowKeysLocked = false;  // Static to persist between frames
                        
                        // First determine current team and size
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM: { currentTeam = mainTeam; teamSize = MAX_PARTY_SIZE; break; }
                            case PARTY_BACKUP_TEAM: { currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE; break; }
                            case PARTY_BANK: { currentTeam = bank; teamSize = MAX_BANK_SIZE; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }

                        // Process view changes first
                        // bool viewChanged = false;
                        
                        if (IsKeyPressed(KEY_A) || (menu.partyViewMode != PARTY_BANK && IsKeyPressed(KEY_LEFT))) {
                            int originalIndex = *selIndex;
                            // int prevViewMode = menu.partyViewMode;
                            menu.partyViewMode = (menu.partyViewMode - 1 + PARTY_VIEW_COUNT) % PARTY_VIEW_COUNT;
                            // viewChanged = true;
                            
                            // Recalculate team size for new view
                            int newTeamSize = 0;
                            switch (menu.partyViewMode) {
                                case PARTY_MAIN_TEAM: newTeamSize = MAX_PARTY_SIZE; break;
                                case PARTY_BACKUP_TEAM: newTeamSize = MAX_BACKUP_SIZE; break;
                                case PARTY_BANK: newTeamSize = MAX_BANK_SIZE; break;
                                default: assert(false && "Unhandled enum value!"); break;
                            }
                            
                            // Preserve index where possible, but ensure it's valid for new view
                            if (originalIndex >= newTeamSize) {
                                *selIndex = newTeamSize - 1;
                            }
                            
                            // Lock arrow keys if going to bank
                            if (menu.partyViewMode == PARTY_BANK) {
                                arrowKeysLocked = true;
                            }
                        }
                        
                        if (IsKeyPressed(KEY_S) || (menu.partyViewMode != PARTY_BANK && IsKeyPressed(KEY_RIGHT))) {
                            int originalIndex = *selIndex;
                            // int prevViewMode = menu.partyViewMode;
                            menu.partyViewMode = (menu.partyViewMode + 1) % PARTY_VIEW_COUNT;
                            // viewChanged = true;
                            
                            // Recalculate team size for new view
                            int newTeamSize = 0;
                            switch (menu.partyViewMode) {
                                case PARTY_MAIN_TEAM: newTeamSize = MAX_PARTY_SIZE; break;
                                case PARTY_BACKUP_TEAM: newTeamSize = MAX_BACKUP_SIZE; break;
                                case PARTY_BANK: newTeamSize = MAX_BANK_SIZE; break;
                                default: assert(false && "Unhandled enum value!"); break;
                            }
                            
                            // Preserve index where possible, but ensure it's valid for new view
                            if (originalIndex >= newTeamSize) {
                                *selIndex = newTeamSize - 1;
                            }
                            
                            // Lock arrow keys if going to bank
                            if (menu.partyViewMode == PARTY_BANK) {
                                arrowKeysLocked = true;
                            }
                        }

                        // Check if arrow keys have been released to unlock them
                        if (arrowKeysLocked) {
                            if (!IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT) && 
                                !IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN)) {
                                arrowKeysLocked = false;
                            }
                        }

                        // Handle view-specific navigation only if keys are not locked
                        if (!arrowKeysLocked) {
                            if (menu.partyViewMode == PARTY_BANK) {
                                // Grid navigation for bank view (3x3 grid)
                                const int columns = 3;
                                int currentRow = (*selIndex) / columns;
                                int currentCol = (*selIndex) % columns;

                                if (IsKeyDownSmooth(KEY_DOWN, &menu.scrollDownKey)) {
                                    currentRow++;
                                    if (currentRow * columns >= teamSize) currentRow = 0;
                                    *selIndex = currentRow * columns + currentCol;
                                    if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                                }
                                if (IsKeyDownSmooth(KEY_UP, &menu.scrollUpKey)) {
                                    currentRow--;
                                    if (currentRow < 0) currentRow = (teamSize - 1) / columns;
                                    *selIndex = currentRow * columns + currentCol;
                                    if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                                }
                                if (IsKeyDownSmooth(KEY_RIGHT, &menu.scrollRightKey) || IsKeyPressed(KEY_RIGHT)) {
                                    currentCol = (currentCol + 1) % columns;
                                    *selIndex = currentRow * columns + currentCol;
                                    if (*selIndex >= teamSize) *selIndex = currentRow * columns;
                                }
                                if (IsKeyDownSmooth(KEY_LEFT, &menu.scrollLeftKey) || IsKeyPressed(KEY_LEFT)) {
                                    currentCol = (currentCol - 1 + columns) % columns;
                                    *selIndex = currentRow * columns + currentCol;
                                    if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                                }
                            } else {
                                // Regular vertical navigation for main/backup teams
                                if (IsKeyDownSmooth(KEY_DOWN, &menu.scrollDownKey)) {
                                    (*selIndex)++;
                                    if (*selIndex >= teamSize) *selIndex = 0;
                                }
                                if (IsKeyDownSmooth(KEY_UP, &menu.scrollUpKey)) {
                                    (*selIndex)--;
                                    if (*selIndex < 0) *selIndex = teamSize - 1;
                                }
                            }
                        }

                        // Confirm -> Go to details
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z)) {
                            if (currentTeam[*selIndex].name[0] != '\0') {
                                menu.currentMenu = MENU_PARTY_DETAILS;
                                menu.subSelectedIndex[MENU_PARTY_DETAILS] = *selIndex;
                            }
                        }

                        // Cancel/back -> Go back to MAIN (same as original code)
                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                            menu.currentMenu = MENU_MAIN;
                            float leaderX = mainTeam[0].x;
                            float leaderY = mainTeam[0].y;

                            if (mainTeam[0].name[0] == '\0')  {
                                (*selIndex) = 0;
                                bool foundReplacement = false;

                                // (A) Look in the rest of mainTeam
                                for (int i = 1; i < MAX_PARTY_SIZE; i++) {
                                    if (mainTeam[i].name[0] != '\0') {
                                        SwapCreatures(&mainTeam[0], &mainTeam[i]);
                                        foundReplacement = true;
                                        break;
                                    }
                                }

                                // (B) If still no occupant, look in backupTeam
                                if (!foundReplacement) {
                                    for (int i = 0; i < MAX_BACKUP_SIZE; i++) {
                                        if (backupTeam[i].name[0] != '\0') {
                                            SwapCreatures(&mainTeam[0], &backupTeam[i]);
                                            foundReplacement = true;
                                            break;
                                        }
                                    }
                                }

                                // (C) If still no occupant, look in bank
                                if (!foundReplacement) {
                                    for (int i = 0; i < MAX_BANK_SIZE; i++) {
                                        if (bank[i].name[0] != '\0') {
                                            SwapCreatures(&mainTeam[0], &bank[i]);
                                            foundReplacement = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            CompactTeam(mainTeam, MAX_PARTY_SIZE);

                                // Restore the leader position if we have a valid leader
                            if (mainTeam[0].name[0] != '\0') {
                                mainTeam[0].x = leaderX;
                                mainTeam[0].y = leaderY;
                            }

                            // SyncTeamStats();

                            // Rebuild followers
                            followerCount = 0;
                            for (int i = 1; i < MAX_PARTY_SIZE; i++) {
                                if (mainTeam[i].name[0] != '\0') {
                                    AddFollower(&mainTeam[i]);
                                }
                            }
                        }

                        // Swap functionality
                        if (IsKeyPressed(KEY_C)) {
                            if (currentTeam[*selIndex].name[0] != '\0') {
                                menu.swapInProgress = true;
                                menu.swapSourceIndex = *selIndex;
                                menu.swapSourceParty = menu.partyViewMode;
                                menu.selectedIndex[MENU_PARTY_SWAP] = *selIndex;
                                menu.currentMenu = MENU_PARTY_SWAP;
                            }
                        }
                        break;
                    }

                    case MENU_PARTY_DETAILS: {
                        int teamSize = 0;
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM: { teamSize = MAX_PARTY_SIZE; break; }
                            case PARTY_BACKUP_TEAM: { teamSize = MAX_BACKUP_SIZE; break; }
                            case PARTY_BANK: { teamSize = MAX_BANK_SIZE; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }

                        if (IsKeyPressed(KEY_LEFT)) {
                            menu.subSelectedIndex[MENU_PARTY_DETAILS]--;
                            if (menu.subSelectedIndex[MENU_PARTY_DETAILS] < 0) menu.subSelectedIndex[MENU_PARTY_DETAILS] = teamSize - 1;
                        }

                        if (IsKeyPressed(KEY_RIGHT)) {
                            (menu.subSelectedIndex[MENU_PARTY_DETAILS])++;
                            if (menu.subSelectedIndex[MENU_PARTY_DETAILS] >= teamSize) menu.subSelectedIndex[MENU_PARTY_DETAILS] = 0;
                        }

                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                            menu.currentMenu = MENU_PARTY;
                            menu.selectedIndex[MENU_PARTY] = menu.subSelectedIndex[MENU_PARTY_DETAILS];
                            // this code here is not working, when i go back its on the wrong index
                        }
                        break;
                    }

                    case MENU_PARTY_SWAP: {
                        int *selIndex = &menu.selectedIndex[MENU_PARTY_SWAP];
                        Creature *currentTeam = NULL;
                        int teamSize = 0;
                        
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM: { currentTeam = mainTeam; teamSize = MAX_PARTY_SIZE; break; }
                            case PARTY_BACKUP_TEAM: { currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE; break; }
                            case PARTY_BANK: { currentTeam = bank; teamSize = MAX_BANK_SIZE; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }

                        if (menu.partyViewMode == PARTY_BANK) {
                            // Grid navigation for bank view (3x3 grid)
                            const int columns = 3;
                            // const int rows = 3;
                            int currentRow = (*selIndex) / columns;
                            int currentCol = (*selIndex) % columns;

                            if (IsKeyPressed(KEY_DOWN)) {
                                currentRow++;
                                if (currentRow * columns >= teamSize) currentRow = 0;
                                *selIndex = currentRow * columns + currentCol;
                                if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                            }
                            if (IsKeyPressed(KEY_UP)) {
                                currentRow--;
                                if (currentRow < 0) currentRow = (teamSize - 1) / columns;
                                *selIndex = currentRow * columns + currentCol;
                                if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                            }
                            if (IsKeyPressed(KEY_RIGHT)) {
                                currentCol = (currentCol + 1) % columns;
                                *selIndex = currentRow * columns + currentCol;
                                if (*selIndex >= teamSize) *selIndex = currentRow * columns;
                            }
                            if (IsKeyPressed(KEY_LEFT)) {
                                currentCol = (currentCol - 1 + columns) % columns;
                                *selIndex = currentRow * columns + currentCol;
                                if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                            }
                        } else {
                            // Regular vertical navigation for main/backup teams
                            if (IsKeyPressed(KEY_DOWN)) {
                                (*selIndex)++;
                                if (*selIndex >= teamSize) *selIndex = 0;
                            }
                            if (IsKeyPressed(KEY_UP)) {
                                (*selIndex)--;
                                if (*selIndex < 0) *selIndex = teamSize - 1;
                            }
                            
                            // A/S to change the partyViewMode (instead of Left/Right)
                            if (IsKeyPressed(KEY_LEFT)) {
                                menu.partyViewMode = (menu.partyViewMode - 1 + PARTY_VIEW_COUNT) % PARTY_VIEW_COUNT;
                                // Adjust selIndex if needed
                                if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                                if (teamSize == 0) *selIndex = 0;
                            }
                            if (IsKeyPressed(KEY_RIGHT)) {
                                menu.partyViewMode = (menu.partyViewMode + 1) % PARTY_VIEW_COUNT;
                                // Adjust selIndex if needed
                                if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                                if (teamSize == 0) *selIndex = 0;
                            }
                        }

                        // A/S to change the partyViewMode (instead of Left/Right)
                        if (IsKeyPressed(KEY_A)) {
                            menu.partyViewMode = (menu.partyViewMode - 1 + PARTY_VIEW_COUNT) % PARTY_VIEW_COUNT;
                            // Adjust selIndex if needed
                            if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                            if (teamSize == 0) *selIndex = 0;
                        }
                        if (IsKeyPressed(KEY_S)) {
                            menu.partyViewMode = (menu.partyViewMode + 1) % PARTY_VIEW_COUNT;
                            // Adjust selIndex if needed
                            if (*selIndex >= teamSize) *selIndex = teamSize - 1;
                            if (teamSize == 0) *selIndex = 0;
                        }

                        // Press X => Cancel the swap
                        if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_BACKSPACE)) {
                            menu.swapInProgress = false;
                            menu.partyViewMode = menu.swapSourceParty;
                            menu.currentMenu = MENU_PARTY;
                        }

                        // Press Z => Finalize swap
                        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_C)) {
                            // Only if they didn't pick the same slot
                            if (!(menu.partyViewMode == menu.swapSourceParty && *selIndex == menu.swapSourceIndex)) {
                                // Store the leader coordinates before any changes
                                float leaderX = mainTeam[0].x;
                                float leaderY = mainTeam[0].y;

                                // 1) Identify the source
                                Creature *sourceTeam = NULL;
                                switch (menu.swapSourceParty) {
                                    case PARTY_MAIN_TEAM: sourceTeam = mainTeam; break;
                                    case PARTY_BACKUP_TEAM: sourceTeam = backupTeam; break;
                                    case PARTY_BANK: sourceTeam = bank; break;
                                }

                                // 2) Swap
                                SwapCreatures(&sourceTeam[menu.swapSourceIndex], &currentTeam[*selIndex]);

                                // 3) Always restore the leader position
                                mainTeam[0].x = leaderX;
                                mainTeam[0].y = leaderY;

                                // (B) If mainTeam was involved at all, rebuild the followers
                                bool mainTeamAffected = false;
                                if (menu.swapSourceParty == 0) mainTeamAffected = true;
                                if (menu.partyViewMode == 0) mainTeamAffected = true;

                                if (mainTeamAffected) {
                                    followerCount = 0;
                                    // Rebuild from mainTeam[1], mainTeam[2], ...
                                    for (int i = 1; i < MAX_PARTY_SIZE; i++) {
                                        if (mainTeam[i].name[0] != '\0') {
                                            AddFollower(&mainTeam[i]);
                                        }
                                    }
                                }
                            } // end if different slot

                            menu.selectedIndex[MENU_PARTY] = *selIndex;
                            menu.swapInProgress = false;
                            menu.currentMenu = MENU_PARTY;
                        }
                        break;
                    }

                    case MENU_ITEMS: {
                        // Determine which item list to display (usable or key items)
                        Item *currentItems = NULL;
                        int itemListSize = 0;
                        int *currentIndex = NULL;
                        const int columns = 2;

                        switch (menu.itemViewMode) {
                            case ITEM_USABLE: { currentItems = playerInventory; itemListSize = itemCount; currentIndex = &menu.usableItemIndex; break; }
                            case ITEM_KEY: { currentItems = keyItemList; itemListSize = keyItemCount; currentIndex = &menu.keyItemIndex; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }
                        
                        // Up/Down for row navigation with smooth scrolling
                        if (IsKeyDownSmooth(KEY_DOWN, &menu.scrollDownKey)) {
                            *currentIndex += columns; // Move down one row
                            if (*currentIndex >= itemListSize) {
                                *currentIndex = *currentIndex % columns; // Keep column position
                            }
                        }
                        if (IsKeyDownSmooth(KEY_UP, &menu.scrollUpKey)) {
                            *currentIndex -= columns; // Move up one row
                            if (*currentIndex < 0) {
                                // Wrap to last row, same column
                                int lastRowIndex = ((itemListSize - 1) / columns) * columns;
                                *currentIndex = lastRowIndex + (*currentIndex + columns) % columns;
                                if (*currentIndex >= itemListSize) *currentIndex = itemListSize - 1;
                            }
                        }
                        
                        // Left/Right for column navigation within the same row (single press)
                        if (IsKeyPressed(KEY_LEFT)) {
                            int row = (*currentIndex) / columns;
                            int col = (*currentIndex) % columns;
                            col = (col - 1 + columns) % columns;
                            int newIndex = row * columns + col;
                            if (newIndex < itemListSize) *currentIndex = newIndex;
                        }
                        if (IsKeyPressed(KEY_RIGHT)) {
                            int row = (*currentIndex) / columns;
                            int col = (*currentIndex) % columns;
                            col = (col + 1) % columns;
                            int newIndex = row * columns + col;
                            if (newIndex < itemListSize) *currentIndex = newIndex;
                        }

                        // A/S to switch between usable items and key items
                        if (IsKeyPressed(KEY_A)) {
                            menu.itemViewMode = ITEM_USABLE;
                        }
                        if (IsKeyPressed(KEY_S)) {
                            menu.itemViewMode = ITEM_KEY;
                        }

                        // Confirm/Use item
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z)) {
                            menu.itemPointer = &currentItems[*currentIndex];
                            if (currentItems[*currentIndex].amount > 0) {
                                if (menu.itemViewMode == ITEM_USABLE) {  // Usable items
                                    if (currentItems[*currentIndex].usageType == USAGE_OVERWORLD || currentItems[*currentIndex].usageType == USAGE_BOTH) {
                                        if (currentItems[*currentIndex].effectType == EFFECT_HEALING || currentItems[*currentIndex].effectType == EFFECT_BUFF) {
                                            printf("Healing with %s in the overworld!\n", currentItems[*currentIndex].name);
                                            menu.currentMenu = MENU_ITEM_PARTY_SELECTION;
                                            menu.itemToUse = currentItems[*currentIndex];  // Save the item for use after selection
                                            menu.itemSelectionInProgress = true;
                                        } 
                                        else if (currentItems[*currentIndex].effectType == EFFECT_OFFENSIVE) {
                                            printf("Using map-modifying item: %s\n", currentItems[*currentIndex].name);
                                            ModifyTerrainWithItem(&currentItems[*currentIndex], &menu);  // Use item like nade or explosion to modify the map
                                            currentGameState = GAME_STATE_OVERWORLD;
                                            menu.currentMenu = MENU_MAIN;
                                        }
                                    }
                                }
                                else if (menu.itemViewMode == ITEM_KEY) {
                                    EquipKeyItem(&currentItems[*currentIndex]);
                                    currentGameState = GAME_STATE_OVERWORLD;  // Return to the overworld
                                    menu.currentMenu = MENU_MAIN;
                                    toolWheel.timer = 120;
                                }
                            }
                        }

                        // Swap items
                        if (IsKeyPressed(KEY_C)) {
                            if (!menu.swapInProgress) {
                                menu.swapInProgress = true;
                                menu.swapSourceIndex = *currentIndex;
                                menu.swapSourceMode = menu.itemViewMode;  // Save the current item list mode
                            } else {
                                // Swap items between selected indices
                                int swapTargetIndex = *currentIndex;
                                int swapSourceIndex = menu.swapSourceIndex;

                                if (menu.itemViewMode == menu.swapSourceMode) {
                                    Item temp = currentItems[swapSourceIndex];
                                    currentItems[swapSourceIndex] = currentItems[swapTargetIndex];
                                    currentItems[swapTargetIndex] = temp;

                                    // Reset swap state
                                    menu.swapInProgress = false;
                                }
                            }
                        }

                        // Cancel/Back
                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                            menu.currentMenu = MENU_MAIN;
                            menu.swapInProgress = false;
                        }

                        break;
                    }

                    // this section needs to be like the party section and we need to write the item that we choose on top.
                    case MENU_ITEM_PARTY_SELECTION: {

                        // Update the healing animation
                        if (menu.healingAnimation.isAnimating) {
                            menu.healingAnimation.frameCounter++;
                            if (menu.healingAnimation.frameCounter >= menu.healingAnimation.frameSpeed) {
                                menu.healingAnimation.frameCounter = 0;
                                menu.healingAnimation.currentFrame++;
                                if (menu.healingAnimation.currentFrame >= menu.healingAnimation.frameCount) {
                                    menu.healingAnimation.isAnimating = false; // End the animation
                                }
                            }
                        }

                        Creature *currentTeam = NULL;  // Assume the party is always mainTeam
                        int teamSize = 0;

                            // Determine which team is currently selected
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM:   { currentTeam = mainTeam;   teamSize = MAX_PARTY_SIZE;   break; }
                            case PARTY_BACKUP_TEAM: { currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE;  break; }
                            case PARTY_BANK:        { currentTeam = bank;       teamSize = MAX_BANK_SIZE;    break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }

                        Item *currentItems = NULL;
                        int itemListSize = 0;
                        int *currentIndex = NULL;

                        switch (menu.itemViewMode) {
                            case ITEM_USABLE: { currentItems = playerInventory; itemListSize = itemCount; currentIndex = &menu.usableItemIndex; break; }  // Usable items
                            case ITEM_KEY: { currentItems = keyItemList; itemListSize = keyItemCount; currentIndex = &menu.keyItemIndex; break; } // Key items
                            default: assert(false && "Unhandled enum value!"); break;
                        }


                        // Navigate party members
                        if (IsKeyPressed(KEY_DOWN)) {
                            (*selIndex)++;
                            if (*selIndex >= teamSize) *selIndex = 0;
                        }
                        if (IsKeyPressed(KEY_UP)) {
                            (*selIndex)--;
                            if (*selIndex < 0) *selIndex = teamSize - 1;
                        }

                        // Change items with A and S keys
                        if (IsKeyPressed(KEY_S)) {
                            (*currentIndex)++;
                            if (*currentIndex >= itemListSize) *currentIndex = 0;
                            menu.itemToUse = currentItems[*currentIndex]; // Update the selected item
                            menu.itemPointer = &currentItems[*currentIndex]; // Update the item pointer
                        }
                        if (IsKeyPressed(KEY_A)) {
                            (*currentIndex)--;
                            if (*currentIndex < 0) *currentIndex = itemListSize - 1;
                            menu.itemToUse = currentItems[*currentIndex]; // Update the selected item
                            menu.itemPointer = &currentItems[*currentIndex]; // Update the item pointer
                        }

                        // Left/Right to change the partyViewMode (switch teams)
                        if (IsKeyPressed(KEY_LEFT)) {
                            menu.partyViewMode = (menu.partyViewMode - 1 + PARTY_VIEW_COUNT) % PARTY_VIEW_COUNT;
                            (*selIndex) = 0; // Reset selection index when switching teams
                        }
                        if (IsKeyPressed(KEY_RIGHT)) {
                            menu.partyViewMode = (menu.partyViewMode + 1) % PARTY_VIEW_COUNT;
                            (*selIndex) = 0; // Reset selection index when switching teams
                        }

                        // Confirm selection
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z)) {
                            if (menu.itemToUse.effectType == EFFECT_OFFENSIVE) {
                                // Item is not usable in this context, play sound
                                PlaySoundByName(&soundManager, "colision", false);
                            } else {
                                Creature *target = &currentTeam[*selIndex];

                                // Check conditions based on item type
                                bool canUseItem = true;
                                if (menu.itemToUse.hpRestore > 0 && target->currentHP >= target->maxHP) {
                                    SetOverworldMessage("HP is already at maximum!");
                                    canUseItem = false;
                                } else if (menu.itemToUse.mpRestore > 0 && target->currentMP >= target->maxMP) {
                                    SetOverworldMessage("MP is already at maximum!");
                                    canUseItem = false;
                                } else if (menu.itemToUse.id == ITEM_PHOENIX_FEATHER && target->currentHP > 0) {
                                    SetOverworldMessage("Phoenix Feather can only be used on fainted creatures!");
                                    canUseItem = false;
                                }
                                if (menu.itemToUse.amount == 0) canUseItem = false;
                                if (target->name[0] == '\0') canUseItem = false;

                                if (canUseItem) {
                                    // Handle different item effects
                                    if (menu.itemToUse.hpRestore > 0 && target->currentHP != 0) {
                                        target->currentHP += menu.itemToUse.hpRestore;
                                        if (target->currentHP > target->maxHP) {
                                            target->currentHP = target->maxHP;
                                        }
                                        
                                    }

                                    if (menu.itemToUse.mpRestore > 0) {
                                        target->currentMP += menu.itemToUse.mpRestore;
                                        if (target->currentMP > target->maxMP) {
                                            target->currentMP = target->maxMP;
                                        }
                                    }

                                    if (menu.itemToUse.id == ITEM_PIECE_OF_CANDY) {
                                        target->gridLevel++;
                                        target->totalGridLevel++;
                                        PlaySoundByName(&soundManager, "levelUp", true);
                                        // GiveStatBonus(target);
                                        target->exp = 0;
                                    }

                                    if (menu.itemToUse.id == ITEM_PHOENIX_FEATHER && target->currentHP == 0) {
                                        // Resurrect the creature
                                        target->currentHP = RANDOMS(1, (int)target->maxHP / 4);
                                    }

                                    // Immediately apply healing to matching follower if in mainTeam
                                    if (currentTeam == mainTeam) {
                                        for (int i = 0; i < followerCount; i++) {
                                            if (strcmp(followers[i].creature->name, target->name) == 0) {
                                                printf("Directly healing follower %s to match mainTeam\n", 
                                                    followers[i].creature->name);
                                                followers[i].creature->currentHP = target->currentHP;
                                                followers[i].creature->currentMP = target->currentMP;
                                                break;
                                            }
                                        }
                                    }

                                    printf("%s was healed by %d HP and %d MP using %s!\n", target->name, menu.itemToUse.hpRestore, menu.itemToUse.mpRestore, menu.itemToUse.name);
                                    // menu.itemPointer->amount--;  // Decrement item amount
                                    RemoveItemFromInventory(menu.itemPointer->id,1);  // Decrement item amount
                                    
                                    if (menu.itemPointer->amount == 0) {
                                        menu.currentMenu = MENU_ITEMS;
                                        menu.itemSelectionInProgress = false;
                                    }
                                    
                                    menu.itemToUse = *menu.itemPointer; // Sync the displayed item with the actual item

                                    // Start the healing animation
                                    menu.healingAnimation = animations.potion; // Use the potion animation
                                    menu.healingAnimation.isAnimating = true;
                                    menu.healingAnimation.currentFrame = 0;
                                    menu.healingAnimation.frameCounter = 0;
                                    menu.healingAnimation.frameSpeed = 1;
                                    menu.healingAnimation.color = GetItemAnimationColor(&menu.itemToUse); // Set the color based on the item

                                    menu.itemSelectionInProgress = false;
                                    SyncTeamStats();
                                } else {
                                    PlaySoundByName(&soundManager, "colision", false); // Play sound for invalid item usage
                                }
                            }
                        }

                        // Cancel/back
                        if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                            menu.currentMenu = MENU_ITEMS;
                            menu.itemSelectionInProgress = false;
                        }

                        break;
                    }

                    // Update the MENU_ACHIEVEMENTS case in the logic section:
                    case MENU_ACHIEVEMENTS: 
                        MenuAchievementsUpdate(selIndex);
                        break;
                    
                    case MENU_ACHIEVEMENTS_DETAILS: 
                        MenuAchievementsDetailsUpdate();
                        break;
                    
                    case MENU_ACHIEVEMENTS_DETAILS_CONQUEROR: 
                        MenuAchievementsDetailsConqueror();
                        break;
                    
                    case MENU_GRID:
                        MenuGridUpdate();
                        break;

                    case MENU_GRID_ACTION:
                        MenuGridActionUpdate();
                        break;

                    case MENU_GRID_ITEM:
                        MenuGridItemUpdate();
                        break;

                    case MENU_GRID_NAVIGATION:
                        MenuGridNavigationUpdate();
                        break;

                    case MENU_GRID_CHECK_STATS:
                        MenuGridCheckStatsUpdate();
                        break;

                    case MENU_GRID_STONE_SELECT:
                        MenuGridStoneSelectUpdate();
                        break;

                    case MENU_SETTINGS:
                        MenuSettingsUpdate(&menu);
                        break;

                    case MENU_SAVE_SELECTION:
                        MenuSaveSelectionUpdate(&menu);
                        break;
                    case MENU_SAVE_CONFIRM:
                        MenuSaveConfirmUpdate(&menu);
                        break;

                    default: assert(false && "Unhandled enum value!"); break;
                    
                }

                break;
            }
              
            case GAME_STATE_FISHING_MINIGAME:
                GameStateFishingMinigameUpdate();
                break;

            case GAME_STATE_PICKAXE_MINIGAME: 
                GameStatePickaxeMinigameUpdate();
                break;

            case GAME_STATE_DIGGING_MINIGAME: 
                GameStateDiggingMinigameUpdate();
                break;
            
            case GAME_STATE_CUTTING_MINIGAME: 
                GameStateCuttingMinigameUpdate();
                break;

            case GAME_STATE_EATING: 
                GameStateEatingUpdate(&eatingState);
                break;

            case GAME_STATE_BATTLE_TRANSITION: {
                frameTracking.transitionFrameCounter++;
 
                // Update particles
                bool allParticlesInactive = true;
                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (particles[i].active) {
                        particles[i].velocity.y += 0.1f; // Gravity
                        particles[i].position.x += particles[i].velocity.x;
                        particles[i].position.y += particles[i].velocity.y;

                        if (particles[i].position.y > SCREEN_HEIGHT) {
                            particles[i].active = false;
                        } else {
                            allParticlesInactive = false;
                        }
                    }
                }

                // If all particles are inactive, transition to battle
                if (allParticlesInactive && frameTracking.transitionFrameCounter >= TRANSITION_DURATION) {
                    // UnloadImage(screenCapture);
                    StartBattleWithRandomEnemy(&battleState);
                    // StartBattle(&battleState, ENEMY_FOLDER);
                    currentGameState = GAME_STATE_BATTLE;
                }
                break;
            }

            case GAME_STATE_BATTLE: {
             
                // Handle battle logic
                HandleBattle(&battleState);
                
                // If battle ends, return to overworld
                if (!battleState.isBattleActive) {
                    if (battleState.messageCount == 0) {
                        if (bossBattleData.bossBattleAccepted) {
                            if (!AnyPlayerAlive(&battleState)) {
                                currentGameState = GAME_STATE_GAME_OVER;
                                bossBattleData.shinyMarkerActive = false;
                            }
                            else if (battleState.runSuccessful) {
                                bossBattleData.shinyMarkerActive = true;
                                bossBattleData.bossBattleAccepted = false;
                                currentGameState = GAME_STATE_OVERWORLD;
                            }
                            else {
                                // boss was defeated
                                bossBattleData.shinyMarkerActive = false; 
                                bossBattleData.bossBattleAccepted = false;
                                printf("boss defeated, load new map NOW!\n");
                                currentGameState = GAME_STATE_OVERWORLD;
                                targetMap = MAP_OVERWORLD_DIGGING;
                                // lets keep all buffs!
                            }
                        }
                        else {
                            if (!AnyPlayerAlive(&battleState)) {
                                currentGameState = GAME_STATE_GAME_OVER;
                            }
                            else {
                                currentGameState = GAME_STATE_OVERWORLD;
                                ResetBuffs();                                  
                            }
                        }

                        if (battleState.hasPendingCreatureToAdd) {
                            AddCreatureToParty(&pendingCreatureToAdd);
                            printf("\n\nname:%s,x:%f,y:%f", pendingCreatureToAdd.name, pendingCreatureToAdd.x, pendingCreatureToAdd.y);
                            AddFollower(&pendingCreatureToAdd);
                            battleState.hasPendingCreatureToAdd = false;
                        }

                        //  Now do cleanup
                        CleanupBattle(&battleState);
                        memset(&battleState, 0 , sizeof(BattleState));
                    }
                }

                // make sure that the boss battle is only triggered once naturally
                if (mainTeam[0].completeFriendship && !bossBattleData.bossBattleTriggered) {
                    bossBattleData.bossBattleTriggered = true;
                }

                UpdateBattleParticipants(&battleState);

                break;
            }

            case GAME_STATE_BEFRIEND_SWAP: {
                // Handle navigation between teams
                if (IsKeyPressed(KEY_LEFT)) {
                    swapMenu.selectedTeam = (swapMenu.selectedTeam - 1 + PARTY_VIEW_COUNT) % PARTY_VIEW_COUNT;
                    swapMenu.selectedIndex = 0;
                }
                if (IsKeyPressed(KEY_RIGHT)) {
                    swapMenu.selectedTeam = (swapMenu.selectedTeam + 1) % PARTY_VIEW_COUNT;
                    swapMenu.selectedIndex = 0;
                }
                if (IsKeyPressed(KEY_DOWN)) {
                    swapMenu.selectedIndex++;
                    if (swapMenu.selectedIndex >= PARTY_VIEW_COUNT) {
                        swapMenu.selectedIndex = 0;
                    }
                }
                if (IsKeyPressed(KEY_UP)) {
                    swapMenu.selectedIndex--;
                    if (swapMenu.selectedIndex < 0) {
                        swapMenu.selectedIndex = PARTY_VIEW_COUNT - 1;
                    }
                }
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z)) {
                    // Prompt for confirmation
                    SetOverworldMessage("Are you sure? (Enter to confirm, Backspace to cancel)");
                    currentGameState = GAME_STATE_CONFIRM_SWAP;
                }
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                    // Release the new creature
                    SetOverworldMessage("You released the creature.");
                    currentGameState = GAME_STATE_OVERWORLD;
                }
                break;
            }

            case GAME_STATE_CONFIRM_SWAP: {
                // Handle selection between Yes/No
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                    swapMenu.confirmSelection = !swapMenu.confirmSelection;
                }
                
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_Z)) {
                    Creature *selectedTeam;
                    if (swapMenu.confirmSelection == 0) { // Yes
                        // Perform the swap (your existing swap logic here)
                        // ... (keep the position preservation logic for main team slot 0)
                        // Perform the swap
                        // Creature *selectedTeam;
                        // int teamSize;
                        switch (swapMenu.selectedTeam) {
                            case PARTY_MAIN_TEAM: 
                                selectedTeam = mainTeam; 
                                // teamSize = MAX_PARTY_SIZE; 
                                break;
                            case PARTY_BACKUP_TEAM: 
                                selectedTeam = backupTeam; 
                                // teamSize = MAX_BACKUP_SIZE; 
                                break;
                            case PARTY_BANK: 
                                selectedTeam = bank; 
                                // teamSize = MAX_BANK_SIZE; 
                                break;
                            default: assert(false && "Unhandled enum value!"); break;
                        }

                        // Preserve x and y if swapping into the first slot of main team
                        if (swapMenu.selectedTeam == PARTY_MAIN_TEAM && swapMenu.selectedIndex == 0) {
                            float originalX = selectedTeam[0].x;
                            float originalY = selectedTeam[0].y;
                            selectedTeam[0] = swapMenu.newCreature;
                            selectedTeam[0].x = originalX;
                            selectedTeam[0].y = originalY;
                        } else {
                            selectedTeam[swapMenu.selectedIndex] = swapMenu.newCreature;
                        }

                        SetOverworldMessage("Creature swapped!");
                        currentGameState = GAME_STATE_OVERWORLD;
                    } else { // No
                        SetOverworldMessage("Swap canceled.");
                        currentGameState = GAME_STATE_BEFRIEND_SWAP;
                    }
                }
                
                if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_X)) {
                    SetOverworldMessage("Swap canceled.");
                    currentGameState = GAME_STATE_BEFRIEND_SWAP;
                }
                break;
            }

            case GAME_STATE_BOSS_BATTLE_TRANSITION: {
                if (bossBattleData.bossBattleAccepted) {
                    currentGameState = GAME_STATE_BATTLE_TRANSITION;
                    battleState.terrain = TERRAIN_BOSS;
                    ScreenBreakingAnimation();
                }
                break;
            }

            case GAME_STATE_GAME_OVER: {
                if (IsKeyPressed(KEY_Z)) {
                    // Reset the game state
                    ResetGame(&battleState);
                    currentGameState = GAME_STATE_OVERWORLD;
                    // fishingMinigameState.fishingEventActive = false;
                    // fishingMinigameState.isFishing = false;
                    mainTeam[0].anims.movementState = IDLE;
                }
                break;
            }

            case GAME_STATE_REALTIME_BATTLE: {
                // --- Check for switch FROM RealTimeBattle ---
                if (toolWheel.tools[toolWheel.selectedToolIndex].type != TOOL_CLAW) {
                    currentGameState = GAME_STATE_OVERWORLD;
                    realTimeBattleState.isActive = false;
                    printf("Switching back to Overworld State (Claw unequipped)\n");
                    break;
                }
                
                // Check for command menu activation
                if (APressed && !realTimeBattleState.waitingForKeyRelease) {
                    // Fully reset and initialize command menu state
                    memset(&commandMenuState, 0, sizeof(CommandMenuState));
                    
                    // Set necessary fields after the memset
                    commandMenuState.previousState = currentGameState;
                    commandMenuState.justEnteredMenu = true;
                    commandMenuState.selectedIndex = 0;
                    commandMenuState.targetSelectionMode = false;
                    commandMenuState.inAttackSubmenu = false;
                    commandMenuState.waitingForKeyRelease = true;
                    commandMenuState.actingParticipant = &battleState.participants[0];
                    commandMenuState.wasXPressed = false;
                    commandMenuState.xHoldTime = 0.0f;
                    commandMenuState.xLongPressExecuted = false;
                    
                    // Transition to the command menu
                    currentGameState = GAME_STATE_COMMAND_MENU;
                    realTimeBattleState.waitingForKeyRelease = true;
                    printf("DEBUG: Fully initialized command menu state\n");
                    break; // Exit the RTBS update immediately after transition
                }
                
                // Run RTBS update logic
                UpdateRealTimeBattle(&realTimeBattleState);
                break;
            }

            case GAME_STATE_COMMAND_MENU: {
                // UpdateRealTimeBattleSlowmo(&realTimeBattleState); // TODO
                UpdateCommandMenu(&commandMenuState); // Call the new update function
                break;
            }

            default: assert(false && "Unhandled enum value!"); break;

        }


// DRAWING
        BeginDrawing();
        ClearBackground(RAYWHITE);
        switch (currentGameState) {
            case GAME_STATE_TITLE_SCREEN: 
                DrawTitleScreen(&mainMenuState);
                break;
            
            case GAME_STATE_MAIN_MENU: 
                DrawMainMenu(&mainMenuState);
                break;
        
            case GAME_STATE_LOAD_SELECTION:
                DrawLoadSelection();
                break;
        
            case GAME_STATE_SETTINGS: 
                DrawSettingsMenu(&mainMenuState);
                break;

            case GAME_STATE_QUIT_CONFIRM:
                DrawQuitConfirm(BPressed, keysPressed, &mainMenuState);
                break;
            
            case GAME_STATE_OVERWORLD: {
                // First draw the base map
                DrawOverworld(BPressed, keysPressed, &bossBattleData, camera);
                
                // Then overlay map-specific elements
                switch (currentMap) {
                    case MAP_OVERWORLD:
                        // Any overworld-specific UI elements would go here
                        break;
                    case MAP_OVERWORLD_DIGGING:
                        DrawPatternCounter();
                        break;
                    case MAP_OVERWORLD_HOLE:
                        break;
                    case MAP_INSIDE_HOLE:
                        break;
                    default: assert(false && "Unhandled enum value!"); break;
                    // Easy to add more map types in the future
                }
                break;
            }

            case GAME_STATE_MENU: {
                MenuState cm = menu.currentMenu;
                int *selIndex = &menu.selectedIndex[cm];

                switch (cm) {
                    case MENU_MAIN: {
                        const char *mainMenuOptions[] = { "Party", "Items", "FriendshipGrid", "Achievements", "Save", "Settings", "Quit"};
                        int mainMenuOptionCount = MENU_MAIN_COUNT;
                        for (int i = 0; i < mainMenuOptionCount; i++) {
                            Color color = (i == *selIndex) ? RED : BLACK;
                            DrawText(mainMenuOptions[i], 100, 100 + i * 30, 20, color);
                        }
                        // Draw description
                            // Add description for Save option
                        const char *descriptions[] = {
                            "View and manage your party members.",
                            "View your items.",
                            "Stats change.",
                            "View your achievements.",
                            "Save your game progress.",
                            "Adjust game settings.",
                            "Return to title screen."
                        };
                        DrawText(descriptions[*selIndex], 10, SCREEN_HEIGHT - 50, 20, BLACK);
                        break;
                    }

                    case MENU_PARTY: {
                        // Draw party members
                        const char *teamNames[] = { "Main Team", "Backup Team", "Bank" };
                        DrawText(teamNames[menu.partyViewMode], 50, 50, 20, BLUE);
                        
                        // Draw navigation hints
                        DrawText("A: Previous Team", 50, 80, 15, GRAY);
                        DrawText("S: Next Team", 200, 80, 15, GRAY);
                        
                        Creature *currentTeam;
                        int teamSize = 0;
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM: { currentTeam = mainTeam; teamSize = MAX_PARTY_SIZE; break; }
                            case PARTY_BACKUP_TEAM: { currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE; break; }
                            case PARTY_BANK: { currentTeam = bank; teamSize = MAX_BANK_SIZE; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }
                        
                        if (menu.partyViewMode == PARTY_BANK) {
                            // Draw bank in a 3x3 grid with simplified info
                            const int columns = 3;
                            const int rows = 3;
                            const int cellWidth = 200;
                            const int cellHeight = 120;
                            const int startX = 100;
                            const int startY = 120;
                            
                            // Calculate current page
                            int itemsPerPage = columns * rows;
                            int currentPage = (*selIndex) / itemsPerPage;
                            int pageStartIndex = currentPage * itemsPerPage;
                            
                            // Draw page indicator
                            char pageInfo[50];
                            sprintf(pageInfo, "Page %d/%d", currentPage + 1, (teamSize + itemsPerPage - 1) / itemsPerPage);
                            DrawText(pageInfo, SCREEN_WIDTH - 150, 50, 20, BLACK);
                            
                            // Draw selection number indicator
                            char selInfo[50];
                            sprintf(selInfo, "Creature %d/%d", *selIndex + 1, teamSize);
                            DrawText(selInfo, SCREEN_WIDTH - 150, 80, 20, BLACK);
                            
                            // Draw grid
                            for (int row = 0; row < rows; row++) {
                                for (int col = 0; col < columns; col++) {
                                    int index = pageStartIndex + row * columns + col;
                                    if (index >= teamSize) continue;
                                    
                                    int x = startX + col * cellWidth;
                                    int y = startY + row * cellHeight;
                                    
                                    Color borderColor = (index == *selIndex) ? RED : LIGHTGRAY;
                                    DrawRectangleLines(x - 5, y - 5, cellWidth - 10, cellHeight - 10, borderColor);
                                    
                                    if (currentTeam[index].name[0] != '\0') {
                                        // Simplified creature info
                                        DrawText(currentTeam[index].name, x, y, 18, BLACK);
                                        DrawText(TextFormat("GLvl: %d", currentTeam[index].gridLevel), x, y + 20, 16, DARKGRAY);

                                        int healthPercent = (currentTeam[index].currentHP * 100) /currentTeam[index].maxHP;
                                        int spriteIndex = 0; // Default to full health sprite
                                        if (healthPercent == 100) spriteIndex = PERFECT;
                                        else if (healthPercent >= 75) spriteIndex = HEALTHY;
                                        else if (healthPercent >= 50) spriteIndex = OK;
                                        else if (healthPercent >= 25) spriteIndex = BAD;
                                        else if (healthPercent > 0 || healthPercent <= 0) spriteIndex = DEADLY;
                                        
                                        // Mini health bar
                                        int barWidth = 150;
                                        int barHeight = 12;
                                        DrawStatBar(x, y + 40, barWidth, barHeight, currentTeam[index].currentHP, currentTeam[index].maxHP, DARKGRAY, RED, true);
                                        
                                        // Sprite with smaller scale
                                        float offset = (index == *selIndex) ? sinf(GetTime() * 20.0f) * 4.0f : 0.0f;
                                        for (int k = 0; k < NUM_TERRAINS; k++) {
                                            DrawTextureEx(currentTeam[index].sprites[k][spriteIndex], (Vector2){x + 100, y + 60 + offset}, 0.0f, 1.5f, WHITE);
                                        }
                                    } else {
                                        DrawText("Empty Slot", x, y + 40, 18, GRAY);
                                    }
                                }
                            }
                        } else {
                            // Original vertical display for main/backup teams
                            for (int i = 0; i < teamSize; i++) {
                                char buf[50];
                                int barWidth = 200;
                                int barHeight = 20;
                                Color color = (i == *selIndex) ? RED : BLACK;
                                
                                if (currentTeam[i].name[0] != '\0') {
                                    sprintf(buf, "%s", currentTeam[i].name);
                                    // int nameTextWidth = MeasureText(buf, 20);
                                    DrawText(TextFormat("GLvl: %d", currentTeam[i].gridLevel), 230, 75 + i * 120, 20, color);
                            
                                    DrawStatBar(230, 120 + i * 120 - 15, barWidth, barHeight, currentTeam[i].currentHP, currentTeam[i].maxHP, DARKGRAY, RED, true);
                                    DrawStatBar(230, 120 + i * 120 + 10, barWidth, barHeight, currentTeam[i].currentMP, currentTeam[i].maxMP, DARKGRAY, BLUE, true);
                                    DrawStatBar(230, 120 + i * 120 + 35, barWidth, barHeight, currentTeam[i].exp, GetExpForNextGridLevel(currentTeam[i].totalGridLevel), DARKGRAY, DARKPURPLE, true);
                                } else {
                                    sprintf(buf, "Empty Slot");
                                }
                            
                                DrawText(buf, 100, 75 + i * 120, 20, color);
                            
                                float offset = (i == *selIndex) ? sinf(GetTime() * 20.0f) * 6.0f : 0.0f;
                            
                                if (currentTeam[i].name[0] != '\0') {
                                    for (int k = 0; k < NUM_TERRAINS; k++) {
                                        DrawTextureEx(currentTeam[i].sprites[k][0], 
                                                    (Vector2){100, 100 + i * 120 + offset}, 
                                                    0.0f, 2.0f, WHITE);
                                    }      
                                }                
                            }
                        }
                        break;
                    }

                    case MENU_PARTY_DETAILS: {
                        // int teamSize = 0;
                        Creature *teamPtr = NULL;
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM: {
                                teamPtr = mainTeam;   
                                // teamSize = MAX_PARTY_SIZE;
                                break;
                            }
                            case PARTY_BACKUP_TEAM: {
                                teamPtr = backupTeam; 
                                // teamSize = MAX_BACKUP_SIZE;
                                break;
                            }
                            case PARTY_BANK: {
                                teamPtr = bank;       
                                // teamSize = MAX_BANK_SIZE;
                                break;
                            }
                            default: assert(false && "Unhandled enum value!"); break;
                        }

                        Creature *selectedCreature = &teamPtr[menu.subSelectedIndex[MENU_PARTY_DETAILS]];
                        
                        if (selectedCreature && selectedCreature->name[0] != '\0') {
                            // Name
                            const char* nameText = TextFormat("Name: %s", selectedCreature->name);
                            DrawText(nameText, 100, 100, 20, BLACK);

                            // Bars
                            DrawStatBar(100, 130, 120, 16, selectedCreature->currentHP, selectedCreature->maxHP, DARKGRAY, RED, true);
                            DrawText("HP", 70, 130, 20, RED);
                            
                            DrawStatBar(100, 150, 120, 16, selectedCreature->currentMP, selectedCreature->maxMP, DARKGRAY, BLUE, true);
                            DrawText("MP", 70, 150, 20, BLUE);
                            
                            // Level, EXP
                            DrawText(TextFormat("GLvl: %d", selectedCreature->gridLevel), 100, 180, 20, BLACK);
                            DrawText(TextFormat("Exp:   %d / %d", selectedCreature->exp, GetExpForNextGridLevel(selectedCreature->totalGridLevel)), 100, 200, 20, BLACK);

                            // Attack, Defense, Speed, etc.
                            DrawText(TextFormat("ATK: %d", selectedCreature->attack),        100, 230, 20, BLACK);
                            DrawText(TextFormat("DEF: %d", selectedCreature->defense),       100, 250, 20, BLACK);
                            DrawText(TextFormat("MAG: %d", selectedCreature->magic),         100, 270, 20, BLACK);
                            DrawText(TextFormat("MDF: %d", selectedCreature->magicdefense),  100, 290, 20, BLACK);
                            DrawText(TextFormat("SPD: %d", selectedCreature->speed),         100, 310, 20, BLACK);
                            DrawText(TextFormat("ACC: %d", selectedCreature->accuracy),      100, 330, 20, BLACK);
                            DrawText(TextFormat("LCK: %d", selectedCreature->luck),          100, 350, 20, BLACK);
                            DrawText(TextFormat("EVA: %d", selectedCreature->evasion),       100, 370, 20, BLACK);
                            // etc.
                            // Draw up to 300px wide, 4px line spacing, etc.
                            DrawTextBoxed(selectedCreature->description, 300, 100, 20, 300, 4, BLACK);
                            // DrawText(TextFormat("Description: %s", selectedCreature->description), 350, 100, 20, BLACK);

                            // Show element type
                            const char* elementNames[] = { "Fire", "Ice", "Thunder", "Water", "Rock", "Normal" };
                            DrawText(TextFormat("Element: %s", elementNames[selectedCreature->elementType]), 
                                    100, 400, 20, BLACK);
                        } else {
                            DrawText("No creature in this slot.", 100, 100, 20, RED);
                        }

                        // Optionally draw a hint
                        DrawText("Press X to return", 10, SCREEN_HEIGHT - 30, 20, GRAY);

                        break;
                    }

                    case MENU_PARTY_SWAP: {
                        // Draw party members
                        const char *teamNames[] = { "Main Team", "Backup Team", "Bank" };
                        DrawText(teamNames[menu.partyViewMode], 50, 50, 20, BLUE);
                        
                        // Draw navigation hints
                        DrawText("A: Previous Team", 50, 80, 15, GRAY);
                        DrawText("S: Next Team", 200, 80, 15, GRAY);
                        DrawText("Z/C: Swap", 350, 80, 15, GRAY);
                        DrawText("X: Cancel", 450, 80, 15, GRAY);
                        
                        // Show which creature is being swapped
                        Creature *sourceTeam = NULL;
                        switch (menu.swapSourceParty) {
                            case PARTY_MAIN_TEAM: sourceTeam = mainTeam; break;
                            case PARTY_BACKUP_TEAM: sourceTeam = backupTeam; break;
                            case PARTY_BANK: sourceTeam = bank; break;
                        }
                        
                        char swapInfo[100];
                        sprintf(swapInfo, "Swapping: %s", sourceTeam[menu.swapSourceIndex].name);
                        DrawText(swapInfo, SCREEN_WIDTH - 350, 50, 20, YELLOW);
                        
                        Creature *currentTeam = NULL;
                        int teamSize = 0;
                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM: { currentTeam = mainTeam; teamSize = MAX_PARTY_SIZE; break; }
                            case PARTY_BACKUP_TEAM: { currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE; break; }
                            case PARTY_BANK: { currentTeam = bank; teamSize = MAX_BANK_SIZE; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }
                        
                        if (menu.partyViewMode == PARTY_BANK) {
                            // Draw bank in a 3x3 grid with simplified info
                            const int columns = 3;
                            const int rows = 3;
                            const int cellWidth = 200;
                            const int cellHeight = 120;
                            const int startX = 100;
                            const int startY = 120;
                            
                            // Calculate current page
                            int itemsPerPage = columns * rows;
                            int currentPage = (*selIndex) / itemsPerPage;
                            int pageStartIndex = currentPage * itemsPerPage;
                            
                            // Draw page indicator
                            char pageInfo[50];
                            sprintf(pageInfo, "Page %d/%d", currentPage + 1, (teamSize + itemsPerPage - 1) / itemsPerPage);
                            DrawText(pageInfo, SCREEN_WIDTH - 150, 80, 20, BLACK);
                            
                            // Draw selection number indicator
                            char selInfo[50];
                            sprintf(selInfo, "Creature %d/%d", *selIndex + 1, teamSize);
                            DrawText(selInfo, SCREEN_WIDTH - 150, 110, 20, BLACK);
                            
                            // Draw grid
                            for (int row = 0; row < rows; row++) {
                                for (int col = 0; col < columns; col++) {
                                    int index = pageStartIndex + row * columns + col;
                                    if (index >= teamSize) continue;
                                    
                                    int x = startX + col * cellWidth;
                                    int y = startY + row * cellHeight;
                                    
                                    // Determine if this is the source of the swap
                                    bool isSwapSourceThisSlot = (menu.swapSourceParty == menu.partyViewMode && 
                                                            menu.swapSourceIndex == index);
                                    
                                    // Decide border color
                                    Color borderColor;
                                    if (isSwapSourceThisSlot) {
                                        borderColor = YELLOW;
                                    } else if (index == *selIndex) {
                                        borderColor = RED;
                                    } else {
                                        borderColor = LIGHTGRAY;
                                    }
                                    
                                    DrawRectangleLines(x - 5, y - 5, cellWidth - 10, cellHeight - 10, borderColor);
                                    
                                    if (currentTeam[index].name[0] != '\0') {
                                        // Simplified creature info
                                        DrawText(currentTeam[index].name, x, y, 18, BLACK);
                                        DrawText(TextFormat("GLvl: %d", currentTeam[index].gridLevel), x, y + 20, 16, DARKGRAY);
                                        
                                        // Mini health bar
                                        int barWidth = 150;
                                        int barHeight = 12;
                                        DrawStatBar(x, y + 40, barWidth, barHeight, 
                                                currentTeam[index].currentHP, currentTeam[index].maxHP, 
                                                DARKGRAY, RED, true);
                                        
                                        // Decide sprite offset
                                        float offset = 0.0f;
                                        if (isSwapSourceThisSlot) {
                                            offset = -4.0f; // Lock bounce at top
                                        } else if (index == *selIndex) {
                                            offset = sinf(GetTime() * 20.0f) * 4.0f;
                                        }
                                        
                                        // Draw shadow under sprite
                                        if (isSwapSourceThisSlot || index == *selIndex) {
                                            DrawEllipseGradient(x + 110, y + 90, 30, 5, BLACK, RAYWHITE);
                                        }
                                        
                                        // Draw sprite
                                        for (int k = 0; k < NUM_TERRAINS; k++) {
                                            DrawTextureEx(currentTeam[index].sprites[k][0], 
                                                        (Vector2){x + 100, y + 60 + offset}, 
                                                        0.0f, 1.5f, WHITE);
                                        }
                                    } else {
                                        DrawText("Empty Slot", x, y + 40, 18, GRAY);
                                    }
                                }
                            }
                        } else {
                            // Original vertical display for main/backup teams
                            for (int i = 0; i < teamSize; i++) {
                                char buf[50];
                                int barWidth = 200;
                                int barHeight = 20;
                                
                                // Determine if this is the source of the swap
                                bool isSwapSourceThisSlot = (menu.swapSourceParty == menu.partyViewMode && menu.swapSourceIndex == i);
                                
                                // Decide text color
                                Color color;
                                if (isSwapSourceThisSlot) {
                                    color = YELLOW;
                                } else if (i == *selIndex) {
                                    color = RED;
                                } else {
                                    color = BLACK;
                                }
                                
                                if (currentTeam[i].name[0] != '\0') {
                                    sprintf(buf, "%s", currentTeam[i].name);
                                    // int nameTextWidth = MeasureText(buf, 20);
                                    DrawText(TextFormat("GLvl: %d", currentTeam[i].gridLevel), 230, 75 + i * 120, 20, color);
                            
                                    DrawStatBar(230, 120 + i * 120 - 15, barWidth, barHeight, currentTeam[i].currentHP, currentTeam[i].maxHP,DARKGRAY, RED, true);
                                    DrawStatBar(230, 120 + i * 120 + 10, barWidth, barHeight, currentTeam[i].currentMP, currentTeam[i].maxMP,DARKGRAY, BLUE, true);
                                    DrawStatBar(230, 120 + i * 120 + 35, barWidth, barHeight, currentTeam[i].exp, GetExpForNextGridLevel(currentTeam[i].totalGridLevel),DARKGRAY, DARKPURPLE, true);
                                } else {
                                    sprintf(buf, "Empty Slot");
                                }
                            
                                DrawText(buf, 100, 75 + i * 120, 20, color);
                            
                                // Decide sprite offset
                                float offset = 0.0f;
                                if (isSwapSourceThisSlot) {
                                    offset = -6.0f; // Lock bounce at top
                                    DrawEllipseGradient(139, 100 + i * 120 + 80, 40, 5, BLACK, RAYWHITE);
                                } else if (i == *selIndex) {
                                    offset = sinf(GetTime() * 20.0f) * 6.0f;
                                    DrawEllipseGradient(139, 100 + i * 120 + 80, 40, 5, BLACK, RAYWHITE);
                                }
                            
                                // Draw sprite
                                if (currentTeam[i].name[0] != '\0') {
                                    for (int k = 0; k < NUM_TERRAINS; k++) {
                                        DrawTextureEx(currentTeam[i].sprites[k][0], (Vector2){100, 100 + i * 120 + offset}, 0.0f, 2.0f, WHITE);
                                    }      
                                }                
                            }
                        }
                        break;
                    }

                    // In the DRAWING part:
                    case MENU_ITEMS: {
                        const char *itemCategories[] = { "Usable Items", "Key Items" };
                        DrawText(itemCategories[menu.itemViewMode], 50, 50, 20, BLUE);
                        
                        Item *currentItems = NULL;
                        int itemListSize = 0;
                        int *currentIndex = NULL;
                        
                        switch (menu.itemViewMode) {
                            case ITEM_USABLE: { currentItems = playerInventory; itemListSize = itemCount; currentIndex = &menu.usableItemIndex; break; }
                            case ITEM_KEY: { currentItems = keyItemList; itemListSize = keyItemCount; currentIndex = &menu.keyItemIndex; break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }
                        
                        // Two-column layout configuration
                        const int columns = 2;
                        const int rows = 10; // 10 rows visible at once
                        const int itemsPerPage = rows * columns;
                        const int columnWidth = 300;
                        const int itemHeight = 30;
                        const int startY = 100;
                        const int startX = 100;
                        
                        // Calculate current page (for drawing purposes)
                        int currentPage = (*currentIndex) / itemsPerPage;
                        int pageStartIndex = currentPage * itemsPerPage;
                        
                        // Draw items in the grid layout (row by row)
                        for (int row = 0; row < rows; row++) {
                            for (int col = 0; col < columns; col++) {
                                int itemIndex = pageStartIndex + row * columns + col;
                                
                                // Stop if we've drawn all items
                                if (itemIndex >= itemListSize) break;
                                
                                // Skip items that aren't unlocked
                                if (!currentItems[itemIndex].isUnlocked) continue;
                                
                                int itemX = startX + col * columnWidth;
                                int itemY = startY + row * itemHeight;
                                
                                char buf[100];
                                if (currentItems[itemIndex].amount > 0 || currentItems[itemIndex].isKeyItem) {
                                    if (menu.itemViewMode == ITEM_USABLE) {
                                        sprintf(buf, "%s (x%d)", currentItems[itemIndex].name, currentItems[itemIndex].amount);
                                    } else {
                                        sprintf(buf, "%s", currentItems[itemIndex].name);
                                    }
                                } else {
                                    sprintf(buf, "%s (Empty)", currentItems[itemIndex].name);
                                }
                                
                                Color color = (itemIndex == *currentIndex) ? RED : BLACK;
                                DrawText(buf, itemX, itemY, 20, color);
                                
                                if (menu.swapInProgress && itemIndex == menu.swapSourceIndex) {
                                    DrawText(" (Swapping)", itemX + 200, itemY, 20, YELLOW);
                                }
                            }
                        }
                        
                        // Draw category indicators (to show A/S keybinding)
                        DrawText("A: Usable Items", SCREEN_WIDTH - 200, 30, 15, menu.itemViewMode == ITEM_USABLE ? GREEN : GRAY);
                        DrawText("S: Key Items", SCREEN_WIDTH - 200, 50, 15, menu.itemViewMode == ITEM_KEY ? GREEN : GRAY);
                        
                        // Page indicators if we have more than one page of items
                        int totalPages = (itemListSize + itemsPerPage - 1) / itemsPerPage;
                        if (totalPages > 1) {
                            char pageInfo[50];
                            sprintf(pageInfo, "Page %d/%d", currentPage + 1, totalPages);
                            DrawText(pageInfo, SCREEN_WIDTH - 150, 80, 20, BLACK);
                        }
                        
                        // Draw instructions or item details at the bottom
                        if (itemListSize > 0) {
                            DrawText(currentItems[*currentIndex].description, 10, SCREEN_HEIGHT - 50, 20, BLACK);
                        } else {
                            DrawText("No items in this category.", 10, SCREEN_HEIGHT - 50, 20, GRAY);
                        }
                        
                        break;
                    }

                    case MENU_ITEM_PARTY_SELECTION: {
                        // Draw party members
                        const char *teamNames[] = { "Main Team", "Backup Team", "Bank" };
                        DrawText(TextFormat("Team: %s", teamNames[menu.partyViewMode]), 50, 50, 20, BLUE);
                        
                        // Draw the item name and amount
                        char itemText[128];
                        sprintf(itemText, "Using: %s (x%d)", menu.itemToUse.name, menu.itemToUse.amount);
                        int itemTextWidth = MeasureText(itemText, 20); // Measure the width of the item text
                        int itemTextX = (SCREEN_WIDTH - itemTextWidth) / 2; // Center the item text horizontally
                            // Check if the item is usable in the current context
                        bool isUsable = (menu.itemToUse.effectType != EFFECT_OFFENSIVE);
                        Color itemColor = isUsable ? BLUE : GRAY; // Grey out unusable items
                        
                        DrawText(itemText, itemTextX, 20, 20, itemColor);

                        // Calculate arrow positions based on the item text width
                        int arrowOffset = 10; // Space between the text and the arrows
                        int leftArrowX = itemTextX - arrowOffset - MeasureText("<", 20); // Left arrow position
                        int rightArrowX = itemTextX + itemTextWidth + arrowOffset; // Right arrow position

                        // Draw left and right arrows
                        DrawText("<", leftArrowX, 20, 20, GRAY); // Left arrow
                        DrawText(">", rightArrowX, 20, 20, GRAY); // Right arrow

                        Creature *currentTeam = NULL;
                        int teamSize = 0;

                        switch (menu.partyViewMode) {
                            case PARTY_MAIN_TEAM:   { currentTeam = mainTeam;   teamSize = MAX_PARTY_SIZE;   break; }
                            case PARTY_BACKUP_TEAM: { currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE;  break; }
                            case PARTY_BANK:        { currentTeam = bank;       teamSize = MAX_BANK_SIZE;    break; }
                            default: assert(false && "Unhandled enum value!"); break;
                        }


                        // float offset = 0;
                        for (int i = 0; i < teamSize; i++) {
                            char buf[50];
                            int barWidth = 200;
                            int barHeight = 20;

                            Color color = (i == *selIndex) ? RED : BLACK;

                            if (currentTeam[i].name[0] != '\0') {
                                sprintf(buf, "%s", currentTeam[i].name);
                                // Level
                                DrawText(TextFormat("GLvl: %d", currentTeam[i].gridLevel), 230, 75 + i * 120, 20, color);
                            

                                DrawStatBar(230, 120 + i * 120 - 15, barWidth, barHeight, currentTeam[i].currentHP,currentTeam[i].maxHP,DARKGRAY, RED, true);
                                DrawStatBar(230, 120 + i * 120 + 10, barWidth, barHeight, currentTeam[i].currentMP,currentTeam[i].maxMP,DARKGRAY, BLUE, true);
                                DrawStatBar(230, 120 + i * 120 + 35, barWidth, barHeight, currentTeam[i].exp,GetExpForNextGridLevel(currentTeam[i].totalGridLevel),DARKGRAY, DARKPURPLE, true);
                            } 
                            else {
                                sprintf(buf, "Empty Slot");
                            }
                            // Color color = (i == *selIndex) ? RED : BLACK;

                            DrawText(buf, 100, 75 + i * 120, 20, color);
                            
                            float offset = (i == *selIndex) ? sinf(GetTime() * 20.0f) * 6.0f : 0.0f;
                            
                            if (currentTeam[i].name[0] != '\0') {
                                for (int k = 0; k < NUM_TERRAINS; k++) {
                                    DrawTextureEx(currentTeam[i].sprites[k][0], (Vector2){100, 100 + i * 120 + offset}, 0.0f, 2.0f, WHITE);
                                }       
                            }                 
                        }

                        // Draw the healing animation
                        if (menu.healingAnimation.isAnimating) {
                            // Creature *target = &currentTeam[*selIndex];
                            int targetScreenX = 100; // X position of the selected party member
                            int targetScreenY = 100 + (*selIndex) * 120; // Y position of the selected party member

                            Texture2D currentFrame = menu.healingAnimation.frames[menu.healingAnimation.currentFrame];
                            DrawTextureEx(currentFrame, (Vector2){targetScreenX, targetScreenY}, 0.0f, 2.0f, menu.healingAnimation.color);
                        }

                        // Draw instructions
                        DrawText(TextFormat("%s", menu.itemToUse.description), 10, SCREEN_HEIGHT - 50, 20, GRAY);
                        // DrawText("Press Z to confirm, X to cancel", 10, SCREEN_HEIGHT - 50, 20, GRAY);
                        // DrawText("Press Left/Right to switch teams", 10, SCREEN_HEIGHT - 80, 20, GRAY);
                        break;
                    }


                    case MENU_ACHIEVEMENTS: 
                        MenuAchievementsDraw(selIndex);
                        break;

                    case MENU_ACHIEVEMENTS_DETAILS: 
                        MenuAchievementsDetailsDraw();
                        break;
                    
                    case MENU_ACHIEVEMENTS_DETAILS_CONQUEROR: 
                        MenuAchievementsDetailsConquerorDraw();
                        break;

                    case MENU_GRID:
                        MenuGridDraw();
                        break;

                    case MENU_GRID_ACTION:
                        MenuGridActionDraw();
                        break;

                    case MENU_GRID_ITEM:
                        MenuGridItemDraw();
                        break;

                    case MENU_GRID_NAVIGATION:
                        MenuGridNavigationDraw();
                        break;

                    case MENU_GRID_CHECK_STATS:
                        MenuGridCheckStatsDraw();
                        break;

                    case MENU_GRID_STONE_SELECT:
                        MenuGridStoneSelectDraw();
                        break;

                    case MENU_SETTINGS:
                        MenuSettingsDraw(BPressed, keysPressed,&menu);
                        break;

                    case MENU_SAVE_SELECTION:
                        MenuSaveSelectionDraw(&menu);
                        break;
                    case MENU_SAVE_CONFIRM:
                        MenuSaveConfirmDraw(&menu);
                        break;

                    default: assert(false && "Unhandled enum value!"); break;

                }
                break;       
            }
            
            case GAME_STATE_FISHING_MINIGAME: 
                GameStateFishingMinigameDraw();
                break;

            case GAME_STATE_PICKAXE_MINIGAME: 
                GameStatePickaxeMinigameDraw();
                break;
            
            case GAME_STATE_DIGGING_MINIGAME: 
                GameStateDiggingMinigameDraw();
                break;

            case GAME_STATE_CUTTING_MINIGAME:
                GameStateCuttingMinigameDraw();
                break;

            case GAME_STATE_EATING: {
                GameStateEatingDraw(&eatingState);
                break;
            }

            case GAME_STATE_BATTLE_TRANSITION: {
                shakeOffset.x = (rand() % (int)(shakeMagnitude * 2)) - shakeMagnitude;
                shakeOffset.y = (rand() % (int)(shakeMagnitude * 2)) - shakeMagnitude;
           
                float alpha = (float)frameTracking.transitionFrameCounter / TRANSITION_DURATION;
                if (alpha > 1.0f) alpha = 1.0f;

                BeginScissorMode( shakeOffset.x, shakeOffset.y, SCREEN_WIDTH, SCREEN_HEIGHT );
                int particleSize = PARTICLE_SIZE;
                // Draw particles
                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (particles[i].active) {
                        DrawRectangleV(particles[i].position, (Vector2){particleSize, particleSize}, particles[i].color);
                    }
                }
                EndScissorMode();
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, alpha));

                break;
            }

            case GAME_STATE_BATTLE: {    
                // UpdateBattleParticipants(&battleState);
                DrawBattleInterface(&battleState);
                break;
            }

            case GAME_STATE_BEFRIEND_SWAP: {
                // Render the swapping menu
                const char *teamNames[] = { "Main Team", "Backup Team", "Bank" };
                DrawText("Select a creature to swap out:", 100, 50, 20, BLACK);
                DrawText(teamNames[swapMenu.selectedTeam], 100, 70, 20, BLUE);

                Creature *currentTeam;
                int teamSize;
                switch (swapMenu.selectedTeam) {
                    case PARTY_MAIN_TEAM: currentTeam = mainTeam; teamSize = MAX_PARTY_SIZE; break;
                    case PARTY_BACKUP_TEAM: currentTeam = backupTeam; teamSize = MAX_BACKUP_SIZE; break;
                    case PARTY_BANK: currentTeam = bank; teamSize = MAX_BANK_SIZE; break;
                    default: assert(false && "Unhandled enum value!"); break;
                }

                for (int i = 0; i < teamSize; i++) {
                    char buf[50];
                    if (currentTeam[i].name[0] != '\0') {
                        sprintf(buf, "%s", currentTeam[i].name);
                    } else {
                        sprintf(buf, "Empty Slot");
                    }
                    Color color = (i == swapMenu.selectedIndex) ? RED : BLACK;
                    DrawText(buf, 100, 100 + i * 30, 20, color);
                }
                DrawText("Press Enter to swap, Backspace to release the new creature.", 10, SCREEN_HEIGHT - 50, 20, BLACK);
                break;
            }
                // Add confirmation dialog if in confirm state
            case GAME_STATE_CONFIRM_SWAP: {
                // Darken background
                DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
                
                // Confirmation box
                Rectangle confirmBox = { SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2 - 100, 400, 200 };
                DrawRectangleRec(confirmBox, LIGHTGRAY);
                DrawRectangleLinesEx(confirmBox, 2, DARKGRAY);
                
                // Confirmation text
                DrawText("Are you sure?", confirmBox.x + 50, confirmBox.y + 30, 30, BLACK);
                DrawText("This cannot be undone!", confirmBox.x + 50, confirmBox.y + 60, 20, DARKGRAY);

                // Yes/No options
                Color yesColor = (swapMenu.confirmSelection == 0) ? RED : BLACK;
                Color noColor = (swapMenu.confirmSelection == 1) ? RED : BLACK;
                
                DrawText("Yes (Z)", confirmBox.x + 100, confirmBox.y + 120, 25, yesColor);
                DrawText("No (X)", confirmBox.x + 250, confirmBox.y + 120, 25, noColor);
                
                DrawText("Press Z/Enter to confirm, X/Backspace to cancel", confirmBox.x + 20, confirmBox.y + 160, 18, DARKGRAY);
                break;
            } 

            case GAME_STATE_BOSS_BATTLE_TRANSITION: {
                static int dialogSelection = 0; // 0 for "Yes", 1 for "No"

                // Draw the dialog
                ClearBackground(BLACK);
                DrawText("Do you want to fight the secret boss?", SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 - 60, 20, WHITE);

                // Draw options with cursor
                int yesX = SCREEN_WIDTH / 2 - 60;
                int noX = SCREEN_WIDTH / 2 + 40;
                int optionsY = SCREEN_HEIGHT / 2;

                DrawText("Yes", yesX, optionsY, 20, WHITE);
                DrawText("No", noX, optionsY, 20, WHITE);

                // Draw cursor
                if (dialogSelection == 0) {
                    DrawText(">", yesX - 30, optionsY, 20, YELLOW);
                } else if (dialogSelection == 1) {
                    DrawText(">", noX - 30, optionsY, 20, YELLOW);
                }

                // Handle input
                if (IsKeyPressed(KEY_RIGHT)) {
                    dialogSelection = (dialogSelection + 1) % 2; // Cycle to NO -> YES
                }
                if (IsKeyPressed(KEY_LEFT)) {
                    dialogSelection = (dialogSelection - 1 + 2) % 2; // Cycle to YES -> NO
                }
                if (IsKeyPressed(KEY_X)) {
                    dialogSelection = 1; // Automatically select "No"
                }
                if (justTransitioned) {
                    justTransitioned = false;
                }
                else {
                        
                    if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER)) {
                        if (dialogSelection == 0) {
                            // Handle "Yes" selection
                            // battleState.shinyMarkerActive = true;
                            bossBattleData.bossBattleAccepted = true;
                            bossBattleData.shinyMarkerActive = false;
                            worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].walkable = true;
                            currentGameState = GAME_STATE_BATTLE_TRANSITION;
                            battleState.terrain = TERRAIN_BOSS;
                            ScreenBreakingAnimation();
                        } 
                        else {
                            // Handle "No" selection
                            bossBattleData.bossBattleTriggered = true;
                            bossBattleData.shinyMarkerActive = true;
                            bossBattleData.bossBattleAccepted = false;
                            // worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x] = (Terrain){TERRAIN_BOSS, 1.0f, false, WHITE, false, 0, 0};
                            bossBattleData.shinyMarkerPosition = (Vector2){mainTeam[0].x, mainTeam[0].y};
                            currentGameState = GAME_STATE_OVERWORLD;
                        }
                    }
                }

                break;
            }

            case GAME_STATE_GAME_OVER: {
                ClearBackground(BLACK);
                DrawText("Game Over", SCREEN_WIDTH / 2 - MeasureText("Game Over", 40) / 2, SCREEN_HEIGHT / 2 - 20, 40, RED);
                DrawText("Press Z to Restart", SCREEN_WIDTH / 2 - MeasureText("Press Z to Restart", 20) / 2, SCREEN_HEIGHT / 2 + 30, 20, WHITE);            
                break;
            }

            case GAME_STATE_REALTIME_BATTLE: {
                DrawRealTimeBattle(&realTimeBattleState, camera, keysPressed);
                break;
            }

            case GAME_STATE_COMMAND_MENU: {
                DrawRealTimeBattle(&realTimeBattleState, camera, keysPressed);
                DrawCommandMenu(&commandMenuState); // Call the new draw function
                break;
            }

            default: assert(false && "Unhandled enum value!"); break;

        }

        // Draw FPS counter, etc. (outside the state switch if always visible)
        DrawFPS(10, 10);
 
        EndDrawing();
    }

    // Restore default settings
    RestoreDefaultSettings();

    // WHY ONLY FOR 0?
    for (int i = 0; i < mainTeam[0].anims.idleFrameCount; i++) {
        UnloadTexture(mainTeam[0].anims.idleTextures[i]);
    }
    for (int j = 0; j < NUM_DIRECTIONS; j++) {
        for (int i = 0; i < mainTeam[0].anims.walkFrameCount[j]; i++) {
            UnloadTexture(mainTeam[0].anims.walkTextures[j][i]);
        }
    }

    UnloadAllAnimations();
    UnloadAllSkill();
    UnloadAllMagic();

    for (int i = 0; i < MAX_TOOLS; i++) {
        if (toolWheel.tools[i].texture.id != 0) {
            UnloadTexture(toolWheel.tools[i].texture);
        }
    }

    // Clean up
    // Free the map memory
    if (worldMap) {
        for (int i = 0; i < mapHeight; i++) {
            free(worldMap[i]);
            if (explored) free(explored[i]);
        }
        free(worldMap);
        if (explored) free(explored);
    }
    
    // Unload textures
    for (int i = 0; i < NUM_TERRAINS; i++) {
        for (int j = 0; j < terrainAnimations[i].frameCount; j++) {
            UnloadTexture(terrainAnimations[i].frames[j]);
        }
    }

    CleanupEnemies();
    CleanupBattle(&battleState);

    // StopMusic(&soundManager);
    CleanupSoundManager(&soundManager);
    CloseWindow();

    return 0;    
}



























///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////////////////////    END MAIN    ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////////////////////    BIG INITIALIZER     ///////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// Comprehensive game initialization
void InitializeGame(void) {
    // ----- SECTION 1: Basic initialization -----
    srand(time(NULL));
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Creature Adventure");
    // Initialize the main menu state first to load settings
    mainMenuState = (MainMenuState){0};
    mainMenuState.targetFPS = 60;  // Default value before loading settings
    menu = (Menu){0};
    
    // Load system settings to get the stored FPS value
    LoadSystemSettings(&mainMenuState);
    
    // Now set the target FPS using the loaded/default value
    SetTargetFPS(mainMenuState.targetFPS);
    targetFPS = mainMenuState.targetFPS;  // Update global variable

        // ----- SECTION 2: Load resources and managers -----
    // Initialize sound manager first so we can use sounds during loading
    InitSoundManager(&soundManager);
    // Load audio for main menu
    // LoadSoundByName(&soundManager, "menuSelect", "resources/sounds/menu_select.wav");
    // LoadSoundByName(&soundManager, "menuConfirm", "resources/sounds/menu_confirm.wav");
    // LoadSoundByName(&soundManager, "menuBack", "resources/sounds/menu_back.wav");
    // LoadSoundByName(&soundManager, "menuAdjust", "resources/sounds/menu_adjust.wav");
    
    // Load main menu graphics (if any)
    // logoTexture = LoadTexture("resources/logo.png");
    // backgroundTexture = LoadTexture("resources/menu_background.png");

    // ----- SECTION 3: Load game data -----
    // Initialize game data (skills, magic, items, creatures, etc.)
    LoadSkillsFromJSON(SKILL_DATA);
    LoadMagicFromJSON(MAGIC_DATA);
    LoadItemsFromJSON(ITEM_DATA);
    LoadCreaturesFromJSON(ENEMY_DATA);
    LoadSoundsAndMusicFromJSON(&soundManager, SOUND_DATA);
    LoadFriendshipGridFromJSON(&menu.gridState, GRID_DATA);

        // ----- SECTION 4: Initialize game world -----
    LoadMap("OverWorld.txt");
    InitMinimap();
    UpdateTileCooldowns();
    LoadTerrainTextures();
    LoadAllAnimations();

        // ----- SECTION 5: Initialize game state objects -----
    // Game state
    currentMap = MAP_OVERWORLD;
    targetMap = MAP_EMPTY;  // Where we want to go
    
    // Camera
    // Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    
    // Battle state
    // BattleState battleState = {0};
    
    // Boss battle data
    // BossBattleData bossBattleData = {0};
    
    // Tool wheel
    InitializeToolWheel(&toolWheel);
    RebuildInventories();
    CheckImprovedToolStatus(&toolWheel);
    UpdateKeyItemListForTools();

    // ----- SECTION 6: Initialize player and creatures -----
    // This will only run for new games, not when loading a save
    // We'll move this code to a separate function for New Game initialization
    
    // ----- SECTION 7: Initialize UI and menus -----
    // Game menu

    menu.selectedCreature = 0;
    menu.scrollUpKey = InitSmoothKey(0.2f, 0.1f);
    menu.scrollDownKey = InitSmoothKey(0.2f, 0.1f);
    menu.scrollRightKey = InitSmoothKey(0.2f, 0.1f);
    menu.scrollLeftKey = InitSmoothKey(0.2f, 0.1f);


    // Minigames
    OverWorldFishing(&fishingMinigameState);
    OverWorldPickaxe(&pickaxeMinigameState);
    OverWorldDigging(&diggingMinigameState);
    OverWorldCutting(&cuttingMinigameState);

        // --- Initialize Real-Time Battle State ---
    InitializeRealTimeBattle();
    // --- End Initialization ---


    // ----- SECTION 8: Initialize Main Menu System -----
    // Create and initialize the main menu state
    // mainMenuState = (MainMenuState){0};
    InitMainMenu(&mainMenuState);
    
    // ----- SECTION 9: Setup initial game state -----
    // Set initial game state to title screen
    currentGameState = GAME_STATE_TITLE_SCREEN;
    // Check for save files at startup and print diagnostics
    RefreshMainMenuSaveStatus(&mainMenuState);
    printf("Save file status at startup: hasAnySaveFile=%d, lastSaveSlot=%d\n", mainMenuState.hasAnySaveFile, mainMenuState.lastSaveSlot);
    
    // Play title music if available
    // PlayMusicByName(&soundManager, "title", true);
    
    // Initialize frame tracking
    InitializeFrameTracking(&frameTracking);
    
    // ----- SECTION 10: Load system settings -----
    // Load any saved settings (volume, controls, etc.)
    // LoadSystemSettings(&mainMenuState);
}

// Function to initialize a new game (called when selecting "New Game")
void InitializeNewGame(void) {
    // Reset any existing game state
    ResetGame(NULL);

    // Load the default settings
    LoadSystemSettings(&mainMenuState);
    // RestoreDefaultSettings();

    // Apply these settings
    // SetTargetFPS(mainMenuState.targetFPS);
    // targetFPS = mainMenuState.targetFPS;
    
    // SetGlobalMusicVolume(mainMenuState.musicVolume);
    // SetGlobalSoundVolume(mainMenuState.sfxVolume);
    
    // Initialize player character
    CopyCreature(&mainTeam[0], &enemyDatabase.creatures[0]);
    CompactTeam(mainTeam, MAX_PARTY_SIZE);
    mainTeam[0].x = 30.0f;
    mainTeam[0].y = 5.0f;
    mainTeam[0].pressRate = 1.0f;

    // CopyCreature(&mainTeam[1], &enemyDatabase.creatures[1]);
    // CopyCreature(&mainTeam[2], &enemyDatabase.creatures[2]);
    
    // Initialize grid progress for all creatures
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] != '\0') {
            LoadCreatureGridProgress(&mainTeam[i]);
        }
    }
    
    // Initialize player history
    for (int i = 0; i < MAX_HISTORY; i++) {
        playerHistory[i] = (Position){ mainTeam[0].x, mainTeam[0].y };
    }
    
    // Initialize achievements
    InitializeAchievements(&achievementState);

    RebuildInventories();
    CheckImprovedToolStatus(&toolWheel);
    // UpdateKeyItemListForTools();
    
    // Set starting items (could be moved to ResetGame)
    // AddItemToInventory(ITEM_POTION, 3);
    // AddItemToInventory(ITEM_ETHER, 1);
    
    // Set current map
    currentMap = MAP_OVERWORLD;
    
    // Start with overworld music
    // PlayMusicByName(&soundManager, "overworld", true);

    // float shakeMagnitude = 5.0f; // Maximum shake offset
    // Vector2 shakeOffset = { 0.0f, 0.0f };
	// Reset minigame states if they persist globally
    ResetMinigameStates();

    // Ensure minimap is ready
    minimapNeedsUpdate = true;
    RefreshMinimap(&mainTeam[0]);

    printf("New Game Initialized with settings from Title Screen.\n");
    DumpGameState(); // Optional: Debug dump
}

// void InitializeGameData(void) {

    // Initialize skills
    // LoadSkillsFromJSON(SKILL_DATA);

    // Initialize magic spells
    // LoadMagicFromJSON(MAGIC_DATA);

    // Initialize items
    // LoadItemsFromJSON(ITEM_DATA);

    // Initialize tool wheel first
    // InitializeToolWheel(&toolWheel);
    
    // Now rebuild inventories correctly
    // RebuildInventories();
    
    // Check for improved tools in the inventory
    // CheckImprovedToolStatus(&toolWheel);
    
    // Make sure the key item list includes tools
    // UpdateKeyItemListForTools();

    // Load enemy data
    // LoadCreaturesFromJSON(ENEMY_DATA);

    // Load sounds from JSON
    // LoadSoundsAndMusicFromJSON(&soundManager, SOUND_DATA);

    // LoadFriendshipGridFromJSON(&menu.gridState, GRID_DATA);
// }


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////////////////////    MISC.   ///////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


void DrawKeyRepresentation(bool keysPressed[]) {
    Color activeColor = DARKGRAY;
    Color inactiveColor = LIGHTGRAY;
    int offset = KEY_SIZE / 2;
    const char *keyLabels[] = { ">", "<", "v", "^", "Z", "X", "A", "S", "C" };
    Vector2 positions[] = {
        { TOP_RIGHTX + KEY_SIZE, TOP_RIGHTY + KEY_SIZE }, // Right
        { TOP_RIGHTX - KEY_SIZE, TOP_RIGHTY + KEY_SIZE }, // Left
        { TOP_RIGHTX, TOP_RIGHTY + KEY_SIZE },            // Down
        { TOP_RIGHTX, TOP_RIGHTY},            // Up
        { TOP_RIGHTX - 90, TOP_RIGHTY + 60 },             // "Z" key (APressed)
        { TOP_RIGHTX - 50, TOP_RIGHTY + 50 },             // "X" key (BPressed)
        { TOP_RIGHTX - 90, TOP_RIGHTY + 20 },             // "A" key
        { TOP_RIGHTX - 50, TOP_RIGHTY + 10 },             // "S" key
        { TOP_RIGHTX + (int)KEY_SIZE*2.5, TOP_RIGHTY + KEY_SIZE }, // "C" key
    };
    int numKeys = sizeof(keyLabels) / sizeof(keyLabels[0]);

    // Define colors for each key
    Color keyColors[numKeys];
    for (int i = 0; i < numKeys; i++) {
        if (i == 4) { // "Z" key
            keyColors[i] = keysPressed[i] ? GREEN : inactiveColor;
        } else if (i == 5) { // "X" key
            keyColors[i] = keysPressed[i] ? RED : inactiveColor;
        } else {
            keyColors[i] = keysPressed[i] ? activeColor : inactiveColor;
        }
    }

    for (int i = 0; i < numKeys; i++) {
        Color color = keyColors[i];
        Vector2 pos = positions[i];

        if (strcmp(keyLabels[i], ">") == 0 || strcmp(keyLabels[i], "<") == 0 ||
            strcmp(keyLabels[i], "v") == 0 || strcmp(keyLabels[i], "^") == 0) {
            // Directional keys (draw rectangles)
            DrawRectangle((int)pos.x, (int)pos.y, KEY_SIZE, KEY_SIZE, color);
            DrawText(keyLabels[i], (int)(pos.x + offset - 5), (int)(pos.y + offset - 10), 20, BLACK);
        } else {
            // Other keys (draw circles)
            DrawCircle((int)pos.x, (int)pos.y, KEY_SIZE / 2, color);
            DrawText(keyLabels[i], (int)(pos.x - KEY_SIZE / 4), (int)(pos.y - KEY_SIZE / 4), 20, BLACK);
        }
    }
}

void DrawKeySymbol(const char *keyLabel, Vector2 position, Color color) {

    if (strcmp(keyLabel, ">") == 0 || strcmp(keyLabel, "<") == 0 ||
        strcmp(keyLabel, "v") == 0 || strcmp(keyLabel, "^") == 0) {
        // Directional keys (draw rectangles)
        DrawRectangle((int)position.x - KEY_SIZE / 2, (int)position.y - KEY_SIZE / 2 - TILE_SIZE / 2, KEY_SIZE, KEY_SIZE, color);
        DrawText(keyLabel, (int)(position.x - 5), (int)(position.y - 10 - TILE_SIZE / 2), 20, BLACK);
    } else {
        // Other keys (draw circles)
        DrawCircle((int)position.x, (int)position.y - TILE_SIZE / 2, KEY_SIZE / 2, color);
        DrawText(keyLabel, (int)(position.x - KEY_SIZE / 4), (int)(position.y - KEY_SIZE / 4 - TILE_SIZE / 2), 20, BLACK);
    }
}

void SetOverworldMessage(const char* message) {
    if (overworldMessageCount < MAX_MESSAGES) {
        strncpy(overworldMessages[overworldMessageCount].text, message, sizeof(overworldMessages[overworldMessageCount].text) - 1);
        overworldMessages[overworldMessageCount].text[sizeof(overworldMessages[overworldMessageCount].text) - 1] = '\0';
        overworldMessages[overworldMessageCount].timer = 120; // Display for 2 seconds at 60 FPS
        overworldMessageCount++;
    }
}

void DrawOverworldMessages(void) {
    // Display overworld messages
    if (overworldMessageCount > 0) {
        Message* currentMessage = &overworldMessages[0];
        if (currentMessage->timer > 0) {
            DrawText(currentMessage->text, 20, SCREEN_HEIGHT - 50, 20, WHITE);
            currentMessage->timer--;
        } else {
            // Remove the message from the queue
            for (int i = 0; i < overworldMessageCount - 1; i++) {
                overworldMessages[i] = overworldMessages[i + 1];
            }
            overworldMessageCount--;
        }
    }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////////////////////    SPRITES & ANIMATION INITIALIZER     ///////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


void HandlePlayerMovement( Creature *c, FrameTracking *frameTracking) {
    // Update key states
    bool BPressed = IsKeyDown(KEY_X); // "X" key


    // Reset isPressed
    c->anims.isPressed = false;

    // Update direction and press durations based on input
    if (IsKeyDown(KEY_RIGHT)) {
        c->anims.direction = RIGHT;
        c->anims.directionPressCounter[RIGHT]++;
        c->anims.isPressed = true;
    } else {
        c->anims.directionPressCounter[RIGHT] = 0;
    }

    if (IsKeyDown(KEY_LEFT)) {
        c->anims.direction = LEFT;
        c->anims.directionPressCounter[LEFT]++;
        c->anims.isPressed = true;
    } else {
        c->anims.directionPressCounter[LEFT] = 0;
    }

    if (IsKeyDown(KEY_DOWN)) {
        c->anims.direction = DOWN;
        c->anims.directionPressCounter[DOWN]++;
        c->anims.isPressed = true;
    } else {
        c->anims.directionPressCounter[DOWN] = 0;
    }

    if (IsKeyDown(KEY_UP)) {
        c->anims.direction = UP;
        c->anims.directionPressCounter[UP]++;
        c->anims.isPressed = true;
    } else {
        c->anims.directionPressCounter[UP] = 0;
    }

    // Movement and stopping logic
    if (c->anims.isPressed) {
        if (c->anims.directionPressCounter[c->anims.direction] > INERTIA_TO_MOTION) {
            // Move when held beyond threshold
            MovePlayer( (c->anims.direction == RIGHT) - (c->anims.direction == LEFT), (c->anims.direction == DOWN) - (c->anims.direction == UP), BPressed);

            c->anims.isMoving = true;
            c->anims.isStopping = false;   // Reset stopping state when moving
            frameTracking->stopDelayCounter = 120;  // Reset delay counter for next stop
        } else {
            // Turn to face direction without moving
            c->anims.isMoving = false;
            c->anims.isStopping = true;
            c->anims.movementState = WALKING;
            frameTracking->currentFrame = 0;
            frameTracking->stopDelayCounter = 120;  // Initialize delay for showing the '0.png' frame
        }
    } else if (c->anims.isMoving) {
        // Transition to stopping
        c->anims.isMoving = false;
        c->anims.isStopping = true;
        frameTracking->stopDelayCounter = 120;
    } else if (c->anims.isStopping && frameTracking->stopDelayCounter > 0) {
        // Countdown delay
        frameTracking->stopDelayCounter--;
    } else if (c->anims.isStopping && frameTracking->stopDelayCounter == 0) {
        // Transition to idle
        c->anims.isStopping = false;
        c->anims.movementState = IDLE;
    }
}

// Convert terrain type to character symbol
char TerrainToSymbol(TerrainType type) {
    switch (type) {
        case TERRAIN_MOUNTAIN: return '^';
        case TERRAIN_WATER: return '~';
        case TERRAIN_GRASS: return '.';
        case TERRAIN_GROUND: return '-';
        case TERRAIN_EMPTY: return ' ';
        case TERRAIN_BOSS: return 'B';
        case TERRAIN_USEDGROUND: return 'u';
        case TERRAIN_CUTGRASS: return 'c';
        case TERRAIN_MOON: return 'o';
        case TERRAIN_ICE: return 'i';
        case TERRAIN_MAGMA: return 'm';
		case TERRAIN_POISON: return 'p';
		case TERRAIN_CLOUDS: return 'l';
		case TERRAIN_WOOD: return 'd';
		case TERRAIN_UNDERWATER: return 'n';
        default: return ' ';
    }
}

// Convert symbol to terrain type
TerrainType SymbolToTerrain(char symbol) {
    switch (symbol) {
        case '^': return TERRAIN_MOUNTAIN;
        case '~': return TERRAIN_WATER;
        case '.': return TERRAIN_GRASS;
        case '-': return TERRAIN_GROUND;
        case ' ': return TERRAIN_EMPTY;
        case 'B': return TERRAIN_BOSS;
        case 'u': return TERRAIN_USEDGROUND;
        case 'c': return TERRAIN_CUTGRASS;
        case 'o': return TERRAIN_MOON;
        case 'i': return TERRAIN_ICE;
        case 'm': return TERRAIN_MAGMA;
		case 'p': return TERRAIN_POISON;
		case 'l': return TERRAIN_CLOUDS;
		case 'd': return TERRAIN_WOOD;
		case 'n': return TERRAIN_UNDERWATER;
        default: return TERRAIN_EMPTY;
    }
}

// void InitializeMapFromFile(const char *folderPath) {
// Function to load any map by filename
bool LoadMap(const char* mapName) {
    // Build correct path
    char fullPath[256];
    sprintf(fullPath, FOLDER_MAP"/%s", mapName);
    
    // Ensure it has the correct extension
    if (strstr(fullPath, ".txt") == NULL) {
        strcat(fullPath, ".txt");
    }
    
    printf("Loading map from: %s\n", fullPath);
    
    // Try to open the file
    char* fileContent = LoadFileText(fullPath);
    if (!fileContent) {
        fprintf(stderr, "Failed to load map file: %s\n", fullPath);
        return false;
    }
    
    // Free existing map if it exists
    if (worldMap) {
        for (int i = 0; i < mapHeight; i++) {
            free(worldMap[i]);
            if (explored) free(explored[i]);
        }
        free(worldMap);
        if (explored) free(explored);
    }
    
    // Determine map dimensions
    int lineCount = 0;
    int currentWidth = 0;
    char* line = strtok(fileContent, "\n");
    char* lines[256]; // Adjust size if maps can have more lines
    
    while (line) {
        int len = strlen(line);
        if (len > 0 && line[len-1] == ',') len--; // Exclude trailing comma
        if (lineCount == 0) currentWidth = len;
        lines[lineCount] = line;
        lineCount++;
        line = strtok(NULL, "\n");
    }
    
    mapHeight = lineCount;
    mapWidth = currentWidth;
    
    // Allocate worldMap
    explored = (bool**)malloc(mapHeight * sizeof(bool*));
    worldMap = (Terrain**)malloc(mapHeight * sizeof(Terrain*));
    
    for (int y = 0; y < mapHeight; y++) {
        explored[y] = (bool*)malloc(mapWidth * sizeof(bool));
        worldMap[y] = (Terrain*)malloc(mapWidth * sizeof(Terrain));
        
        const char* currentLine = lines[y];
        int len = strlen(currentLine);
        if (currentLine[len-1] == ',') len--;
        
        for (int x = 0; x < mapWidth; x++) {
            explored[y][x] = false;
            if (x >= len) { // Handle short lines
                SetTerrainAtPosition(x, y, TERRAIN_EMPTY);
                continue;
            }
            
            // Convert the character to a terrain type
            TerrainType type = SymbolToTerrain(currentLine[x]);
            SetTerrainAtPosition(x, y, type);
        }
    }
    
    UnloadFileText(fileContent);
    
    // Attempt to load JSON metadata if available
    char jsonPath[256];
    strcpy(jsonPath, fullPath);
    char* ext = strrchr(jsonPath, '.');
    if (ext) *ext = '\0'; // Remove extension
    strcat(jsonPath, ".json");
    
    // TODO: Load JSON metadata if needed
    
    printf("Map loaded successfully: %dx%d\n", mapWidth, mapHeight);
    return true;
}


void TransitionToNewMap(const char* mapName, float startX, float startY) {
    LoadMap(mapName);
    mainTeam[0].x = startX;
    mainTeam[0].y = startY;
    
    // Reset player history for the new map
    for (int i = 0; i < MAX_HISTORY; i++) {
        playerHistory[i] = (Position){ mainTeam[0].x, mainTeam[0].y };
    }
    
    // Update minimap
    minimapNeedsUpdate = true;
    RefreshMinimap(&mainTeam[0]);
}


////////////////////////////////////////////
/////////OVERWORLD MAP 2 SETUP//////////////
////////////////////////////////////////////

void TransitionToOverworldState(void) {
    TransitionToNewMap("OverWorld.txt", mainTeam[0].x, mainTeam[0].y);
}

// Initialize the tunnel state with pattern tracking
void TransitionToTunnelState(void) {
    // Debug print
    printf("TransitionToPatternState called!\n");
    // Load the map with the existing ground pattern
    TransitionToNewMap("OverWorld2DigPatterns.txt", mainTeam[0].x, mainTeam[0].y);
    
    // Define the pattern coordinates
    Vector2 patternCoordinates[8] = {
        {38, 16}, {37, 17}, {41, 16}, {42, 17}, 
        {42, 20}, {41, 21}, {37, 20}, {38, 21}
    };
    
    // Initialize pattern tracking (no need to set terrain)
    InitializeGroundPattern(patternCoordinates, 8);
}

void InitializeGroundPattern(Vector2 coordinates[], int count) {
    groundPattern.active = true;
    groundPattern.totalTiles = count;
    groundPattern.completed = false;
    groundPattern.dugTiles = 0;
    
    // Store the pattern coordinates
    for (int i = 0; i < count; i++) {
        groundPattern.coordinates[i] = coordinates[i];
    }
    
    // Notify the player
    SetOverworldMessage("Dig the pattern to reveal a hidden path!");
}

bool CheckPatternCompleted(void) {
    if (!groundPattern.active || groundPattern.completed) {
        return groundPattern.completed;
    }

    // Reset the counter first
    groundPattern.dugTiles = 0;

    // Count how many tiles in the pattern are currently dug
    // int dugTiles = 0;
    for (int i = 0; i < groundPattern.totalTiles; i++) {
        int x = (int)groundPattern.coordinates[i].x;
        int y = (int)groundPattern.coordinates[i].y;
        
        // Check if this tile is in the dug state
        if (worldMap[y][x].type == TERRAIN_USEDGROUND && worldMap[y][x].isModified) {
            groundPattern.dugTiles++;
        }
    }
    
    // If all tiles are dug, pattern is complete
    if (groundPattern.dugTiles >= groundPattern.totalTiles) {
        groundPattern.completed = true;
        SetOverworldMessage("Pattern complete! A path has opened!");
        targetMap = MAP_OVERWORLD_HOLE;
        return true;
    }
    
    return false;
}


// Draw the pattern progress counter
void DrawPatternCounter(void) {
    if (!groundPattern.active || groundPattern.completed) return;

    // Draw progress text
    DrawText(TextFormat("Pattern: %d/%d tiles dug", groundPattern.dugTiles, groundPattern.totalTiles), 
             SCREEN_WIDTH - 200, 10, 20, WHITE);
}

void UpdatePatternStatus(void) {
    if (groundPattern.active && !groundPattern.completed) {
        CheckPatternCompleted();
    }
}

////////////////////////////////////////////
/////////OVERWORLD MAP 3 SETUP//////////////
////////////////////////////////////////////

void TransitionToHoleState(void) {
    printf("TransitionToHoleState called!\n");

    TransitionToNewMap("OverWorld3BigHole.txt", mainTeam[0].x, mainTeam[0].y);

        // Define the pattern coordinates
    Vector2 patternCoordinates[16] = {
        {38, 17}, {39,17}, {40,17}, {41,17}, 
        {38, 18}, {39,18}, {40,18}, {41,18}, 
        {38, 19}, {39,19}, {40,19}, {41,19}, 
        {38, 20}, {39,20}, {40,20}, {41,20}, 
  
    };
    
    // Initialize pattern tracking (no need to set terrain)
    InitializeHolePattern(patternCoordinates, 16);
}

void InitializeHolePattern(Vector2 coordinates[], int count) {
    holePattern.active = true;
    holePattern.totalTiles = count;
    holePattern.completed = false;
    
    // Store the pattern coordinates
    for (int i = 0; i < count; i++) {
        holePattern.coordinates[i] = coordinates[i];
    }

    for (int i = 0; i < holePattern.totalTiles; i++) {
        int x = (int)holePattern.coordinates[i].x;
        int y = (int)holePattern.coordinates[i].y;
        
        // Check if this tile is in the dug state make them permanent
        if (worldMap[y][x].type == TERRAIN_USEDGROUND) {
            worldMap[y][x].cooldownTimer = 99999;
        }
    }
    
    // Notify the player
    SetOverworldMessage("Dig the pattern to reveal a hidden path!");
}

bool CheckHoleStatus(void) {
    for (int i = 0; i < holePattern.totalTiles; i++) {
        int x = (int)holePattern.coordinates[i].x;
        int y = (int)holePattern.coordinates[i].y;
        
        // Check if this tile is in the dug state
        if (worldMap[y][x].type == TERRAIN_USEDGROUND && y == (int)mainTeam[0].y && x == (int)mainTeam[0].x) {
            targetMap = MAP_INSIDE_HOLE;
            printf("TransitionToHoleState called!\n");
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////
/////////INSIDE_HOLE MAP 4 SETUP////////////
////////////////////////////////////////////


bool LoadTiledMap(const char* jsonFilePath, const char* tilesetPath) {
    // Load the JSON file
    char* jsonText = LoadFileText(jsonFilePath);
    if (!jsonText) {
        printf("Failed to load JSON file: %s\n", jsonFilePath);
        return false;
    }
    
    // Parse with cJSON
    cJSON* root = cJSON_Parse(jsonText);
    UnloadFileText(jsonText);
    
    if (!root) {
        printf("Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
        return false;
    }
    
    // Extract map properties
    currentTileMap.width = cJSON_GetObjectItem(root, "width")->valueint;
    currentTileMap.height = cJSON_GetObjectItem(root, "height")->valueint;
    currentTileMap.tileWidth = cJSON_GetObjectItem(root, "tilewidth")->valueint;
    currentTileMap.tileHeight = cJSON_GetObjectItem(root, "tileheight")->valueint;
    
    // Load tileset
    currentTileMap.tileset = LoadTexture(tilesetPath);
    
    // Parse layers
    cJSON* layers = cJSON_GetObjectItem(root, "layers");
    currentTileMap.layerCount = 0;
    
    for (int i = 0; i < cJSON_GetArraySize(layers); i++) {
        cJSON* layer = cJSON_GetArrayItem(layers, i);
        
        // Extract layer type
        const char* type = cJSON_GetObjectItem(layer, "type")->valuestring;
        
        if (strcmp(type, "tilelayer") == 0) {
            // Handle tile layer
            int layerIndex = currentTileMap.layerCount++;
            
            // Copy layer name
            strncpy(currentTileMap.layers[layerIndex].name, 
                    cJSON_GetObjectItem(layer, "name")->valuestring, 
                    31);
            
            // Set visibility
            currentTileMap.layers[layerIndex].visible = 
                cJSON_GetObjectItem(layer, "visible")->valueint == 1;
            
            // Extract tile data
            cJSON* data = cJSON_GetObjectItem(layer, "data");
            int dataSize = cJSON_GetArraySize(data);
            
            currentTileMap.layers[layerIndex].data = (int*)malloc(dataSize * sizeof(int));
            
            for (int j = 0; j < dataSize; j++) {
                currentTileMap.layers[layerIndex].data[j] = 
                    cJSON_GetArrayItem(data, j)->valueint;
            }
        } 
        else if (strcmp(type, "objectgroup") == 0) {
            // Handle object layer (collisions or portals)
            const char* name = cJSON_GetObjectItem(layer, "name")->valuestring;
            
            if (strstr(name, "Collision") != NULL) {
                // Collision layer
                cJSON* objects = cJSON_GetObjectItem(layer, "objects");
                
                for (int j = 0; j < cJSON_GetArraySize(objects); j++) {
                    cJSON* object = cJSON_GetArrayItem(objects, j);
                    cJSON* polygon = cJSON_GetObjectItem(object, "polygon");
                    
                    if (polygon) {
                        int objIndex = currentTileMap.collisionCount++;
                        int pointCount = cJSON_GetArraySize(polygon);
                        
                        // Get object position
                        float objX = cJSON_GetObjectItem(object, "x")->valuedouble;
                        float objY = cJSON_GetObjectItem(object, "y")->valuedouble;
                        
                        currentTileMap.collisions[objIndex].pointCount = pointCount;
                        currentTileMap.collisions[objIndex].points = 
                            (Vector2*)malloc(pointCount * sizeof(Vector2));
                        
                        for (int k = 0; k < pointCount; k++) {
                            cJSON* point = cJSON_GetArrayItem(polygon, k);
                            float x = cJSON_GetObjectItem(point, "x")->valuedouble;
                            float y = cJSON_GetObjectItem(point, "y")->valuedouble;
                            
                            // Convert to world coordinates
                            currentTileMap.collisions[objIndex].points[k] = 
                                (Vector2){ objX + x, objY + y };
                        }
                    }
                }
            } 
            else if (strstr(name, "Portal") != NULL) {
                // Portal layer
                cJSON* objects = cJSON_GetObjectItem(layer, "objects");
                
                for (int j = 0; j < cJSON_GetArraySize(objects); j++) {
                    cJSON* object = cJSON_GetArrayItem(objects, j);
                    
                    // Check if it's a point
                    if (cJSON_GetObjectItem(object, "point")->valueint == 1) {
                        int portalIndex = currentTileMap.portalCount++;
                        
                        currentTileMap.portals[portalIndex].position = 
                            (Vector2){ 
                                cJSON_GetObjectItem(object, "x")->valuedouble,
                                cJSON_GetObjectItem(object, "y")->valuedouble 
                            };
                        
                        // Default values - you would set these with properties
                        currentTileMap.portals[portalIndex].targetMap = MAP_OVERWORLD;
                        currentTileMap.portals[portalIndex].targetPosition = (Vector2){ 10, 10 }; // Default position
                        
                        // Check for custom properties
                        cJSON* properties = cJSON_GetObjectItem(object, "properties");
                        if (properties) {
                            // Parse portal properties (target map, coordinates, etc.)
                            // This depends on how you've set up your properties in Tiled
                        }
                    }
                }
            }
        }
    }
    
    cJSON_Delete(root);
    return true;
}

void TransitionToInsideHoleState(void) {
    printf("Transitioning to inside hole map...\n");
    
    // Load the Tiled map
    if (!LoadTiledMap("./game/assets/tiles/cave/test2.tmj", 
                      "./game/assets/tiles/cave/SNES - Final Fantasy 6 - Cave Tiles.png")) {
        printf("Failed to load Tiled map!\n");
        return;
    }
    
    // Place the player at the entry portal
    if (currentTileMap.portalCount > 0) {
        mainTeam[0].x = currentTileMap.portals[0].position.x / currentTileMap.tileWidth;
        mainTeam[0].y = currentTileMap.portals[0].position.y / currentTileMap.tileHeight;
    } else {
        // Default position if no portal defined
        mainTeam[0].x = 10;
        mainTeam[0].y = 10;
    }
    
    // Update minimap and history
    for (int i = 0; i < MAX_HISTORY; i++) {
        playerHistory[i] = (Position){ mainTeam[0].x, mainTeam[0].y };
    }
    
    minimapNeedsUpdate = true;
    RefreshMinimap(&mainTeam[0]);
}


void DrawTiledMap(Camera2D camera) {
    // Calculate visible area
    int startX = (int)(-camera.offset.x / (currentTileMap.tileWidth * camera.zoom));
    int startY = (int)(-camera.offset.y / (currentTileMap.tileHeight * camera.zoom));
    int endX = startX + (int)(GetScreenWidth() / (currentTileMap.tileWidth * camera.zoom)) + 2;
    int endY = startY + (int)(GetScreenHeight() / (currentTileMap.tileHeight * camera.zoom)) + 2;
    
    // Clamp to map boundaries
    startX = startX < 0 ? 0 : startX;
    startY = startY < 0 ? 0 : startY;
    endX = endX > currentTileMap.width ? currentTileMap.width : endX;
    endY = endY > currentTileMap.height ? currentTileMap.height : endY;
    
    // Calculate tiles per row in the tileset
    int tilesPerRow = currentTileMap.tileset.width / currentTileMap.tileWidth;
    
    // Draw each visible layer
    for (int l = 0; l < currentTileMap.layerCount; l++) {
        if (!currentTileMap.layers[l].visible) continue;
        
        // Draw each visible tile
        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                int tileIndex = y * currentTileMap.width + x;
                int tileId = currentTileMap.layers[l].data[tileIndex];
                
                if (tileId == 0) continue; // Empty tile
                
                // Adjust for Tiled's 1-based indexing
                tileId--;
                
                // Calculate source rectangle
                Rectangle srcRect = {
                    (tileId % tilesPerRow) * currentTileMap.tileWidth,
                    (tileId / tilesPerRow) * currentTileMap.tileHeight,
                    currentTileMap.tileWidth,
                    currentTileMap.tileHeight
                };
                
                // Calculate destination position
                Vector2 pos = {
                    x * currentTileMap.tileWidth * camera.zoom + camera.offset.x,
                    y * currentTileMap.tileHeight * camera.zoom + camera.offset.y
                };
                
                // Draw the tile
                DrawTextureRec(currentTileMap.tileset, srcRect, pos, WHITE);
            }
        }
    }
    
    // Debug: Draw collision polygons
    for (int i = 0; i < currentTileMap.collisionCount; i++) {
        for (int j = 0; j < currentTileMap.collisions[i].pointCount - 1; j++) {
            DrawLineV(
                (Vector2){
                    currentTileMap.collisions[i].points[j].x * camera.zoom + camera.offset.x,
                    currentTileMap.collisions[i].points[j].y * camera.zoom + camera.offset.y
                },
                (Vector2){
                    currentTileMap.collisions[i].points[j+1].x * camera.zoom + camera.offset.x,
                    currentTileMap.collisions[i].points[j+1].y * camera.zoom + camera.offset.y
                },
                RED
            );
        }
        // Close the polygon
        DrawLineV(
            (Vector2){
                currentTileMap.collisions[i].points[currentTileMap.collisions[i].pointCount-1].x * camera.zoom + camera.offset.x,
                currentTileMap.collisions[i].points[currentTileMap.collisions[i].pointCount-1].y * camera.zoom + camera.offset.y
            },
            (Vector2){
                currentTileMap.collisions[i].points[0].x * camera.zoom + camera.offset.x,
                currentTileMap.collisions[i].points[0].y * camera.zoom + camera.offset.y
            },
            RED
        );
    }
}

bool CheckCollisionWithTiledMap(Vector2 position, float radius) {
    // Check collisions with all collision polygons
    for (int i = 0; i < currentTileMap.collisionCount; i++) {
        // Simple circle vs polygon collision
        // Check if the circle (player) intersects with any edge of the polygon
        for (int j = 0; j < currentTileMap.collisions[i].pointCount; j++) {
            Vector2 p1 = currentTileMap.collisions[i].points[j];
            Vector2 p2 = currentTileMap.collisions[i].points[(j + 1) % currentTileMap.collisions[i].pointCount];
            
            // Check distance from point to line segment
            if (CheckCollisionPointLineMine(position, p1, p2, radius)) {
                return true;
            }
        }
    }
    
    return false;
}

// Helper function to check if a point is near a line segment
bool CheckCollisionPointLineMine(Vector2 point, Vector2 p1, Vector2 p2, float threshold) {
    // Calculate squared length of the line segment
    float lengthSq = pow(p2.x - p1.x, 2) + pow(p2.y - p1.y, 2);
    if (lengthSq == 0) return false; // Line segment is a point
    
    // Calculate the projection of point onto the line
    float t = ((point.x - p1.x) * (p2.x - p1.x) + (point.y - p1.y) * (p2.y - p1.y)) / lengthSq;
    t = t < 0 ? 0 : (t > 1 ? 1 : t); // Clamp to [0,1]
    
    // Calculate the closest point on the line segment
    Vector2 projection = {
        p1.x + t * (p2.x - p1.x),
        p1.y + t * (p2.y - p1.y)
    };
    
    // Calculate distance to the closest point
    float distance = sqrt(pow(point.x - projection.x, 2) + pow(point.y - projection.y, 2));
    
    return distance <= threshold;
}

void CheckForPortals(void) {
    // Vector2 portalTargetPosition = {0};
    for (int i = 0; i < currentTileMap.portalCount; i++) {
        // Convert to tile coordinates
        float portalTileX = currentTileMap.portals[i].position.x / currentTileMap.tileWidth;
        float portalTileY = currentTileMap.portals[i].position.y / currentTileMap.tileHeight;
        
        // Check if player is near the portal
        if (fabs(mainTeam[0].x - portalTileX) < 0.5f && 
            fabs(mainTeam[0].y - portalTileY) < 0.5f) {
            
            // Activate portal
            targetMap = currentTileMap.portals[i].targetMap;
            
            // Save target position for when we transition
            // portalTargetPosition = currentTileMap.portals[i].targetPosition;
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////DRAW OVERWORLD//////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

// Helper to draw antialiased edges around a terrain cluster
void DrawClusterEdges(int startX, int startY, int width, int height, TerrainType type, float viewportX, float viewportY) {
    int endX = startX + width;
    int endY = startY + height;
    int borderWidth = 2;
    
    // Draw antialiased edges where this cluster meets different terrain types
    
    // Top edge
    for (int x = startX; x < endX; x++) {
        if (startY > 0 && worldMap[startY-1][x].type != type) {
            Color currentColor = worldMap[startY][x].color;
            Color neighborColor = worldMap[startY-1][x].color;
            Color blendedColor = {
                (currentColor.r + neighborColor.r) / 2,
                (currentColor.g + neighborColor.g) / 2,
                (currentColor.b + neighborColor.b) / 2,
                120 // Alpha
            };
            
            float drawX = (x - viewportX) * TILE_SIZE;
            float drawY = (startY - viewportY) * TILE_SIZE;
            DrawRectangle(drawX, drawY, TILE_SIZE, borderWidth, blendedColor);
        }
    }
    
    // Bottom edge
    for (int x = startX; x < endX; x++) {
        if (endY < mapHeight && worldMap[endY][x].type != type) {
            Color currentColor = worldMap[endY-1][x].color;
            Color neighborColor = worldMap[endY][x].color;
            Color blendedColor = {
                (currentColor.r + neighborColor.r) / 2,
                (currentColor.g + neighborColor.g) / 2,
                (currentColor.b + neighborColor.b) / 2,
                120 // Alpha
            };
            
            float drawX = (x - viewportX) * TILE_SIZE;
            float drawY = (endY - viewportY) * TILE_SIZE - borderWidth;
            DrawRectangle(drawX, drawY, TILE_SIZE, borderWidth, blendedColor);
        }
    }
    
    // Left edge
    for (int y = startY; y < endY; y++) {
        if (startX > 0 && worldMap[y][startX-1].type != type) {
            Color currentColor = worldMap[y][startX].color;
            Color neighborColor = worldMap[y][startX-1].color;
            Color blendedColor = {
                (currentColor.r + neighborColor.r) / 2,
                (currentColor.g + neighborColor.g) / 2,
                (currentColor.b + neighborColor.b) / 2,
                120 // Alpha
            };
            
            float drawX = (startX - viewportX) * TILE_SIZE;
            float drawY = (y - viewportY) * TILE_SIZE;
            DrawRectangle(drawX, drawY, borderWidth, TILE_SIZE, blendedColor);
        }
    }
    
    // Right edge
    for (int y = startY; y < endY; y++) {
        if (endX < mapWidth && worldMap[y][endX].type != type) {
            Color currentColor = worldMap[y][endX-1].color;
            Color neighborColor = worldMap[y][endX].color;
            Color blendedColor = {
                (currentColor.r + neighborColor.r) / 2,
                (currentColor.g + neighborColor.g) / 2,
                (currentColor.b + neighborColor.b) / 2,
                120 // Alpha
            };
            
            float drawX = (endX - viewportX) * TILE_SIZE - borderWidth;
            float drawY = (y - viewportY) * TILE_SIZE;
            DrawRectangle(drawX, drawY, borderWidth, TILE_SIZE, blendedColor);
        }
    }
}

// Allocate a 2D boolean array for cluster tracking
bool** AllocateClusterTileArray(int width, int height) {
    bool** clusterTile = (bool**)malloc(height * sizeof(bool*));
    for (int i = 0; i < height; i++) {
        clusterTile[i] = (bool*)calloc(width, sizeof(bool)); // calloc initializes to 0 (false)
    }
    return clusterTile;
}

// Free the allocated 2D array
void FreeClusterTileArray(bool** clusterTile, int height) {
    for (int i = 0; i < height; i++) {
        free(clusterTile[i]);
    }
    free(clusterTile);
}

// Find and draw clusters of a specific terrain type
void FindAndDrawTerrainClusters(TerrainType terrainType, float viewportX, float viewportY, int startX, int startY, int endX, int endY, bool** clusterTile) {
    // Find clusters
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            // Skip if already processed or not the target terrain type
            if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight || 
                clusterTile[y][x] || worldMap[y][x].type != terrainType) {
                continue;
            }
            
            // Find the max width of this cluster
            int width = 1;
            while (x + width < mapWidth && x + width < endX && 
                   worldMap[y][x + width].type == terrainType && 
                   !clusterTile[y][x + width]) {
                width++;
            }
            
            // Find the max height of this cluster
            int height = 1;
            bool validRow = true;
            while (validRow && y + height < mapHeight && y + height < endY) {
                for (int dx = 0; dx < width; dx++) {
                    if (worldMap[y + height][x + dx].type != terrainType || 
                        clusterTile[y + height][x + dx]) {
                        validRow = false;
                        break;
                    }
                }
                if (validRow) height++;
            }
            
            // Only process clusters of at least 2x2 size
            if (width >= 2 && height >= 2) {
                // Mark all tiles in this cluster
                for (int dy = 0; dy < height; dy++) {
                    for (int dx = 0; dx < width; dx++) {
                        clusterTile[y + dy][x + dx] = true;
                    }
                }
                
                // Draw the cluster as a single large tile
                float drawX = (x - viewportX) * TILE_SIZE;
                float drawY = (y - viewportY) * TILE_SIZE;
                
                TerrainAnimation *anim = &terrainAnimations[terrainType];
                if (anim->frameCount > 0) {
                    // Draw a stretched texture for the entire cluster
                    Texture2D texture = anim->frames[anim->frameIndex];
                    DrawTexturePro(
                        texture,
                        (Rectangle){ 0, 0, texture.width, texture.height },
                        (Rectangle){ drawX, drawY, width * TILE_SIZE, height * TILE_SIZE },
                        (Vector2){ 0, 0 },
                        0.0f,
                        worldMap[y][x].color
                    );
                    
                    // Add antialiasing at the edges
                    DrawClusterEdges(x, y, width, height, terrainType, viewportX, viewportY);
                }
            }
        }
    }
}

Vector2 DrawMap(void) {
    // Calculate viewport based on player position
    float viewportX = mainTeam[0].x - (SCREEN_WIDTH / (2.0f * TILE_SIZE));
    float viewportY = mainTeam[0].y - (SCREEN_HEIGHT / (2.0f * TILE_SIZE));
    
    // Keep viewport within map boundaries
    if (viewportX < 0) viewportX = 0;
    if (viewportY < 0) viewportY = 0;
    if (viewportX > mapWidth - SCREEN_WIDTH / TILE_SIZE) viewportX = mapWidth - SCREEN_WIDTH / TILE_SIZE;
    if (viewportY > mapHeight - SCREEN_HEIGHT / TILE_SIZE) viewportY = mapHeight - SCREEN_HEIGHT / TILE_SIZE;
    
    // Calculate sub-tile offsets for smooth scrolling
    // float offsetX = (viewportX - (int)viewportX) * TILE_SIZE;
    // float offsetY = (viewportY - (int)viewportY) * TILE_SIZE;
    
    // Calculate visible range
    int startX = (int)viewportX;
    int startY = (int)viewportY;
    int endX = startX + (SCREEN_WIDTH / TILE_SIZE) + 2; // +2 to prevent edge artifacts
    int endY = startY + (SCREEN_HEIGHT / TILE_SIZE) + 2;
    
    // Clamp to map dimensions
    if (endX > mapWidth) endX = mapWidth;
    if (endY > mapHeight) endY = mapHeight;
    
    // Create a dynamically allocated 2D array for tracking clustered tiles
    bool** clusterTile = AllocateClusterTileArray(mapWidth, mapHeight);
    
    // Process terrain types that should be clustered
    FindAndDrawTerrainClusters(TERRAIN_USEDGROUND, viewportX, viewportY, startX, startY, endX, endY, clusterTile);
    
    // Add more terrain types to be clustered here as needed
    // FindAndDrawTerrainClusters(TERRAIN_CUTGRASS, viewportX, viewportY, startX, startY, endX, endY, clusterTile);
    // FindAndDrawTerrainClusters(TERRAIN_GROUND, viewportX, viewportY, startX, startY, endX, endY, clusterTile);
    
    // Second pass: Draw all visible individual tiles (skipping clustered ones)
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight && !clusterTile[y][x]) {
                // Calculate screen draw coordinates with offset
                float drawX = (x - viewportX) * TILE_SIZE;
                float drawY = (y - viewportY) * TILE_SIZE;
                
                // Get tile type and validate it's in range
                TerrainType type = worldMap[y][x].type;
                
                // Draw the animated tile with exact grid coordinates for proper antialiasing
                if (type >= 0 && type < NUM_TERRAINS) {
                    DrawAnimatedTile(
                        &terrainAnimations[type], 
                        drawX, 
                        drawY, 
                        TILE_SIZE, 
                        worldMap[y][x].color,
                        &worldMap[y][x],
                        x,  // Exact grid X for antialiasing
                        y   // Exact grid Y for antialiasing
                    );
                }
            }
        }
    }
    
    // Free the allocated memory
    FreeClusterTileArray(clusterTile, mapHeight);
    
    return (Vector2){ viewportX, viewportY };
}

Color GetTerrainColor(TerrainType type) {
    switch (type) {
        case TERRAIN_MOUNTAIN: return BROWN;
        case TERRAIN_WATER: return BLUE;
        case TERRAIN_GRASS: return GREEN;
        case TERRAIN_GROUND: return LIGHTGRAY;
        case TERRAIN_EMPTY: return LIGHTGRAY;
        case TERRAIN_BOSS: return RED;
        case TERRAIN_USEDGROUND: return DARKGRAY;
        case TERRAIN_CUTGRASS: return LIME;
        case TERRAIN_MOON: return RAYWHITE;
        case TERRAIN_ICE: return WHITE;
        case TERRAIN_MAGMA: return RED;
		case TERRAIN_POISON: return PURPLE;
		case TERRAIN_CLOUDS: return WHITE;
		case TERRAIN_WOOD: return DARKGREEN;
		case TERRAIN_UNDERWATER: return DARKBLUE;
        default: return GRAY;
    }
}


// Platform-independent directory file loading
int LoadTexturesFromDirectory(const char *folderPath, Texture2D textures[], int maxFrames, int resizeWidth, int resizeHeight) {
    int loadedCount = 0;
    
    printf("Loading textures from directory: %s\n", folderPath);
    
    FilePathList files = LoadDirectoryFiles(folderPath);
    printf("Found %d files in directory\n", files.count);
    
    for (int i = 0; i < files.count && loadedCount < maxFrames; i++) {
        printf("Examining file: %s\n", files.paths[i]);
        
        if (IsFileExtension(files.paths[i], ".png")) {
            printf("Loading PNG file: %s\n", files.paths[i]);
            
            Image img = LoadImage(files.paths[i]);
            if (img.data != NULL) {
                printf("Image loaded successfully, dimensions: %dx%d\n", img.width, img.height);
                
                if (resizeWidth > 0 && resizeHeight > 0) {
                    ImageResizeNN(&img, resizeWidth, resizeHeight);
                    printf("Image resized to %dx%d\n", resizeWidth, resizeHeight);
                }
                
                textures[loadedCount] = LoadTextureFromImage(img);
                UnloadImage(img);
                loadedCount++;
                
                printf("Texture loaded successfully (%d/%d)\n", loadedCount, maxFrames);
            } else {
                printf("Failed to load image: %s\n", files.paths[i]);
            }
        } else {
            printf("Skipping non-PNG file: %s\n", files.paths[i]);
        }
    }
    
    UnloadDirectoryFiles(files);
    printf("Total textures loaded: %d\n", loadedCount);
    
    return loadedCount;
}

// Get default properties for a terrain type
void GetTerrainDefaults(TerrainType type, float* drag, bool* walkable, Color* color, bool* isModified, int* cooldown, int* threshold) {
    *drag = 1.0f;
    *walkable = true;
    *color = GetTerrainColor(type);
    *isModified = false;
    *cooldown = 0;
    *threshold = 9999;
    
    switch (type) {
        case TERRAIN_MOUNTAIN:
            *walkable = false;
            break;
        case TERRAIN_WATER:
            *walkable = false;
            break;
        case TERRAIN_GRASS:
            *drag = 1.2f;
            *cooldown = GRASS_COOLDOWN_DURATION;
            break;
        case TERRAIN_GROUND:
            *drag = 1.2f;
            *cooldown = GROUND_COOLDOWN_DURATION;
            break;
        case TERRAIN_EMPTY:
            *threshold = GetRandomValue(300, 500);
            break;
        case TERRAIN_BOSS:
            break;
        case TERRAIN_USEDGROUND:
            *isModified = true;
            break;
        case TERRAIN_CUTGRASS:
            *isModified = true;
            break;
        case TERRAIN_MOON:
            *drag = 1.5f;
			*threshold = GetRandomValue(150, 300);
            break;
        case TERRAIN_ICE:
            *drag = 1.5f;
            *walkable = false;
			*threshold = GetRandomValue(150, 300);
            break;
        case TERRAIN_MAGMA:
            *drag = 1.5f;
            *walkable = false;
			*threshold = GetRandomValue(150, 300);
            break;
		case TERRAIN_POISON:
			*drag = 0.8f;          // Slows down player slightly
			*walkable = false;      // Can walk but takes damage
			*threshold = GetRandomValue(150, 300);
			break;
		case TERRAIN_CLOUDS:
			*drag = 2.0f;          // Significantly slower movement (wind resistance)
			*walkable = false;
			*threshold = GetRandomValue(200, 400);
			break;
        case TERRAIN_WOOD:
            *drag = 1.5f;
            *walkable = false;
			*threshold = GetRandomValue(300, 500);
            break;
		case TERRAIN_UNDERWATER:
			*drag = 0.5f;
			*walkable = false;
			*threshold = GetRandomValue(100, 200); // Higher encounter chance underwater
			break;
        default:
            break;
    }
}

// Load textures for terrain types
void LoadTerrainTextures(void) {
    const char* terrainFolders[NUM_TERRAINS] = {
        "./game/assets/terrain/mountain",
        "./game/assets/terrain/water",
        "./game/assets/terrain/grass",
        "./game/assets/terrain/ground",
        "./game/assets/terrain/empty",
        "./game/assets/terrain/boss",
        "./game/assets/terrain/groundUsed",
        "./game/assets/terrain/cutGrass",
        "./game/assets/terrain/moon",
        "./game/assets/terrain/ice",
        "./game/assets/terrain/magma",
        "./game/assets/terrain/poison",
        "./game/assets/terrain/clouds",
		"./game/assets/terrain/wood",
		"./game/assets/terrain/underWater",
        // Add paths for your other terrains
		// The special markers don't need texture folders as they're just for editor use
		NULL,
		NULL,
		NULL,
		NULL
    };

	printf("Starting to load terrain textures...\n");
    
    for (int i = 0; i < NUM_TERRAINS; i++) {
        // Initialize animation state
        terrainAnimations[i].frameCount = 0;
        terrainAnimations[i].frameCounter = 0;
        terrainAnimations[i].frameIndex = 0;
        
        // Try to load textures from the directory if it exists
		printf("Checking directory: %s\n", terrainFolders[i]);
        
        if (DirectoryExists(terrainFolders[i])) {
			printf("Directory exists: %s\n", terrainFolders[i]);
            terrainAnimations[i].frameCount = LoadTexturesFromDirectory( terrainFolders[i],  terrainAnimations[i].frames,  MAX_FRAMES,  0, 0 );
			printf("Loaded %d textures for terrain type %d\n", terrainAnimations[i].frameCount, i);
        } else {
            printf("Directory does not exist: %s\n", terrainFolders[i]);
        }
        
        // If no textures were loaded, create a default one
        if (terrainAnimations[i].frameCount == 0) {
            printf("Creating default texture for terrain type %d\n", i);
            Image img = GenImageChecked(TILE_SIZE, TILE_SIZE, TILE_SIZE/4, TILE_SIZE/4, GetTerrainColor(i), ColorAlpha(GetTerrainColor(i), 0.7f));
            terrainAnimations[i].frames[0] = LoadTextureFromImage(img);
            UnloadImage(img);
            terrainAnimations[i].frameCount = 1;
        }
    }
	printf("Finished loading terrain textures\n");
}

// InitializeTerrain
void SetTerrainAtPosition(int x, int y, TerrainType type) {
    if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
        float drag;
        bool walkable;
        Color color;
        bool isModified;
        int cooldown;
        int threshold;
        
        GetTerrainDefaults(type, &drag, &walkable, &color, &isModified, &cooldown, &threshold);
        
        worldMap[y][x].type = type;
        worldMap[y][x].drag = drag;
        worldMap[y][x].walkable = walkable;
        worldMap[y][x].color = color;
        worldMap[y][x].isModified = isModified;
        worldMap[y][x].cooldownTimer = cooldown;
        worldMap[y][x].encounterThreshold = threshold;

        worldMap[y][x].isAnimating = false;
        worldMap[y][x].animationTimer = 0;
        worldMap[y][x].animationDuration = 0;

		for (int i = 0; i < 16; i++) {
			worldMap[y][x].animatingSprites[i] = false;
			worldMap[y][x].spriteAnimTimers[i] = 0;
			worldMap[y][x].spriteDurations[i] = 0;
		}
    }
}



// Checks if the tile is "normally" walkable and whether
// the tool/party composition grants special walkability.
bool IsTileWalkable(int tileX, int tileY, Tool* selectedTool) {
    // 1) Retrieve the base terrain from the worldMap
    Terrain tile = worldMap[tileY][tileX];

    // 2) If the tile is already walkable in worldMap, we can return true immediately
    if (tile.walkable) return true;

    // 3) If tile is NOT walkable, check special conditions:
    
    // Condition A: The tool is a claw (makes anything walkable)
    if (selectedTool->type == TOOL_CLAW) {
        return true;
    }

    // Condition B: If the tile is water, see if the party has terrain == TERRAIN_WATER
    if (tile.type == TERRAIN_WATER && PartyHasWaterAccess()) {
        return true;
    }

    // Condition C: If the tile is mountain, see if the party has terrain == TERRAIN_MOUNTAIN
    if (tile.type == TERRAIN_MOUNTAIN && PartyHasMountainAccess()) {
        return true;
    }

    // If none of these conditions are met, it remains unwalkable
    return false;
}

// Decide how much drag to apply. 
float GetTileDrag(int tileX, int tileY, Tool* selectedTool) {
    Terrain tile = worldMap[tileY][tileX];
    
    // If the tile is overridden by the claw, or party's access to water/mountain,
    // we might reduce drag to 1.0. 
    if (selectedTool->type == TOOL_CLAW) {
        return 1.0f;
    }
    
    // If we have water access on water, maybe also do 1.0 or any logic you prefer
    if (tile.type == TERRAIN_WATER && PartyHasWaterAccess()) {
        return 1.0f;
    }
    // If we have mountain access on mountain
    if (tile.type == TERRAIN_MOUNTAIN && PartyHasMountainAccess()) {
        return 1.0f;
    }
    
    // Otherwise just return the tile's default
    return tile.drag;
}

bool PartyHasWaterAccess(void) {
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] != '\0') {
            for (int j = 0; j < mainTeam[i].terrainCount; j++) {
                if (mainTeam[i].terrains[j] == TERRAIN_WATER || mainTeam[i].creatureType == WATER_TYPE) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool PartyHasMountainAccess(void) {
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] != '\0') {
            for (int j = 0; j < mainTeam[i].terrainCount; j++) {
                if (mainTeam[i].terrains[j] == TERRAIN_MOUNTAIN || mainTeam[i].creatureType == ROCK_TYPE) {
                    return true;
                }
            }
        }
    }
    return false;
}


// Define a tile's world-space rectangle (x=2.0 means tile column 2)
Rectangle GetTileWorldRect(int tileX, int tileY) {
    return (Rectangle){
        .x = tileX,
        .y = tileY,
        .width = 1.0f, // 1 tile wide
        .height = 1.0f // 1 tile tall
    };
}



Rectangle GetPlayerWorldRect() {
    return (Rectangle){
        .x = mainTeam[0].x + PLAYER_OFFSET_X,
        .y = mainTeam[0].y + PLAYER_OFFSET_Y,
        .width = PLAYER_WIDTH,
        .height = PLAYER_HEIGHT
    };
}

bool CheckTileCollision(Tool *selectedTool) {
    Rectangle playerRect = GetPlayerWorldRect();
    
    // Get tiles the player might be overlapping
    int startX = (int)floorf(playerRect.x);
    int endX = (int)ceilf(playerRect.x + playerRect.width);
    int startY = (int)floorf(playerRect.y);
    int endY = (int)ceilf(playerRect.y + playerRect.height);

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;

            Rectangle tileRect = GetTileWorldRect(x, y);
            if (!IsTileWalkable(x,y,selectedTool) && CheckCollisionRecs(playerRect, tileRect)) 
            {
                return true; // Collision detected
            }
        }
    }
    return false;
}

void MovePlayer(float dx, float dy, bool BPressed) {
    float moveIncrease = BPressed ? moveIncrement * 2 : moveIncrement;

    Tool* selectedTool = &toolWheel.tools[toolWheel.selectedToolIndex];

    float targetX = mainTeam[0].x + dx * moveIncrease;
    float targetY = mainTeam[0].y + dy * moveIncrease;


    if (targetX < 0 || targetX >= mapWidth - 1 || targetY < 0 || targetY >= mapHeight - 1) {
        return;
    }

    int tileX  = (int)targetX;
    int tileY  = (int)targetY;
    explored[tileY][tileX] = true;

    int visionRange = 1;
    int newlyExplored = 0;

    for (int y = tileY - visionRange; y <= tileY + visionRange; y++) {
        for (int x = tileX - visionRange; x <= tileX + visionRange; x++) {
            if (x >= 0 && x < mapWidth && y >= 0 && y < mapHeight) {
                if (!explored[y][x]) {
                    explored[y][x] = true;
                    newlyExplored++; // Count each new tile
                }
            }
        }
    }

    if (newlyExplored > 0) {
        TrackAchievementMap(newlyExplored);
    }


    // Calculate adjacent tiles with boundary protection
    int tileXE = (int)ceilf(targetX);
    int tileYE = (int)ceilf(targetY);

        // Clamp adjacent tiles to map boundaries
    tileXE = tileXE >= mapWidth ? mapWidth - 1 : tileXE;
    tileYE = tileYE >= mapHeight ? mapHeight - 1 : tileYE;

    // Compute on-the-fly
    // bool walkablePrimary  = IsTileWalkable(tileX, tileY, selectedTool);
    // bool walkableExtended = IsTileWalkable(tileXE, tileYE, selectedTool);

    float dragPrimary  = GetTileDrag(tileX, tileY, selectedTool);
    // float dragExtended = GetTileDrag(tileXE, tileYE, selectedTool);

    // Tentative new position
    float newX = mainTeam[0].x + dx * moveIncrease;
    float newY = mainTeam[0].y + dy * moveIncrease;

    // Check collision at new position
    float oldX = mainTeam[0].x;
    float oldY = mainTeam[0].y;
    mainTeam[0].x = newX;
    mainTeam[0].y = newY;

    if (CheckTileCollision(selectedTool)) {
        // Collision! Revert position
        mainTeam[0].x = oldX;
        mainTeam[0].y = oldY;
        PlaySoundByName(&soundManager, "colision", false);
    }

    else {
        // If both are walkable, apply the drag from the primary tile
        // (or you could combine them somehow, but typically you just pick the main tile)
        float finalDrag = dragPrimary; 
        // If you want to combine them: float finalDrag = (dragPrimary + dragExtended) * 0.5f; etc.

        targetX = (targetX - mainTeam[0].x) * finalDrag + mainTeam[0].x;
        targetY = (targetY - mainTeam[0].y) * finalDrag + mainTeam[0].y;

        minimapNeedsUpdate = true;

        mainTeam[0].x = targetX;
        mainTeam[0].y = targetY;

        // Add steps to the terrain type the player is actually on
        TerrainType steppedType = worldMap[tileY][tileX].type; 
        mainTeam[0].steps[steppedType]++;

        // Keep a history
        playerHistory[historyIndex] = (Position){ targetX, targetY };
        historyIndex = (historyIndex + 1) % MAX_HISTORY;
    }


    // printf("targetX: %f, targetY: %f, tileX: %d, tileY: %d, extendedX: %d, extendedY: %d, \n",
        //    targetX, targetY, tileX, tileY, tileXE, tileYE);
}



// MAYBE ADD A TERRAIN PARAMETER FIX? BASICALLY IF IT CAN BE PUT ANYWHERE, IF SO NO ANTIALISAING
// OR MAYBE ITS BECAUSE ITS AN OVERLAID TERRAIN
void DrawAnimatedTile(TerrainAnimation *terrainAnim, int x, int y, int tileSize, Color color, Terrain *tile, int exactGridX, int exactGridY) {
    if (terrainAnim->frameCount <= 0) return;
    
    int textureWidth = terrainAnim->frames[0].width;
    int textureHeight = terrainAnim->frames[0].height;
    
    // Determine subdivisions
    int subdivisionsX = tileSize / textureWidth;
    int subdivisionsY = tileSize / textureHeight;
    
    if (subdivisionsX <= 0) subdivisionsX = 1;
    if (subdivisionsY <= 0) subdivisionsY = 1;
    
    // Special terrain types that have individual sprite animations
    TerrainType specialTerrains[] = {TERRAIN_GRASS, TERRAIN_WOOD, TERRAIN_MAGMA, TERRAIN_POISON, TERRAIN_ICE, TERRAIN_CLOUDS, TERRAIN_UNDERWATER};
    bool isSpecialTerrain = false;
    
    for (int i = 0; i < sizeof(specialTerrains)/sizeof(TerrainType); i++) {
        if (tile->type == specialTerrains[i]) {
            isSpecialTerrain = true;
            break;
        }
    }
    
    // Draw each sprite in the tile
    for (int subY = 0; subY < subdivisionsY; subY++) {
        for (int subX = 0; subX < subdivisionsX; subX++) {
            int spriteIndex = subY * subdivisionsX + subX;
            int frameToUse = 0; // Default to first frame
            
            // For special terrains, check if this specific sprite is animating
            if (isSpecialTerrain && tile->isAnimating) {
                if (tile->animatingSprites[spriteIndex]) {
                    // Get frame count for this terrain
                    int framesAvailable = terrainAnim->frameCount;
                    
                    // Only try to animate if we have frames
                    if (framesAvailable > 0) {
                        // Calculate which frame to use - divide by ticksPerFrame to slow down animation
                        int ticksPerFrame = 10;
                        int frameIndex = (tile->spriteAnimTimers[spriteIndex] / ticksPerFrame) % framesAvailable;
                        frameToUse = frameIndex;
                    }
                }
            } else if (!isSpecialTerrain) {
                // For regular terrain, use the current animation frame
                frameToUse = terrainAnim->frameIndex;
            }
            
            // Draw the sprite with the correct frame
            int posX = x + subX * textureWidth;
            int posY = y + subY * textureHeight;
            DrawTexture(terrainAnim->frames[frameToUse], posX, posY, color);
        }
    }
    
    // Use the exact grid coordinates passed to the function
    int gridX = exactGridX;
    int gridY = exactGridY;
    
    // Apply antialiasing at terrain borders - very narrow (1-2 pixel) transition
    if (gridX >= 0 && gridX < mapWidth && gridY >= 0 && gridY < mapHeight) {
        // Check four adjacent tiles (up, down, left, right)
        const int dx[] = {0, 0, -1, 1};
        const int dy[] = {-1, 1, 0, 0};
        const int edge[] = {0, 1, 2, 3}; // Top, Bottom, Left, Right in correct order
        
        for (int i = 0; i < 4; i++) {
            int nx = gridX + dx[i];
            int ny = gridY + dy[i];
            
            // Skip if out of bounds
            if (nx < 0 || nx >= mapWidth || ny < 0 || ny >= mapHeight)
                continue;
            
            // If neighbor has different terrain type, draw a subtle antialiased edge
            if (worldMap[ny][nx].type != worldMap[gridY][gridX].type) {
                Color currentColor = worldMap[gridY][gridX].color;
                Color neighborColor = worldMap[ny][nx].color;
                
                // Adjust alpha based on zoom level for better visual at different zoom levels
                int alpha = 120;
                if (alpha < 60) alpha = 60;
                if (alpha > 180) alpha = 180;
                
                // Create a blended color with partial transparency
                Color blendedColor = {
                    (currentColor.r + neighborColor.r) / 2,
                    (currentColor.g + neighborColor.g) / 2,
                    (currentColor.b + neighborColor.b) / 2,
                    alpha // Zoom-adaptive transparency
                };
                
                // Draw a very thin antialiased border (just 1-2 pixels wide)
                int borderWidth = 2;
                if (tileSize > 20) borderWidth = 2; // Use 2px for larger tiles
                
                if (edge[i] == 0) { // Top edge
                    DrawRectangle(x, y, tileSize, borderWidth, blendedColor);
                } else if (edge[i] == 1) { // Bottom edge
                    DrawRectangle(x, y + tileSize - borderWidth, tileSize, borderWidth, blendedColor);
                } else if (edge[i] == 2) { // Left edge
                    DrawRectangle(x, y, borderWidth, tileSize, blendedColor);
                } else { // Right edge
                    DrawRectangle(x + tileSize - borderWidth, y, borderWidth, tileSize, blendedColor);
                }
            }
        }
    }
}


void DrawAnimatedBackground(int y, int height) {
    srand(time(NULL));
    static float time = 0.0f;
    time += GetFrameTime();
    float randomNumber = rand() % 361;

    for (int i = 0; i < GetScreenWidth(); i++) {
        for (int j = y; j < y + height; j++) {
            float value = sinf(i * 0.05f * time) * cosf(j * 0.05f * time);
            // Color color = ColorFromHSV((value + 1.0f) * 270.0f, 0.7f, 0.7f);
            // Color color = ColorFromHSV((value + 1.0f) * randomNumber, 0.5f, 0.5f);
            Color color = ColorFromHSV((value + 1.0f) * randomNumber, 0.2f, 0.2f);
            DrawPixel(i, j, color);
        }
    }
}

void DestroyMountainNearby(PickaxeMinigameState *state) {
    Rectangle interactionArea = GetInteractionArea(&mainTeam[0]);
    bool destroyed = false;

    // Check all tiles in interaction area
    int startX = (int)floorf(interactionArea.x);
    int endX = (int)ceilf(interactionArea.x + interactionArea.width);
    int startY = (int)floorf(interactionArea.y);
    int endY = (int)ceilf(interactionArea.y + interactionArea.height);

    for(int y = startY; y < endY && !destroyed; y++) {
        for(int x = startX; x < endX && !destroyed; x++) {
            if(x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;

            Rectangle tileRect = GetTileWorldRect(x, y);
            if(CheckCollisionRecs(interactionArea, tileRect) &&
               (worldMap[y][x].type == TERRAIN_MOUNTAIN ||worldMap[y][x].type == TERRAIN_ICE)) {
                // Destroy the mountain
                worldMap[y][x] = (Terrain){
                    .type = TERRAIN_EMPTY,
                    .walkable = true,
                    .drag = 1.5f,
                    .color = BROWN,
                    .isModified = false,
                    .cooldownTimer = 0,
                    .encounterThreshold = IsKeyPressed(B) ? RANDOMS(80, 160) : RANDOMS(300, 500)
                };
                destroyed = true;
                printf("Mountain destroyed at (%d, %d)\n", x, y);
            }
        }
    }

    if(!destroyed) {
        printf("No mineable rock in front of player\n");
        SetOverworldMessage("Nothing to mine here!");
    }
}

// Combined terrain animation update function
void UpdateTerrainAnimation(void) {
    // Define the desired animation cycle time (in frames)
    const int ANIMATION_CYCLE_FRAMES = 60; // Full animation cycle in 1 second at 60 FPS
    
    // Global animation frame update
    static int frameDelay = 0;
    frameDelay++;
    
    // Update global animation frames every 10 frames
    if (frameDelay >= 10) {
        frameDelay = 0;
        
        for (int i = 0; i < NUM_TERRAINS; i++) {
            // Skip special terrains that are randomly animated
            if (i != TERRAIN_GRASS && i != TERRAIN_WOOD && i != TERRAIN_MAGMA && i != TERRAIN_POISON 
                && i != TERRAIN_ICE && i != TERRAIN_CLOUDS && i != TERRAIN_UNDERWATER) {
                
                // Get the number of frames for this terrain
                int frameCount = terrainAnimations[i].frameCount > 0 ? terrainAnimations[i].frameCount : 1;
                
                // For terrains with very few frames, advance the animation faster
                // This ensures all animations complete a full cycle in roughly the same time
                static float animTimers[NUM_TERRAINS] = {0};
                
                // Increment the timer for this terrain
                animTimers[i] += (float)ANIMATION_CYCLE_FRAMES / (frameCount * 10);
                
                // If we need to advance a frame
                if (animTimers[i] >= 1.0f) {
                    // Advance the frame
                    terrainAnimations[i].frameIndex = (terrainAnimations[i].frameIndex + 1) % frameCount;
                    
                    // Keep the fractional part for smooth timing
                    animTimers[i] = fmodf(animTimers[i], 1.0f);
                }
            }
        }
    }
    
    // Special terrains that should have random animation triggers with their probabilities (%)
    TerrainType specialTerrains[] = {TERRAIN_GRASS, TERRAIN_WOOD, TERRAIN_MAGMA, TERRAIN_POISON, TERRAIN_ICE, TERRAIN_CLOUDS, TERRAIN_UNDERWATER};
    int animationProbability[] = {100, 5, 5, 5, 5, 5, 5}; // Higher chance for grass, lower for others
    int numSpecialTerrains = sizeof(specialTerrains) / sizeof(TerrainType);
    
    // Update all existing animations
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            TerrainType type = worldMap[y][x].type;
            
            // Check if this is a special terrain type
            bool isSpecialTerrain = false;
            // int terrainIndex = -1;
            for (int i = 0; i < numSpecialTerrains; i++) {
                if (type == specialTerrains[i]) {
                    isSpecialTerrain = true;
                    // terrainIndex = i; // Store the index for probability reference
                    break;
                }
            }
            
            if (isSpecialTerrain) {
                // If already animating, update the sprite animations
                if (worldMap[y][x].isAnimating) {
                    worldMap[y][x].animationTimer++;
                    
                    // Update each sprite
                    for (int i = 0; i < 16; i++) {
                        if (worldMap[y][x].animatingSprites[i]) {
                            worldMap[y][x].spriteAnimTimers[i]++;
                            
                            // Check if this sprite's animation is complete
                            if (worldMap[y][x].spriteAnimTimers[i] >= worldMap[y][x].spriteDurations[i]) {
                                worldMap[y][x].animatingSprites[i] = false;
                            }
                        }
                    }
                    
                    // Check if all sprites have finished animating
                    bool allFinished = true;
                    for (int i = 0; i < 16; i++) {
                        if (worldMap[y][x].animatingSprites[i]) {
                            allFinished = false;
                            break;
                        }
                    }
                    
                    // If all are done, or main timer is up, stop animation
                    if (allFinished || worldMap[y][x].animationTimer >= worldMap[y][x].animationDuration) {
                        worldMap[y][x].isAnimating = false;
                    }
                }
            }
        }
    }
    
    // Check for new sprite animations (100 random checks per frame)
    for (int i = 0; i < 100; i++) {
        int x = GetRandomValue(0, mapWidth - 1);
        int y = GetRandomValue(0, mapHeight - 1);
        
        TerrainType tileType = worldMap[y][x].type;
        
        // Check if this tile is one of our special types
        bool isSpecialTerrain = false;
        int terrainIndex = -1;
        for (int j = 0; j < numSpecialTerrains; j++) {
            if (tileType == specialTerrains[j]) {
                isSpecialTerrain = true;
                terrainIndex = j; // Store index for probability reference
                break;
            }
        }
        
        if (isSpecialTerrain && terrainIndex >= 0) {
            // Choose a random sprite within the tile (0-15)
            int spriteIndex = GetRandomValue(0, 15);
            
            // Get animation probability for this terrain type
            int probability = animationProbability[terrainIndex];
            
            // If this sprite isn't already animating, check based on terrain-specific probability
            if (!worldMap[y][x].animatingSprites[spriteIndex]) {
                if (GetRandomValue(0, 100) < probability) {
                    // Get the animation for this terrain type
                    TerrainAnimation *anim = &terrainAnimations[tileType];
                    
                    // Only proceed if there are multiple frames to animate
                    if (anim->frameCount > 1) {
                        // Check if this is the first animating sprite for this tile
                        bool firstAnimation = !worldMap[y][x].isAnimating;
                        
                        // Make sure the tile is marked as animating
                        worldMap[y][x].isAnimating = true;
                        
                        // Reset the main animation timer if this is the first sprite
                        if (firstAnimation) {
                            worldMap[y][x].animationTimer = 0;
                            worldMap[y][x].animationDuration = GetRandomValue(300, 600);
                        }
                        
                        // Start this specific sprite's animation
                        worldMap[y][x].animatingSprites[spriteIndex] = true;
                        worldMap[y][x].spriteAnimTimers[spriteIndex] = 0;
                        
                        // Duration just needs to be long enough to play through the frame sequence once
                        int framesInSequence = anim->frameCount;
                        int ticksPerFrame = 10; // How many game ticks each frame is shown
                        worldMap[y][x].spriteDurations[spriteIndex] = framesInSequence * ticksPerFrame;
                    }
                }
            }
        }
    }
}



// Helper function to get interaction area based on direction
Rectangle GetInteractionArea(Creature *c) {
    Rectangle base = GetPlayerWorldRect();
    
    switch(c->anims.direction) {
        case UP:
            return (Rectangle){base.x, base.y - INTERACTIONRANGE, 
                              base.width, INTERACTIONRANGE};
        case DOWN:
            return (Rectangle){base.x, base.y + base.height, 
                              base.width, INTERACTIONRANGE};
        case LEFT:
            return (Rectangle){base.x - INTERACTIONRANGE, base.y, 
                              INTERACTIONRANGE, base.height};
        case RIGHT:
            return (Rectangle){base.x + base.width, base.y, 
                              INTERACTIONRANGE, base.height};
        default:
            return base;
    }
}


bool CheckTerrainNearPlayerArea(TerrainType type) {
    Rectangle interactionArea = GetInteractionArea(&mainTeam[0]);
    bool validTerrain = false;

    // Check all tiles overlapping interaction area
    int startX = (int)floorf(interactionArea.x);
    int endX = (int)ceilf(interactionArea.x + interactionArea.width);
    int startY = (int)floorf(interactionArea.y);
    int endY = (int)ceilf(interactionArea.y + interactionArea.height);

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;
            
            Rectangle tileRect = GetTileWorldRect(x, y);
            if (CheckCollisionRecs(interactionArea, tileRect) &&  worldMap[y][x].type == type) {
                validTerrain = true;
                return validTerrain;
                break;
            }
        }
        if (validTerrain) {
            return validTerrain;
            break;
        }
    }
    return validTerrain;
}

// Returns found position or (-1,-1) if not found
Vector2 CheckTerrainInPlayerArea(TerrainType type) {
    Rectangle playerArea = GetInteractionArea(&mainTeam[0]);
    
    int startX = (int)floorf(playerArea.x);
    int endX = (int)ceilf(playerArea.x + playerArea.width);
    int startY = (int)floorf(playerArea.y);
    int endY = (int)ceilf(playerArea.y + playerArea.height);

    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;
            
            if (worldMap[y][x].type == type) {
                return (Vector2){x, y}; // Return tile coordinates
            }
        }
    }
    printf("NOT VALID");
    return (Vector2){-1, -1}; // Invalid position
}


void InitializeFrameTracking(FrameTracking *state) {
    state->currentFrame = 0;
    state->frameCounter = 0;
    state->idleFrameSpeed = RANDOMS(80, 120);
    state->walkFrameSpeed = 8;
    state->transitionFrameCounter = 0; // Counter for transition frames
    state->stopDelayCounter = 0; // Delay counter for showing stopping frame
    state->increasingFrames = true;
}

void UpdateTileCooldowns(void) {
    for (int y = 0; y < mapHeight; y++) {
        for (int x = 0; x < mapWidth; x++) {
            Terrain *tile = &worldMap[y][x];
            if (tile->isModified) {
                if (tile->cooldownTimer > 0) {
                    tile->cooldownTimer--;
                } else {
                    // Revert terrain type and dug state
                    tile->isModified = false;
                    if (tile->type == TERRAIN_USEDGROUND) tile->type = TERRAIN_GROUND;
                    else if (tile->type == TERRAIN_CUTGRASS) tile->type = TERRAIN_GRASS;
                }
            }
           
        }
    }
}



void UpdatePlayerAnimation(Creature* creature, FrameTracking* frameTracking) {
    // Increment frame counter
    frameTracking->frameCounter++;
    
    // Handle different movement states
    switch (creature->anims.movementState) {
        case WALKING:
            // Get frame count for current direction
            int maxFrames = creature->anims.walkFrameCount[creature->anims.direction];
            
            if (creature->anims.isMoving) {
                // Active walking - cycle through frames
                if (frameTracking->frameCounter >= frameTracking->walkFrameSpeed) {
                    frameTracking->frameCounter = 0;
                    frameTracking->currentFrame = (frameTracking->currentFrame + 1) % maxFrames;
                }
            } else if (creature->anims.isStopping) {
                // Stopping animation - show frame 0
                frameTracking->currentFrame = 0;
                
                // After stopDelayCounter expires, will transition to IDLE in the movement function
            }
            break;
            
        // Handle other states as needed (FISHING, PICKAXE, etc.)
        case IDLE:
        default:
            // Idle animation (loop through frames at idleFrameSpeed rate)
            if (frameTracking->frameCounter >= frameTracking->idleFrameSpeed) {
                frameTracking->frameCounter = 0;
                
                // Oscillate between frames for idle animation
                if (frameTracking->increasingFrames) {
                    frameTracking->currentFrame++;
                    if (frameTracking->currentFrame >= creature->anims.idleFrameCount - 1) {
                        frameTracking->increasingFrames = false;
                    }
                } else {
                    frameTracking->currentFrame--;
                    // if (frameTracking->currentFrame <= 0) {
                    if (frameTracking->currentFrame < 0) {
                        frameTracking->currentFrame = 0; // Reset to 0 if we went negative
                        frameTracking->increasingFrames = true;
                    }
                }
            }
            break;
    }
}

Texture2D SpriteTexturing(Texture2D texture, int sizex, int sizey) {
    Image sprite = LoadImageFromTexture(texture);
    if (sprite.data == NULL) {
        fprintf(stderr, "Failed to load image from texture");
    }
    ImageResizeNN(&sprite, sizex, sizey);
    Texture2D resizedSprite = LoadTextureFromImage(sprite);
    UnloadImage(sprite);

    return resizedSprite;
}



void LoadAnimationFrames(Animation* animation, const char* folderPath, int resizeWidth, int resizeHeight) {
    animation->frameCount = 0;

    // Attempt to load frames
    animation->frameCount = LoadTexturesFromDirectory(folderPath, animation->frames, MAX_FRAMES, resizeWidth, resizeHeight);

    if (animation->frameCount == 0) {
        fprintf(stderr, "Warning: No animation frames loaded for path '%s'\n", folderPath);
        // Optionally set a default animation or leave as is
    }

    animation->currentFrame = 0;
    animation->frameSpeed = 1;
    animation->frameCounter = 0;
    animation->isAnimating = false;
    animation->color = WHITE;
}

void LoadAllAnimations() {
    struct AnimationConfig {
        Animation* target;
        const char* path;
        int width;
        int height;
    };

    struct AnimationConfig configs[] = {
        {&animations.attack,    ATTACK_ANIMATION,    TILE_SIZE, TILE_SIZE},
        {&animations.potion,    POTION_ANIMATION,    0, 0},
        {&animations.explosion, EXPLOSION_ANIMATION, 0, 0},
        {&animations.nade,      NADE_ANIMATION,      0, 0}
    };

    for (size_t i = 0; i < sizeof(configs)/sizeof(configs[0]); i++) {
        LoadAnimationFrames(configs[i].target, configs[i].path, 
                           configs[i].width, configs[i].height);
    }
}

void UnloadAllAnimations() {
    Animation* all[] = {
        &animations.attack, &animations.potion,
        &animations.explosion, &animations.nade
    };

    for (size_t i = 0; i < sizeof(all)/sizeof(all[0]); i++) {
        for (int j = 0; j < all[i]->frameCount; j++) {
            UnloadTexture(all[i]->frames[j]);
        }
        all[i]->frameCount = 0; // Mark as unloaded
    }
}

Texture2D LoadFirstTextureFromDirectory(const char *filePath, int resizeWidth, int resizeHeight) {
    Texture2D texture = {0};
    Image image = LoadImage(filePath);

    if (image.data != NULL) {
        if (resizeWidth > 0 && resizeHeight > 0) {
            ImageResizeNN(&image, resizeWidth, resizeHeight);
        }

        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        return texture; // Return after loading the first texture
    } 
    else {
        fprintf(stderr, "Failed to load image: %s\n", filePath);
    }

    return texture;
}


// ============ MAIN OVERWORLD UPDATE FUNCTION ============

bool UpdateBombAnimation(void) {
    if (!menu.bombAnimation.isAnimating) {
        return false; // Animation not playing
    }

    menu.bombAnimation.frameCounter++;
    if (menu.bombAnimation.frameCounter >= menu.bombAnimation.frameSpeed) {
        menu.bombAnimation.frameCounter = 0;
        menu.bombAnimation.currentFrame++;
        if (menu.bombAnimation.currentFrame >= menu.bombAnimation.frameCount) {
            // Animation done
            menu.bombAnimation.isAnimating = false;

            // 2) Now transform the terrain if we have a pending bomb
            // Inside the main loop's GAME_STATE_OVERWORLD case
            if (menu.bombPending.active) {
                int destroyedCount = 0;
                if (menu.bombPending.isExplosion) {
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            int tx = menu.bombPending.centerX + dx;
                            int ty = menu.bombPending.centerY + dy;
                            if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight) {
                                if (worldMap[ty][tx].type == TERRAIN_MOUNTAIN || worldMap[ty][tx].type == TERRAIN_ICE) {
                                    worldMap[ty][tx].type = TERRAIN_EMPTY;
                                    worldMap[ty][tx].walkable = true;
                                    worldMap[ty][tx].drag = 1.5f;
                                    destroyedCount++;
                                    TrackMountainBroken();
                                    PlaySoundByName(&soundManager,"bomb",true);
                                }
                            }
                        }
                    }
                    SetOverworldMessage(destroyedCount > 0 ? "Boom! The explosion cleared the area." : "The explosion had no effect.");
                } else {
                    int tx = menu.bombPending.centerX;
                    int ty = menu.bombPending.centerY;
                    if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight) {
                        if (worldMap[ty][tx].type == TERRAIN_MOUNTAIN || worldMap[ty][tx].type == TERRAIN_ICE) {
                            worldMap[ty][tx].type = TERRAIN_EMPTY;
                            worldMap[ty][tx].walkable = true;
                            worldMap[ty][tx].drag = 1.5f;
                            destroyedCount++;
                            TrackMountainBroken();
                            PlaySoundByName(&soundManager,"bomb",false);
                            SetOverworldMessage("Boom! The mountain was destroyed.");
                        } else {
                            SetOverworldMessage("The nade had no effect.");
                        }
                    }
                }

                // Decrement item count only if tiles were destroyed
                if (destroyedCount > 0 && menu.bombPending.item != NULL) {
                    // menu.bombPending.item->amount--;
                    RemoveItemFromInventory(menu.bombPending.item->id,1);
                    if (menu.bombPending.item->amount < 0) {
                        menu.bombPending.item->amount = 0;
                    }
                }

                menu.bombPending.active = false;
                menu.bombPending.item = NULL; // Clear the item reference
            }
        }
    }
    // Update terrain animations
    UpdateTerrainAnimations();
    
    return true; // Animation is playing
}

// Update all terrain-related animations
void UpdateTerrainAnimations(void) {
    UpdateTileCooldowns();
    UpdateTerrainAnimation();
}

// Handle player movement and followers
void UpdatePlayerAndFollowers(void) {
    TrackButtonPresses();
    HandlePlayerMovement(&mainTeam[0], &frameTracking);
    UpdateFollowers(&mainTeam[0], &frameTracking);
    UpdatePlayerAnimation(&mainTeam[0], &frameTracking);
}


// Check if player should trigger boss battle
bool CheckBossBattleTrigger(bool APressed, BossBattleData* bossBattleData) {
    // Check friendship condition
    if (IsKeyPressed(KEY_F) || (mainTeam[0].completeFriendship && !bossBattleData->bossBattleTriggered)) {
        currentGameState = GAME_STATE_BOSS_BATTLE_TRANSITION;
        return true;
    }
    
    // Check shiny marker
    if (bossBattleData->shinyMarkerActive && 
        (int)mainTeam[0].x == (int)bossBattleData->shinyMarkerPosition.x && 
        (int)mainTeam[0].y == (int)bossBattleData->shinyMarkerPosition.y) {
        printf("\n\n shiny is active and we are on top of it \n\n");
        
        if (APressed) {
            currentGameState = GAME_STATE_BOSS_BATTLE_TRANSITION;
            justTransitioned = true;
            return true;
        }
    }
    
    return false;
}

// Handle debug functionality
void HandleDebugFeatures(BattleState* battleState) {
    if (IsKeyPressed(KEY_G)) {
        // Debug feature: Unlock all magic and skills for main creature
        printf("\n--- DEBUG: Unlocking all magic and skills ---\n");
        Creature* mainCreature = &mainTeam[0];
        int skillsAdded = 0;
        int magicAdded = 0;
        
        // Unlock all skills
        for (int i = 0; i < skillCount; i++) {
            if (!CreatureHasSkill(mainCreature, skillList[i].id)) {
                if (mainCreature->skillCount < MAX_SKILL_PER_CREATURE) {
                    mainCreature->availableSkillIDs[mainCreature->skillCount++] = skillList[i].id;
                    skillList[i].isUnlocked = true;  // Make sure it's marked as unlocked
                    skillsAdded++;
                } else {
                    printf("Warning: Max skills reached, couldn't add more\n");
                    break;
                }
            }
        }
        
        // Unlock all magic
        for (int i = 0; i < magicCount; i++) {
            if (!CreatureHasMagic(mainCreature, magicList[i].id)) {
                if (mainCreature->magicCount < MAX_MAGIC_PER_CREATURE) {
                    mainCreature->availableMagicIDs[mainCreature->magicCount++] = magicList[i].id;
                    magicList[i].isUnlocked = true;  // Make sure it's marked as unlocked
                    magicAdded++;
                } else {
                    printf("Warning: Max magic reached, couldn't add more\n");
                    break;
                }
            }
        }
        
        SetOverworldMessage(TextFormat("DEBUG: Added %d skills and %d magic spells", skillsAdded, magicAdded));
        printf("Added %d skills and %d magic spells to %s\n", skillsAdded, magicAdded, mainCreature->name);
        printf("------------------------------------\n");
    }
    
    if (IsKeyPressed(KEY_D) || RandomEncounterOnEmpty()) { 
        currentGameState = GAME_STATE_BATTLE_TRANSITION;
        battleState->terrain = TERRAIN_EMPTY;
        ScreenBreakingAnimation();
    }
}


// Main function that updates all overworld elements
void UpdateOverworld(bool APressed, bool LPressed, bool RPressed, BossBattleData* bossBattleData,BattleState* battleState) {
    
    // Update bomb animations if active
    if (UpdateBombAnimation()) {
        return; // Skip other updates if animation is playing
    }
    
    // Player movement and followers
    UpdatePlayerAndFollowers();
    
    // Minimap
    UpdateMinimap();
    
    // Terrain animations
    UpdateTerrainAnimations();
    
    // Check for friendship completion
    HasBefriendedAllFieldTypes();
    
    // Boss battle triggers
    if (CheckBossBattleTrigger(APressed, bossBattleData)) {
        return; // Skip other updates if transitioning to boss battle
    }
    
    // Debug features
    HandleDebugFeatures(battleState);
    
    // Check for menu transition
    if (IsKeyPressed(KEY_ENTER)) {
        currentGameState = GAME_STATE_MENU;
        menu.currentMenu = MENU_MAIN;
        return;
    }
    
    if (currentMap == MAP_OVERWORLD_HOLE) {
        if (CheckHoleStatus()) return;
    }
    
    // Update tool wheel
    UpdateToolWheel(LPressed, RPressed);
    
    // Handle tool usage
    HandleToolUsage(APressed, battleState);
}



// ============ PLAYER AND FOLLOWER DRAWING ============

// Draw the player character
// void DrawPlayer(int playerScreenX, int playerScreenY) {
//     if (mainTeam[0].anims.movementState == WALKING) {
//         DrawTexture(mainTeam[0].anims.walkTextures[mainTeam[0].anims.direction][frameTracking.currentFrame], 
//                     playerScreenX, playerScreenY, WHITE);
//     }
//     else {
//         DrawTexture(mainTeam[0].anims.idleTextures[frameTracking.currentFrame], 
//                     playerScreenX, playerScreenY, WHITE);
//     }
// }

void DrawPlayer(int screenX, int screenY) {
    Texture2D currentFrameTexture = {0}; // Default to empty

    if (mainTeam[0].anims.movementState == IDLE) {
        // Ensure currentFrame is within bounds for idle animation
        int frameIndex = frameTracking.currentFrame % mainTeam[0].anims.idleFrameCount;
        if (frameIndex < 0) frameIndex = 0; // Safety check
        currentFrameTexture = mainTeam[0].anims.idleTextures[frameIndex];
    } else if (mainTeam[0].anims.movementState == WALKING) {
        // Ensure currentFrame is within bounds for walk animation in the current direction
        int frameIndex = frameTracking.currentFrame % mainTeam[0].anims.walkFrameCount[mainTeam[0].anims.direction];
         if (frameIndex < 0) frameIndex = 0; // Safety check
        currentFrameTexture = mainTeam[0].anims.walkTextures[mainTeam[0].anims.direction][frameIndex];
    }
    // Add other states like fishing, digging etc. if needed

    if (currentFrameTexture.id != 0) {
        DrawTexture(currentFrameTexture, screenX, screenY, WHITE);
    } else {
        // Draw placeholder if no texture found (optional debug)
        DrawRectangle(screenX, screenY, TILE_SIZE, TILE_SIZE, PINK);
    }
}


// Draw all followers
// Updated DrawFollowers function to properly handle aggressive AI
void DrawFollowers(Vector2 viewport) {
    for (int i = 0; i < followerCount; i++) {
        if (followers[i].creature->currentHP <= 0) continue;
        
        int followerScreenX = (followers[i].creature->x - viewport.x) * TILE_SIZE;
        int followerScreenY = (followers[i].creature->y - viewport.y) * TILE_SIZE;
        
        // Get appropriate direction to use for drawing
        Direction displayDirection = followers[i].lastDirection;
        
        // Validate direction index
        if (displayDirection < 0 || displayDirection >= NUM_DIRECTIONS) {
            displayDirection = DOWN; // Use a safe default
            printf("Warning: Invalid direction %d for follower %d\n", 
                  displayDirection, i);
        }
        
        // Validate frame index based on direction
        int frameToUse = followers[i].lastFrame;
        int maxFrames;
        
        if (followers[i].creature->anims.movementState == WALKING) {
            maxFrames = followers[i].creature->anims.walkFrameCount[displayDirection];
            if (frameToUse >= maxFrames) frameToUse = 0;
            
            if (maxFrames > 0 && followers[i].creature->anims.walkTextures[displayDirection][frameToUse].id != 0) {
                DrawTexture(followers[i].creature->anims.walkTextures[displayDirection][frameToUse], 
                           followerScreenX, followerScreenY, WHITE);
            } else {
                // Fallback: draw idle texture if walk texture is invalid
                DrawTexture(followers[i].creature->anims.idleTextures[0], 
                           followerScreenX, followerScreenY, WHITE);
            }
        } else {
            maxFrames = followers[i].creature->anims.idleFrameCount;
            if (frameToUse >= maxFrames) frameToUse = 0;
            
            if (maxFrames > 0 && followers[i].creature->anims.idleTextures[frameToUse].id != 0) {
                DrawTexture(followers[i].creature->anims.idleTextures[frameToUse], 
                           followerScreenX, followerScreenY, WHITE);
            } else {
                // Fallback: draw a simple rectangle if no valid textures
                DrawRectangle(followerScreenX, followerScreenY, TILE_SIZE, TILE_SIZE, BLUE);
            }
        }
    }
}

// ============ UI ELEMENT DRAWING ============

// Draw player state information
void DrawPlayerState(bool BPressed) {
    if (BPressed && mainTeam[0].anims.isMoving) {
        DrawText("State: RUNNING", 10, 90, 20, YELLOW); 
    } else if (mainTeam[0].anims.isMoving) {
        DrawText("State: MOVING", 10, 90, 20, BLUE);
    } else if (mainTeam[0].anims.isStopping) {
        DrawText("State: STOPPING", 10, 90, 20, ORANGE);
    } else if (mainTeam[0].anims.movementState == IDLE) {
        DrawText("State: IDLE", 10, 90, 20, DARKGRAY);
    }
}

// Draw debug information
void DrawDebugInfo(float playerX, float playerY, int terrainType) {
    DrawText(TextFormat("X, Y, T: (%.1f, %.1f, %s)", playerX, playerY, TerrainTypeToString(terrainType)), 
             420, 75, 16, BLACK);
}

// Draw player stats
void DrawPlayerStats(void) {
    DrawText(TextFormat("GLvl: %d", mainTeam[0].gridLevel), 10, 120, 20, BLACK);
    DrawText(TextFormat("HP: %d/%d", mainTeam[0].currentHP, mainTeam[0].maxHP), 10, 140, 20, BLACK);
    DrawText(TextFormat("EXP: %d/%d", mainTeam[0].exp, GetExpForNextGridLevel(mainTeam[0].totalGridLevel)), 10, 160, 20, BLACK);
    DrawText(TextFormat("Attack: %d", mainTeam[0].attack), 10, 180, 20, BLACK);
    DrawText(TextFormat("Defense: %d", mainTeam[0].defense), 10, 200, 20, BLACK);
}

// Draw tool wheel if active
void DrawToolWheelIfActive(void) {
    if (toolWheel.timer > 0) {
        DrawToolWheel(&toolWheel);
    }
}

// ============ SPECIAL EFFECTS DRAWING ============

// Draw shiny marker for boss battles
void DrawShinyMarker(Vector2 viewport, BossBattleData* bossBattleData) {
    if (bossBattleData->shinyMarkerActive) {
        int markerScreenX = (bossBattleData->shinyMarkerPosition.x - viewport.x) * TILE_SIZE;
        int markerScreenY = (bossBattleData->shinyMarkerPosition.y - viewport.y) * TILE_SIZE;
        
        DrawAnimatedTile(&terrainAnimations[TERRAIN_BOSS], 
                          markerScreenX, markerScreenY, 
                          TILE_SIZE, WHITE,
                          &worldMap[(int)bossBattleData->shinyMarkerPosition.y][(int)bossBattleData->shinyMarkerPosition.x], 
                          bossBattleData->shinyMarkerPosition.x, bossBattleData->shinyMarkerPosition.y);
    }
}

// Draw bomb animation if active
void DrawBombAnimation(Vector2 viewport) {
    if (!menu.bombAnimation.isAnimating) {
        return;
    }
    
    Texture2D currentFrame = menu.bombAnimation.frames[menu.bombAnimation.currentFrame];
    
    float scale = 2.0f;  // or your chosen scale
    float halfWidth  = (currentFrame.width  * scale) / 2.0f;
    float halfHeight = (currentFrame.height * scale) / 2.0f;
    
    if (menu.bombPending.isExplosion) {
        // Draw explosion animation on all 3x3 tiles
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int tx = menu.bombPending.centerX + dx;
                int ty = menu.bombPending.centerY + dy;
                if (tx >= 0 && tx < mapWidth && ty >= 0 && ty < mapHeight) {
                    if (worldMap[ty][tx].type == TERRAIN_MOUNTAIN || worldMap[ty][tx].type == TERRAIN_ICE) {
                        int bx = (tx - viewport.x) * TILE_SIZE;
                        int by = (ty - viewport.y) * TILE_SIZE;
                        float tileCenterX = bx + (TILE_SIZE / 2.0f);
                        float tileCenterY = by + (TILE_SIZE / 2.0f);
                        DrawTextureEx(currentFrame, 
                                      (Vector2){tileCenterX - halfWidth, tileCenterY - halfHeight}, 
                                      0.0f, scale, menu.bombAnimation.color);
                    }
                }
            }
        }
    } 
    else {
        // Draw Nade animation on the single target tile
        int bx = (menu.bombPending.centerX - viewport.x) * TILE_SIZE;
        int by = (menu.bombPending.centerY - viewport.y) * TILE_SIZE;
        float tileCenterX = bx + (TILE_SIZE / 2.0f);
        float tileCenterY = by + (TILE_SIZE / 2.0f);
        DrawTextureEx(currentFrame, 
                      (Vector2){tileCenterX - halfWidth, tileCenterY - halfHeight}, 
                      0.0f, scale, menu.bombAnimation.color);
    }
}

// Draw the tunnel pattern if active
void DrawTunnelPattern(Vector2 viewport) {
    // If you implement the tunnel pattern feature, add its drawing code here
}

// ============ MAIN OVERWORLD DRAWING FUNCTION ============

// Main function to draw all overworld elements
void DrawOverworld(bool BPressed, bool keysPressed[], BossBattleData* bossBattleData, Camera2D camera) {
    // Draw the map and get viewport
    Vector2 viewport = DrawMap();
    
    // Set up camera
    camera.target = (Vector2){ mainTeam[0].x * TILE_SIZE, mainTeam[0].y * TILE_SIZE };
    camera.offset = (Vector2){ SCREEN_WIDTH/2.0f, SCREEN_HEIGHT/2.0f };
    
    // Calculate player screen position
    int playerScreenX = (mainTeam[0].x - viewport.x) * TILE_SIZE;
    int playerScreenY = (mainTeam[0].y - viewport.y) * TILE_SIZE;
    
    // Draw characters
    DrawPlayer(playerScreenX, playerScreenY);
    DrawFollowers(viewport);
    
    // Draw special effects
    DrawShinyMarker(viewport,bossBattleData);
    DrawBombAnimation(viewport);
    DrawTunnelPattern(viewport);
    
    // Draw UI elements
    // DrawPlayerState(BPressed);
    // DrawDebugInfo(mainTeam[0].x, mainTeam[0].y, worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].type);
    DrawKeyRepresentation(keysPressed);
    // DrawPlayerStats();
    DrawToolWheelIfActive();
    DrawOverworldMessages();
    
    // Draw minimap (commented out in your code)
    DrawMinimap(&mainTeam[0], camera);
}

// Draw any custom map elements specific to a particular map
// void DrawCustomMapElements(GameState mapState, Vector2 viewport) {
//     // Add custom drawing code for specific map states
//     switch (mapState) {
//         case GAME_STATE_OVERWORLD_TUNNEL:
//             // Draw tunnel-specific elements
//             // For example, you can draw timer, tile counts, etc.
//             break;
            
//         case GAME_STATE_OVERWORLD_CAVE:
//             // Draw cave-specific elements
//             break;
            
//         default:
//             // No custom elements for this map state
//             break;
//     }
// }


// void SetHoleWorldMap(void) {
//     Vector2 coordinateToSet[8] = {{37,16}, {36,17}, {40,16}, {41,17}, {41,20}, {40,21}, {37,21}, {36,20}};
//     for (int i = 0; i < 8; i++) {
//         SetTerrainAtPosition(coordinateToSet[i].x, coordinateToSet[i].y, TERRAIN_GROUND);
//     }
// }


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////////////////////    MINIMAP    ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



void InitMinimap(void) {
    miniMapRenderTexture = LoadRenderTexture(MINIMAP_TEXTURE_SIZE, MINIMAP_TEXTURE_SIZE);
    BeginTextureMode(miniMapRenderTexture);
        ClearBackground(BLANK);
    EndTextureMode();
}

void RefreshMinimap(Creature *player) {
    BeginTextureMode(miniMapRenderTexture);
        ClearBackground(BLANK);

        float visibleWidth, visibleHeight, startX, startY;

        if (showFullMap) {
            // Show the entire map:
            startX = 0;
            startY = 0;
            visibleWidth = (float)mapWidth;
            visibleHeight = (float)mapHeight;
        } else {
            // Original adaptive zoom calculation (unchanged)
            float mapDiagonal = sqrtf(mapWidth * mapWidth + mapHeight * mapHeight);
            float zoom = mapDiagonal / 500.0f;
            if (zoom > 4.0f) zoom = 4.0f;
            if (zoom < 0.5f) zoom = 0.5f;

            visibleWidth = MINIMAP_TEXTURE_SIZE * zoom;
            visibleHeight = MINIMAP_TEXTURE_SIZE * zoom;

            // Center view on player
            startX = player->x - visibleWidth / 2;
            startY = player->y - visibleHeight / 2;
        }

        // Clamp start positions to map boundaries
        startX = fmaxf(startX, 0);
        startY = fmaxf(startY, 0);
        startX = fminf(startX, mapWidth - visibleWidth);
        startY = fminf(startY, mapHeight - visibleHeight);

        // Draw map tiles into the render texture
        for (int y = 0; y < MINIMAP_TEXTURE_SIZE; y++) {
            for (int x = 0; x < MINIMAP_TEXTURE_SIZE; x++) {
                // Calculate world coordinates with proper scaling
                float u = (float)x / (MINIMAP_TEXTURE_SIZE - 1); // Normalized coordinate [0, 1]
                float v = (float)y / (MINIMAP_TEXTURE_SIZE - 1); // Normalized coordinate [0, 1]

                int worldX = (int)(startX + u * visibleWidth);
                int worldY = (int)(startY + v * visibleHeight);

                // Clamp worldX and worldY to ensure they stay within map bounds
                worldX = fminf(worldX, mapWidth - 1);
                worldY = fminf(worldY, mapHeight - 1);

                if (worldX >= 0 && worldX < mapWidth && worldY >= 0 && worldY < mapHeight) {
                    // Only render explored tiles
                    if (explored[worldY][worldX]) {
                        Color tileColor = worldMap[worldY][worldX].color;
                        tileColor = ColorAlpha(tileColor, 0.9f);
                        DrawPixel(x, y, tileColor);
                    } else {
                        // Render unexplored tiles as black or with a fog effect
                        DrawPixel(x, y, BLACK);
                    }
                }
            }
        }

        // Draw player indicator
        float px = (player->x - startX) / visibleWidth * MINIMAP_TEXTURE_SIZE;
        float py = (player->y - startY) / visibleHeight * MINIMAP_TEXTURE_SIZE;
        if (frameTracking.minimapBlinkCounter++ % 30 < 15) {
            DrawRectangle(px, py, 2, 2, YELLOW);
        }
    EndTextureMode();
}

void DrawMinimap(Creature *player, Camera2D camera) {

    // Access the global toolWheel struct (assuming it's global based on usage elsewhere)
    Tool* selectedTool = &toolWheel.tools[toolWheel.selectedToolIndex];
    if (selectedTool != NULL && selectedTool->type == TOOL_CLAW) {
         // If the selected tool is the Claw, do not draw the minimap
         return;
    }


    // Define the two candidate minimap positions (in pixels)
    Rectangle minimapRectTop = {
        MINIMAP_PADDING,
        MINIMAP_PADDING,
        MINIMAP_SCREEN_WIDTH,
        MINIMAP_SCREEN_HEIGHT
    };
    Rectangle minimapRectBottom = {
        MINIMAP_PADDING,
        GetScreenHeight() - MINIMAP_SCREEN_HEIGHT - MINIMAP_PADDING,
        MINIMAP_SCREEN_WIDTH,
        MINIMAP_SCREEN_HEIGHT
    };

    Rectangle minimapDest;

    // Otherwise, choose based on the player's position.
    float thresholdY = MINIMAP_SCREEN_HEIGHT + MINIMAP_PADDING; // extra margin to avoid flickering
    float thresholdX = MINIMAP_SCREEN_WIDTH + MINIMAP_PADDING;  // extra margin
    if (mainTeam[0].y * TILE_SIZE - MINIMAP_PADDING * 4 > thresholdY ||
        mainTeam[0].x * TILE_SIZE - MINIMAP_PADDING * 4 > thresholdX) {
        minimapDest = minimapRectTop;
    } else {
        minimapDest = minimapRectBottom;
    }


    DrawTexturePro(
        miniMapRenderTexture.texture,
        (Rectangle){0, 0, MINIMAP_TEXTURE_SIZE, -MINIMAP_TEXTURE_SIZE},
        minimapDest,
        (Vector2){0, 0},
        0.0f,
        WHITE
    );

    // Draw a border (optional)
    DrawRectangleLinesEx(
        (Rectangle){minimapDest.x - 2, minimapDest.y - 2,
                    minimapDest.width + 4, minimapDest.height + 4},
        2,
        ColorAlpha(GOLD, 0.7f)
    );
}

// Update minimap if needed
void UpdateMinimap(void) {
    if (minimapNeedsUpdate) {
        RefreshMinimap(&mainTeam[0]);
        minimapNeedsUpdate = false;
    }

    if (IsKeyPressed(KEY_C)) {
        showFullMap = !showFullMap;
        RefreshMinimap(&mainTeam[0]);
        minimapNeedsUpdate = false;
    }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////////////////////    TOOLS    //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////


// Initialize the tool wheel with all tools
void InitializeToolWheel(ToolWheel* wheel) {
    wheel->selectedToolIndex = 0; // Default tool is claw
    wheel->visibleToolCount = 6;
    wheel->userExplicitlySelected = false;
    wheel->timer = 0; // Initialize timer too

    // --- Initialize new fields ---
    wheel->clawSwitchChargeA = 0.0f;
    wheel->clawSwitchChargeS = 0.0f;
    wheel->isHoldingA = false;
    wheel->isHoldingS = false;
    
    // Define the tools
    wheel->tools[TOOL_CLAW] = (Tool){
        .type = TOOL_CLAW,
        .name = "Claw",
        .imagePath = "./game/assets/tools/claw.png",
        .usableTerrain = TERRAIN_EMPTY, // Can be used anywhere
        .texture = {0},
        .isImprovedVersion = false,
        .isUnlocked = true,      // Claw is always available
        .itemID = ITEM_CLAW,
        .standardVersionType = TOOL_CLAW
    };
    
    wheel->tools[TOOL_FISHING_ROD] = (Tool){
        .type = TOOL_FISHING_ROD,
        .name = "Rod",
        .imagePath = "./game/assets/tools/rod.png",
        .usableTerrain = TERRAIN_WATER,
        .texture = {0},
        .isImprovedVersion = false,
        .isUnlocked = true,      // Base rod is always available
        .itemID = ITEM_FISHING_ROD,
        .standardVersionType = TOOL_FISHING_ROD
    };
    
    wheel->tools[TOOL_GOLDEN_FISHING_ROD] = (Tool){
        .type = TOOL_GOLDEN_FISHING_ROD,
        .name = "Improved Rod",
        .imagePath = "./game/assets/tools/goldenRod.png",
        .usableTerrain = TERRAIN_WATER,
        .texture = {0},
        .isImprovedVersion = true,
        .isUnlocked = false,     // Initially locked until achievement
        .itemID = ITEM_GOLDEN_FISHING_ROD,
        .standardVersionType = TOOL_FISHING_ROD
    };
    
    wheel->tools[TOOL_PICKAXE] = (Tool){
        .type = TOOL_PICKAXE,
        .name = "Pickaxe",
        .imagePath = "./game/assets/tools/pickaxe.png",
        .usableTerrain = TERRAIN_MOUNTAIN,
        .texture = {0},
        .isImprovedVersion = false,
        .isUnlocked = true,      // Base pickaxe is always available
        .itemID = ITEM_PICKAXE,
        .standardVersionType = TOOL_PICKAXE
    };
    
    wheel->tools[TOOL_GOLDEN_PICKAXE] = (Tool){
        .type = TOOL_GOLDEN_PICKAXE,
        .name = "Improved Pickaxe",
        .imagePath = "./game/assets/tools/goldenPickaxe.png",
        .usableTerrain = TERRAIN_MOUNTAIN,
        .texture = {0},
        .isImprovedVersion = true,
        .isUnlocked = false,     // Initially locked until achievement
        .itemID = ITEM_GOLDEN_PICKAXE,
        .standardVersionType = TOOL_PICKAXE
    };
    
    wheel->tools[TOOL_SCISSORS] = (Tool){
        .type = TOOL_SCISSORS,
        .name = "Scissors",
        .imagePath = "./game/assets/tools/scissors.png",
        .usableTerrain = TERRAIN_GRASS,
        .texture = {0},
        .isImprovedVersion = false,
        .isUnlocked = true,      // Base scissors are always available
        .itemID = ITEM_SCISSORS,
        .standardVersionType = TOOL_SCISSORS
    };
    
    wheel->tools[TOOL_GOLDEN_SCISSORS] = (Tool){
        .type = TOOL_GOLDEN_SCISSORS,
        .name = "Improved Scissors",
        .imagePath = "./game/assets/tools/goldenScissors.png",
        .usableTerrain = TERRAIN_GRASS,
        .texture = {0},
        .isImprovedVersion = true,
        .isUnlocked = false,     // Initially locked until achievement
        .itemID = ITEM_GOLDEN_SCISSORS,
        .standardVersionType = TOOL_SCISSORS
    };
    
    wheel->tools[TOOL_SHOVEL] = (Tool){
        .type = TOOL_SHOVEL,
        .name = "Shovel",
        .imagePath = "./game/assets/tools/shovel.png",
        .usableTerrain = TERRAIN_GROUND,
        .texture = {0},
        .isImprovedVersion = false,
        .isUnlocked = true,      // Base shovel is always available
        .itemID = ITEM_SHOVEL,
        .standardVersionType = TOOL_SHOVEL
    };
    
    wheel->tools[TOOL_GOLDEN_SHOVEL] = (Tool){
        .type = TOOL_GOLDEN_SHOVEL,
        .name = "Improved Shovel",
        .imagePath = "./game/assets/tools/goldenShovel.png",
        .usableTerrain = TERRAIN_GROUND,
        .texture = {0},
        .isImprovedVersion = true,
        .isUnlocked = false,     // Initially locked until achievement
        .itemID = ITEM_GOLDEN_SHOVEL,
        .standardVersionType = TOOL_SHOVEL
    };
    
    wheel->tools[TOOL_TEETH] = (Tool){
        .type = TOOL_TEETH,
        .name = "Teeth",
        .imagePath = "./game/assets/tools/teeth.png",
        .usableTerrain = TERRAIN_EMPTY,
        .texture = {0},
        .isImprovedVersion = false,
        .isUnlocked = true,      // Teeth are always available
        .itemID = ITEM_TEETH,
        .standardVersionType = TOOL_TEETH
    };
    
    // Load the textures
    for (int i = 0; i < MAX_TOOLS; i++) {
        Image image = LoadImage(wheel->tools[i].imagePath);
        if (image.data != NULL) {
            ImageResizeNN(&image, 64, 64); // Resize for the wheel icons
            wheel->tools[i].texture = LoadTextureFromImage(image);
            UnloadImage(image);
        } else {
            fprintf(stderr, "Failed to load tool image: %s\n", wheel->tools[i].imagePath);
        }
    }
    
    // Check inventory for improved tools and mark them as unlocked
    CheckImprovedToolStatus(wheel);
}

// Update tool status based on inventory
void CheckImprovedToolStatus(ToolWheel* wheel) {
    // Check each improved tool if it's in the inventory
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (wheel->tools[i].isImprovedVersion) {
            ItemID improvedID = wheel->tools[i].itemID;
            // Fix the syntax error here
            if (IsItemInInventory(improvedID, masterItemList[improvedID].isKeyItem)) {
                wheel->tools[i].isUnlocked = true;
            }
        }
    }
}

// 8. Modified DrawToolWheel that correctly displays the selected tool
void DrawToolWheel(ToolWheel* wheel) {
    int centerX = SCREEN_WIDTH - 100;
    int centerY = SCREEN_HEIGHT - 100;
    int radius = 80; // Main wheel outer radius

    // --- Prepare for Conditional Resistance Drawing ---
    int currentIndex = wheel->selectedToolIndex;
    ToolType currentType = GetToolTypeFromIndex(wheel, currentIndex);
    int nextIndexL = FindNextValidTool(wheel, currentIndex, false);
    int nextIndexR = FindNextValidTool(wheel, currentIndex, true);
    ToolType nextTypeL = GetToolTypeFromIndex(wheel, nextIndexL);
    ToolType nextTypeR = GetToolTypeFromIndex(wheel, nextIndexR);

    // Determine if resistance mechanic is currently applicable
    bool resistanceApplies = (currentType == TOOL_CLAW || nextTypeL == TOOL_CLAW || nextTypeR == TOOL_CLAW);

    float relevantCharge = 0.0f;
    if (resistanceApplies) {
        if (currentType == TOOL_CLAW) {
            // Switching AWAY from claw, show the higher charge of A or S
            relevantCharge = fmaxf(wheel->clawSwitchChargeA, wheel->clawSwitchChargeS);
        } else {
            // Switching TO the claw. Show charge based on which key is held *and* leads to the claw.
            if (nextTypeL == TOOL_CLAW && wheel->isHoldingA) {
                relevantCharge = wheel->clawSwitchChargeA;
            } else if (nextTypeR == TOOL_CLAW && wheel->isHoldingS) {
                relevantCharge = wheel->clawSwitchChargeS;
            }
            // If neither A nor S is held (or the held key doesn't lead to claw), charge stays 0.
        }
    }
    // --- End Preparation ---


    // --- Draw Main Wheel Segments ---
    int baseToolIndices[MAX_TOOLS];
    int baseToolCount = 0;
    int clawSegmentIndex = -1; // To store the index of the Claw segment

    for (int i = 0; i < MAX_TOOLS; i++) {
        if (!wheel->tools[i].isImprovedVersion) {
            int currentBaseIndex = baseToolCount; // Store current index before incrementing
            baseToolIndices[baseToolCount++] = i;
            // Check if this base tool is the Claw
            if (wheel->tools[i].type == TOOL_CLAW) {
                 clawSegmentIndex = currentBaseIndex; // Store the segment index (0 to baseToolCount-1)
            }
        }
    }

    float anglePerTool = 360.0f / baseToolCount;
    float clawStartAngle = 0.0f;
    float clawEndAngle = 0.0f;

    for (int i = 0; i < baseToolCount; i++) {
        float startAngle = i * anglePerTool - 90; // -90 to start at the top
        float endAngle = startAngle + anglePerTool;
        int baseToolIndex = baseToolIndices[i];
        ToolType baseType = GetToolTypeFromIndex(wheel, baseToolIndex);

        // Store claw angles if this is the claw segment
        if (i == clawSegmentIndex) {
            clawStartAngle = startAngle;
            clawEndAngle = endAngle;
        }

        bool isHighlighted = false;
        ToolType selectedType = GetToolTypeFromIndex(wheel, wheel->selectedToolIndex);

        if (baseType == selectedType) {
            isHighlighted = true;
        }

        Color segmentColor = isHighlighted ? LIGHTGRAY : GRAY;
        DrawRing((Vector2){centerX, centerY}, radius - 20, radius, startAngle, endAngle, 0, segmentColor);
    }
    // --- End Main Wheel Segments ---


    // --- Draw Conditional Claw Resistance Segment ---
    if (resistanceApplies && clawSegmentIndex != -1 && relevantCharge > 0.01f) // Only draw if relevant and charge > 0
    {
        // Calculate the end angle for the fill based on charge
        float fillEndAngle = clawStartAngle + (clawEndAngle - clawStartAngle) * relevantCharge;

        // Draw the background for the segment (optional, could just draw filled part)
        // DrawRing((Vector2){centerX, centerY}, CLAW_RESISTANCE_SEGMENT_INNER_RADIUS, CLAW_RESISTANCE_SEGMENT_OUTER_RADIUS, clawStartAngle, clawEndAngle, 0, DARKGRAY);

        // Draw the filled yellow portion
        DrawRing((Vector2){centerX, centerY}, CLAW_RESISTANCE_SEGMENT_INNER_RADIUS, CLAW_RESISTANCE_SEGMENT_OUTER_RADIUS, clawStartAngle, fillEndAngle, 0, YELLOW);

        // Optional: Draw Threshold marker on this specific segment
        float thresholdAngle = clawStartAngle + (clawEndAngle - clawStartAngle) * CLAW_SWITCH_THRESHOLD;
        Vector2 thresholdPosStart = { centerX + cosf(DEG2RAD * thresholdAngle) * CLAW_RESISTANCE_SEGMENT_INNER_RADIUS,
                                      centerY + sinf(DEG2RAD * thresholdAngle) * CLAW_RESISTANCE_SEGMENT_INNER_RADIUS };
        Vector2 thresholdPosEnd = { centerX + cosf(DEG2RAD * thresholdAngle) * CLAW_RESISTANCE_SEGMENT_OUTER_RADIUS,
                                    centerY + sinf(DEG2RAD * thresholdAngle) * CLAW_RESISTANCE_SEGMENT_OUTER_RADIUS };
        DrawLineV(thresholdPosStart, thresholdPosEnd, RED); // Draw a red line marker
    }
    // --- End Conditional Resistance Segment ---


    // Draw inner circle (unchanged)
    DrawCircle(centerX, centerY, radius - 20, BLACK);

    // Draw selected tool in the center (unchanged)
    Texture2D selectedTexture = wheel->tools[wheel->selectedToolIndex].texture;
    if (selectedTexture.id != 0) {
        DrawTexture(selectedTexture, centerX - selectedTexture.width / 2, centerY - selectedTexture.height / 2, WHITE);
    }
    
    // Draw tool name
    // const char* toolName = wheel->tools[wheel->selectedToolIndex].name;
    // int textWidth = MeasureText(toolName, 20);
    // DrawText(toolName, centerX - textWidth/2, centerY + 50, 20, WHITE);
}


// Function to update the KeyItemList to show improved tools when they're unlocked
void UpdateKeyItemListForTools() {
    // First collect all tools that have both standard and improved versions
    bool hasImprovedToolUnlocked[MAX_TOOLS] = {false};
    
    // Check which improved tools are unlocked
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (toolWheel.tools[i].isImprovedVersion && toolWheel.tools[i].isUnlocked) {
            ToolType standardType = toolWheel.tools[i].standardVersionType;
            hasImprovedToolUnlocked[standardType] = true;
        }
    }
    
    // Now make sure the appropriate tool items are in the key item list
    for (int i = 0; i < MAX_TOOLS; i++) {
        // Skip improved versions - we'll add them separately
        if (toolWheel.tools[i].isImprovedVersion) continue;
        
        // Always add the standard version
        bool standardFound = false;
        for (int j = 0; j < keyItemCount; j++) {
            if (keyItemList[j].id == toolWheel.tools[i].itemID) {
                keyItemList[j].isUnlocked = true;
                standardFound = true;
                break;
            }
        }
        
        if (!standardFound && toolWheel.tools[i].itemID != ITEM_NONE) {
            // Find this item in the master list
            for (int j = 0; j < masterItemCount; j++) {
                if (masterItemList[j].id == toolWheel.tools[i].itemID) {
                    if (keyItemCount < MAX_KEY_ITEMS) {
                        keyItemList[keyItemCount] = masterItemList[j];
                        keyItemList[keyItemCount].isUnlocked = true;
                        keyItemCount++;
                    }
                    break;
                }
            }
        }
        
        // Add the improved version if unlocked
        if (hasImprovedToolUnlocked[i]) {
            // Find the improved tool
            for (int j = 0; j < MAX_TOOLS; j++) {
                if (toolWheel.tools[j].isImprovedVersion && 
                    toolWheel.tools[j].standardVersionType == i) {
                    
                    bool improvedFound = false;
                    for (int k = 0; k < keyItemCount; k++) {
                        if (keyItemList[k].id == toolWheel.tools[j].itemID) {
                            keyItemList[k].isUnlocked = true;
                            improvedFound = true;
                            break;
                        }
                    }
                    
                    if (!improvedFound && toolWheel.tools[j].itemID != ITEM_NONE) {
                        // Find this item in the master list
                        for (int k = 0; k < masterItemCount; k++) {
                            if (masterItemList[k].id == toolWheel.tools[j].itemID) {
                                if (keyItemCount < MAX_KEY_ITEMS) {
                                    keyItemList[keyItemCount] = masterItemList[k];
                                    keyItemList[keyItemCount].isUnlocked = true;
                                    keyItemCount++;
                                }
                                break;
                            }
                        }
                    }
                    
                    break;
                }
            }
        }
    }
}

// 1. First, add a debugging function to check key item contents
void DebugPrintKeyItems() {
    printf("===== KEY ITEMS (%d) =====\n", keyItemCount);
    for (int i = 0; i < keyItemCount; i++) {
        printf("%d. %s (ID: %d, isKeyItem: %s)\n", 
               i, 
               keyItemList[i].name, 
               keyItemList[i].id, 
               keyItemList[i].isKeyItem ? "true" : "false");
    }
    printf("========================\n");
}

// 2. Rebuild inventories to ensure proper categorization
void RebuildInventories() {
    // Clear existing inventories
    itemCount = 0;
    keyItemCount = 0;
    
    // First fill player inventory with usable items
    for (int i = 0; i < masterItemCount; i++) {
        if (masterItemList[i].isUnlocked && !masterItemList[i].isKeyItem) {
            if (itemCount < MAX_ITEMS) {
                playerInventory[itemCount++] = masterItemList[i];
            }
        }
    }
    
    // Then fill key item list with key items only
    for (int i = 0; i < masterItemCount; i++) {
        if (masterItemList[i].isUnlocked && masterItemList[i].isKeyItem) {
            if (keyItemCount < MAX_KEY_ITEMS) {
                keyItemList[keyItemCount++] = masterItemList[i];
            }
        }
    }
    
    // Sort key items for consistent display
    SortKeyItems();
    
    DebugPrintKeyItems();
}


// Custom sort function for key items
void SortKeyItems() {
    // Simple bubble sort for key items by ID
    for (int i = 0; i < keyItemCount - 1; i++) {
        for (int j = 0; j < keyItemCount - i - 1; j++) {
            if (keyItemList[j].id > keyItemList[j + 1].id) {
                // Swap items
                Item temp = keyItemList[j];
                keyItemList[j] = keyItemList[j + 1];
                keyItemList[j + 1] = temp;
            }
        }
    }
}

// Helper to sort items
void SortItems(Item* items, int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (items[j].id > items[j + 1].id) {
                Item temp = items[j];
                items[j] = items[j + 1];
                items[j + 1] = temp;
            }
        }
    }
}

// 6. Improved key item initialization
void InitializeKeyItems() {
    printf("Initializing key items...\n");
    // Reset the key item list to empty
    keyItemCount = 0;
    
    // Loop through master list and add actual key items
    for (int i = 0; i < masterItemCount; i++) {
        if (masterItemList[i].isKeyItem && masterItemList[i].isUnlocked) {
            if (keyItemCount < MAX_KEY_ITEMS) {
                keyItemList[keyItemCount++] = masterItemList[i];
                printf("Added key item: %s (ID: %d)\n", masterItemList[i].name, masterItemList[i].id);
            }
        }
    }
    
    // Sort for consistent display
    SortKeyItems();
    
    DebugPrintKeyItems();
}



bool IsToolUsable(ToolWheel* wheel) {
    Tool* selectedTool = &wheel->tools[wheel->selectedToolIndex];

    Rectangle playerArea = GetInteractionArea(&mainTeam[0]);
    
    int startX = (int)floorf(playerArea.x);
    int endX = (int)ceilf(playerArea.x + playerArea.width);
    int startY = (int)floorf(playerArea.y);
    int endY = (int)ceilf(playerArea.y + playerArea.height);

    for(int y = startY; y < endY; y++) {
        for(int x = startX; x < endX; x++) {
            if(x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) continue;
            
            if(worldMap[y][x].type == selectedTool->usableTerrain) {
                return true; // Return tile coordinates
            }
        }
    }
    return false; // Invalid position
}

// Find improved version of a tool if available
int FindImprovedVersion(ToolWheel* wheel, int toolIndex) {
    ToolType baseType = wheel->tools[toolIndex].type;
    
    // Only search if this is a base tool (not already improved)
    if (!wheel->tools[toolIndex].isImprovedVersion) {
        for (int i = 0; i < MAX_TOOLS; i++) {
            if (wheel->tools[i].isImprovedVersion && 
                wheel->tools[i].standardVersionType == baseType &&
                wheel->tools[i].isUnlocked) {
                return i;
            }
        }
    }
    
    return -1; // No improved version found or already improved
}

// 6. Modified function to automatically switch to improved version
void SwitchToImprovedIfAvailable(ToolWheel* wheel) {
    // If user explicitly selected a version, respect that choice
    if (wheel->userExplicitlySelected) {
        return;
    }
    
    int currentIndex = wheel->selectedToolIndex;
    
    // Only switch if this is a standard (non-improved) tool
    if (!wheel->tools[currentIndex].isImprovedVersion) {
        ToolType baseType = wheel->tools[currentIndex].type;
        
        // Look for an improved version
        for (int i = 0; i < MAX_TOOLS; i++) {
            if (wheel->tools[i].isImprovedVersion &&
                wheel->tools[i].standardVersionType == baseType &&
                wheel->tools[i].isUnlocked) {
                wheel->selectedToolIndex = i;
                printf("Auto-switched to improved version: %s\n", wheel->tools[i].name);
                break;
            }
        }
    }
}


// Completely rewritten FindNextValidTool function to fix navigation issues
int FindNextValidTool(ToolWheel* wheel, int currentIndex, bool clockwise) {
    int direction = clockwise ? 1 : -1;
    int startIndex = currentIndex;
    
    // Get the tool type of the current tool
    ToolType currentType;
    if (wheel->tools[currentIndex].isImprovedVersion) {
        currentType = wheel->tools[currentIndex].standardVersionType;
    } else {
        currentType = wheel->tools[currentIndex].type;
    }
    
    // First find the next base tool that is a different type
    int nextBaseIndex = -1;
    for (int attempt = 0; attempt < MAX_TOOLS; attempt++) {
        // Move to next position
        int checkIndex = (startIndex + direction * (attempt + 1) + MAX_TOOLS) % MAX_TOOLS;
        
        // Only consider base tools that are a different type
        if (!wheel->tools[checkIndex].isImprovedVersion) {
            ToolType checkType = wheel->tools[checkIndex].type;
            
            // Found a different base tool
            if (checkType != currentType) {
                nextBaseIndex = checkIndex;
                break;
            }
        }
    }
    
    // If we couldn't find another base tool, return original
    if (nextBaseIndex == -1) {
        return startIndex;
    }
    
    // Now check if this base tool has an improved version that's unlocked
    // ToolType nextBaseType = wheel->tools[nextBaseIndex].type;
    // for (int i = 0; i < MAX_TOOLS; i++) {
    //     if (wheel->tools[i].isImprovedVersion && 
    //         wheel->tools[i].standardVersionType == nextBaseType &&
    //         wheel->tools[i].isUnlocked) {
    //         // Found an improved version - use it
    //         printf("Tool navigation: %d (%s) -> %d (%s) [improved available]\n", 
    //                startIndex, wheel->tools[startIndex].name,
    //                i, wheel->tools[i].name);
    //         return i;
    //     }
    // }
    
    // // No improved version - use the base tool
    // printf("Tool navigation: %d (%s) -> %d (%s) [base only]\n", 
    //        startIndex, wheel->tools[startIndex].name,
    //        nextBaseIndex, wheel->tools[nextBaseIndex].name);
    return nextBaseIndex;
}

// Simplified UpdateToolWheel function
void UpdateToolWheel(bool LPressed, bool RPressed) {
    bool holdingA = IsKeyDown(KEY_A);
    bool holdingS = IsKeyDown(KEY_S);

    toolWheel.isHoldingA = holdingA;
    toolWheel.isHoldingS = holdingS;

    float frameTime = GetFrameTime();

    int currentIndex = toolWheel.selectedToolIndex;
    ToolType currentType = GetToolTypeFromIndex(&toolWheel, currentIndex);
    int nextIndexL = FindNextValidTool(&toolWheel, currentIndex, false);
    int nextIndexR = FindNextValidTool(&toolWheel, currentIndex, true);
    ToolType nextTypeL = GetToolTypeFromIndex(&toolWheel, nextIndexL);
    ToolType nextTypeR = GetToolTypeFromIndex(&toolWheel, nextIndexR);

    bool switchedThisFrame = false;

    // --- Instant Switch Logic (Not involving Claw) ---
    if (LPressed && !switchedThisFrame) {
        if (currentType != TOOL_CLAW && nextTypeL != TOOL_CLAW) {
            if (nextIndexL != currentIndex) {
                toolWheel.selectedToolIndex = nextIndexL;
                toolWheel.userExplicitlySelected = false;
                toolWheel.timer = 120; // Show wheel on instant switch
                switchedThisFrame = true;
                // printf("Switched tool instantly via A (no claw)\n");
            }
        }
    }

    if (RPressed && !switchedThisFrame) {
        if (currentType != TOOL_CLAW && nextTypeR != TOOL_CLAW) {
             if (nextIndexR != currentIndex) {
                toolWheel.selectedToolIndex = nextIndexR;
                toolWheel.userExplicitlySelected = false;
                toolWheel.timer = 120; // Show wheel on instant switch
                switchedThisFrame = true;
                //  printf("Switched tool instantly via S (no claw)\n");
             }
        }
    }

    // --- Resistance Charge Update ---
    // Determine if the current hold *could* trigger a resistance switch
    bool resistanceAppliesA = holdingA && (currentType == TOOL_CLAW || nextTypeL == TOOL_CLAW);
    bool resistanceAppliesS = holdingS && (currentType == TOOL_CLAW || nextTypeR == TOOL_CLAW);

    // Handle 'A' key charge
    if (holdingA) {
        // Only charge if resistance applies to this direction
        if (currentType == TOOL_CLAW || nextTypeL == TOOL_CLAW) {
            toolWheel.clawSwitchChargeA += CLAW_FILL_RATE * frameTime;
            if (toolWheel.clawSwitchChargeA > 1.0f) toolWheel.clawSwitchChargeA = 1.0f;
        } else {
             toolWheel.clawSwitchChargeA = 0.0f; // Reset charge if A is held but doesn't apply to claw
        }
    } else { // Not holding A
        toolWheel.clawSwitchChargeA -= CLAW_DECAY_RATE * frameTime;
        if (toolWheel.clawSwitchChargeA < 0.0f) toolWheel.clawSwitchChargeA = 0.0f;
    }

    // Handle 'S' key charge
    if (holdingS) {
         // Only charge if resistance applies to this direction
        if (currentType == TOOL_CLAW || nextTypeR == TOOL_CLAW) {
            toolWheel.clawSwitchChargeS += CLAW_FILL_RATE * frameTime;
            if (toolWheel.clawSwitchChargeS > 1.0f) toolWheel.clawSwitchChargeS = 1.0f;
        } else {
             toolWheel.clawSwitchChargeS = 0.0f; // Reset charge if S is held but doesn't apply to claw
        }
    } else { // Not holding S
        toolWheel.clawSwitchChargeS -= CLAW_DECAY_RATE * frameTime;
        if (toolWheel.clawSwitchChargeS < 0.0f) toolWheel.clawSwitchChargeS = 0.0f;
    }

    // --- Activate Timer when Charging for Claw Switch ---
    // If holding A/S *and* this action involves the claw, show the wheel timer
    if (resistanceAppliesA || resistanceAppliesS) {
         toolWheel.timer = 120; // Set/Reset timer while charging
    }
    // --- End Timer Activation ---


    // --- Resistance Switch Logic (Only when involving Claw) ---
    // Check 'A' threshold
    if (!switchedThisFrame && toolWheel.clawSwitchChargeA >= CLAW_SWITCH_THRESHOLD) {
        // Condition already checked by resistanceAppliesA basically
        if (currentType == TOOL_CLAW || nextTypeL == TOOL_CLAW) {
             if (nextIndexL != currentIndex) {
                toolWheel.selectedToolIndex = nextIndexL;
                toolWheel.userExplicitlySelected = false;
                toolWheel.timer = 120; // Ensure timer is set on switch
                switchedThisFrame = true;
                // printf("Switched tool via A (resistance - involves claw)\n");
             }
        }
        toolWheel.clawSwitchChargeA = 0.0f; // Reset charge after check/switch
    }

    // Check 'S' threshold
    if (!switchedThisFrame && toolWheel.clawSwitchChargeS >= CLAW_SWITCH_THRESHOLD) {
         // Condition already checked by resistanceAppliesS basically
        if (currentType == TOOL_CLAW || nextTypeR == TOOL_CLAW) {
            if (nextIndexR != currentIndex) {
                toolWheel.selectedToolIndex = nextIndexR;
                toolWheel.userExplicitlySelected = false;
                toolWheel.timer = 120; // Ensure timer is set on switch
                switchedThisFrame = true;
                // printf("Switched tool via S (resistance - involves claw)\n");
            }
        }
        toolWheel.clawSwitchChargeS = 0.0f; // Reset charge after check/switch
    }

    // Update wheel visibility timer (decrement if > 0)
    // This should happen AFTER potential timer resets above
    if (toolWheel.timer > 0 && !holdingA && !holdingS && !switchedThisFrame) {
         // Only decrement timer if not actively charging for claw or just switched
         // Or simply always decrement if > 0:
         // if (toolWheel.timer > 0) {
         toolWheel.timer--;
         //}
    } else if (toolWheel.timer > 0 && (holdingA || holdingS || switchedThisFrame)) {
        // If holding for claw OR just switched, timer was likely just reset to 120,
        // so we decrement it once here to start the countdown.
        toolWheel.timer--;
    }
}


// Handle tool usage
void HandleToolUsage(bool APressed, BattleState* battleState) {
    if (APressed) {
        Tool* selectedTool = &toolWheel.tools[toolWheel.selectedToolIndex];
        // bool isImproved = selectedTool->isImprovedVersion;
        
        if (IsToolUsable(&toolWheel)) {
            battleState->terrain = selectedTool->usableTerrain;
            
            switch (selectedTool->type) {
                case TOOL_CLAW: {
                    SetOverworldMessage("You swing your claw!");
                    break;
                }
                case TOOL_FISHING_ROD: {
                    StartFishingMinigame(&fishingMinigameState, &mainTeam[0]);
                    break;
                }
                case TOOL_GOLDEN_FISHING_ROD: {
                    // Skip minigame for improved rod
                    bool validWater = CheckTerrainNearPlayerArea(TERRAIN_WATER);
                    if (!validWater) {
                        SetOverworldMessage("No fishable water nearby!");
                        return;
                    }
                    
                    SetOverworldMessage("Your improved rod catches something immediately!");
                    fishingMinigameState.counter++;
                    TrackFishingSuccess();
                    currentGameState = GAME_STATE_BATTLE_TRANSITION;
                    ScreenBreakingAnimation();
                    break;
                }
                case TOOL_PICKAXE: {
                    StartPickaxeMinigame(&pickaxeMinigameState, &mainTeam[0]);
                    break;
                }
                case TOOL_GOLDEN_PICKAXE: {
                    // Skip minigame for improved pickaxe
                    bool validMountain = CheckTerrainNearPlayerArea(TERRAIN_MOUNTAIN);
                    if (!validMountain) {
                        SetOverworldMessage("No mineable rock here!");
                        return;
                    }
                    
                    SetOverworldMessage("Your improved pickaxe easily breaks the rock!");
                    pickaxeMinigameState.counter++;
                    TrackPickaxeSuccess();
                    TrackMountainBroken();
                    DestroyMountainNearby(&pickaxeMinigameState);
                    PlaySoundByName(&soundManager, "bomb", false);
                    currentGameState = GAME_STATE_BATTLE_TRANSITION;
                    ScreenBreakingAnimation();
                    break;
                }
                case TOOL_SCISSORS: {
                    Vector2 grassPos = CheckTerrainInPlayerArea(TERRAIN_GRASS);
                    if (grassPos.x >= 0 && grassPos.y >= 0) { // Valid position found
                        StartCuttingMinigame(&cuttingMinigameState, &mainTeam[0], grassPos);
                    } else {
                        SetOverworldMessage("No grass to cut here!");
                    }
                    break;
                }
                case TOOL_GOLDEN_SCISSORS: {
                    Vector2 grassPos = CheckTerrainInPlayerArea(TERRAIN_GRASS);
                    if (grassPos.x >= 0 && grassPos.y >= 0) { // Valid position found
                        // Skip minigame for improved scissors
                        SetOverworldMessage("Your improved scissors cut with ease!");
                        cuttingMinigameState.counter++;
                        TrackCuttingSuccess();
                        
                        // Change grass to cutgrass sprite
                        worldMap[(int)grassPos.y][(int)grassPos.x].isModified = true;
                        worldMap[(int)grassPos.y][(int)grassPos.x].type = TERRAIN_CUTGRASS;
                        
                        currentGameState = GAME_STATE_BATTLE_TRANSITION;
                        ScreenBreakingAnimation();
                    } else {
                        SetOverworldMessage("No grass to cut here!");
                    }
                    break;
                }
                case TOOL_SHOVEL: {
                    Vector2 groundPos = CheckTerrainInPlayerArea(TERRAIN_GROUND);
                    if (groundPos.x >= 0 && groundPos.y >= 0) {
                        StartDiggingMinigame(&diggingMinigameState, &mainTeam[0], groundPos);
                    } else {
                        SetOverworldMessage("No diggable ground here!");
                    }
                    break;
                }
                case TOOL_GOLDEN_SHOVEL: {
                    Vector2 groundPos = CheckTerrainInPlayerArea(TERRAIN_GROUND);
                    if (groundPos.x >= 0 && groundPos.y >= 0) {
                        // Skip minigame for improved shovel
                        SetOverworldMessage("Your improved shovel digs with precision!");
                        diggingMinigameState.counter++;
                        TrackDiggingSuccess();
                        
                        // Update tile to dug state
                        Terrain *dugTile = &worldMap[(int)groundPos.y][(int)groundPos.x];
                        dugTile->isModified = true;
                        dugTile->type = TERRAIN_USEDGROUND;
                        dugTile->cooldownTimer = GROUND_COOLDOWN_DURATION;
                        
                        if (currentMap == MAP_OVERWORLD_DIGGING && groundPattern.active) {
                            UpdatePatternStatus();
                        }
                        
                        // Force an enemy encounter
                        diggingMinigameState.diggingSuccessful = true;
                        currentGameState = GAME_STATE_BATTLE_TRANSITION;
                        ScreenBreakingAnimation();
                    } else {
                        SetOverworldMessage("No diggable ground here!");
                    }
                    break;
                }
                case TOOL_TEETH: {
                    SetOverworldMessage("You eat something!");
                    StartOverWorldEating(&eatingState);
                    break;
                }
                default:
                    SetOverworldMessage("This tool is not implemented yet!");
                    break;
            }
        } else {
            SetOverworldMessage("You cannot use that tool here!");
        }
    }
}

// 2. Improved EquipKeyItem that remembers explicit selections
void EquipKeyItem(Item *item) {
    printf("Equipping key item: %s (ID: %d)\n", item->name, item->id);
    
    // Find matching tool
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (toolWheel.tools[i].itemID == item->id && toolWheel.tools[i].isUnlocked) {
            toolWheel.selectedToolIndex = i;
            
            // Remember that user explicitly selected this version
            toolWheel.userExplicitlySelected = true;
            
            printf("Equipped tool: %s\n", toolWheel.tools[i].name);
            return;
        }
    }
    
    printf("No matching tool found for item: %s\n", item->name);
}


// Call this whenever an achievement grants a golden tool
void GrantImprovedTool(ItemID improvedToolID) {
    // Add the item to inventory
    AddItemToInventory(improvedToolID, 1);
    
    // Update tool status
    for (int i = 0; i < MAX_TOOLS; i++) {
        if (toolWheel.tools[i].itemID == improvedToolID) {
            toolWheel.tools[i].isUnlocked = true;
            
            // Also update key item list to show the new tool
            UpdateKeyItemListForTools();
            
            // Sort key items for consistent display
            SortKeyItems();
            
            break;
        }
    }
}



////////////////////////
/// BATTLE FUNCTIONS ///
////////////////////////


void LoadItemsFromJSON(const char *filePath) {
    char *fileContent = LoadFileText(filePath);
    if (fileContent == NULL) {
        fprintf(stderr, "Failed to load item data file: %s\n", filePath);
        return;
    }

    cJSON *json = cJSON_Parse(fileContent);
    if (json == NULL) {
        fprintf(stderr, "Failed to parse JSON data.\n");
        UnloadFileText(fileContent);
        return;
    }

    int itemCountInFile = cJSON_GetArraySize(json);
    if (itemCountInFile > MAX_ITEMS + MAX_KEY_ITEMS) {
        fprintf(stderr, "Too many items in JSON file. Maximum allowed is %d.\n", MAX_ITEMS + MAX_KEY_ITEMS);
        itemCountInFile = MAX_ITEMS + MAX_KEY_ITEMS;
    }

    for (int i = 0; i < itemCountInFile; i++) {
        cJSON *itemJSON = cJSON_GetArrayItem(json, i);
        if (itemCount >= MAX_ITEMS) break;

        Item item = {0};  // Temporary item

        // Parse fields
        cJSON *idJSON = cJSON_GetObjectItem(itemJSON, "id"); // Get ID as string now
        cJSON *nameJSON = cJSON_GetObjectItem(itemJSON, "name");
        cJSON *descriptionJSON = cJSON_GetObjectItem(itemJSON, "description");
        cJSON *effectTypeJSON = cJSON_GetObjectItem(itemJSON, "effectType");
        cJSON *usageTypeJSON = cJSON_GetObjectItem(itemJSON, "usageType");
        cJSON *hpRestoreJSON = cJSON_GetObjectItem(itemJSON, "hpRestore");
        cJSON *mpRestoreJSON = cJSON_GetObjectItem(itemJSON, "mpRestore");
        cJSON *damageJSON = cJSON_GetObjectItem(itemJSON, "damage");
        cJSON *targetTypeJSON = cJSON_GetObjectItem(itemJSON, "targetType");
        cJSON *amountJSON = cJSON_GetObjectItem(itemJSON, "amount");
        cJSON *isKeyItemJSON = cJSON_GetObjectItem(itemJSON, "isKeyItem");
        cJSON *overworldEffectJSON = cJSON_GetObjectItem(itemJSON, "overworldEffect");
        cJSON *dropChanceJSON = cJSON_GetObjectItem(itemJSON, "dropChance");
        cJSON *isUnlockedJSON = cJSON_GetObjectItem(itemJSON, "isUnlocked");
        cJSON *speedCostJSON = cJSON_GetObjectItem(itemJSON, "speedCost");

        // Assign parsed data to the item
        // Use StringToItemID to convert string to enum
        item.id = idJSON ? StringToItemID(idJSON->valuestring) : ITEM_NONE;
        strncpy(item.name, nameJSON ? nameJSON->valuestring : "Unknown", sizeof(item.name) - 1);
        strncpy(item.description, descriptionJSON ? descriptionJSON->valuestring : "", sizeof(item.description) - 1);
        item.effectType = StringToEffectType(effectTypeJSON ? effectTypeJSON->valuestring : "EFFECT_NONE");
        item.usageType = StringToUsageType(usageTypeJSON ? usageTypeJSON->valuestring : "USAGE_BOTH");
        item.hpRestore = hpRestoreJSON ? hpRestoreJSON->valueint : 0;
        item.mpRestore = mpRestoreJSON ? mpRestoreJSON->valueint : 0;
        item.damage = damageJSON ? damageJSON->valueint : 0;
        item.targetType = StringToTargetType(targetTypeJSON ? targetTypeJSON->valuestring : "TARGET_NONE");
        item.amount = amountJSON ? amountJSON->valueint : 0;
        item.isKeyItem = isKeyItemJSON ? cJSON_IsTrue(isKeyItemJSON) : false;
        item.overworldEffect = overworldEffectJSON ? cJSON_IsTrue(overworldEffectJSON) : false;
        item.dropChance = dropChanceJSON ? (float)dropChanceJSON->valuedouble : 0.0f;
        item.isUnlocked = isUnlockedJSON ? cJSON_IsTrue(isUnlockedJSON) : false;
        item.speedCost = speedCostJSON ? (float)speedCostJSON->valuedouble : standardSpeedCost;

        // Add to master list
        if (masterItemCount < MAX_ITEMS + MAX_KEY_ITEMS) {
            masterItemList[masterItemCount++] = item;
            printf("Loaded item %s (%s): %s\n", ItemIDToString(item.id), item.name, item.description); // Using ItemIDToString here
        }

        // Only add to player inventory if unlocked
        // if (item.isUnlocked) {
        //     if (item.isKeyItem) {
        //         if (keyItemCount < MAX_KEY_ITEMS) {
        //             keyItemList[keyItemCount++] = item;
        //         } else {
        //             fprintf(stderr, "Too many key items in JSON file.\n");
        //         }
        //     } else {
        //         if (itemCount < MAX_ITEMS) {
        //             playerInventory[itemCount++] = item;
        //         } else {
        //             fprintf(stderr, "Too many usable items in JSON file.\n");
        //         }
        //     }
        // }
    }

    cJSON_Delete(json);
    UnloadFileText(fileContent);

    printf("Loaded %d usable items and %d key items from JSON file.\n", itemCount, keyItemCount);
}



void LoadCreaturesFromJSON(const char *filePath) {
    char *fileContent = LoadFileText(filePath);
    if (fileContent == NULL) {
        fprintf(stderr, "Failed to load enemy data file: %s\n", filePath);
        return;
    }

    cJSON *json = cJSON_Parse(fileContent);
    if (json == NULL) {
        fprintf(stderr, "Failed to parse JSON data.\n");
        UnloadFileText(fileContent);
        return;
    }

    int enemyCount = cJSON_GetArraySize(json);
    printf("Number of enemies in JSON: %d\n", enemyCount);
    enemyDatabase.creatures = malloc(sizeof(Creature) * enemyCount);
    if (enemyDatabase.creatures == NULL) {
        fprintf(stderr, "Failed to allocate memory for enemies.\n");
        return;
    }
    enemyDatabase.enemyCount = enemyCount;

    for (int i = 0; i < enemyCount; i++) {
        cJSON *enemyJSON = cJSON_GetArrayItem(json, i);
        Creature *enemy = &enemyDatabase.creatures[i];
        memset(enemy, 0, sizeof(Creature));

        // Load basic stats

        cJSON *idJSON =                     cJSON_GetObjectItem(enemyJSON, "id");
        cJSON *nameJSON =                   cJSON_GetObjectItem(enemyJSON, "name");
        cJSON *terrainsJSON =               cJSON_GetObjectItem(enemyJSON, "terrains");
        cJSON *expJSON =                    cJSON_GetObjectItem(enemyJSON, "exp"); // AP level
        cJSON *giveExpJSON =                cJSON_GetObjectItem(enemyJSON, "giveExp"); // give exp after battle
        cJSON *maxHPJSON =                  cJSON_GetObjectItem(enemyJSON, "maxHP");
        cJSON *maxMPJSON =                  cJSON_GetObjectItem(enemyJSON, "maxMP");
        cJSON *attackJSON =                 cJSON_GetObjectItem(enemyJSON, "attack");
        cJSON *defenseJSON =                cJSON_GetObjectItem(enemyJSON, "defense");
        cJSON *speedJSON =                  cJSON_GetObjectItem(enemyJSON, "speed");
        cJSON *magicJSON =                  cJSON_GetObjectItem(enemyJSON, "magic");
        cJSON *magicdefenseJSON =           cJSON_GetObjectItem(enemyJSON, "magicdefense");
        cJSON *luckJSON =                   cJSON_GetObjectItem(enemyJSON, "luck");
        cJSON *accuracyJSON =               cJSON_GetObjectItem(enemyJSON, "accuracy");
        cJSON *evasionJSON =                cJSON_GetObjectItem(enemyJSON, "evasion");
        cJSON *tempAttackBuffJSON =         cJSON_GetObjectItem(enemyJSON, "tempAttackBuff");
        cJSON *tempDefenseBuffJSON =        cJSON_GetObjectItem(enemyJSON, "tempDefenseBuff");
		cJSON *tempSpeedBuffJSON =          cJSON_GetObjectItem(enemyJSON, "tempSpeedBuff");
		cJSON *tempMagicBuffJSON =          cJSON_GetObjectItem(enemyJSON, "tempMagicBuff");    
		cJSON *tempMagicdefenseBuffJSON =   cJSON_GetObjectItem(enemyJSON, "tempMagicdefenseBuff");    
		cJSON *tempLuckBuffJSON =           cJSON_GetObjectItem(enemyJSON, "tempLuckBuff");
		cJSON *tempAccuracyBuffJSON =       cJSON_GetObjectItem(enemyJSON, "tempAccuracyBuff");
		cJSON *tempEvasionBuffJSON =        cJSON_GetObjectItem(enemyJSON, "tempEvasionBuff");
        cJSON *descriptionJSON =            cJSON_GetObjectItem(enemyJSON, "description");
        cJSON *achievementDescriptionJSON = cJSON_GetObjectItem(enemyJSON, "achievementDescription");
		cJSON *hitJSON =                    cJSON_GetObjectItem(enemyJSON, "hit");
        if (hitJSON && cJSON_IsBool(hitJSON)) {
            enemy->hit = cJSON_IsTrue(hitJSON);
        } else {
            // Handle missing or invalid "hit" field
            enemy->hit = false; // or some default value
        }
        cJSON *elementTypeJSON =            cJSON_GetObjectItem(enemyJSON, "elementType");
        cJSON *spriteFolderJSON =           cJSON_GetObjectItem(enemyJSON, "spriteFolderName");
        // cJSON *availableMagicJSON =         cJSON_GetObjectItem(enemyJSON, "availableMagic");
        // cJSON *availableSkillsJSON =        cJSON_GetObjectItem(enemyJSON, "availableSkills"); // New: Parse availableSkills
        cJSON *creatureTypeJSON =           cJSON_GetObjectItem(enemyJSON, "creatureType"); // New: Parse availableSkills
        cJSON *eatenItemJSON =              cJSON_GetObjectItem(enemyJSON, "eaten");

        cJSON *gridLevelJSON =               cJSON_GetObjectItem(enemyJSON, "gridLevel");

        // Set enemy stats
        enemy->id = idJSON->valueint;
        strcpy(enemy->name, nameJSON->valuestring);
        strcpy(enemy->description, descriptionJSON->valuestring);
        strcpy(enemy->achievementDescription, achievementDescriptionJSON->valuestring);
        enemy->exp = expJSON->valueint;
        enemy->giveExp = giveExpJSON->valueint;
        enemy->maxHP = maxHPJSON->valueint;
        enemy->currentHP = enemy->maxHP;
        enemy->maxMP = maxMPJSON->valueint;
        enemy->currentMP = enemy->maxMP;
        enemy->attack = attackJSON->valueint;
        enemy->defense = defenseJSON->valueint;
        enemy->speed = speedJSON->valueint;
        enemy->magic = magicJSON->valueint;
        enemy->magicdefense = magicdefenseJSON->valueint;
        enemy->luck = luckJSON->valueint;
        enemy->accuracy = accuracyJSON->valueint;
        enemy->evasion = evasionJSON->valueint;
        enemy->tempAttackBuff = tempAttackBuffJSON->valueint;
        enemy->tempDefenseBuff = tempDefenseBuffJSON->valueint;
        enemy->tempSpeedBuff = tempSpeedBuffJSON->valueint;
        enemy->tempMagicBuff = tempMagicBuffJSON->valueint;
        enemy->tempMagicdefenseBuff = tempMagicdefenseBuffJSON->valueint;
        enemy->tempLuckBuff = tempLuckBuffJSON->valueint;
        enemy->tempAccuracyBuff = tempAccuracyBuffJSON->valueint;
        enemy->tempEvasionBuff = tempEvasionBuffJSON->valueint;
        enemy->elementType = StringToElementType(elementTypeJSON->valuestring);
        enemy->creatureType = StringToCreatureType(creatureTypeJSON->valuestring);
        enemy->eatenItem = StringToItemID(eatenItemJSON->valuestring);
        enemy->gridLevel = gridLevelJSON ? gridLevelJSON->valueint : 0;
        enemy->totalGridLevel = gridLevelJSON ? gridLevelJSON->valueint : 0; // accumulated gridlevel for next level exp calcs

        // Load default magic
        cJSON *defaultMagicJSON = cJSON_GetObjectItem(enemyJSON, "defaultMagic");
        if (defaultMagicJSON && cJSON_IsArray(defaultMagicJSON)) {
            int defaultMagicCount = cJSON_GetArraySize(defaultMagicJSON);
            for (int j = 0; j < defaultMagicCount && j < MAX_MAGIC_PER_CREATURE; j++) {
                cJSON *magicItem = cJSON_GetArrayItem(defaultMagicJSON, j);
                if (cJSON_IsString(magicItem)) {
                    enemy->availableMagicIDs[enemy->magicCount++] = StringToMagicID(magicItem->valuestring);
                }
            }
        }

        // Load default skills
        cJSON *defaultSkillsJSON = cJSON_GetObjectItem(enemyJSON, "defaultSkills");
        if (defaultSkillsJSON && cJSON_IsArray(defaultSkillsJSON)) {
            int defaultSkillCount = cJSON_GetArraySize(defaultSkillsJSON);
            for (int j = 0; j < defaultSkillCount && j < MAX_SKILL_PER_CREATURE; j++) {
                cJSON *skillItem = cJSON_GetArrayItem(defaultSkillsJSON, j);
                if (cJSON_IsString(skillItem)) {
                    enemy->availableSkillIDs[enemy->skillCount++] = StringToSkillID(skillItem->valuestring);
                }
            }
        }
       

        // Load sprite
        const char *spriteFolderName = spriteFolderJSON->valuestring;

        LoadCreatureAnimations(enemy, FOLDER_FOLLOWERS);

        for (int i = 0; i < MAX_NODES_PER_CREATURE; i++) {
            enemy->unlockedNodes[i] = -1;
        }

        // Set core node as first unlocked node
        enemy->unlockedNodes[0] = enemy->currentNode; // Assuming core node is the starting position

        // Load terrains
        if (terrainsJSON != NULL && cJSON_IsArray(terrainsJSON)) {
            int terrainCount = cJSON_GetArraySize(terrainsJSON);
            enemy->terrainCount = terrainCount;


            for (int j = 0; j < terrainCount && j < NUM_TERRAINS; j++) {
                cJSON *terrainItem = cJSON_GetArrayItem(terrainsJSON, j);
                // const char *terrainFileName = terrainItem->valuestring;
                TerrainType terrain = StringToTerrainType(terrainItem->valuestring);
                enemy->terrains[j] = terrain;
                printf(" %d", enemy->terrains[j]);
                for (int k = 0; k < HEALTH_STATES; k++) {
                    char spritePath[256];
                    snprintf(spritePath, sizeof(spritePath), "%s/%d.png", spriteFolderName, k);
                    printf("\n%s/%d\n", spriteFolderName, k);
                    // if its a boss battle increase sprites' size
                    if (terrain != TERRAIN_BOSS) {
                        enemy->sprites[terrain][k] = LoadFirstTextureFromDirectory(spritePath, SPRITE_SIZE, SPRITE_SIZE);
                    }
                    else {
                        enemy->sprites[terrain][k] = LoadFirstTextureFromDirectory(spritePath, SPRITE_SIZE*2, SPRITE_SIZE*2);

                    }
                    if (enemy->sprites[terrain][k].id == 0) {
                        printf("Failed to load sprite for enemy %s on terrain %d\n", enemy->name, terrain);
                    }
                }
            }
            printf("\n");
        }        
    }

    // sort based on id
    qsort(enemyDatabase.creatures, enemyCount, sizeof(Creature), CompareCreaturesByID);


    cJSON_Delete(json);
    UnloadFileText(fileContent);
}


void CopyCreature(Creature *dest, const Creature *src) {
    memcpy(dest, src, sizeof(Creature));
}



Color GetDamageColor(EffectType effectType) {
    if (effectType == EFFECT_OFFENSIVE) {
        return WHITE;
    } else if (effectType == EFFECT_HEALING) {
        return CYAN;
    } else if (effectType == EFFECT_DEBUFF) {
        return RED;
    } else return WHITE;
}


void SetBattleMessage(BattleState *battleState, const char *message) {
    if (battleState->messageCount < MAX_MESSAGES) {
        strncpy(battleState->messageQueue[battleState->messageCount].text, message, sizeof(battleState->messageQueue[battleState->messageCount].text) - 1);
        battleState->messageQueue[battleState->messageCount].text[sizeof(battleState->messageQueue[battleState->messageCount].text) - 1] = '\0';
        if (IsKeyDown(KEY_Z)) {
            battleState->messageQueue[battleState->messageCount].timer = 1; // Set to desired duration
        }
        else {
            battleState->messageQueue[battleState->messageCount].timer = 5; // Set to desired duration
        }
        battleState->messageCount++;
    }
}

//UPDATE WITH NEW LEVELING SYSTEM
int GetExpForNextGridLevel(int gridLevel) {
    // Maximum grid level after which the experience requirement stays constant
    const int MAX_VARIABLE_GRID_LEVEL = 1001;
    const int MAX_EXP_REQUIREMENT = 22000;
    
    // If we've reached the maximum variable level, return the constant value
    if (gridLevel >= MAX_VARIABLE_GRID_LEVEL) {
        return MAX_EXP_REQUIREMENT;
    }
    
    // FFX-inspired formula: AP Req = 5*(TSL + 1) + [(TSL^3) / 50]
    // Where TSL = Total Sphere Level (gridLevel in our case)
    int baseRequirement = 5 * (gridLevel + 1);
    int cubicComponent = (gridLevel * gridLevel * gridLevel) / 50;
    
    return baseRequirement + cubicComponent;
}

// adds random encounters to the mix
void StartBattleWithRandomEnemy(BattleState *battleState) {
    // Collect all enemies that can appear on the given terrain
    Creature *possibleEnemies[MAX_BATTLE_ENEMIES];
    int possibleEnemyCount = 0;

    for (int i = 0; i < enemyDatabase.enemyCount; i++) {
        Creature *enemy = &enemyDatabase.creatures[i];
        for (int j = 0; j < enemy->terrainCount; j++) {
            if (enemy->terrains[j] == battleState->terrain) {
                possibleEnemies[possibleEnemyCount++] = enemy;
                break;
            }
        }
    }

    if (enemyDatabase.creatures == NULL) {
        printf("Enemy database is not initialized.\n");
        return;
    }


    if (possibleEnemyCount == 0) {
        printf("No enemies available for this terrain.\n");
        return;
    }

    int numEnemies = rand() % MAX_BATTLE_ENEMIES + 1; // At least one enemy
    printf("\n number of enemies %d \n", numEnemies);

    for (int i = 0; i < numEnemies; i++) {
        int randomIndex = rand() % possibleEnemyCount;
        Creature *selectedCreature = possibleEnemies[randomIndex];

        // Create a copy of the enemy creature
        Creature *enemyCopy = malloc(sizeof(Creature));
        if (enemyCopy == NULL) {
            fprintf(stderr, "Failed to allocate memory for enemy.\n");
            exit(EXIT_FAILURE);
        }
        *enemyCopy = *selectedCreature;

        // Add to participants
        battleState->participants[battleState->participantCount].creature = enemyCopy;
        battleState->participants[battleState->participantCount].isPlayer = false;
        battleState->participants[battleState->participantCount].nextActionTime = 0.0f;
        battleState->participants[battleState->participantCount].isActive = true;

        float speed = battleState->participants[battleState->participantCount].creature->speed + battleState->participants[battleState->participantCount].creature->tempSpeedBuff;
        if (speed <= 0) speed = 1.0f; // Prevent division by zero or negative speed
        battleState->participants[battleState->participantCount].nextActionTime = 1.0f/speed;
            

        battleState->participantCount++;
    }

    // Start the battle with the selected enemy
    StartBattle(battleState);
}

// adds players team to the mix
void StartBattle(BattleState *battleState) {
    battleState->commandConfirmed = false;
    battleState->selectedCommand = COMMAND_ATTACK;
    battleState->isBattleActive = true;
    battleState->runAttempts = 0;
    battleState->flickerTimer = 0;
    battleState->inSubmenu = false;
    battleState->targetSelectionActive = false;
    battleState->previousSkillUsed = -1;
    battleState->previousMagicUsed = -1;
    battleState->previousItemUsed = -1;
    battleState->rolloutConsecutiveUses = 0;
    battleState->numberOfTurns = 0;
    battleState->scrollUpKey = InitSmoothKey(0.2f, 0.1f);
    battleState->scrollDownKey = InitSmoothKey(0.2f, 0.1f);
    battleState->speedCost = standardSpeedCost;

    // Initialize swap-related fields
    battleState->inSwapMode = false;
    battleState->swapSelectedIndex = 0;
    battleState->swapMenuDelay = 0;

    // Add player party characters to participants
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] != '\0') {
            battleState->participants[battleState->participantCount].creature = &mainTeam[i];
            battleState->participants[battleState->participantCount].isPlayer = true;
            battleState->participants[battleState->participantCount].isActive = true;
            
            float speed = battleState->participants[battleState->participantCount].creature->speed + battleState->participants[battleState->participantCount].creature->tempSpeedBuff;
            if (speed <= 0) speed = 1.0f; // Prevent division by zero or negative speed
            battleState->participants[battleState->participantCount].nextActionTime = 1.0f/speed;

            battleState->participantCount++;
        }
    }

    printf("\n Total number of participants %d \n", battleState->participantCount);

    // copy to track order
    for (int i = 0; i < battleState->participantCount; i++) {
        battleState->turnOrder[i] = &battleState->participants[i];
    }

    SetBattleMessage(battleState, "Battle Start!");
}



// MAGIC DAMAGE IS CALCULATED!
void HandleBattle(BattleState *battleState) {
    
    if (!battleState->isBattleActive) {
        battleState->attackAnimation.isAnimating = false;
        battleState->attackAnimation.currentFrame = 0;
        battleState->attackAnimation.frameCounter = 0;
        return; // Do not process any further battle commands
    }

    if (battleState->messageCount > 0) {
        return;
    }


    // Get the participant whose turn it is
    SortTurnOrder(battleState);

    int nextIndex = -1;
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->turnOrder[i]->isActive && battleState->turnOrder[i]->creature->currentHP > 0) {
            nextIndex = i;
            break;
        }
    }

    if (nextIndex == -1) {
        // No active participants left -> battle ends?
        return;
    }

    BattleParticipant *nextUp = battleState->turnOrder[nextIndex];
    battleState->currentParticipant = nextUp;
    
    // If the current participant at turnOrder[0] is not active, skip them.
    if (!nextUp->isActive || nextUp->creature->currentHP <= 0) {
        UpdateNextActionTime(battleState, nextUp);
    }


    if (!battleState->attackAnimation.isAnimating) {
            // Player's turn
        if (nextUp->isPlayer) {

            if (IsKeyPressed(KEY_C) && !battleState->attackAnimation.isAnimating && 
                !battleState->inSubmenu && !battleState->commandConfirmed && 
                !battleState->targetSelectionActive) {
                SwapPlayerCommand(battleState, nextUp);
                return;
            }
            
            // Handle ongoing swap mode
            if (battleState->inSwapMode) {
                SwapPlayerCommand(battleState, nextUp);
                return; // Skip rest of battle processing while in swap mode
            }

            // Are we in Magic/Skill/Items?
            if (battleState->inSubmenu && battleState->commandConfirmed) {
                // printf("\n IN SUBMENU AND CONFIRMED\n");
                ExecutePlayerCommand(battleState, nextUp);
            }
            // initially from startbattle commandconfirmed is falso so enters here
            else if (!battleState->commandConfirmed) {
                UpdateBattleCommand(battleState, nextUp);
            }
            else {
                // printf("\n NOT IN SUBMENU AND CONFIRMED COMMAND\n");
                ExecutePlayerCommand(battleState, nextUp);
            }

        }
        else {
            // enemy turn
            if (nextUp->creature->currentHP > 0 && nextUp->isActive) {
                EnemyAttack(battleState, nextUp);
                UpdateNextActionTime(battleState,nextUp);
            }
            else {
                UpdateNextActionTime(battleState,nextUp);
            }
        }
    }


     // Handle attack animation if it's active
    else if (!battleState->decisionMade) {
        // ATTACK
        if (battleState->selectedCommand == COMMAND_ATTACK) {
            BattleParticipant *defender = &battleState->participants[battleState->targetIndex];
            printf("\n%s is going to attack %s",nextUp->creature->name,defender->creature->name);
            ProcessAttack(battleState, nextUp, defender);
        }

        // MAGIC
        else if (battleState->selectedCommand == COMMAND_MAGIC) {
            // int damage = 0;
            if (battleState->currentTargetType == TARGET_ALL_ENEMIES) {
                for (int i = 0; i < battleState->participantCount; i++) {
                    BattleParticipant *defender = &battleState->participants[i];
                    if (!defender->isPlayer && defender->creature->currentHP > 0) {
                        BattleParticipant *defender = &battleState->participants[i];
                        ProcessMagic(battleState, nextUp, defender);   
                    }                         
                }
            }
            else if (battleState->currentTargetType == TARGET_ALL_ALLIES) {
                for (int i = 0; i < battleState->participantCount; i++) {
                    BattleParticipant *defender = &battleState->participants[i];
                    if (defender->isPlayer && defender->creature->currentHP > 0) {
                        BattleParticipant *defender = &battleState->participants[i];
                        ProcessMagic(battleState, nextUp, defender); 
                    }  
                }
            }
            else {
                BattleParticipant *defender = &battleState->participants[battleState->targetIndex];
                ProcessMagic(battleState, nextUp, defender); 
                // Display damage message
            } 
        }
            // Reset action flags
        // battleState->actionInProgress = false;

        // ITEM
        else if (battleState->selectedCommand == COMMAND_ITEM) {
            if (battleState->currentTargetType == TARGET_ALL) {
                for (int i = 0; i < battleState->participantCount; i++) {
                    BattleParticipant *defender = &battleState->participants[i];
                    ProcessItem(battleState, defender);
                }
            }
            else {
                BattleParticipant *defender = &battleState->participants[battleState->targetIndex];
                printf("\n%s is going to attack %s",nextUp->creature->name,defender->creature->name);
                ProcessItem(battleState, defender);
            }
        }

        // SKILL
        else if (battleState->selectedCommand == COMMAND_SKILL) {
            if (battleState->currentTargetType == TARGET_ALL_ENEMIES) {
                for (int i = 0; i < battleState->participantCount; i++) {
                    BattleParticipant *defender = &battleState->participants[i];
                    if (!defender->isPlayer && defender->creature->currentHP > 0) {
                        BattleParticipant *defender = &battleState->participants[i];
                        ProcessSkill(battleState, nextUp, defender);   
                    }                         
                }
            }
            else if (battleState->currentTargetType == TARGET_ALL_ALLIES) {
                for (int i = 0; i < battleState->participantCount; i++) {
                    BattleParticipant *defender = &battleState->participants[i];
                    if (defender->isPlayer && defender->creature->currentHP > 0) {
                        BattleParticipant *defender = &battleState->participants[i];
                        ProcessSkill(battleState, nextUp, defender); 
                    }  
                }
            }
            else {
                BattleParticipant *defender = &battleState->participants[battleState->targetIndex];
                ProcessSkill(battleState, nextUp, defender);
            }
            
        }

    }
    else {
        // printf("\nbattleState->attackAnimation.frameCounter %d and battleState->attackAnimation.currentFrame %d\n",battleState->attackAnimation.frameCounter,battleState->attackAnimation.currentFrame);
        battleState->attackAnimation.frameCounter++;
        if (battleState->attackAnimation.frameCounter >= battleState->attackAnimation.frameSpeed) {
            battleState->attackAnimation.frameCounter = 0;
            battleState->attackAnimation.currentFrame++;

            if (battleState->attackAnimation.currentFrame >= battleState->attackAnimation.frameCount) {
                battleState->attackAnimation.isAnimating = false;
                UpdateNextActionTime(battleState, nextUp);
            }
        }
    }
    
    
    // TODO: CHANGE EXP HANDLING WITH MORE PLAYER
    // Check if battle has ended after each turn
    if (IsBattleOver(battleState) && !battleState->attackAnimation.isAnimating) {
        // TODO totalexp = sum all enemies exp
        GainEXP(battleState);
        GainRewards(battleState);
        battleState->isBattleActive = false;
        // UnloadTexture(battleState->enemySprite);  // Unload enemy sprite
    }
}

// Function to handle creature swapping
void SwapPlayerCommand(BattleState *battleState, BattleParticipant *participant) {
    // If we just entered swap mode, set initial state
    if (!battleState->inSwapMode) {
        battleState->inSwapMode = true;
        battleState->swapSelectedIndex = 0;
        battleState->swapMenuDelay = 7; // Short delay to prevent accidental input
        return;
    }
    
    // Handle delay to prevent accidental input
    if (battleState->swapMenuDelay > 0) {
        battleState->swapMenuDelay--;
        return;
    }
    
    // Navigate through backup creatures
    if (IsKeyPressed(KEY_DOWN)) {
        battleState->swapSelectedIndex++;
        if (battleState->swapSelectedIndex >= MAX_BACKUP_SIZE || 
            backupTeam[battleState->swapSelectedIndex].name[0] == '\0') {
            battleState->swapSelectedIndex = 0;
        }
    }
    
    if (IsKeyPressed(KEY_UP)) {
        battleState->swapSelectedIndex--;
        if (battleState->swapSelectedIndex < 0) {
            // Find the last valid creature
            int lastValid = 0;
            for (int i = 0; i < MAX_BACKUP_SIZE; i++) {
                if (backupTeam[i].name[0] != '\0') {
                    lastValid = i;
                }
            }
            battleState->swapSelectedIndex = lastValid;
        }
    }
    
    // Confirm swap with Z, ENTER, or C
    if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_C)) {
        // Make sure there's a valid creature to swap with
        if (battleState->swapSelectedIndex < MAX_BACKUP_SIZE && 
            backupTeam[battleState->swapSelectedIndex].name[0] != '\0') {
            
            // Find the index of the current creature in the main team
            int currentCreatureIdx = -1;
            for (int i = 0; i < MAX_PARTY_SIZE; i++) {
                if (mainTeam[i].name[0] != '\0' && &mainTeam[i] == participant->creature) {
                    currentCreatureIdx = i;
                    break;
                }
            }
            
            if (currentCreatureIdx != -1) {
                // Before swapping, store the position of the main team creature if it's the first one
                float originalX = mainTeam[currentCreatureIdx].x;
                float originalY = mainTeam[currentCreatureIdx].y;

                // Perform the swap
                Creature temp = mainTeam[currentCreatureIdx];
                mainTeam[currentCreatureIdx] = backupTeam[battleState->swapSelectedIndex];
                backupTeam[battleState->swapSelectedIndex] = temp;

                // If we're swapping the first creature (player avatar), preserve its position
                if (currentCreatureIdx == 0) {
                    mainTeam[0].x = originalX;
                    mainTeam[0].y = originalY;
                }
                
                // Update the participant to point to the new creature
                participant->creature = &mainTeam[currentCreatureIdx];
                
                // Add a message about the swap
                char swapMessage[128];
                sprintf(swapMessage, "%s swapped in for battle!", participant->creature->name);
                SetBattleMessage(battleState, swapMessage);
                
                // Exit swap mode
                battleState->inSwapMode = false;
                
                // Re-sort the turn order if needed
                SortTurnOrder(battleState);
                return;
            }
        }
        
        // If we reach here, either there wasn't a valid creature or something went wrong
        // Just exit swap mode
        battleState->inSwapMode = false;
    }
    
    // Exit swap mode with X or ESC
    if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ESCAPE)) {
        battleState->inSwapMode = false;
    }
}

// Add this function to draw the swap UI
void DrawSwapMenu(BattleState *battleState) {
    // int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Left panel for swap options
    int panelWidth = 200;
    int panelHeight = screenHeight - 200; // Leave space at top and bottom
    int panelX = 20;
    int panelY = 100;
    
    // Draw the panel background
    DrawRectangle(panelX, panelY, panelWidth, panelHeight, (Color){0, 0, 50, 200});
    DrawRectangleLines(panelX, panelY, panelWidth, panelHeight, WHITE);
    
    // Title
    DrawText("SWAP CREATURE", panelX + 10, panelY + 10, 20, YELLOW);
    // DrawText("------------", panelX + 10, panelY + 35, 20, YELLOW);
    DrawText("(Z/Enter/C: Confirm)", panelX + 10, panelY + 35, 18, YELLOW);
    
    // Check if there are any backup creatures
    bool hasBackups = false;
    for (int i = 0; i < MAX_BACKUP_SIZE; i++) {
        if (backupTeam[i].name[0] != '\0') {
            hasBackups = true;
            break;
        }
    }
    
    if (!hasBackups) {
        // No backup creatures available
        DrawText("No backup creatures", panelX + 20, panelY + 80, 18, GRAY);
        DrawText("available!", panelX + 20, panelY + 100, 18, GRAY);
        DrawText("Press X to cancel", panelX + 20, panelY + panelHeight - 40, 18, WHITE);
        return;
    }
    
    // Draw each backup creature
    int yOffset = 60;
    int boxHeight = 70;
    int boxPadding = 5;
    
    for (int i = 0; i < MAX_BACKUP_SIZE; i++) {
        if (backupTeam[i].name[0] == '\0') continue; // Skip empty slots
        
        Color boxColor = (i == battleState->swapSelectedIndex) ? DARKBLUE : (Color){20, 20, 20, 255};
        Color textColor = (i == battleState->swapSelectedIndex) ? YELLOW : WHITE;
        
        // Box for this creature
        DrawRectangle(panelX + 10, panelY + yOffset, panelWidth - 20, boxHeight, boxColor);
        DrawRectangleLines(panelX + 10, panelY + yOffset, panelWidth - 20, boxHeight, WHITE);
        
        // Selection indicator
        if (i == battleState->swapSelectedIndex) {
            DrawText(">", panelX, panelY + yOffset + boxHeight/2 - 10, 20, YELLOW);
        }
        
        // Creature details
        DrawText(backupTeam[i].name, panelX + 20, panelY + yOffset + 5, 18, textColor);
        DrawText(TextFormat("HP: %d/%d", backupTeam[i].currentHP, backupTeam[i].maxHP), 
                 panelX + 20, panelY + yOffset + 25, 16, textColor);
        DrawText(TextFormat("MP: %d/%d", backupTeam[i].currentMP, backupTeam[i].maxMP), 
                 panelX + 20, panelY + yOffset + 45, 16, textColor);
        
        yOffset += boxHeight + boxPadding;
    }
    
    // Instructions at the bottom
    // DrawText("Up/Down: Select", panelX + 20, panelY + panelHeight - 60, 16, WHITE);
    // DrawText("Z/Enter/C: Confirm", panelX + 20, panelY + panelHeight - 40, 16, WHITE);
    // DrawText("X: Cancel", panelX + 20, panelY + panelHeight - 20, 16, WHITE);
}



void UpdateBattleParticipants(BattleState *battleState) {
    for (int i = 0; i < battleState->participantCount; i++) {
        BattleParticipant *participant = &battleState->participants[i];
        
        // Handle death state
        if (participant->creature->currentHP <= 0 && participant->isActive && !participant->isPlayer) {
            if (!participant->deathActivated) {
                participant->deathActivated = true;
                participant->deathTimer = DEATH_TIMER; // 2 seconds at 60 FPS
            } else {
                participant->deathTimer--;
                if (participant->deathTimer <= 0) {
                    participant->isActive = false; // Remove from battle after animation
                }
            }
        }
    }
}

EffectType GetEffectForTerrain(TerrainType terrain) {
    EffectType targetEffect = EFFECT_NONE;
    // Determine item type based on terrain (unchanged)
    if (terrain == TERRAIN_GRASS) {
        targetEffect = EFFECT_HEALING;
    } else if (terrain == TERRAIN_EMPTY) {
        targetEffect = EFFECT_OFFENSIVE;
    } else targetEffect = EFFECT_BUFF;

    return targetEffect;
}

//UPDATE WITH NEW LEVELING SYSTEM
void GainRewards(BattleState *battleState) {
    TerrainType terrain = battleState->terrain;

    for (int i = 0; i < battleState->participantCount; i++) {
        BattleParticipant *participant = &battleState->participants[i];
        bool validParticipant = (!participant->isPlayer && participant->creature->currentHP <= 0) ||
                               (participant->isPlayer && participant->creature->currentHP > 0);

        if (validParticipant) {
            EffectType targetEffect = GetEffectForTerrain(terrain);

            for (int j = 0; j < masterItemCount; j++) {
                Item *masterItem = &masterItemList[j];
                
                // Skip invalid items
                if (masterItem->isKeyItem || 
                    masterItem->effectType != targetEffect || 
                    masterItem->dropChance <= 0.0f) continue;

                // Roll for drop
                if ((float)rand()/RAND_MAX < masterItem->dropChance) {
                    if (!masterItem->isUnlocked) {
                        // Unlock and add to appropriate inventory
                        masterItem->isUnlocked = true;
                        AddNewUnlockedItem(*masterItem);
                    }
                    AddItemToInventory(masterItem->id, 1);
                }
            }
        }
    }
}

// Improved AddItemToInventory with better handling
void AddItemToInventory(int itemID, int quantity) {
    if (itemID == ITEM_NONE) return;  // Handle invalid item IDs
    
    // Find the item in the master list
    Item* masterItem = NULL;
    for (int i = 0; i < masterItemCount; i++) {
        if (masterItemList[i].id == itemID) {
            masterItem = &masterItemList[i];
            break;
        }
    }
    
    if (masterItem == NULL) {
        printf("Error: Item ID %d not found in master list!\n", itemID);
        return;
    }
    
    // Make sure the item is unlocked
    if (!masterItem->isUnlocked) {
        masterItem->isUnlocked = true;
        AddNewUnlockedItem(*masterItem);
    }
    
    // Now add to the appropriate inventory
    if (masterItem->isKeyItem) {
        // First try to add to existing key item
        for (int i = 0; i < keyItemCount; i++) {
            if (keyItemList[i].id == itemID) {
                keyItemList[i].amount += quantity;
                printf("Added %d to existing key item %s (total: %d)\n", 
                       quantity, keyItemList[i].name, keyItemList[i].amount);
                return;
            }
        }
        
        // If not found, add as new key item
        if (keyItemCount < MAX_KEY_ITEMS) {
            keyItemList[keyItemCount] = *masterItem;
            keyItemList[keyItemCount].amount = quantity;
            keyItemList[keyItemCount].isUnlocked = true;
            printf("Added new key item %s (amount: %d)\n", 
                   keyItemList[keyItemCount].name, quantity);
            keyItemCount++;
        } else {
            printf("Error: Key item inventory full, can't add %s\n", masterItem->name);
        }
    } else {
        // First try to add to existing usable item
        for (int i = 0; i < itemCount; i++) {
            if (playerInventory[i].id == itemID) {
                playerInventory[i].amount += quantity;
                printf("Added %d to existing item %s (total: %d)\n", 
                       quantity, playerInventory[i].name, playerInventory[i].amount);
                return;
            }
        }
        
        // If not found, add as new usable item
        if (itemCount < MAX_ITEMS) {
            playerInventory[itemCount] = *masterItem;
            playerInventory[itemCount].amount = quantity;
            playerInventory[itemCount].isUnlocked = true;
            printf("Added new item %s (amount: %d)\n", 
                   playerInventory[itemCount].name, quantity);
            itemCount++;
        } else {
            printf("Error: Item inventory full, can't add %s\n", masterItem->name);
        }
    }
}


void RemoveItemFromInventory(int itemID, int quantity) {
    // Check regular items // key items do not decrease!
    for (int i = 0; i < itemCount; i++) {
        if (playerInventory[i].id == itemID) {
            playerInventory[i].amount -= quantity;
            if (playerInventory[i].amount <= 0) {
                // Remove the item by shifting the array
                for (int j = i; j < itemCount - 1; j++) {
                    playerInventory[j] = playerInventory[j + 1];
                }
                itemCount--; // Update itemCount
            }
            return;
        }
    }
}

// 7. Update the implementation of IsItemInInventory
bool IsItemInInventory(int itemID, bool checkKeyItems) {
    if (itemID == ITEM_NONE) return false;
    
    if (checkKeyItems) {
        for (int i = 0; i < keyItemCount; i++) {
            if (keyItemList[i].id == itemID) {
                return true;
            }
        }
    } else {
        for (int i = 0; i < itemCount; i++) {
            if (playerInventory[i].id == itemID) {
                return true;
            }
        }
    }
    
    return false;
}


// Enhanced function to add new unlocked items with better handling
void AddNewUnlockedItem(Item item) {
    // First unlock the item in the master list
    for (int i = 0; i < masterItemCount; i++) {
        if (masterItemList[i].id == item.id) {
            masterItemList[i].isUnlocked = true;
            break;
        }
    }
    
    // Then add to the appropriate inventory if not already there
    if (item.isKeyItem) {
        // Check if already in key item list
        bool found = false;
        for (int i = 0; i < keyItemCount; i++) {
            if (keyItemList[i].id == item.id) {
                keyItemList[i].isUnlocked = true;
                found = true;
                break;
            }
        }
        
        // Add to key item list if not found
        if (!found && keyItemCount < MAX_KEY_ITEMS) {
            printf("Adding new key item: %s (ID: %d)\n", item.name, item.id);
            keyItemList[keyItemCount] = item;
            keyItemList[keyItemCount].isUnlocked = true;
            keyItemCount++;
        }
    } else {
        // Check if already in player inventory
        bool found = false;
        for (int i = 0; i < itemCount; i++) {
            if (playerInventory[i].id == item.id) {
                playerInventory[i].isUnlocked = true;
                found = true;
                break;
            }
        }
        
        // Add to player inventory if not found
        if (!found && itemCount < MAX_ITEMS) {
            playerInventory[itemCount] = item;
            playerInventory[itemCount].isUnlocked = true;
            itemCount++;
        }
    }
}


// TODO CHANGE THIS?
void UpdateBattleCommand(BattleState *battleState, BattleParticipant *currentParticipant) {
    if (battleState->inSubmenu) {
        // Handle submenu input
        // this is never reached! REPEATED CODE IN ExecutePlayerCommand!
        HandleSubmenuInput(battleState, currentParticipant);
        // printf("\n IN SUBMENU \n");
    } else if (battleState->targetSelectionActive) {
        // Handle target selection input
        HandleTargetSelectionInput(battleState);
        // printf("\n TARGET SELECTED \n");
    } else if (!battleState->commandConfirmed) {
        // Handle main command menu input
        // printf("\n SELECT COMMAND \n");
        HandleMainCommandInput(battleState);

    }
}

void HandleMainCommandInput(BattleState *battleState) {
    if (IsKeyPressed(KEY_UP)) {
        battleState->selectedCommand = (battleState->selectedCommand - COMMAND_COLS + COMMAND_MAX) % COMMAND_MAX;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        battleState->selectedCommand = (battleState->selectedCommand + COMMAND_COLS) % COMMAND_MAX;
    }
    if (IsKeyPressed(KEY_LEFT)) {
        battleState->selectedCommand = (battleState->selectedCommand % COMMAND_COLS == 0) ? battleState->selectedCommand + (COMMAND_COLS - 1) : battleState->selectedCommand - 1;
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        battleState->selectedCommand = (battleState->selectedCommand % COMMAND_COLS == COMMAND_COLS - 1) ? battleState->selectedCommand - (COMMAND_COLS - 1) : battleState->selectedCommand + 1;
    }
    if (IsKeyPressed(KEY_Z)) {
        if (battleState->selectedCommand == COMMAND_SKILL || battleState->selectedCommand == COMMAND_MAGIC || battleState->selectedCommand == COMMAND_ITEM) {
            // Open the appropriate submenu
            battleState->inSubmenu = true;
            // battleState->commandConfirmed = true;
            switch (battleState->selectedCommand) {
                case COMMAND_SKILL:
                    battleState->submenuType = SUBMENU_SKILL;
                    battleState->submenuCount = skillCount;
                    break;
                case COMMAND_MAGIC:
                    battleState->submenuType = SUBMENU_MAGIC;
                    battleState->submenuCount = magicCount;
                    break;
                case COMMAND_ITEM:
                    battleState->submenuType = SUBMENU_ITEM;
                    battleState->submenuCount = itemCount;
                    break;
                default:
                    break;
            }
            EnterSubmenu(battleState);
            // battleState->submenuSelection = 0;
            // printf("\n MAGIC/SKILL/ITEM SELECTED \n");
        } 
        else {
            // ATTACK SELECTED
            battleState->currentTargetType = GetTargetTypeForAction(battleState);
            if (actionRequiresTarget(battleState)) {
                battleState->targetSelectionActive = true;
                validCount = GetAllValidTargets(battleState, validTargets, MAX_PARTICIPANTS);
                printf("\nvalid count %d", validCount);
                if (validCount > 0) {
                    battleState->targetIndex = validTargets[0]; // Start with the first valid one
                    currentTargetPosition = 0;
                }
                // printf("\n ATTACK SELECTED \n");
                // note that commandConfirmed is FALSE here
            } 
            // RUN OR DEFENSE SELECTED
            else {
                battleState->commandConfirmed = true;
            }
        }
    }
}

void HandleSubmenuInput(BattleState *battleState, BattleParticipant *currentParticipant) {
    int maxItems = battleState->submenuCount;
    int selected = battleState->submenuSelection;
    int startIndex = battleState->submenuStartIndex;
    bool selectionChanged = false;

    if (IsKeyDownSmooth(KEY_UP, &battleState->scrollUpKey)) {
        if (selected > 0) {
            selected--;
            selectionChanged = true;
        }
    }

    if (IsKeyDownSmooth(KEY_DOWN, &battleState->scrollDownKey)) {
        if (selected < maxItems - 1) {
            selected++;
            selectionChanged = true;
        }
    }

    // Move UP by 4 items with A
    if (IsKeyPressed(KEY_A)) {
        if (selected > 0) {
            int newSelected = selected - VISIBLE_SUBMENU_ITEMS;
            if (newSelected < 0) newSelected = 0;
            if (newSelected != selected) {
                selected = newSelected;
                selectionChanged = true;
            }
        }
    }
    if (IsKeyPressed(KEY_S)) {
        if (selected < maxItems - 1) {
            int newSelected = selected + VISIBLE_SUBMENU_ITEMS;
            if (newSelected > maxItems - 1) newSelected = maxItems - 1;
            if (newSelected != selected) {
                selected = newSelected;
                selectionChanged = true;
            }
        }
    }

        // Update startIndex if needed
    if (selected < startIndex) {
        startIndex = selected;
        selectionChanged = true;
    } else if (selected >= startIndex + VISIBLE_SUBMENU_ITEMS) {
        startIndex = selected - (VISIBLE_SUBMENU_ITEMS - 1);
        selectionChanged = true;
    }

    // Write the local changes back to battleState if any change occurred
    if (selectionChanged) {
        battleState->submenuSelection = selected;
        battleState->submenuStartIndex = startIndex;
    }


        // Confirm selection with Z
    if (IsKeyPressed(KEY_Z)) {
        bool canPerformAction = false;

        switch (battleState->submenuType) {
            case SUBMENU_MAGIC: {
                if (selected < battleState->filteredMagicCount) {
                    Magic *selectedMagic = battleState->filteredMagic[selected];
                    if (currentParticipant->creature->currentMP >= selectedMagic->mpCost) {
                        canPerformAction = true;
                    }
                }
                break;
            }
            case SUBMENU_SKILL: {
                if (selected < battleState->filteredSkillCount) {
                    Skill *selectedSkill = battleState->filteredSkills[selected];
                    printf("Selected skill: %s (ID: %s)\n", selectedSkill->name, SkillIDToString(selectedSkill->id));
                    if (currentParticipant->creature->currentMP >= selectedSkill->mpCost) {
                        canPerformAction = true;
                    }
                }
                break;
            }
            case SUBMENU_ITEM: {
                if (selected < battleState->filteredItemCount) {
                    Item *selectedItem = battleState->filteredItems[selected];
                    if (selectedItem->amount > 0) {
                        canPerformAction = true;
                    }
                }
                break;
            }
            default:
                break;
        }

        // Proceed only if the action is valid
        if (canPerformAction) {
            battleState->selectedAction = selected;
            LeaveSubmenu(battleState);

            switch (battleState->submenuType) {
                case SUBMENU_MAGIC:
                    battleState->selectedMagic = battleState->filteredMagic[selected];
                    battleState->speedCost = battleState->selectedMagic->speedCost;
                    break;
                case SUBMENU_SKILL:
                    battleState->selectedSkill = battleState->filteredSkills[selected];
                    printf("Setting selected skill to: %s (ID: %s)\n", 
                           battleState->selectedSkill->name, 
                           SkillIDToString(battleState->selectedSkill->id));
                    battleState->speedCost = battleState->selectedSkill->speedCost;
                    break;
                case SUBMENU_ITEM:
                    battleState->selectedItem = battleState->filteredItems[selected];
                    battleState->speedCost = battleState->selectedItem->speedCost;
                    break;
                default:
                    break;
            }

            // Rest of your action handling code
            battleState->currentTargetType = GetTargetTypeForAction(battleState);
            if (actionRequiresTarget(battleState)) {
                battleState->targetSelectionActive = true;
                validCount = GetAllValidTargets(battleState, validTargets, MAX_PARTICIPANTS);
                if (validCount > 0) {
                    battleState->targetIndex = validTargets[0]; // Start with first valid one
                    currentTargetPosition = 0;
                }
            } else {
                battleState->commandConfirmed = true;
            }
        } else {
            SetBattleMessage(battleState, "Cannot perform the selected action!");
        }
    }

    // Cancel with X
    if (IsKeyPressed(KEY_X)) {
        LeaveSubmenu(battleState);
    }

    // Update selection/indices if changed
    if (selectionChanged) {
        battleState->submenuSelection = selected;
        battleState->submenuStartIndex = startIndex;

        // Remember last selection for this submenu type if desired
        switch (battleState->submenuType) {
            case SUBMENU_MAGIC:
                battleState->lastMagicSelection = selected;
                break;
            case SUBMENU_SKILL:
                battleState->lastSkillSelection = selected;
                break;
            case SUBMENU_ITEM:
                battleState->lastItemSelection = selected;
                break;
            default:
                break;
        }
    }
}

void EnterSubmenu(BattleState *battleState) {
    battleState->inSubmenu = true;
    
    // Build filtered lists for this submenu
    BuildFilteredLists(battleState);
    
    // Calculate the appropriate starting selection and view position
    switch (battleState->submenuType) {
        case SUBMENU_MAGIC: {
            // Use the last selection or default to 0
            int lastSelection = battleState->lastMagicSelection;
            battleState->submenuSelection = MIN(lastSelection, battleState->filteredMagicCount - 1);
            break;
        }
        case SUBMENU_SKILL: {
            int lastSelection = battleState->lastSkillSelection;
            battleState->submenuSelection = MIN(lastSelection, battleState->filteredSkillCount - 1);
            break;
        }
        case SUBMENU_ITEM: {
            int lastSelection = battleState->lastItemSelection;
            battleState->submenuSelection = MIN(lastSelection, battleState->filteredItemCount - 1);
            break;
        }
        default:
            battleState->submenuSelection = 0;
            break;
    }

// Ensure selection is valid
    if (battleState->submenuSelection < 0) 
        battleState->submenuSelection = 0;
    
    // Set the scroll position to show the selection
    // This keeps the selected item visible in the submenu without jumping
    if (battleState->submenuSelection >= VISIBLE_SUBMENU_ITEMS) {
        battleState->submenuStartIndex = battleState->submenuSelection - (VISIBLE_SUBMENU_ITEMS - 1);
    } else {
        battleState->submenuStartIndex = 0;
    }
}


// Call this when entering a submenu
void BuildFilteredLists(BattleState *battleState) {
    // Reset filtered counts
    battleState->filteredItemCount = 0;
    battleState->filteredMagicCount = 0;
    battleState->filteredSkillCount = 0;
    BattleParticipant *participant = battleState->currentParticipant;

    // Filter based on submenu type
    switch (battleState->submenuType) {
        case SUBMENU_MAGIC:
            for (int i = 0; i < magicCount; i++) {
                bool hasThisMagic = CreatureHasMagic(participant->creature, magicList[i].id);
                if (magicList[i].isUnlocked && hasThisMagic) {
                    battleState->filteredMagic[battleState->filteredMagicCount++] = &magicList[i];
                }
            }
            battleState->submenuCount = battleState->filteredMagicCount;
            break;
            
        case SUBMENU_SKILL:
            for (int i = 0; i < skillCount; i++) {               
                bool hasThisSkill = CreatureHasSkill(participant->creature, skillList[i].id);
                if (skillList[i].isUnlocked && hasThisSkill) {
                    battleState->filteredSkills[battleState->filteredSkillCount++] = &skillList[i];
                }
            }
            battleState->submenuCount = battleState->filteredSkillCount;
            break;
            
        case SUBMENU_ITEM:
            for (int i = 0; i < itemCount; i++) {
                if (playerInventory[i].isUnlocked && playerInventory[i].amount > 0 && 
                    (playerInventory[i].usageType == USAGE_BOTH || playerInventory[i].usageType == USAGE_BATTLE)) {
                    battleState->filteredItems[battleState->filteredItemCount++] = &playerInventory[i];
                }
            }
            battleState->submenuCount = battleState->filteredItemCount;
            break;
            
        default:
            break;
    }
}

void LeaveSubmenu(BattleState *battleState) {
    // Save current selection to remember it even after leaving
    int selected = battleState->submenuSelection;
    switch (battleState->submenuType) {
        case SUBMENU_MAGIC:
            battleState->lastMagicSelection = selected;
            break;
        case SUBMENU_SKILL:
            battleState->lastSkillSelection = selected;
            break;
        case SUBMENU_ITEM:
            battleState->lastItemSelection = selected;
            break;
        default:
            break;
    }
    battleState->inSubmenu = false;
}

void HandleTargetSelectionInput(BattleState *battleState) {
    if (!battleState->toggleTargetSelectionMode && battleState->currentTargetType!=TARGET_ALL) {
        if (IsKeyPressed(KEY_LEFT)) {
            // battleState->targetIndex = GetPreviousTargetIndex(battleState, battleState->targetIndex);
            currentTargetPosition = (currentTargetPosition - 1 + validCount) % validCount;
            printf("\nleft position %d", currentTargetPosition);
            battleState->targetIndex = validTargets[currentTargetPosition];
            printf("\n Is selected player? %d",battleState->participants[battleState->targetIndex].isPlayer );
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            // battleState->targetIndex = GetNextTargetIndex(battleState, battleState->targetIndex);
            currentTargetPosition = (currentTargetPosition + 1) % validCount;
            printf("\nright position %d", currentTargetPosition);
            battleState->targetIndex = validTargets[currentTargetPosition];
            printf("\n Is selected player? %d",battleState->participants[battleState->targetIndex].isPlayer);
        }
    }
    // Implement toggling between single and multiple targets
    if (IsKeyPressed(KEY_C)) {
        ToggleTargetSelectionMode(battleState);
        
    }
    if (IsKeyPressed(KEY_Z)) {
        // Confirm target
        battleState->commandConfirmed = true;
        battleState->targetSelectionActive = false;
        battleState->toggleTargetSelectionMode = false;
        
    }
    if (IsKeyPressed(KEY_X)) {
        // Cancel target selection
        battleState->targetSelectionActive = false;
        battleState->toggleTargetSelectionMode = false;
        // Clear selected action data
        battleState->selectedMagic = NULL;
        battleState->selectedSkill = NULL;
        battleState->selectedItem = NULL;
        battleState->currentParticipant = NULL;  // Add this
        // Reset the speed cost to default so that actions do not take previous speedcosts...
        battleState->speedCost = standardSpeedCost;

        // Return to command menu or submenu as appropriate
        if (battleState->submenuType != SUBMENU_NONE) {
            battleState->inSubmenu = true;
        }
        if (battleState->selectedCommand == COMMAND_ATTACK) {
            battleState->inSubmenu = false;
        }
    }
    // printf("\n SELECTED TARGET: %d\n",battleState->targetIndex); 
}

void ToggleTargetSelectionMode(BattleState *battleState) {
    // Do not allow toggling for TARGET_SINGLE
    if (battleState->currentTargetType == TARGET_SINGLE) {
        return;
    }
    // flip the toggle everytime we enter here!
    battleState->toggleTargetSelectionMode ^= true;
    switch (battleState->currentTargetType) {

        case TARGET_ALL_ENEMIES:
        case TARGET_ALL_ALLIES:
            battleState->currentTargetType = TARGET_ANY;
            break;

        case TARGET_ANY:
            if (battleState->participants[battleState->targetIndex].isPlayer) {
                battleState->currentTargetType = TARGET_ALL_ALLIES;
            }
            if (!battleState->participants[battleState->targetIndex].isPlayer) {
                battleState->currentTargetType = TARGET_ALL_ENEMIES;
            }
            
            break;
        case TARGET_ALL:
            battleState->currentTargetType = TARGET_ALL;

        default:
            // For other target types (TARGET_SELF, TARGET_NONE, TARGET_ALL) no toggling
            break;
    }
}


TargetType GetTargetTypeForAction(BattleState *battleState) {
    // Check if we have selected a Magic/Skill/Item already
    switch (battleState->selectedCommand) {
        case COMMAND_ATTACK:
            return TARGET_SINGLE;
        case COMMAND_MAGIC:
            return battleState->selectedMagic->targetType;  
        case COMMAND_ITEM:
            return battleState->selectedItem->targetType;                      
        case COMMAND_SKILL:
            return battleState->selectedSkill->targetType;  
        case COMMAND_DEFEND:
            return TARGET_SELF;
        case COMMAND_RUN:
            return TARGET_NONE;
        default:
            return TARGET_NONE;
    }
}

bool actionRequiresTarget(BattleState *battleState) {
    TargetType targetType = battleState->currentTargetType;
    switch (targetType) {
        case TARGET_SELF:
        case TARGET_NONE:
            return false; // No target selection needed
        default:
            return true; // Requires target selection
    }
}

int GetAllValidTargets(BattleState *battleState, int *validTargets, int maxTargets) {
    int count = 0;
    int allies_alive[MAX_PARTICIPANTS];
    int allyCount_alive = 0;
    int enemies_alive[MAX_PARTICIPANTS];
    int enemyCount_alive = 0;
    int allies_dead[MAX_PARTICIPANTS];
    int allyCount_dead = 0;


    // Determine if we are using Phoenix Feather
    bool usingPhoenixDown = (battleState->selectedCommand == COMMAND_ITEM && battleState->selectedItem && battleState->selectedItem->id == ITEM_PHOENIX_FEATHER);

    // Collect all valid participants first, separating alive/dead allies and alive enemies
    for (int i = 0; i < battleState->participantCount && count < maxTargets; i++) {
        if (IsValidTarget(battleState, i)) { // Use IsValidTarget to filter correctly based on item/spell
            if (battleState->participants[i].isPlayer) {
                if (battleState->participants[i].creature->currentHP <= 0) {
                    allies_dead[allyCount_dead++] = i; // Dead allies
                } else {
                    allies_alive[allyCount_alive++] = i; // Alive allies
                }
            } else {
                enemies_alive[enemyCount_alive++] = i; // Alive enemies
            }
        }
    }

    if (usingPhoenixDown) {
        // For Phoenix Down: Prioritize dead allies first, then alive allies, then enemies
        if (allyCount_dead > 0) {
            validTargets[count++] = allies_dead[0]; // Select the first dead ally as default if available
        } else if (allyCount_alive > 0) {
             validTargets[count++] = allies_alive[0]; // If no dead allies, default to first alive ally
        }

        // Add ALL dead allies first
        for (int a = 0; a < allyCount_dead && count < maxTargets; a++) {
            if (!(allyCount_dead > 0 && a == 0)) validTargets[count++] = allies_dead[a]; // avoid adding the first one twice
        }
        // Then add all alive allies
        for (int a = 0; a < allyCount_alive && count < maxTargets; a++) {
             if (!(allyCount_alive > 0 && allyCount_dead == 0 && a == 0)) validTargets[count++] = allies_alive[a]; // avoid adding the first alive ally twice if no dead allies were first
        }
        // Finally alive enemies (though phoenix down usually targets allies)
        for (int e = 0; e < enemyCount_alive && count < maxTargets; e++) {
            validTargets[count++] = enemies_alive[e];
        }
    }
    else {
        // For Healing (non-Phoenix) and Damage: Use existing logic (Alive Allies then Enemies for healing, Enemies then Allies for damage)
        bool isHealing = false;
         if (battleState->selectedCommand == COMMAND_MAGIC && battleState->selectedMagic) {
            if (IsHealingMagic(battleState->selectedMagic)) {
                isHealing = true;
            }
        } else if (battleState->selectedCommand == COMMAND_ITEM && battleState->selectedItem) {
            if (IsHealingItem(battleState->selectedItem)) {
                isHealing = true;
            }
        }


        if (isHealing) {
            for (int a = 0; a < allyCount_alive && count < maxTargets; a++) {
                validTargets[count++] = allies_alive[a];
            }
            for (int e = 0; e < enemyCount_alive && count < maxTargets; e++) {
                validTargets[count++] = enemies_alive[e];
            }
        } else {
            for (int e = 0; e < enemyCount_alive && count < maxTargets; e++) {
                validTargets[count++] = enemies_alive[e];
            }
            for (int a = 0; a < allyCount_alive && count < maxTargets; a++) {
                validTargets[count++] = allies_alive[a];
            }
        }
    }

    return count;
}


bool IsValidTarget(BattleState *battleState, int index) {
    BattleParticipant *participant = &battleState->participants[index];
    TargetType targetType = battleState->currentTargetType;
    Creature *targetedCreature = participant->creature;

    bool isDead = (targetedCreature->currentHP <= 0);
    bool isAlly = participant->isPlayer;

    // Determine if we are using Phoenix Feather
    bool usingPhoenixDown = (battleState->selectedCommand == COMMAND_ITEM && battleState->selectedItem && battleState->selectedItem->id == ITEM_PHOENIX_FEATHER);

    // Determine if we are using a healing item (but not Phoenix Feather)
    bool isHealingItemNotPhoenixDown = (battleState->selectedCommand == COMMAND_ITEM && battleState->selectedItem && IsHealingItem(battleState->selectedItem) && !usingPhoenixDown);

    bool isMagicHealing = (battleState->selectedCommand == COMMAND_MAGIC && battleState->selectedMagic && IsHealingMagic(battleState->selectedMagic));

    bool isHealing = isHealingItemNotPhoenixDown || isMagicHealing;


    // Rules:
    // - Phoenix Feather can target dead or alive allies.
    // - Other healing items CANNOT target dead allies.
    // - Damaging actions CANNOT target dead participants.

    if (usingPhoenixDown && isAlly) {
        if (targetType == TARGET_SINGLE || targetType == TARGET_ANY || targetType == TARGET_ALLY || targetType == TARGET_ALL_ALLIES) {
            return isAlly; // Phoenix Down valid on both dead and alive allies
        }
    }

    if ((isHealingItemNotPhoenixDown || isMagicHealing) && isAlly) {
        if (targetType == TARGET_SINGLE || targetType == TARGET_ANY || targetType == TARGET_ALLY || targetType == TARGET_ALL_ALLIES) {
            return isAlly && !isDead; // Healing items (not phoenix) only valid on alive allies
        }
    }

    // if (isMagicHealing && isAlly) {
    //     if (targetType == TARGET_SINGLE || targetType == TARGET_ANY || targetType == TARGET_ALLY || targetType == TARGET_ALL_ALLIES) {
    //         return isAlly && !isDead; // Healing magic only valid on alive allies
    //     }
    // }

    // For non-healing items and damaging actions, must be alive
    if (isDead && !usingPhoenixDown && !isHealing) return false;


    switch (targetType) {
        case TARGET_SELF:
            return (participant == battleState->currentParticipant && !isDead); // Self must be alive

        case TARGET_ALLY:
        case TARGET_ALL_ALLIES:
            return isAlly && !isDead; // Allies must be alive (except for Phoenix Down logic above)

        case TARGET_ENEMY:
        case TARGET_ALL_ENEMIES:
            return !isAlly && !isDead; // Enemies must be alive

        case TARGET_SINGLE:
        case TARGET_ANY:
            return !isDead; // Any alive target (unless phoenix down and dead ally logic applies)

        case TARGET_ALL:
            return false; // Usually no selection needed

        default:
            return false;
    }
}



// TODO CHANGE THIS
// to encorage strategy, trying to run and failing increases speed, defending increases defense, attacking increase attack. (these effects are not permanant and are only temperorary during battle)
// ExecutePlayerCommand processes the chosen command when confirmed
// it only gets here if battleState->commandConfirmed is TRUE
void ExecutePlayerCommand(BattleState *battleState, BattleParticipant *participant) {
    battleState->currentParticipant = participant;  // Ensure actor is set
    int delayTimer = 12;
    
    Creature *attackerCreature = participant->creature;
    switch (battleState->selectedCommand) {
        // BattleParticipant *defender = &battleState->participants[battleState->targetIndex];
        case COMMAND_SKILL: {
            battleState->inSubmenu = true;  // Open submenu for these commands
            battleState->submenuType = SUBMENU_SKILL;
            battleState->submenuSelection = skillCount;
            battleState->submenuCount = skillCount;
            attackerCreature->tempLuckBuff += RANDOMS(1,3);
            attackerCreature->tempSpeedBuff += RANDOMS(1,3);
            attackerCreature->tempAttackBuff += RANDOMS(1,3);
            battleState->commandMenuDelay = delayTimer;
            UseSkill(battleState, participant);

            break;
        }
        case COMMAND_MAGIC: {
            battleState->inSubmenu = true;  // Open submenu for these commands
            battleState->submenuType = SUBMENU_MAGIC;
            battleState->submenuSelection = magicCount;
            battleState->submenuCount = magicCount;
            attackerCreature->tempMagicBuff += RANDOMS(1,3);
            attackerCreature->tempMagicdefenseBuff += RANDOMS(1,3);
            battleState->rolloutConsecutiveUses = 0;
            battleState->commandMenuDelay = delayTimer;
            UseMagic(battleState, participant);

            break;
        }
        case COMMAND_ITEM: {
            battleState->inSubmenu = true;  // Open submenu for these commands
            battleState->submenuType = SUBMENU_ITEM;
            battleState->submenuSelection = itemCount;
            battleState->submenuCount = itemCount;
            attackerCreature->tempLuckBuff += RANDOMS(1,3);
            battleState->rolloutConsecutiveUses = 0;
            battleState->commandMenuDelay = delayTimer;
            UseItem(battleState, participant);

            break;
        }
        case COMMAND_ATTACK: {
            // attack buffs
            attackerCreature->tempAttackBuff += RANDOMS(1,3);
            attackerCreature->tempAccuracyBuff += RANDOMS(1,3);
            battleState->rolloutConsecutiveUses = 0;
            battleState->commandMenuDelay = delayTimer;
            UseAttack(battleState,participant);

            break;
        }
        case COMMAND_DEFEND: {
            // defense buffs
            attackerCreature->tempDefenseBuff += RANDOMS(1,3);
            attackerCreature->tempMagicdefenseBuff += RANDOMS(1,3);
            attackerCreature->tempAccuracyBuff += RANDOMS(1,3);
            (attackerCreature->currentHP + 5 > attackerCreature->maxHP) ? (attackerCreature->currentHP = attackerCreature->maxHP) : (attackerCreature->currentHP = attackerCreature->currentHP + RANDOMS(1,5));
            (attackerCreature->currentMP + 3 > attackerCreature->maxMP) ? (attackerCreature->currentMP = attackerCreature->maxMP) : (attackerCreature->currentMP = attackerCreature->currentMP + RANDOMS(1,3));
            // battleState->currentTargetType == TARGET_SELF;
            // defense debuffs
            attackerCreature->tempEvasionBuff -= RANDOMS(1,3);
            battleState->rolloutConsecutiveUses = 0;
            battleState->commandMenuDelay = delayTimer-3;
            battleState->speedCost = 0.9f*standardSpeedCost;
            printf("\nDEFENDE\n");
            UpdateNextActionTime(battleState,participant);

            break;
        }
        case COMMAND_RUN: {
            // battleState->currentTargetType == TARGET_SELF;
            PlayerRunning(battleState);
            // if it fails improve slightly the speed
            attackerCreature->tempSpeedBuff += RANDOMS(1,3);
            battleState->rolloutConsecutiveUses = 0;
            battleState->commandMenuDelay = delayTimer-3;
            printf("\nRUN\n");
            UpdateNextActionTime(battleState,participant);

            break;
        }
        default:
            break;
    }
    
    // Reset flags
    battleState->commandConfirmed = false;
    battleState->targetSelectionActive = false;
    battleState->inSubmenu = false;

} 

    
    


// TODO CHANGE BATTLESTATE->ENEMY
void DrawBattleInterface(BattleState *battleState) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Ensure there's at least one participant in turnOrder
    if (battleState->participantCount == 0) {
        ClearBackground(BLACK);
        DrawText("No participants in battle!", 20, 20, 20, RED);
        return;
    }


    int nextIndex = -1;
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->turnOrder[i]->isActive && battleState->turnOrder[i]->creature->currentHP > 0) {
            nextIndex = i;
            break;
        }
    }

    if (nextIndex == -1) {
        // No active participants left -> battle ends?
        return;
    }

    BattleParticipant *nextUp = battleState->turnOrder[nextIndex]; // Who is next to act
    static int blinkCounter = 0;
    blinkCounter++;
    bool visible = ((blinkCounter / 1) % 2 == 0); // toggles every 30 frames

    // Shake or blink effect if needed
    bool shouldBlink = false; // If you have conditions for blink
    int shakeOffsetY = (shouldBlink) ? 10 : 0;

    // Background
    ClearBackground(shouldBlink ? WHITE : BLACK);

    // Layout constants
    int characterBoxHeight = 100;
    int characterBoxWidth = 120;
    int characterBoxYPos = screenHeight - characterBoxHeight + shakeOffsetY;

    int commandBoxHeight = 80;
    int commandBoxY = 40;
    int boarderX = 20;

    int backgroundY = commandBoxY + commandBoxHeight + 20;
    int backgroundHeight = screenHeight - commandBoxHeight - characterBoxHeight;

    // Draw any animated background or scenery
    DrawAnimatedBackground(backgroundY, backgroundHeight);
   


    // Draw enemies
    if (battleState->isBattleActive) {
        int aliveEnemies = 0;
        for (int i = 0; i < battleState->participantCount; i++) {
            if ((!battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0 && battleState->participants[i].isActive) || battleState->participants[i].deathTimer > 0) {
                aliveEnemies++;
            }
        }

        
        Vector2 enemyPositions[MAX_PARTICIPANTS];
        int enemyIndices[MAX_PARTICIPANTS];
        int enemyCount = 0;
        int spriteWidth = 0;
        int spriteHeight = 0;
        Texture2D enemySprite[MAX_PARTICIPANTS];
        float scale = 11.0f / 3.0f;

        // if (battleState->deathTimer == 0) battleState->deathActivated=false;

        if (aliveEnemies >= 0) {
            if (aliveEnemies == 0) aliveEnemies = 1;
            int enemySpacing = screenWidth / (aliveEnemies + 1);
            for (int i = 0; i < battleState->participantCount; i++) {
                BattleParticipant *participant = &battleState->participants[i];
                if ((!participant->isPlayer && participant->creature->currentHP > 0 && participant->isActive) || participant->deathTimer > 0) {
                    // Inside the enemy drawing loop:
                    int healthPercent = (participant->creature->currentHP * 100) / participant->creature->maxHP;

                    int spriteIndex = 0; // Default to full health sprite
                    // DO SOME STATE CHANGE CHECK IF BLINKING. ADD VARIABLE TO BATTLESTATE!
                    if (healthPercent == 100) spriteIndex = PERFECT;
                    else if (healthPercent >= 75) spriteIndex = HEALTHY;
                    else if (healthPercent >= 50) spriteIndex = OK;
                    else if (healthPercent >= 25) spriteIndex = BAD;
                    else if (healthPercent > 0 || healthPercent <= 0) spriteIndex = DEADLY;

                    Texture2D spriteHealth = participant->creature->sprites[battleState->terrain][spriteIndex];
                    
                    spriteWidth = spriteHealth.width;
                    spriteHeight = spriteHealth.height;
                    // printf("\nSpirte number: %d Sprite height: %d Sprite width: %d \n", i, spriteHeight, spriteWidth);

                    int ex = enemySpacing * (enemyCount + 1) - (spriteWidth * scale / 2);
                    int ey = backgroundY + 40;

                    // Store enemy position and index
                    enemyPositions[enemyCount] = (Vector2){ex, ey};
                    enemyIndices[enemyCount] = i;
                    enemySprite[enemyCount] = spriteHealth;
                    
                    // Draw enemy sprite
                    int c = 255*((float)participant->deathTimer/2.0f)/DEATH_TIMER;
                    if (c < 0) c = 0;

                    if (healthPercent <= 0) DrawTextureEx(spriteHealth, (Vector2){ex, ey}, 0.0f, scale, (Color){c,c,c,255});
                    else DrawTextureEx(spriteHealth, (Vector2){ex, ey}, 0.0f, scale, WHITE);

                    // before selecting the new enemy to attack
                    if (battleState->targetSelectionActive) {
                        // these lines need to be here, if we are selecting enemy reset everything
                        participant->pendingDamage = -1;
                        participant->damageOffsetY = 0.0f;
                        participant->effectType = EFFECT_NONE; // Reset action type

                        int drawX = ex + (spriteWidth * scale / 2) - 5;
                        int drawY = ey - 20;
                        switch (battleState->currentTargetType) {
                            case TARGET_ENEMY:
                            case TARGET_SINGLE:
                            case TARGET_ANY:
                                 if (i == battleState->targetIndex) {
                                    DrawText("^", drawX, drawY, 20, YELLOW);
                                }
                                break;

                            case TARGET_ALL:
                            case TARGET_ALL_ENEMIES:
                                if (visible) {
                                    DrawText("^", drawX, drawY, 20, YELLOW);
                                }
                                break;

                            default:
                                break;
                        }
                    }
                    enemyCount++;
                
            
            

                    if (battleState->decisionMade) {
                        if (participant->damageTimer > 0) {
                            // Compute vertical offset
                            participant->damageOffsetY -= 0.5f;

                            // Determine the color based on the action type
                            Color damageColor = GetDamageColor(participant->effectType);
                            
                            damageColor.a = 255*participant->damageTimer/DAMAGE_TIMER;

                            // Draw the damage number above the enemy's position

                            if (participant->pendingDamage >= 0 && participant->effectType !=EFFECT_DEBUFF && participant->effectType !=EFFECT_BUFF) {
                                DrawText(TextFormat("%d",participant->pendingDamage),ex,ey + participant->damageOffsetY,40, damageColor);
                            }                            

                            // Decrement the timer
                            participant->damageTimer = participant->damageTimer - 5;
                            battleState->attackAnimation.currentFrame++;  // DO NOT REMOVE, makes attack animations smoother!
                            if (participant->damageTimer == 0) {
                                // Reset pending damage once the animation is complete
                                participant->pendingDamage = -1;
                                participant->damageOffsetY = 0.0f;
                                participant->effectType = EFFECT_NONE; // Reset action type
                            }
                        }
                    }  
                }
            }

            int targetEnemyIndex = -1;
            for (int e = 0; e < enemyCount; e++) {
                if (enemyIndices[e] == battleState->targetIndex) {
                    targetEnemyIndex = e;
                    break;
                }
            }
        
        // make it slightly smaller than the enemy sprite 11/3 > 11/4
            if (battleState->attackAnimation.isAnimating) {
                Texture2D attackFrame = battleState->attackAnimation.frames[battleState->attackAnimation.currentFrame];
                float scaleAttack = 11.0f / 4.0f;
                if (targetEnemyIndex != -1) {
                    Vector2 pos = enemyPositions[targetEnemyIndex]; 
                    int spriteX = pos.x;
                    int spriteY = pos.y;
                        
                    // Load attack animation
                    if (battleState->selectedCommand == COMMAND_ATTACK) {
                        // note that the slash images are slightly bigger than the other (bigger in terms of content not pixel size, the pixel size is the same)
                        float dx = attackFrame.width;
                        float dy = attackFrame.height;
                        DrawTextureEx(attackFrame, (Vector2){spriteX + dx, spriteY + dy - 20}, 0.0f, scaleAttack, battleState->attackAnimation.color);
                        DrawTextureEx(attackFrame, (Vector2){spriteX + dx, spriteY + dy + 20}, 0.0f, scaleAttack, battleState->attackAnimation.color);
                    }

                    else if (battleState->selectedCommand == COMMAND_MAGIC) {
                        Magic *selectedMagic = battleState->selectedMagic;
                        printf("Drawing animation for magic: %s (frame: %d/%d)\n",  selectedMagic->name,  battleState->attackAnimation.currentFrame, battleState->attackAnimation.frameCount);

                        float scaleMagic = scaleAttack * 2.0f;
                        if (battleState->currentTargetType == TARGET_ALL_ENEMIES) {
                            for (int i = 0; i < enemyCount; i++) {
                                Vector2 mpos = enemyPositions[i];
                                DrawTextureEx(attackFrame, (Vector2){mpos.x , mpos.y }, 0.0f, scaleMagic, battleState->attackAnimation.color);
                            }
                        }
                        else {
                            DrawTextureEx(attackFrame, (Vector2){spriteX , spriteY }, 0.0f, scaleMagic, battleState->attackAnimation.color);
                        }
                    }

                    else if (battleState->selectedCommand == COMMAND_ITEM) {
                        Item *selectedItem = battleState->selectedItem;
                        if (IsHealingItem(selectedItem)) {
                            int distribution = 8;
                            for (int i = 0; i <= distribution; i++) {
                                int R = battleState->attackAnimation.color.r;
                                int G = battleState->attackAnimation.color.g;
                                int B = battleState->attackAnimation.color.b;
                                // int confinement = spriteWidth;
                                int X = RANDOMS(spriteX + spriteWidth, spriteX + 2 * spriteWidth);
                                int Y = RANDOMS(spriteY + spriteHeight, spriteY + 2 * spriteHeight);
                                int Rot = RANDOMS(0,360);
                                int A = RANDOMS(180,360);
                                DrawTextureEx(attackFrame, (Vector2){X, Y}, Rot, scaleAttack, (Color){R,G,B,A});
                            }
                        }
                        else if (battleState->currentTargetType == TARGET_ALL) {
                            int distribution = 20;
                                for (int i = 0; i <= distribution; i++) {
                                    int R = battleState->attackAnimation.color.r;
                                    int G = battleState->attackAnimation.color.g;
                                    int B = battleState->attackAnimation.color.b;
                                    int X = RANDOMS(0, screenWidth);
                                    int Y = RANDOMS(0, screenHeight);
                                    int Rot = RANDOMS(0,360);
                                    int A = RANDOMS(180,360);
                                    DrawTextureEx(attackFrame, (Vector2){X, Y}, Rot, scaleAttack, (Color){R,G,B,A});
                                }
                        }
                        else {
                            float dx = attackFrame.width * scaleAttack / 2;
                            float dy = attackFrame.height * scaleAttack / 2;
                            DrawTextureEx(attackFrame, (Vector2){spriteX + dx, spriteY + dy}, 0.0f, scaleAttack, battleState->attackAnimation.color);
                        }
                    }
                    else if (battleState->selectedCommand == COMMAND_SKILL) {

                        float scaleSkill = scaleAttack * 2.0f;

                        Skill *selectedSkill = battleState->selectedSkill;
                        printf("Drawing animation for skill: %s (frame: %d/%d)\n", selectedSkill->name, battleState->attackAnimation.currentFrame, battleState->attackAnimation.frameCount);
                        if (strcmp(selectedSkill->name, "Rollout") == 0) {
                            float counterframes = (float)battleState->attackAnimation.currentFrame/battleState->attackAnimation.frameCount;
                            float Y = (spriteY - screenHeight) * counterframes + screenHeight;
                            int dx = 0;
                            if (battleState->attackAnimation.currentFrame < 3) {
                                dx = battleState->attackAnimation.frames->width * 3/2;
                            }
                            else if ((battleState->attackAnimation.currentFrame >= 3) && (battleState->attackAnimation.currentFrame < 6)) {
                                dx = battleState->attackAnimation.frames->width / 2;
                            }
                            else {
                                dx = -2*battleState->attackAnimation.frames->width;
                            }
                            // enemy->sprites[battleState->terrain].width = 50?
                            DrawTextureEx(attackFrame, (Vector2){spriteX + dx + 50 * scaleAttack / 2, Y},  360/3*battleState->attackAnimation.currentFrame%3, scaleAttack, WHITE);
                        }

                        else if (strcmp(selectedSkill->name, "Charm") == 0) {
                            if (battleState->currentTargetType == TARGET_ALL_ENEMIES) {
                                for (int i = 0; i < enemyCount; i++) {
                                    Vector2 mpos = enemyPositions[i];
                                    DrawTextureEx(attackFrame, (Vector2){mpos.x , mpos.y }, 0.0f, scaleAttack, battleState->attackAnimation.color);
                                    DrawTextureEx(enemySprite[i], (Vector2){mpos.x, mpos.y}, 0.0f, scale, (Color){255*((float)battleState->attackAnimation.currentFrame)/battleState->attackAnimation.frameCount,0,0,255*((float)battleState->attackAnimation.currentFrame)/battleState->attackAnimation.frameCount});
                                    DrawText(battleState->participants[i].statChange, mpos.x, mpos.y*0.9,40, GetDamageColor(EFFECT_DEBUFF));
                                }
                            }
                            else {
                                DrawTextureEx(enemySprite[targetEnemyIndex], (Vector2){spriteX, spriteY}, 0.0f, scale, (Color){255*((float)battleState->attackAnimation.currentFrame)/battleState->attackAnimation.frameCount,0,0,255*((float)battleState->attackAnimation.currentFrame)/battleState->attackAnimation.frameCount});    
                                DrawTextureEx(attackFrame, (Vector2){spriteX , spriteY }, 0.0f, scaleAttack, battleState->attackAnimation.color);
                                DrawText(battleState->participants[targetEnemyIndex].statChange, spriteX, spriteY*0.9,40, GetDamageColor(EFFECT_DEBUFF));
                            }
                        }

                        else {
                            float dx = attackFrame.width / 2;
                            float dy = attackFrame.height / 2;
                            DrawTextureEx(attackFrame, (Vector2){spriteX + dx, spriteY + dy}, 0.0f, scaleSkill, battleState->attackAnimation.color);
                        }
                    }

                    // For the enemy that is the current target (or for all enemies if desired):
                    
                }
            }
        }
    }



    // Drawing Allies
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        Creature *creature = &mainTeam[i];
        if (creature->name[0] == '\0') continue;

        bool isCurrent = (nextUp->creature == creature && nextUp->isPlayer);
        int shiftHeight = isCurrent ? 10 : 0;

        int characterBoxX = 20 + i * (characterBoxWidth + 10);
        
        Color hpColor = WHITE;
        Color mpColor = WHITE;
        Color nameColor = WHITE;
        Color boxColor = DARKPURPLE;
        Color borderColor = WHITE;

        // Check if dead first
        if (creature->currentHP == 0) {
            // Dead
            hpColor = RED;
            mpColor = RED;
            nameColor = RED;
            boxColor = BLACK;
            borderColor = GRAY;
        } else {
            // Alive, now check low HP/MP
            if (creature->maxHP > 0 && creature->currentHP < creature->maxHP / 4) {
                hpColor = YELLOW;
            }
            
            if (creature->maxMP > 0 && creature->currentMP < creature->maxMP / 4) {
                mpColor = YELLOW;
            } else if (creature->maxMP > 0 && creature->currentMP == 0) {
                mpColor = RED;
            }
        }

        // Draw
        DrawRectangle(characterBoxX, characterBoxYPos - shiftHeight, characterBoxWidth, characterBoxHeight, boxColor);
        DrawRectangleLines(characterBoxX, characterBoxYPos - shiftHeight, characterBoxWidth, characterBoxHeight, borderColor);
        DrawText(creature->name, characterBoxX + 10, characterBoxYPos + 10 - shiftHeight, 20, nameColor);
        DrawText(TextFormat("HP: %d/%d", creature->currentHP, creature->maxHP), characterBoxX + 10, characterBoxYPos + 40 - shiftHeight, 20, hpColor);
        DrawText(TextFormat("MP: %d/%d", creature->currentMP, creature->maxMP), characterBoxX + 10, characterBoxYPos + 60 - shiftHeight, 20, mpColor);

        // Get participant index for this ally
        int participantIndex = GetParticipantIndexForAlly(battleState, creature);
        if (participantIndex == -1) continue;

        BattleParticipant *ally = &battleState->participants[participantIndex];

        // Draw damage/healing numbers if applicable
        if (ally->pendingDamage >= 0 && ally->damageTimer > 0 && ally->creature->currentHP>0) {
            // Compute the vertical offset (for example, float upward over time)
            ally->damageOffsetY -= 0.5f;  // adjust this value to control speed

            // Determine the color based on the action type
            Color damageColor = WHITE;
            if (ally->effectType == EFFECT_OFFENSIVE) {
                damageColor = WHITE; // Damage to allies is red
            } else if (ally->effectType == EFFECT_HEALING) {
                damageColor = CYAN; // Healing is green
            }

            damageColor.a = 255*ally->damageTimer/DAMAGE_TIMER;

            // Draw the damage number above the ally's character box
            DrawText(TextFormat("%d", ally->pendingDamage),
                    characterBoxX + characterBoxWidth / 2, // Center above the box
                    characterBoxYPos - shiftHeight - 20 + ally->damageOffsetY, // Position above the box
                    40, damageColor);

            // Decrement the timer
            ally->damageTimer= ally->damageTimer -4;
            if (ally->damageTimer <= 0) {
                // Reset pending damage once the animation is complete
                ally->pendingDamage = -1;
                ally->damageOffsetY = 0.0f;
                ally->effectType = EFFECT_NONE; // Reset action type
            }
        }

        if (battleState->targetSelectionActive && participantIndex != -1) {
            int drawX = characterBoxX + characterBoxWidth / 2;
            int drawY = characterBoxYPos - shiftHeight - 20;
            // Show arrow logic:
            switch (battleState->currentTargetType) {
                case TARGET_ALLY:
                case TARGET_SINGLE:
                case TARGET_ANY:
                    if (participantIndex == battleState->targetIndex) {
                        DrawText("^", drawX, drawY, 20, YELLOW);
                    }
                    break;
                case TARGET_ALL:
                case TARGET_ALL_ALLIES:
                    if (visible) {
                        if (creature->currentHP > 0) DrawText("^", drawX, drawY, 20, YELLOW);
                    }
                    break;

                default:
                    // No arrow for other target types (like TARGET_SELF, TARGET_NONE, TARGET_ALL if it means everyone)
                    break;
            }
        }

        if (battleState->attackAnimation.isAnimating) {
            Texture2D attackFrame = battleState->attackAnimation.frames[battleState->attackAnimation.currentFrame];
            float scaleAttack = 11.0f / 4.0f;
            if (battleState->selectedCommand == COMMAND_ITEM) {
                Item *selectedItem = battleState->selectedItem;
                if (IsHealingItem(selectedItem)) {
                    if (battleState->participants[battleState->targetIndex].isPlayer) {
                        int distribution = 8;
                        for (int i = 0; i <= distribution; i++) {
                            int R = battleState->attackAnimation.color.r;
                            int G = battleState->attackAnimation.color.g;
                            int B = battleState->attackAnimation.color.b;
                            int confinement = 200;
                            int X = confinement + i * (screenWidth - 2*confinement)/distribution + RANDOMS(-10,10);
                            int Y = RANDOMS(screenHeight*3/4,screenHeight);
                            int Rot = RANDOMS(0,360);
                            int A = RANDOMS(180,360);
                            DrawTextureEx(attackFrame, (Vector2){X, Y}, Rot, scaleAttack, (Color){R,G,B,A});
                        }   
                    }
                }
            }
            if (battleState->selectedCommand == COMMAND_MAGIC) {
                Magic *selectedMagic = battleState->selectedMagic;
                if (selectedMagic->baseDamage < 0) {
                    if (battleState->participants[battleState->targetIndex].isPlayer) {
                        int distribution = 8;
                        for (int i = 0; i <= distribution; i++) {
                            int R = battleState->attackAnimation.color.r;
                            int G = battleState->attackAnimation.color.g;
                            int B = battleState->attackAnimation.color.b;
                            int confinement = 200;
                            int X = confinement + i * (screenWidth - 2*confinement)/distribution + RANDOMS(-10,10);
                            int Y = RANDOMS(screenHeight*3/4,screenHeight);
                            int Rot = RANDOMS(0,360);
                            int A = RANDOMS(180,360);
                            DrawTextureEx(attackFrame, (Vector2){X, Y}, Rot, scaleAttack, (Color){R,G,B,A});
                        }   
                    }
                }
            }
        }
    }




    if (nextUp->isPlayer && nextUp->isActive && battleState->commandMenuDelay <= 0 && !IsBattleOver(battleState) && !battleState->attackAnimation.isAnimating) {
        DrawRectangleLines(boarderX, commandBoxY, screenWidth - boarderX * 2, commandBoxHeight, WHITE);
        const char *commands[COMMAND_MAX] = {"Attack", "Skill", "Magic", "Item", "Defend", "Run"};
        int commandWidth = (screenWidth - boarderX * 2) / COMMAND_COLS;
        int commandHeight = commandBoxHeight / COMMAND_ROWS;
        for (int i = 0; i < COMMAND_MAX; i++) {
            int col = i % COMMAND_COLS;
            int row = i / COMMAND_COLS;
            int textX = boarderX * 2 + col * commandWidth;
            int textY = commandBoxY + 10 + row * commandHeight;
            if (i == battleState->selectedCommand) {
                DrawText(">", textX - 20, textY, 20, WHITE);
            }
            DrawText(commands[i], textX, textY, 20, WHITE);
        }
    }
    else {
        battleState->commandMenuDelay--;
    }





    // Draw Turn Order (modified version)
    int turnDisplayX = screenWidth - 100; // Give more width
    int turnDisplayY = backgroundY + 20;
    int boxSize = 40; // Size of each sprite box
    int padding = 5;
    // DrawText("Turn Order (A/S scroll)", turnDisplayX, turnDisplayY - 30, 20, WHITE);

    // Get predicted turns

    // BattleParticipant *predictedTurns[TOTAL_PREDICTED_TURNS];
    // PredictFutureTurns(battleState, predictedTurns);
    // takes into account preselected action.



    if (!battleState->attackAnimation.isAnimating) {
        if (!battleState->commandConfirmed) {
            PredictFutureTurns(battleState, battleState->predictedTurns);
        }

        // Draw swap interface if in swap mode
        if (battleState->inSwapMode) {
            DrawSwapMenu(battleState);
        }
        // Handle scrolling input
        // cannot scroll turns while on a submenu selection
        if (nextUp->isPlayer && nextUp->isActive && battleState->commandMenuDelay <= 0 && !IsBattleOver(battleState)) {
            if (!battleState->inSubmenu) {
                // Handle smooth scrolling
                if (IsKeyDownSmooth(KEY_A, &battleState->scrollUpKey)) {
                    battleState->turnOrderScrollOffset--;
                }

                if (IsKeyDownSmooth(KEY_S, &battleState->scrollDownKey)) {
                    battleState->turnOrderScrollOffset++;
                }

                // Clamp scroll offset
                battleState->turnOrderScrollOffset = CLAMP(battleState->turnOrderScrollOffset, 0, TOTAL_PREDICTED_TURNS - VISIBLE_TURN_ORDERS);
            }

            // Draw visible range
            for (int i = 0; i < VISIBLE_TURN_ORDERS; i++) {
                int turnIndex = i + battleState->turnOrderScrollOffset;
                if (turnIndex >= TOTAL_PREDICTED_TURNS) break;
                
                BattleParticipant *p = battleState->predictedTurns[turnIndex];
                // Color color = p->isPlayer ? BLUE : RED;
                // const char *indicator = (turnIndex == 0) ? ">> " : "";

                Vector2 position = { turnDisplayX, turnDisplayY + i * (boxSize + padding)};

                Texture2D sprite = p->creature->sprites[p->creature->terrains[0]][PERFECT];

                    // Draw background box
                Color borderColor = p->isPlayer ? BLUE : RED;
                // if (turnIndex == 0) borderColor = YELLOW; // Highlight current turn

                DrawRectangle(position.x - 2, position.y - 2, boxSize + 4, boxSize + 4, borderColor);
                
                DrawRectangleLinesEx((Rectangle){position.x - 2, position.y - 2,boxSize + 4, boxSize + 4}, 2, borderColor);
                
                // Draw sprite
                DrawTexturePro( sprite, (Rectangle){0, 0, sprite.width, sprite.height}, (Rectangle){position.x, position.y, boxSize, boxSize}, (Vector2){0}, 0.0f, WHITE );
            }

            // Draw scroll indicators
            if (battleState->turnOrderScrollOffset > 0) {
                DrawText("^", turnDisplayX + boxSize/2 - MeasureText("^", 20)/2, turnDisplayY - MeasureText("^", 20) - padding, 20, WHITE);
            }
            if (battleState->turnOrderScrollOffset < TOTAL_PREDICTED_TURNS - VISIBLE_TURN_ORDERS) {
                DrawText("v", turnDisplayX + boxSize/2 - MeasureText("v", 20)/2, turnDisplayY + VISIBLE_TURN_ORDERS * boxSize + MeasureText("v", 20) + padding, 20, WHITE);
            }
        }
    }



    // Display messages
    if (battleState->messageCount > 0) {
        Message *currentMessage = &battleState->messageQueue[0];
        if (currentMessage->timer > 0) {
            DrawText(currentMessage->text, 20, characterBoxYPos - 30, 20, WHITE);
            currentMessage->timer--;
        } else {
            // Shift messages
            for (int i = 0; i < battleState->messageCount - 1; i++) {
                battleState->messageQueue[i] = battleState->messageQueue[i + 1];
            }
            battleState->messageCount--;
        }
    }

    if (battleState->shakeTimer > 0) {
        battleState->shakeTimer--;
    }
    if (battleState->flickerTimer > 0) {
        battleState->flickerTimer--;
    }

    if (battleState->inSubmenu) {
        DrawSubmenu(battleState, nextUp);
    }
}

void DrawSubmenu(BattleState *battleState, BattleParticipant *nextUp) {
    static int blinkCounter = 0;
    blinkCounter++;
    bool visible = ((blinkCounter / 2) % 2 == 0); // toggles every 30 frames

    int submenuWidth = 300;
    int submenuHeight = 200;
    int submenuX = (GetScreenWidth() - submenuWidth) / 2;
    int submenuY = (GetScreenHeight() - submenuHeight) / 2;

    DrawRectangle(submenuX, submenuY, submenuWidth, submenuHeight, DARKGRAY);
    DrawRectangleLines(submenuX, submenuY, submenuWidth, submenuHeight, WHITE);

    int textY = submenuY + 20;
    int textX = submenuX + 20;
    int maxItems = battleState->submenuCount;
    const char *title = "";

    switch (battleState->submenuType) {
        case SUBMENU_MAGIC: title = "Select Magic"; break;
        case SUBMENU_SKILL: title = "Select Skill"; break;
        case SUBMENU_ITEM: title = "Select Item"; break;
        default: break;
    }
    

    // Draw on-screen debug info for skills
    if (battleState->submenuType == SUBMENU_SKILL) {
        char debugText[100];
        sprintf(debugText, "Creature: %s, Found skills: %d", nextUp->creature->name, maxItems);
        DrawText(debugText, 10, GetScreenHeight() - 60, 20, YELLOW);
    }
    // Draw on-screen debug info for magic
    if (battleState->submenuType == SUBMENU_MAGIC) {
        char debugText[100];
        sprintf(debugText, "Creature: %s, Found magic: %d", nextUp->creature->name, maxItems);
        DrawText(debugText, 10, GetScreenHeight() - 60, 20, YELLOW);
    }

    DrawText(title, textX, textY, 20, WHITE);
    textY += 40;

    // Arrows (no wrap)
    // Up arrow
    if (battleState->submenuStartIndex > 0) {
        DrawText("^", submenuX + submenuWidth - 30, submenuY + 20, 20, WHITE);
    } else {
        DrawText("^", submenuX + submenuWidth - 30, submenuY + 20, 20, GRAY);
    }

    // Down arrow
    if (battleState->submenuStartIndex + VISIBLE_SUBMENU_ITEMS < maxItems) {
        DrawText("v", submenuX + submenuWidth - 30, submenuY + submenuHeight - 20, 20, WHITE);
    } else {
        DrawText("v", submenuX + submenuWidth - 30, submenuY + submenuHeight - 20, 20, GRAY);
    }

        // Draw visible items
    for (int i = battleState->submenuStartIndex; i < battleState->submenuStartIndex + VISIBLE_SUBMENU_ITEMS; i++) {
        int itemY = textY + (i - battleState->submenuStartIndex) * 30;
        bool isSelected = (i == battleState->submenuSelection);

        switch (battleState->submenuType) {
            case SUBMENU_MAGIC: {
                if (i < maxItems) { // Ensure i is within bounds
                    Magic *magic = battleState->filteredMagic[i];
                    if (magic && magic->name[0] != '\0') { // Add a null check for safety
                        if (isSelected && visible) DrawText(">", textX - 20, itemY, 20, WHITE);
                        Color magicAvailableColor = (magic->mpCost > nextUp->creature->currentMP ? GRAY : WHITE);
                        DrawText(TextFormat("%s", magic->name), textX, itemY, 20, magicAvailableColor);
                        DrawText(TextFormat("(MP %d)", magic->mpCost), submenuX + submenuWidth - 90, itemY, 20, magicAvailableColor);
                    } else {
                        printf("Warning: Null or empty magic name at index %d\n", i);
                    }
                }
            } break;

            case SUBMENU_SKILL: {
                if (i < maxItems) { // Ensure i is within bounds
                    Skill *skill = battleState->filteredSkills[i];
                    if (skill && skill->name[0]!='\0') {
                        if (isSelected && visible) DrawText(">", textX - 20, itemY, 20, WHITE);
                        Color skillAvailableColor = (skill->mpCost > nextUp->creature->currentMP ? GRAY : WHITE);
                        DrawText(TextFormat("%s", skill->name), textX, itemY, 20, skillAvailableColor);
                        DrawText(TextFormat("(MP %d)", skill->mpCost), submenuX + submenuWidth - 90, itemY, 20, skillAvailableColor);
                    } else {
                        printf("Warning: Null or empty skill name at index %d\n", i);
                    }
                }
            } break;

            case SUBMENU_ITEM: {
                if (i < maxItems) { // Ensure i is within bounds
                    Item *item = battleState->filteredItems[i];
                    if (item && item->isUnlocked && item->amount > 0 && item->name[0]!='\0' && 
                        (item->usageType == USAGE_BOTH || item->usageType == USAGE_BATTLE)) {
                        if (isSelected && visible) DrawText(">", textX - 20, itemY, 20, WHITE);
                        Color itemAvailableColor = (item->amount > 0 ? WHITE : GRAY);
                        DrawText(TextFormat("%s", item->name), textX, itemY, 20, itemAvailableColor);
                        DrawText(TextFormat("(x%d)", item->amount), submenuX + submenuWidth - 80, itemY, 20, itemAvailableColor);
                    }
                }
            } break;

            default:
                break;
        }
    }
    
    // Show additional debug info on screen
    if (maxItems == 0) {
        DrawText("No items found in this category!", textX, textY + 40, 20, RED);
    }
}


void PlayerRunning(BattleState *battleState) {
    // printf("\nRUNNING\n");
    ++battleState->runAttempts;
    battleState->rolloutConsecutiveUses = 0;
    battleState->speedCost = 0.7f;

    int speedParty = 0;
    int numParty = 0;
    int speedEnemy = 0;
    int numEnemy = 0;

    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer) {
            speedParty += battleState->participants[i].creature->speed + battleState->participants[i].creature->tempSpeedBuff; 
            numParty++;
        }
        else {
            speedEnemy += battleState->participants[i].creature->speed + battleState->participants[i].creature->tempSpeedBuff; 
            numEnemy++;
        }
    }
    
    if ((float)speedParty/(float)numParty - RANDOMS(1,3) > (float)speedEnemy/(float)numEnemy) {
        SetBattleMessage(battleState, "You successfully escaped!");
        battleState->isBattleActive = false;
        battleState->runSuccessful = true;
        // UnloadTexture(battleState->enemySprite); // Unload enemy sprite if escaped
    } 
    else {
        SetBattleMessage(battleState, "Failed to escape...");
    }
}

// Helper function to set up damage display
void SetDamageDisplay(BattleParticipant *target, int damage, EffectType effectType) {
    target->pendingDamage = damage;
    target->damageTimer = DAMAGE_TIMER;  // 60 frames = ~1 second at 60 FPS it never passes the 10...
    target->damageOffsetY = 0.0f;
    target->effectType = effectType;
    // strncpy(target->statChange , "", sizeof(target->statChange) - 1);
    // target->statChange[sizeof(target->statChange) - 1] = '\0';
}

// Common attack processing logic
void ProcessAttackInternal(BattleState *battleState, BattleParticipant *attacker, BattleParticipant *defender, bool isEnemyAttack) {
    if (!battleState->isBattleActive || !attacker || !defender) return;

    Creature *attackerCreature = attacker->creature;
    Creature *defenderCreature = defender->creature;
    if (!attackerCreature || !defenderCreature) return;

    // Calculate hit chance
    bool didHit = isEnemyAttack ? WillEnemyHit(attackerCreature, defenderCreature)
                                : WillItHit(attackerCreature, defenderCreature);
    
    // Debug logging
    printf("\n%s attacks %s - Hit: %d\n", attackerCreature->name, 
           defenderCreature->name, didHit);

    battleState->speedCost = standardSpeedCost;

    battleState->decisionMade = true;
    if (!isEnemyAttack) battleState->rolloutConsecutiveUses = 0;

    if (!didHit) {
        SetBattleMessage(battleState, TextFormat("%s's attack missed!", attackerCreature->name));
        return;
    }

    // Calculate and apply damage
    int damage = attackerCreature->attack + attackerCreature->tempAttackBuff
               - defenderCreature->defense - defenderCreature->tempDefenseBuff;
    damage = MAX(damage, 0);  // Ensure non-negative damage
    
    defenderCreature->currentHP -= damage;
    defenderCreature->currentHP = CLAMP(defenderCreature->currentHP, 0, defenderCreature->maxHP);

    // Set up damage display
    SetDamageDisplay(defender, damage, EFFECT_OFFENSIVE);

    // Battle message
    char message[128];
    snprintf(message, sizeof(message), "%s dealt %d damage to %s!", 
             attackerCreature->name, damage, defenderCreature->name);
    SetBattleMessage(battleState, message);

    // Handle defender defeat
    if (defenderCreature->currentHP <= 0) {
        defenderCreature->currentHP = 0;
        // defender->isActive = false;
        if (!isEnemyAttack) {
            OnEnemyDefeated(battleState, defender);
        }
    }

    // Enemy attack specific effects
    if (isEnemyAttack) {
        battleState->shakeTimer = 10;
    }
}

void UseAttack(BattleState *battleState, BattleParticipant *attacker) {
    if (!battleState->isBattleActive || !attacker || !attacker->creature) return;

    // Configure attack animation
    battleState->attackAnimation = animations.attack;
    battleState->attackAnimation.isAnimating = true;
    battleState->attackAnimation.currentFrame = 0;
    battleState->attackAnimation.frameCounter = 0;
    battleState->attackAnimation.frameSpeed = 0;
    
    battleState->flickerTimer = FLICKER_DURATION_FRAMES;
    battleState->selectedCommand = COMMAND_ATTACK;
}

void ProcessAttack(BattleState *battleState, BattleParticipant *attacker, BattleParticipant *defender) {
    ProcessAttackInternal(battleState, attacker, defender, false);
}

void EnemyAttack(BattleState *battleState, BattleParticipant *attacker) {
    printf("\nENEMY ATTACK INITIATED\n");
    
    BattleParticipant *defender = SelectRandomAlivePlayer(battleState);
    if (!defender) {
        printf("All players defeated - no valid target\n");
        return;
    }

    ProcessAttackInternal(battleState, attacker, defender, true);
}

BattleParticipant* SelectRandomAlivePlayer(BattleState *battleState) {
    // Collect indices of alive player characters
    int aliveIndices[MAX_PARTY_SIZE];
    int count = 0;
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0) {
            aliveIndices[count++] = i;
            printf("\n ENEMY ATTACKING FOUND PLAYER %s\n",battleState->participants[i].creature->name);
        }
    }
    if (count == 0) return NULL;
    int randomIndex = rand() % count;
    printf("INDEX FOR ENEMY TARGETTING %d\n",randomIndex);
    return &battleState->participants[aliveIndices[randomIndex]];
}


void UseItem(BattleState *battleState,  BattleParticipant *attacker) {
    // printf("\nATTACKING\n");
    if (!battleState->isBattleActive || attacker == NULL) return;

    Creature *attackerCreature = attacker->creature;

    if (!attackerCreature) return;

    // Item *selectedItem = &itemList[battleState->selectedAction];
    Item *selectedItem = battleState->selectedItem;

    if (strcmp(selectedItem->name, "Potion") == 0 && selectedItem->amount > 0) {
        battleState->attackAnimation = animations.potion;
        // battleState->attackAnimation.color = GREEN;
    } 
    else if (strcmp(selectedItem->name, "Super Potion") == 0 && selectedItem->amount > 0) {
        battleState->attackAnimation = animations.potion;
        // battleState->attackAnimation.color = DARKGREEN;
    } 
    else if (strcmp(selectedItem->name, "Ether") == 0 && selectedItem->amount > 0) {
        battleState->attackAnimation = animations.potion;
        // battleState->attackAnimation.color = RED;
    }
    else if (strcmp(selectedItem->name, "Phoenix Feather") == 0 && selectedItem->amount > 0) {
        battleState->attackAnimation = animations.potion;
        // battleState->attackAnimation.color = GOLD;
    }
    else if (strcmp(selectedItem->name, "Nade") == 0 && selectedItem->amount > 0) {
        battleState->attackAnimation = animations.nade;
        // random number everytime its called
        selectedItem->hpRestore = -RANDOMS(50,100);
    }
    else if (strcmp(selectedItem->name, "Explosion") == 0 && selectedItem->amount > 0) {
        battleState->attackAnimation = animations.explosion;
        // selectedItem->hpRestore = -RANDOMS(20,50);
    }

    battleState->attackAnimation.color = GetItemAnimationColor(selectedItem);
    battleState->attackAnimation.isAnimating = true;
    battleState->attackAnimation.currentFrame = 0;
    battleState->attackAnimation.frameCounter = 0;
    battleState->attackAnimation.frameSpeed = 1;
    battleState->selectedItem = selectedItem;
    battleState->selectedCommand = COMMAND_ITEM;

    // TODO implement decrement in the inventory!
}



Color GetItemAnimationColor(Item *item) {
    if (strcmp(item->name, "Potion") == 0) {
        return GREEN;
    } else if (strcmp(item->name, "Super Potion") == 0) {
        return DARKGREEN;
    } else if (strcmp(item->name, "Ether") == 0) {
        return RED;
    } else if (strcmp(item->name, "Phoenix Feather") == 0) {
        return GOLD;
    } else {
        return WHITE; // Default color
    }
}


void ProcessItem(BattleState *battleState,  BattleParticipant *defender) {
    // printf("\nATTACKING\n");
    if (!battleState->isBattleActive || defender == NULL) return;

    Creature *defenderCreature = defender->creature;

    if (!defenderCreature) return;

    // Item *selectedItem = &itemList[battleState->selectedAction];
    Item *selectedItem = battleState->selectedItem;
    battleState->rolloutConsecutiveUses = 0;
    battleState->decisionMade = true;
    battleState->speedCost = selectedItem->speedCost;

    // TODO GARANTEE THAT THE ITEMS CAN BE USED ON ENEMIES OR ALLIES
    if (strcmp(selectedItem->name, "Explosion") == 0 && selectedItem->amount > 0) {
        selectedItem->hpRestore = -RANDOMS(20,50); // actual random put inside the Process codes not the Use!
    }


    if (selectedItem->hpRestore < 0) {
        int damage = selectedItem->hpRestore;
        defender->pendingDamage = -damage; // Healing is represented as negative damage
        defender->damageTimer = 60;      // display for 60 frames (about 1 sec at 60 FPS)
        defender->damageOffsetY = 0.0f;    // start with no offset
        defender->effectType = EFFECT_OFFENSIVE; // Set action type to heal
        defenderCreature->currentHP += selectedItem->hpRestore;
    }
    else {
        if (defenderCreature->currentHP > 0) {
            defenderCreature->currentHP += selectedItem->hpRestore;
            if (defenderCreature->currentHP > defenderCreature->maxHP) {
                defenderCreature->currentHP = defenderCreature->maxHP;
            }
            defenderCreature->currentMP += selectedItem->mpRestore;
            if (defenderCreature->currentMP > defenderCreature->maxMP) {
                defenderCreature->currentMP = defenderCreature->maxMP;
            }

            if (selectedItem->hpRestore == 0) defender->pendingDamage = selectedItem->mpRestore;
            else defender->pendingDamage = selectedItem->hpRestore;

            defender->damageTimer = 60;      // display for 60 frames (about 1 sec at 60 FPS)
            defender->damageOffsetY = 0.0f;    // start with no offset
            defender->effectType = EFFECT_HEALING; // Set action type to heal
        }
    }

    if (defenderCreature->currentHP == 0 && strcmp(selectedItem->name, "Phoenix Feather") == 0 && selectedItem->amount>0) {
        int reviveHealing = RANDOMS(1, (int)defenderCreature->maxHP / 4);
        defenderCreature->currentHP = reviveHealing;
        defender->isActive = true;
        defender->pendingDamage = reviveHealing; // Healing is represented as negative damage
        defender->damageTimer = 60;      // display for 60 frames (about 1 sec at 60 FPS)
        defender->damageOffsetY = 0.0f;    // start with no offset
        defender->effectType = EFFECT_HEALING; // Set action type to heal

        // Find the maximum `nextActionTime` among all participants
        float maxNextActionTime = 0.0f; 
        for (int i = 0; i < battleState->participantCount; i++) { // Fixed condition
            float t = battleState->turnOrder[i]->nextActionTime;
            if (maxNextActionTime < t) {
                maxNextActionTime = t;
            }
        }

        // Assign `nextActionTime` for the revived participant
        defender->nextActionTime = maxNextActionTime + 0.01f; // Slightly more than the max
    }


    char message[128];
    snprintf(message, sizeof(message), "You used %s!", selectedItem->name);
    SetBattleMessage(battleState, message);
    // removing the item should be done at the end so that the reference is not lost if removed before...!
    RemoveItemFromInventory(selectedItem->id,1);

    if (defenderCreature->currentHP <= 0) {
        defenderCreature->currentHP = 0;
        // defender->isActive = false;
        OnEnemyDefeated(battleState, defender); 
    }
}

//UPDATE WITH NEW LEVELING SYSTEM
void GainEXP(BattleState *battleState) {
    int totalExp = 0;
    // Sum up experience from all defeated enemies
    for (int i = 0; i < battleState->participantCount; i++) {
        BattleParticipant *participant = &battleState->participants[i];
        if (!participant->isPlayer && participant->creature->currentHP <= 0) {
            totalExp += participant->creature->giveExp;
            TrackEnemyDefeated(participant->creature->id);
        }
    }
    
    int alivePlayerCount = 0;
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0) {
            alivePlayerCount++;
        }
    }
    
    if (alivePlayerCount == 0) return;
    
    // Calculate EXP per player
    int expPerPlayer = totalExp / alivePlayerCount;
    SetBattleMessage(battleState, TextFormat("You gained %d AP!", expPerPlayer));
    
    // Distribute EXP and check for grid level increases
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0) {
            Creature *player = battleState->participants[i].creature;
            player->exp += expPerPlayer;
            
            // Check for grid level up
            int expForNextLevel = GetExpForNextGridLevel(player->totalGridLevel);
            while (player->exp >= expForNextLevel) {
                player->exp -= expForNextLevel;
                player->gridLevel++;
                player->totalGridLevel++;
                PlaySoundByName(&soundManager, "levelUp", true);
                
                // Update available movement points in the grid
                FriendshipGrid* grid = GetCurrentGrid();
                if (grid) {
                    grid->availablePoints = player->gridLevel;
                }
                
                SetBattleMessage(battleState, TextFormat("%s gained a Grid Level! (GLvl %d)", 
                                                       player->name, player->gridLevel));
                
                // Update the next level requirement
                expForNextLevel = GetExpForNextGridLevel(player->totalGridLevel);
            }
        }
    }
}


bool WillItHit(Creature *attacker, Creature *defender) {
    return (50 + (attacker->accuracy + 
    attacker->tempAccuracyBuff +
    attacker->speed +
    attacker->tempSpeedBuff +
    attacker->luck +
    attacker->tempLuckBuff)
    > 
   (defender->evasion +
    defender->speed +
    defender->luck))
    ?
    true :
    false; 
}

bool WillEnemyHit( Creature *attacker, Creature *defender) {
    return ((defender->evasion + 
    defender->speed +
    defender->luck)
    < 
   50 + (attacker->accuracy +
    attacker->speed +
    attacker->luck +
    attacker->tempAccuracyBuff +
    attacker->tempSpeedBuff +
    attacker->tempLuckBuff))
    ?
    true :
    false; 
}

void UpdateNextActionTime(BattleState *battleState, BattleParticipant *participant) {

        
    for (int i = 0; i < battleState->participantCount; i++) {
        printf("Turn = %d Participant %s: nextActionTime = %.6f CurrentHP = %d Attackbuff?=%d\n",
        battleState->numberOfTurns,
        battleState->participants[i].creature->name,
        battleState->participants[i].nextActionTime,
        battleState->participants[i].creature->currentHP,
        battleState->participants[i].creature->tempAttackBuff);
    }

    float speed = participant->creature->speed + participant->creature->tempSpeedBuff;
    if (speed <= 0) speed = 1.0f; // Prevent division by zero or negative speed

    participant->nextActionTime += (1.0f / speed) * battleState->speedCost;
    battleState->decisionMade = false;
    battleState->numberOfTurns++;
    battleState->speedCost = 1.0f;

    for (int i = 0; i < battleState->participantCount; i++) {
        printf("Turn = %d Participant %s: nextActionTime = %.6f CurrentHP = %d Attackbuff?=%d\n",
        battleState->numberOfTurns,
        battleState->participants[i].creature->name,
        battleState->participants[i].nextActionTime,
        battleState->participants[i].creature->currentHP,
        battleState->participants[i].creature->tempAttackBuff);
    }
}


void SortTurnOrder(BattleState *battleState) {
    // Create array of indices
    int indices[battleState->participantCount];
    for (int i = 0; i < battleState->participantCount; i++) 
        indices[i] = i;

    // Stable sort using bubble sort (better for small n)
    for (int i = 0; i < battleState->participantCount-1; i++) {
        for (int j = 0; j < battleState->participantCount-i-1; j++) {
            float a = battleState->participants[indices[j]].nextActionTime;
            float b = battleState->participants[indices[j+1]].nextActionTime;
            
            if (fabs(a - b) <= FLOAT_EPSILON) {
                // Tie-breaker: if speeds differ, swap so that higher speed comes first
                if (battleState->participants[indices[j]].creature->speed < battleState->participants[indices[j+1]].creature->speed) {
                    // Swap indices to have the higher speed participant come first
                    int temp = indices[j];
                    indices[j] = indices[j+1];
                    indices[j+1] = temp;
                }
            } else if (a > b + FLOAT_EPSILON) {
                // Regular swap when a is definitively greater than b
                int temp = indices[j];
                indices[j] = indices[j+1];
                indices[j+1] = temp;
            }
        }
    }

    // Rebuild turnOrder array
    for (int i = 0; i < battleState->participantCount; i++) {
        battleState->turnOrder[i] = &battleState->participants[indices[i]];
    }
}



int GetNextTargetIndex(BattleState *battleState, int currentIndex) {
    int index = currentIndex;
    do {
        index = (index + 1) % battleState->participantCount;
        if (IsValidTarget(battleState, index)) {
            return index;
        }
    } while (index != currentIndex);
    return currentIndex; // No valid targets found
}

int GetPreviousTargetIndex(BattleState *battleState, int currentIndex) {
    int index = currentIndex;
    do {
        index = (index - 1 + battleState->participantCount) % battleState->participantCount;
        if (IsValidTarget(battleState, index)) {
            return index;
        }
    } while (index != currentIndex);
    return currentIndex; // No valid targets found
}



bool IsHealingItem(Item *item) {
    // Assuming you have a field in Item that specifies the item type
    // printf("It is healing\n");
    printf("item %s is healing? %d\n", item->name, item->effectType == EFFECT_HEALING);
    return item->effectType == EFFECT_HEALING;
}

bool IsHealingMagic(Magic *magic) {
    // Assuming you have a field in Item that specifies the item type
    // printf("It is healing\n");
    printf("item %s is healing? %d\n", magic->name, magic->effectType == EFFECT_HEALING);
    return magic->effectType == EFFECT_HEALING;
}


bool IsBattleOver(BattleState *battleState) {
    bool allPlayersDefeated = true;
    bool allEnemiesDefeated = true;

    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0) {
            allPlayersDefeated = false;
        }
        if (!battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0) {
            allEnemiesDefeated = false;
        }
    }

    return (allPlayersDefeated || allEnemiesDefeated);
}


void OnEnemyDefeated(BattleState *battleState, BattleParticipant *defender) {
    // Check if enemy is already in the defeat list
    Creature *defenderCreature = defender->creature;
    int index = -1;
    for (int i = 0; i < enemyDefeatCount; i++) {
        if (strcmp(enemyDefeatList[i].name, defenderCreature->name) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1 && enemyDefeatCount < MAX_ENEMY_TYPES) {
        // Add new enemy to the list
        strcpy(enemyDefeatList[enemyDefeatCount].name, defenderCreature->name);
        enemyDefeatList[enemyDefeatCount].defeatCount = 1;
        enemyDefeatCount++;
    } else if (index != -1) {
        // Increment defeat count
        enemyDefeatList[index].defeatCount++;
    }
}


void CleanupEnemies(void) {
    for (int i = 0; i < enemyDatabase.enemyCount; i++) {
        Creature *enemy = &enemyDatabase.creatures[i];
        for (int j = 0; j < NUM_TERRAINS; j++) {
            for (int k = 0; k < HEALTH_STATES; k++) {
                if (enemy->sprites[j][k].id != 0) {
                    UnloadTexture(enemy->sprites[j][k]);
                }
            }
        }
    }
    free(enemyDatabase.creatures);
    enemyDatabase.creatures = NULL;
    enemyDatabase.enemyCount = 0;
}

void CleanupBattle(BattleState *battleState) {
    // Free dynamically allocated enemy copies
    for (int i = 0; i < battleState->participantCount; i++) {
        if (!battleState->participants[i].isPlayer && battleState->participants[i].creature != NULL) {
            free(battleState->participants[i].creature);
            battleState->participants[i].creature = NULL;
        }
    }
    battleState->participantCount = 0;
    battleState->isBattleActive = false;
    battleState->messageCount = 0;
    battleState->currentParticipant = NULL;
}



// void ResetGame(BattleState *battleState) {
//     // Reset player positions and stats
//     for (int i = 0; i < MAX_PARTY_SIZE; i++) {
//         Creature *player = &mainTeam[i];
//         if (player->name[0] != '\0') {
//             player->x = 25.0f;
//             player->y = 5.0f;
//             player->currentHP = player->maxHP;
//             player->currentMP = player->maxMP;
//             // Reset temporary buffs/debuffs
//             player->tempAttackBuff = 0;
//             player->tempDefenseBuff = 0;
//             player->tempSpeedBuff = 0;
//             player->tempMagicBuff = 0;
//             player->tempMagicdefenseBuff = 0;
//             player->tempLuckBuff = 0;
//             player->tempAccuracyBuff = 0;
//             player->tempEvasionBuff = 0;
//         }
//     }

//     // Clean up battle participants
//     for (int i = 0; i < battleState->participantCount; i++) {
//         if (!battleState->participants[i].isPlayer) {
//             // Free enemy creature copies
//             free(battleState->participants[i].creature);
//             battleState->participants[i].creature = NULL;
//         }
//     }
//     battleState->participantCount = 0;
//     battleState->isBattleActive = false;

//     // Clear battleState
//     memset(battleState, 0, sizeof(BattleState));
// }


void AddCreatureToParty(Creature *newCreature) {
    printf("\n--- AddCreatureToParty DEBUG ---\n");
    printf("Adding %s (Type: %d) to party\n", newCreature->name, newCreature->creatureType);
    
    // Initialize bonus tracker for the new creature
    InitializeCreatureBonusTracker(newCreature);
    
    // Load grid progress before adding to any team
    printf("Loading grid progress for new creature\n");
    LoadCreatureGridProgress(newCreature);
    
    // Apply type-specific unlocks from existing creatures
    printf("Applying type-specific unlocks\n");
    ApplyTypeUnlocksToCreature(&menu.gridState, newCreature);
    
    // Try to add to mainTeam
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] == '\0') {
            printf("Added to main team slot %d\n", i);
            mainTeam[i] = *newCreature;
            // guarantees that swapping doesn't make the character appear in 0,0...
            mainTeam[i].x = mainTeam[0].x;
            mainTeam[i].y = mainTeam[0].y;
            SetOverworldMessage("New creature added to main team!");
            printf("-------------------------\n");
            return;
        }
    }

    // Try to add to backupTeam
    for (int i = 0; i < MAX_BACKUP_SIZE; i++) {
        if (backupTeam[i].name[0] == '\0') {
            backupTeam[i] = *newCreature;
            SetOverworldMessage("New creature added to backup team!");
            return;
        }
    }

    // Try to add to bank
    for (int i = 0; i < MAX_BANK_SIZE; i++) {
        if (bank[i].name[0] == '\0') {
            bank[i] = *newCreature;
            SetOverworldMessage("New creature added to bank!");
            return;
        }
    }

    // All teams are full; transition to swapping state
    currentGameState = GAME_STATE_BEFRIEND_SWAP;

    // Initialize swap menu
    swapMenu.newCreature = *newCreature;
    swapMenu.selectedTeam = PARTY_BANK; // Start with bank team to avoid misclicks
    swapMenu.selectedIndex = 0;

    printf("-------------------------\n");
}

void ResetBuffs(void) {
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] != '\0') {
            mainTeam[i].tempAccuracyBuff = 0;
            mainTeam[i].tempAttackBuff = 0;
            mainTeam[i].tempDefenseBuff = 0;
            mainTeam[i].tempEvasionBuff = 0;
            mainTeam[i].tempLuckBuff = 0;
            mainTeam[i].tempMagicBuff = 0;
            mainTeam[i].tempMagicdefenseBuff = 0;
            mainTeam[i].tempSpeedBuff = 0;
        }
    }
}

bool AnyPlayerAlive(BattleState *battleState) {
    bool anyPlayerAlive = false;
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer && battleState->participants[i].creature->currentHP > 0) {
            anyPlayerAlive = true;
            break;
        }
    }
    return anyPlayerAlive;
}


int GetPartyIndexFromCreature(Creature *c) {
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (&mainTeam[i] == c) {
            return i; // Return the index in mainTeam where this creature is found
        }
    }
    return -1; // Not found
}


// A helper function to get the participant index for a given ally creature:
int GetParticipantIndexForAlly(BattleState *battleState, Creature *ally) {
    for (int i = 0; i < battleState->participantCount; i++) {
        if (battleState->participants[i].isPlayer && battleState->participants[i].creature == ally) {
            return i;
        }
    }
    return -1; // Not found
}


void LoadCreatureAnimations(Creature *c, const char *baseFolder) {
    // e.g. baseFolder = "C:/someFolder/befriendedCharacters/Bob"

    // Build path for idle
    char idlePath[256];
    snprintf(idlePath, sizeof(idlePath), "%s/%s/idle", baseFolder, c->name);
    // LoadTexturesFromDirectory checks how many frames exists and loads them into the textures
    c->anims.idleFrameCount = LoadTexturesFromDirectory(
        idlePath, c->anims.idleTextures, MAX_FRAMES, TILE_SIZE, TILE_SIZE
    );

    // Build and load walk directions
    char walkPaths[NUM_DIRECTIONS][256];
    snprintf(walkPaths[RIGHT], sizeof(walkPaths[RIGHT]), "%s/%s/right", baseFolder, c->name);
    snprintf(walkPaths[LEFT],  sizeof(walkPaths[LEFT]),  "%s/%s/left",  baseFolder, c->name);
    snprintf(walkPaths[UP],    sizeof(walkPaths[UP]),    "%s/%s/up",    baseFolder, c->name);
    snprintf(walkPaths[DOWN],  sizeof(walkPaths[DOWN]),  "%s/%s/down",  baseFolder, c->name);

    c->anims.walkFrameCount[RIGHT] = LoadTexturesFromDirectory(walkPaths[RIGHT], c->anims.walkTextures[RIGHT], MAX_FRAMES, TILE_SIZE, TILE_SIZE);
    c->anims.walkFrameCount[LEFT] = LoadTexturesFromDirectory(walkPaths[LEFT], c->anims.walkTextures[LEFT], MAX_FRAMES, TILE_SIZE, TILE_SIZE);
    c->anims.walkFrameCount[UP] = LoadTexturesFromDirectory(walkPaths[UP], c->anims.walkTextures[UP], MAX_FRAMES, TILE_SIZE, TILE_SIZE);
    c->anims.walkFrameCount[DOWN] = LoadTexturesFromDirectory(walkPaths[DOWN], c->anims.walkTextures[DOWN], MAX_FRAMES, TILE_SIZE, TILE_SIZE);

    c->anims.fishingFrameCount = LoadTexturesFromDirectory(FOLDER_FISHING, c->anims.fishingTextures, MAX_FRAMES, TILE_SIZE, TILE_SIZE);
    c->anims.pickaxeFrameCount = LoadTexturesFromDirectory(FOLDER_PICKAXE, c->anims.pickaxeTextures, MAX_FRAMES, 25*2, TILE_SIZE);
    c->anims.diggingFrameCount = LoadTexturesFromDirectory(FOLDER_DIGGING, c->anims.diggingTextures, MAX_FRAMES, 21*2, TILE_SIZE);
    c->anims.cuttingFrameCount = LoadTexturesFromDirectory(FOLDER_CUTTING, c->anims.cuttingTextures, MAX_FRAMES, 32*2, TILE_SIZE);

    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        c->anims.directionPressCounter[i] = 0;
    }
    c->anims.movementState = IDLE;
    c->anims.direction = DOWN;

    c->anims.isPressed = false;
    c->anims.isMoving = false;
    c->anims.isStopping = false;

    if (c->anims.idleFrameCount == 0) {
        printf("Idle textures failed to load for %s\n", c->name);
    }
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        if (c->anims.walkFrameCount[i] == 0) {
            printf("Walk textures failed to load for %s in direction %d\n", c->name, i);
        }
    }
}


void AddFollower(Creature *newFollower) {
    if (followerCount < MAX_FOLLOWERS) {
        Follower *f = &followers[followerCount];
        // f->creature = newFollower; // Copy the new follower's data
        // Allocate memory for a new creature
        f->creature = (Creature*)malloc(sizeof(Creature));
        // Copy all data from newFollower to the newly allocated creature
        memcpy(f->creature, newFollower, sizeof(Creature));
        f->creature->x = mainTeam[0].x;
        f->creature->y = mainTeam[0].y;

        f->lastDirection = mainTeam[0].anims.direction; // Initialize last direction
        f->lastFrame = 0;        // Start with the first frame
        f->followerFrameCounter = 0; // Reset the frame counter

                // Initialize behavior settings
        f->behavior = FOLLOWER_BEHAVIOR_FOLLOW;
        f->attackCooldown = 0.0f;
        f->isAttacking = false;

        followerCount++;
        SetOverworldMessage(TextFormat("%s now follows you!", f->creature->name));
    } else {
        SetOverworldMessage("You cannot have more followers!");
    }
}



// // Modified UpdateFollowers to handle different behavior modes
// void UpdateFollowers(Creature *c, FrameTracking *f) {
//     float deltaTime = GetFrameTime();

//     for (int i = 0; i < followerCount; i++) {
//         // Update attack cooldown
//         if (followers[i].attackCooldown > 0) {
//             followers[i].attackCooldown -= deltaTime;
//         }

//         // Behavior-specific updates
//         if (followers[i].behavior == FOLLOWER_BEHAVIOR_FOLLOW || followers[i].isAttacking) {
//             // Classic following behavior
//             int followerIndex = (int)(historyIndex - (i + 1) * lag + MAX_HISTORY) % MAX_HISTORY;
//             followers[i].creature.x = playerHistory[followerIndex].x;
//             followers[i].creature.y = playerHistory[followerIndex].y;
//         } 
//         else if (followers[i].behavior == FOLLOWER_BEHAVIOR_AGGRESSIVE) {
//             // Aggressive behavior - find and approach creatures
//             int targetIndex = FindNearestCreatureToFollower(i);
            
//             if (targetIndex >= 0) {
//                 // Target found, move toward it
//                 OverworldCreature* target = &realTimeBattleState.creatures[targetIndex];
                
//                 // Calculate direction to target
//                 float dx = target->position.x - followers[i].creature.x;
//                 float dy = target->position.y - followers[i].creature.y;
//                 float dist = sqrtf(dx*dx + dy*dy);
                
//                 // Only move if not already at target
//                 if (dist > 1.0f) {
//                     // Set direction based on movement
//                     if (fabsf(dx) > fabsf(dy)) {
//                         followers[i].creature.anims.direction = (dx > 0) ? RIGHT : LEFT;
//                     } else {
//                         followers[i].creature.anims.direction = (dy > 0) ? DOWN : UP;
//                     }
                    
//                     // Calculate movement
//                     float moveSpeed = 1.0f * deltaTime;
//                     float dirX = dx / dist;
//                     float dirY = dy / dist;
                    
//                     // New position
//                     float newX = followers[i].creature.x + dirX * moveSpeed;
//                     float newY = followers[i].creature.y + dirY * moveSpeed;
                    
//                     // Check if valid position (simplified collision check)
//                     if (newX >= 0 && newX < mapWidth && newY >= 0 && newY < mapHeight &&
//                         worldMap[(int)newY][(int)newX].walkable) {
                        
//                         // Update position
//                         followers[i].creature.x = newX;
//                         followers[i].creature.y = newY;
//                     }
//                 }
                
//                 // Check if close enough to attack
//                 if (dist <= 1.5f && followers[i].attackCooldown <= 0) {
//                     // Perform attack
//                     PerformFollowerAttack(&followers[i], i, targetIndex);
                    
//                     // Reset cooldown
//                     followers[i].attackCooldown = 3.0f + ((float)RANDOMS(0, 200) / 100.0f);
//                 }
//             } 
//             else {
//                 // No target in range, default to following
//                 int followerIndex = (int)(historyIndex - (i + 1) * lag + MAX_HISTORY) % MAX_HISTORY;
//                 followers[i].creature.x = playerHistory[followerIndex].x;
//                 followers[i].creature.y = playerHistory[followerIndex].y;
//             }
//         }
        
//         // Animation updates - these stay the same
//         if (!c->anims.isStopping) {
//             followers[i].followerFrameCounter++;
//         }
       
//         int followerFrameCounted = followers[i].creature.anims.walkFrameCount[c->anims.direction];
//         if (c->anims.movementState == WALKING) {
//             followers[i].lastDirection = c->anims.direction;
           
//             if (followers[i].followerFrameCounter >= FOLLOWER_FRAME_SPEED) {
//                 followers[i].lastFrame = (followers[i].lastFrame + 1) % followerFrameCounted;
//                 followers[i].followerFrameCounter = 0;
//             }
//         }
//         else if (c->anims.movementState == IDLE) {
//             if (followers[i].followerFrameCounter >= f->idleFrameSpeed) {
//                 followers[i].lastFrame = rand() % followers[i].creature.anims.idleFrameCount;
//                 followers[i].followerFrameCounter = 0;
//             }
//         }
//     }
// }

// Function to find the nearest creature to a specific follower
// int FindNearestCreatureToFollower(int followerIndex) {
//     int nearestIndex = -1;
//     float nearestDist = realTimeBattleState.engagementRadius * 1.5f; // Maximum search radius
    
//     Follower* follower = &followers[followerIndex];
    
//     for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
//         if (!realTimeBattleState.creatures[i].active) continue;
        
//         // Calculate distance
//         float dx = realTimeBattleState.creatures[i].position.x - follower->creature.x;
//         float dy = realTimeBattleState.creatures[i].position.y - follower->creature.y;
//         float dist = sqrtf(dx*dx + dy*dy);
        
//         if (dist < nearestDist) {
//             nearestDist = dist;
//             nearestIndex = i;
//         }
//     }
    
//     return nearestIndex;
// }

void HasBefriendedAllFieldTypes() {
    // Store names of befriended creatures
    char friendNames[MAX_PARTY_SIZE + MAX_BACKUP_SIZE + MAX_BANK_SIZE][50]; // 50 is max length of names
    int friendIndex = 0;

    // Add all names from mainTeam, backupTeam, and bank to friendNames
    for (int i = 0; i < MAX_PARTY_SIZE; i++) {
        if (mainTeam[i].name[0] != '\0') {
            strcpy(friendNames[friendIndex], mainTeam[i].name);
            friendIndex++;
        }
    }
    for (int i = 0; i < MAX_BACKUP_SIZE; i++) {
        if (backupTeam[i].name[0] != '\0') {
            strcpy(friendNames[friendIndex], backupTeam[i].name);
            friendIndex++;
        }
    }
    for (int i = 0; i < MAX_BANK_SIZE; i++) {
        if (bank[i].name[0] != '\0') {
            strcpy(friendNames[friendIndex], bank[i].name);
            friendIndex++;
        }
    }

    // Store names of all enemies
    char enemyNames[MAX_ENEMY_TYPES][50];
    int enemyIndex = 0;
    for (int i = 0; i < enemyDatabase.enemyCount; i++) {    
        if (enemyDatabase.creatures[i].name[0] != '\0') {
            if (enemyDatabase.creatures[i].terrains[0] != StringToTerrainType("BOSS")) {
                strcpy(enemyNames[enemyIndex], enemyDatabase.creatures[i].name);
                enemyIndex++;
            }
        }
    }

    // Check for missing creatures
    mainTeam[0].completeFriendship = true;
    // printf("Missing creatures:\n");
    for (int i = 0; i < enemyIndex; i++) {
        bool found = false;
        for (int j = 0; j < friendIndex; j++) {
            if (strcmp(enemyNames[i], friendNames[j]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            mainTeam[0].completeFriendship = false;
            // printf("- %s\n", enemyNames[i]);
        }
    }
}



bool RandomEncounterOnEmpty() {
    if (worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].type == TERRAIN_EMPTY) {
        // printf("\nChecking for encounter on EMPTY terrain...\n");

        // Calculate encounter chance
        float encounterChance = mainTeam[0].pressRate * (float)mainTeam[0].steps[TERRAIN_EMPTY] / (float)worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].encounterThreshold;

        // printf("\nEncounter Chance: %.2f%%\n", encounterChance * 100);

        // Check if a random encounter should occur
        if (50 < (encounterChance * 100)) {
            // Reset step counter and re-roll encounter threshold
            mainTeam[0].steps[TERRAIN_EMPTY] = 0;
            worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].encounterThreshold =  IsKeyPressed(B) ? RANDOMS(80, 160) : RANDOMS(300, 500);
            TrackRunningSuccess();
            // printf("\nEncounter triggered!\n");
            return true; // Random encounter triggered
        }
    }
    else {
        mainTeam[0].steps[TERRAIN_EMPTY] = 0;
    } 
    return false;
}




void TrackButtonPresses() {
    // Record current time in seconds
    float currentTime = (float)clock() / CLOCKS_PER_SEC;

    // Static variables to keep track of state between calls
    static int lastPressedKey = -1; // Track the last direction key pressed (-1 means none)
    static float lastPressTime = 0.0f; // Time of the last valid direction change

    // Current direction key being pressed
    int currentKey = -1;
    if (IsKeyPressed(KEY_UP)) currentKey = KEY_UP;
    else if (IsKeyPressed(KEY_DOWN)) currentKey = KEY_DOWN;
    else if (IsKeyPressed(KEY_LEFT)) currentKey = KEY_LEFT;
    else if (IsKeyPressed(KEY_RIGHT)) currentKey = KEY_RIGHT;

    // Check for valid direction change
    if (currentKey != -1 && currentKey != lastPressedKey) {
        buttonPressCount++;
        lastPressedKey = currentKey; // Update last pressed key
        lastPressTime = currentTime; // Update the time of the last valid press
    }

    // If no direction change or key is held, reset press rate
    if (currentTime - lastPressTime > 0.2f) { // 0.2s threshold for "holding"
        mainTeam[0].pressRate = 1.0f;
    } else {
        // Calculate presses per second and update multiplier
        float pressesPerSecond = buttonPressCount / (currentTime - lastCheckTime);

        // Update multiplier (e.g., exponential growth for high press rates)
        mainTeam[0].pressRate = 1.0f + pressesPerSecond * 0.1f; // Adjust 0.5f for sensitivity
        if (mainTeam[0].pressRate > 2.0f) {
            mainTeam[0].pressRate = 2.0f; // Cap multiplier
        }
    }

    // Reset count and timer every second
    if (currentTime - lastCheckTime >= 1.0f) {
        buttonPressCount = 0;
        lastCheckTime = currentTime;
    }
}


void ScreenBreakingAnimation() {
    frameTracking.transitionFrameCounter = 0;
    // Capture the current screen
    Image screenshot = LoadImageFromScreen();
    if (screenshot.data == NULL) {
        fprintf(stderr, "Failed to load screen");
    }
    Image screenCapture = ImageCopy(screenshot);
    UnloadImage(screenshot);

    // Initialize particles
    int particleCount = 0;
    int particleSize = PARTICLE_SIZE;
    int screenWidth = screenCapture.width;
    int screenHeight = screenCapture.height;

    for (int y = 0; y < screenHeight; y += particleSize) {
        for (int x = 0; x < screenWidth; x += particleSize) {
            if (particleCount < MAX_PARTICLES) {
                Color pixelColor = GetImageColor(screenCapture, x, y);
                particles[particleCount].position = (Vector2){ x, y };
                particles[particleCount].velocity = (Vector2){ GetRandomValue(-3, 3), GetRandomValue(1, 5) };
                particles[particleCount].color = pixelColor;
                particles[particleCount].active = true;
                particleCount++;
            }
        }
    }
}


void DrawStatBar(int x, int y, int width, int height, int current, int maximum, Color backColor, Color frontColor, bool putText) {
    float ratio = (maximum > 0) ? (float)current / (float)maximum : 0.0f;
    // Draw the background
    DrawRectangle(x, y, width, height, backColor);
    // Draw the filled portion
    DrawRectangle(x, y, (int)(width * ratio), height, frontColor);
    
    // Optionally, draw a border
    DrawRectangleLines(x, y, width, height, BLACK);
    
    // Draw text on top if you want
    if (putText) DrawText(TextFormat("%d/%d", current, maximum), x + 5, y + 2, 14, WHITE);
}


//---------------------------------------------------------
// 1) Word-wrap utility
//---------------------------------------------------------
void DrawTextBoxed(const char *text, int posX, int posY, int fontSize, int maxWidth, int lineSpacing, Color color) {
    int wordsCount = 0;
    const char **words = TextSplit(text, ' ', &wordsCount); 
    // NOTE: TextSplit() is a Raylib utility that splits a string by a delimiter
    // and returns an array of "const char*". (Requires Raylib 4.0+)

    if (words == NULL) {
        // If for some reason splitting fails, fallback
        DrawText(text, posX, posY, fontSize, color);
        return;
    }

    // We'll accumulate text word by word and measure it
    char lineBuffer[512] = { 0 };
    lineBuffer[0] = '\0';

    int lineY = posY;
    for (int i = 0; i < wordsCount; i++)
    {
        // Check if adding this word exceeds maxWidth
        // Temporarily add word to lineBuffer + space
        char testBuffer[512];
        snprintf(testBuffer, sizeof(testBuffer), "%s%s%s", lineBuffer, (lineBuffer[0] == '\0') ? "" : " ", words[i]);
        
        int textWidth = MeasureText(testBuffer, fontSize);
        if (textWidth > maxWidth && lineBuffer[0] != '\0')
        {
            // Draw the current line, then start a new line
            DrawText(lineBuffer, posX, lineY, fontSize, color);
            lineY += (fontSize + lineSpacing);
            // Begin a new buffer with the current word
            snprintf(lineBuffer, sizeof(lineBuffer), "%s", words[i]);
        }
        else
        {
            // Keep accumulating
            snprintf(lineBuffer, sizeof(lineBuffer), "%s", testBuffer);
        }
    }

    // Draw the last line (if any)
    if (lineBuffer[0] != '\0')
    {
        DrawText(lineBuffer, posX, lineY, fontSize, color);
    }

    MemFree(words); // if using Raylib’s TextSplit
}


void SwapCreatures(Creature *a, Creature *b) {
    Creature temp = *a;
    *a = *b;
    *b = temp;
}


bool IsKeyDownSmooth(int key, KeySmoothState *state) {
    bool isDown = IsKeyDown(key);
    bool triggered = false;

    if (isDown) {
        if (!state->wasDown) {
            // Initial press - always trigger
            state->holdDuration = 0;
            triggered = true;
        } else {
            // Update hold duration
            state->holdDuration += GetFrameTime();
            
            // Check if we should trigger repeat
            if (state->holdDuration > state->initialDelay) {
                float repeatTime = state->holdDuration - state->initialDelay;
                if (fmod(repeatTime, state->repeatInterval) < GetFrameTime()) {
                    triggered = true;
                }
            }
        }
    } else {
        // Reset when key is released
        state->holdDuration = 0;
    }

    state->wasDown = isDown;
    return triggered;
}


KeySmoothState InitSmoothKey(float delay, float interval) {
    return (KeySmoothState){
        .holdDuration = 0,
        .initialDelay = delay,
        .repeatInterval = interval,
        .wasDown = false
    };
}


// A helper function to linearly interpolate between two colors.
// t = 0.0  => returns color1
// t = 1.0  => returns color2
static Color ColorLerp(Color c1, Color c2, float t) {
    Color result;
    result.r = (unsigned char)(c1.r + (c2.r - c1.r)*t);
    result.g = (unsigned char)(c1.g + (c2.g - c1.g)*t);
    result.b = (unsigned char)(c1.b + (c2.b - c1.b)*t);
    result.a = (unsigned char)(c1.a + (c2.a - c1.a)*t);
    return result;
}

// DrawEllipseGradient()
//  Draws a 2D ellipse at (centerX, centerY) of size (radiusH, radiusV) 
//  with a radial gradient blending from colorInner (center) 
//  to colorOuter (edge).
void DrawEllipseGradient(int centerX, int centerY, int radiusH, int radiusV, Color colorInner, Color colorOuter) {
    // We'll step from the largest ellipse (outer edge) down to smallest (inner).
    // You can adjust the step count or direction for a smoother/faster approach.
    const int steps = 16;  // Increase for smoother gradient, or decrease for performance

    for (int i = 0; i < steps; i++)
    {
        // fraction goes from 0.0 -> 1.0 as we draw from outside -> inside
        float fraction = (float)i / (steps - 1);

        // The current ellipse radii, going from full size (fraction=0) to near-zero (fraction=1)
        int curRadiusH = (int)(radiusH * (1.0f - fraction));
        int curRadiusV = (int)(radiusV * (1.0f - fraction));

        // Interpolate color from outer to inner
        // If you want colorInner at fraction=0, invert fraction below:
        Color col = ColorLerp(colorOuter, colorInner, fraction);

        // Draw the ellipse at this step
        DrawEllipse(centerX, centerY, curRadiusH, curRadiusV, col);
    }
}

// Comparison function for qsort
int CompareCreaturesByID(const void *a, const void *b) {
    const Creature *ca = (const Creature *)a;
    const Creature *cb = (const Creature *)b;
    return (ca->id - cb->id);
}



void ModifyTerrainWithItem(Item *item, Menu *menu) {
    
    // Set up animation and bomb pending based on item type
    if (strcmp(item->name, "Nade") == 0) {
        menu->bombAnimation = animations.nade;
        menu->bombPending.isExplosion = false;
        Vector2 pos = CheckTerrainInPlayerArea(TERRAIN_MOUNTAIN);
        menu->bombPending.centerX = pos.x;
        menu->bombPending.centerY = pos.y;
        SetOverworldMessage("Boom! The mountain will be destroyed...");
    } 
    else if (strcmp(item->name, "Explosion") == 0) {                
        menu->bombAnimation = animations.explosion;
        menu->bombPending.isExplosion = true;
        menu->bombPending.centerX = mainTeam[0].x+0.5;
        menu->bombPending.centerY = mainTeam[0].y+0.5;
        SetOverworldMessage("Boom! Explosion incoming...");
    }

    menu->bombAnimation.isAnimating = true;
    menu->bombAnimation.currentFrame = 0;
    menu->bombAnimation.frameCounter = 0;
    menu->bombAnimation.frameSpeed = 10;
    menu->bombAnimation.color = GetItemAnimationColor(item);
    menu->bombPending.active = true;
    menu->bombPending.item = item;
}

void CompactTeam(Creature team[], int size) {
    int writeIndex = 0;
    for (int readIndex = 0; readIndex < size; readIndex++) {
        // If this slot is NOT empty, copy it to the writeIndex if needed
        if (team[readIndex].name[0] != '\0') {
            if (readIndex != writeIndex) {
                team[writeIndex] = team[readIndex];
                // Mark old slot as empty
                team[readIndex].name[0] = '\0';
            }
            writeIndex++;
        }
    }

    // If you want, you can also zero out the rest from writeIndex onward:
    for (int i = writeIndex; i < size; i++) {
        team[i].name[0] = '\0'; // ensure truly empty
    }
}

void PredictFutureTurns(BattleState *battleState, BattleParticipant **predictedOrder) {
    SimulatedTurn simulated[MAX_PARTICIPANTS];
    BattleParticipant* currentActor = battleState->currentParticipant;

    // Initialize simulation state
    for (int i = 0; i < battleState->participantCount; i++) {
        simulated[i] = (SimulatedTurn){
            .original = &battleState->participants[i],
            .simulatedTime = battleState->participants[i].nextActionTime,
            .effectiveSpeed = battleState->participants[i].creature->speed + 
                              battleState->participants[i].creature->tempSpeedBuff,
            .pendingActionSpeed = standardSpeedCost  // Default normal speed
        };

        // If the participant is dead or inactive, set simulated time to a huge value
        if (!battleState->participants[i].isActive || battleState->participants[i].creature->currentHP <= 0) {
            simulated[i].simulatedTime = FLT_MAX;  // include <float.h> at the top
        }

        // Apply speed modifier ONLY to the current actor
        if (battleState->targetSelectionActive && simulated[i].original == currentActor) {
            simulated[i].pendingActionSpeed = battleState->speedCost;
        }

        if (simulated[i].effectiveSpeed <= 0) {
            simulated[i].effectiveSpeed = standardSpeedCost;
        }
    }

    // Simulate turns
    for (int turn = 0; turn < TOTAL_PREDICTED_TURNS; turn++) {
        int nextIndex = FindNextParticipant(simulated, battleState->participantCount);
        // If no valid participant is found, break out of the loop.
        if (nextIndex == -1) break;
        predictedOrder[turn] = simulated[nextIndex].original;

        // Apply speed modifier only once for the current actor's next turn
        float actionSpeed = simulated[nextIndex].pendingActionSpeed;
        simulated[nextIndex].pendingActionSpeed = standardSpeedCost;  // Reset after application

        simulated[nextIndex].simulatedTime += (1.0f / simulated[nextIndex].effectiveSpeed) * actionSpeed;
    }
}

int FindNextParticipant(SimulatedTurn *simulated, int participantCount) {
    int nextIndex = -1;
    float minTime = FLT_MAX;
    
    for (int i = 0; i < participantCount; i++) {
        // Skip participants with huge simulated times (i.e. dead/inactive)
        if (simulated[i].simulatedTime >= FLT_MAX) {
            continue;
        }
        
        float currentTime = simulated[i].simulatedTime;
        
        if (currentTime < (minTime - FLOAT_EPSILON)) {
            minTime = currentTime;
            nextIndex = i;
        }
        else if (fabs(currentTime - minTime) <= FLOAT_EPSILON) {
            if (simulated[i].effectiveSpeed > simulated[nextIndex].effectiveSpeed) {
                nextIndex = i;
            }
            else if (simulated[i].effectiveSpeed == simulated[nextIndex].effectiveSpeed) {
                if (i < nextIndex) {
                    nextIndex = i;
                }
            }
        }
    }
    
    return nextIndex;
}

void DrawMenuItems() {
    // Determine which item list to display
    Item *currentItems = NULL;
    int itemListSize = 0;
    int *currentIndex = NULL;
    
    switch (menu.itemViewMode) {
        case ITEM_USABLE: { 
            currentItems = playerInventory; 
            itemListSize = itemCount; 
            currentIndex = &menu.usableItemIndex; 
            break; 
        }
        case ITEM_KEY: { 
            currentItems = keyItemList; 
            itemListSize = keyItemCount; 
            currentIndex = &menu.keyItemIndex; 
            break; 
        }
        default: assert(false && "Unhandled enum value!"); break;
    }
    
    // Two-column layout configuration
    const int columns = 2;
    const int rows = 10; // 10 rows visible at once
    const int itemsPerPage = rows * columns;
    const int columnWidth = 300;
    const int itemHeight = 30;
    const int startY = 100;
    const int startX = 100;
    
    // Calculate current page
    int currentPage = (*currentIndex) / itemsPerPage;
    int pageStartIndex = currentPage * itemsPerPage;
    
    // Draw items in the grid layout
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            int itemIndex = pageStartIndex + row * columns + col;
            
            // Stop if we've drawn all items
            if (itemIndex >= itemListSize) break;
            
            // Skip items that aren't unlocked
            if (!currentItems[itemIndex].isUnlocked) continue;
            
            int itemX = startX + col * columnWidth;
            int itemY = startY + row * itemHeight;
            
            char buf[100];
            if (currentItems[itemIndex].amount > 0 || currentItems[itemIndex].isKeyItem) {
                if (menu.itemViewMode == ITEM_USABLE) {
                    sprintf(buf, "%s (x%d) [ID:%d]", 
                           currentItems[itemIndex].name, 
                           currentItems[itemIndex].amount,
                           currentItems[itemIndex].id);
                } else {
                    sprintf(buf, "%s [ID:%d]", 
                           currentItems[itemIndex].name,
                           currentItems[itemIndex].id);
                }
            } else {
                sprintf(buf, "%s (Empty) [ID:%d]", 
                       currentItems[itemIndex].name,
                       currentItems[itemIndex].id);
            }
            
            Color color = (itemIndex == *currentIndex) ? RED : BLACK;
            DrawText(buf, itemX, itemY, 20, color);
            
            if (menu.swapInProgress && itemIndex == menu.swapSourceIndex) {
                DrawText(" (Swapping)", itemX + 200, itemY, 20, YELLOW);
            }
        }
    }
}




// Add this function somewhere in main.c
ToolType GetToolTypeFromIndex(ToolWheel* wheel, int index) {
    if (index < 0 || index >= MAX_TOOLS) {
        // Handle invalid index if necessary, maybe return a specific value like TOOL_NONE
        // For simplicity here, assuming index is usually valid when called
        return wheel->tools[0].type; // Default to claw if index is somehow bad
    }
    // If the tool at the index is an improved version, return its standard base type
    if (wheel->tools[index].isImprovedVersion) {
        return wheel->tools[index].standardVersionType;
    }
    // Otherwise, return the tool's own type
    return wheel->tools[index].type;
}


















/////////////////////////////////////////////////////////////////////////////
/////////////////////////// REAL TIME BATTLE ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


void InitializeCommandMenu() {
    commandMenuState.selectedIndex = 0;
    commandMenuState.justEnteredMenu = false;
    commandMenuState.targetSelectionMode = false;
    commandMenuState.selectedTargetIndex = -1;
    commandMenuState.inAISubmenu = false;
    commandMenuState.waitingForKeyRelease = false;
    commandMenuState.wasXPressed = false;
    commandMenuState.xHoldTime = 0.0f;
    commandMenuState.xLongPressExecuted = false;
}




// Modified function for checking random encounters in RTBS
bool RandomEncounterRTBS(RealTimeBattleState* rtbsState) {
    // Only attempt to spawn if we have room for more creatures
    if (rtbsState->creatureCount >= MAX_RTBS_CREATURES) {
        return false;
    }
    
    // Update spawn timer
    rtbsState->spawnTimer -= GetFrameTime();
    if (rtbsState->spawnTimer > 0) {
        return false;
    }
    
    // Reset spawn timer
    rtbsState->spawnTimer = rtbsState->spawnInterval;
    
    // Get player's current terrain
    TerrainType currentTerrain = worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].type;
    
    // Always have a minimum chance of encounter when in RTBS mode
    float baseEncounterChance = 15.0f; // 15% base chance
    
    // If moving, add the standard calculation
    float movementEncounterChance = 0.0f;
    if (mainTeam[0].steps[currentTerrain] > 0) {
        movementEncounterChance = mainTeam[0].pressRate * 
                                 (float)mainTeam[0].steps[currentTerrain] / 
                                 (float)worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].encounterThreshold;
        movementEncounterChance *= 100.0f; // Convert to percentage
    }
    
    // Total chance is base chance + movement chance
    float totalEncounterChance = baseEncounterChance + movementEncounterChance;
    
    // Cap at 75% max
    if (totalEncounterChance > 75.0f) {
        totalEncounterChance = 75.0f;
    }
    
    printf("Encounter check: base=%.1f%%, movement=%.1f%%, total=%.1f%%\n", 
           baseEncounterChance, movementEncounterChance, totalEncounterChance);
    
    // Check if we should spawn a creature
    int randomRoll = RANDOMS(1, 100);
    printf("Random roll: %d vs chance: %.1f\n", randomRoll, totalEncounterChance);
    
    if (randomRoll <= totalEncounterChance) {
        // If successful, reset step counter and re-roll threshold
        mainTeam[0].steps[currentTerrain] = 0;
        worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].encounterThreshold = RANDOMS(300, 500);
        
        // Spawn a creature
        printf("Encounter roll successful! Spawning creature...\n");
        SpawnOverworldCreature(rtbsState, currentTerrain);
        return true;
    }
    
    printf("No encounter this time.\n");
    return false;
}


void UpdateOverworldCreatures(RealTimeBattleState* rtbsState) {
    float deltaTime = GetFrameTime();

    // Access the global or passed-in difficulty level
    SettingsGameplayDifficultyType currentDifficulty = (SettingsGameplayDifficultyType)mainMenuState.difficultyLevel;

    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        OverworldCreature* creature = &rtbsState->creatures[i];

        if (!creature->active) continue;

        // Update lifespan (unaffected by difficulty)
        creature->lifespan -= deltaTime;
        if (creature->lifespan <= 0.0f) {
            creature->active = false;
            rtbsState->creatureCount--;
            continue;
        }

        // Update attack cooldown timer (unaffected by difficulty)
        if (creature->attackCooldown > 0) {
            creature->attackCooldown -= deltaTime;
        }

        // Skip movement if currently attacking (unaffected by difficulty)
        if (creature->isAttacking) {
            continue;
        }

        // --- Difficulty Adjustment: Attack Cooldown Reset ---
        // Check if creature can attack
        if (creature->attackCooldown <= 0 && creature->withinPlayerRange) {
            int targetIndex;
            if (CanCreatureAttackParty(rtbsState, creature, &targetIndex)) {
                // Attack player or follower
                CreatureAttackParty(rtbsState, creature, i);

                // --- Reset cooldown based on difficulty ---
                float baseCooldown = 3.0f; // Base time for Normal
                float randomFactor = ((float)RANDOMS(0, 100) / 100.0f); // 0.0 to 1.5 seconds randomness

                switch (currentDifficulty) {
                    case SETTINGS_EASY_MODE:
                        // Longer cooldown on Easy
                        creature->attackCooldown = (baseCooldown * 1.5f) + randomFactor; // ~4.5 - 6.0 sec
                        break;
                    case SETTINGS_HARD_MODE:
                        // Shorter cooldown on Hard
                        creature->attackCooldown = (baseCooldown * 0.6f) + randomFactor; // ~1.8 - 3.3 sec
                        break;
                    case SETTINGS_NORMAL_MODE:
                    default:
                        // Normal cooldown
                        creature->attackCooldown = baseCooldown + randomFactor; // ~3.0 - 4.5 sec
                        break;
                }

                // Ensure minimum cooldown to prevent spamming even on Hard
                if (creature->attackCooldown < 1.2f) {
                    creature->attackCooldown = 1.2f;
                }

                continue; // Skip movement after attacking
            }
        }

        // Calculate distance to player (unaffected by difficulty)
        float dx = mainTeam[0].x - creature->position.x;
        float dy = mainTeam[0].y - creature->position.y;
        float distanceToPlayer = sqrtf(dx * dx + dy * dy);

        // Check if player is within engagement range
        bool wasInRange = creature->withinPlayerRange;
        creature->withinPlayerRange = (distanceToPlayer <= creature->engagementRadius);

        // If player just entered range, show a message (unaffected by difficulty)
        if (creature->withinPlayerRange && !wasInRange) {
            char message[100];
            sprintf(message, "%s is ready to battle!", creature->creature.name);
            SetOverworldMessage(message);
        }

        // Update despawn timer (unaffected by difficulty)
        if (creature->withinPlayerRange) {
            creature->despawnTimer = 10.0f; // Reset timer when player is nearby
        } else {
            creature->despawnTimer -= deltaTime;
            if (creature->despawnTimer <= 0.0f) {
                creature->active = false;
                rtbsState->creatureCount--;
                continue;
            }
        }

        // AI movement decision timer
        bool creatureMoved = false;
        creature->moveTimer -= deltaTime;

        // FIX: Restructure the movement decision logic to properly handle all modes
        if (creature->moveTimer <= 0.0f) {
            // Time to make a new movement decision

            if (currentDifficulty == SETTINGS_HARD_MODE) {
                // --- HARD MODE: Always track the player ---
                float dx_p = mainTeam[0].x - creature->position.x;
                float dy_p = mainTeam[0].y - creature->position.y;
                float distanceToPlayer_p = sqrtf(dx_p * dx_p + dy_p * dy_p);

                // Check if creature is already very close (attack range)
                bool inAttackRange = (distanceToPlayer_p <= 1.0f);

                if (distanceToPlayer_p > 0.01f) { // Avoid division by zero if overlapping
                    if (!inAttackRange) {
                        // Move towards player if not yet in attack range
                        creature->moveDirection.x = dx_p / distanceToPlayer_p;
                        creature->moveDirection.y = dy_p / distanceToPlayer_p;
                    } else {
                        // In attack range - stop moving horizontally/vertically to prepare attack
                        creature->moveDirection.x = 0;
                        creature->moveDirection.y = 0;
                    }

                    // Always face the player on Hard mode when deciding movement
                    if (fabsf(dx_p) > fabsf(dy_p)) {
                        creature->creature.anims.direction = (dx_p > 0) ? RIGHT : LEFT;
                    } else {
                        creature->creature.anims.direction = (dy_p > 0) ? DOWN : UP;
                    }
                } else {
                    // If somehow directly on top, stop moving
                    creature->moveDirection.x = 0;
                    creature->moveDirection.y = 0;
                }
                
                // Update direction decision very frequently on Hard
                creature->moveTimer = 0.5f; // FIXED: Increased to reduce spinning (was 0.05)
            }
            else {
                // EASY or NORMAL MODE behavior
                if (creature->withinPlayerRange) {
                    // FIX: This will now run when withinPlayerRange AND timer expired
                    // Pursue/face player logic
                    float dx_p = mainTeam[0].x - creature->position.x;
                    float dy_p = mainTeam[0].y - creature->position.y;
                    float distanceToPlayer_p = sqrtf(dx_p * dx_p + dy_p * dy_p);

                    if (distanceToPlayer_p > 1.0f) { // Move towards player
                        creature->moveDirection.x = dx_p / distanceToPlayer_p;
                        creature->moveDirection.y = dy_p / distanceToPlayer_p;
                        
                        // Set facing direction
                        if (fabsf(creature->moveDirection.x) > fabsf(creature->moveDirection.y)) {
                            creature->creature.anims.direction = (creature->moveDirection.x > 0) ? RIGHT : LEFT;
                        } else {
                            creature->creature.anims.direction = (creature->moveDirection.y > 0) ? DOWN : UP;
                        }
                    } else { // Stop and face player
                        creature->moveDirection.x = 0;
                        creature->moveDirection.y = 0;
                        
                        // Set facing direction
                        if (fabsf(dx_p) > fabsf(dy_p)) {
                            creature->creature.anims.direction = (dx_p > 0) ? RIGHT : LEFT;
                        } else {
                            creature->creature.anims.direction = (dy_p > 0) ? DOWN : UP;
                        }
                    }
                    
                    creature->moveTimer = 0.2f; // Update decision quickly when engaged
                } else {
                    // Not in range - random movement logic between 0 and 2pi
                    float angle = ((float)RANDOMS(0, 628) / 100.0f);
                    creature->moveDirection.x = cosf(angle);
                    creature->moveDirection.y = sinf(angle);
                    
                    // Set facing direction
                    if (fabsf(creature->moveDirection.x) > fabsf(creature->moveDirection.y)) {
                        creature->creature.anims.direction = (creature->moveDirection.x > 0) ? RIGHT : LEFT;
                    } else {
                        creature->creature.anims.direction = (creature->moveDirection.y > 0) ? DOWN : UP;
                    }
                    
                    // FIX: Longer random movement duration
                    creature->moveTimer = 1.5f + ((float)RANDOMS(0, 150) / 100.0f);
                }
            }
        }
        // No else clause here - we use the last direction until timer expires

        // --- Difficulty Adjustment: Movement Speed ---
        float baseMoveSpeedFactor = 2.5f; // Speed factor on Normal
        float difficultyMoveMultiplier = 1.0f; // Normal multiplier

        switch (currentDifficulty) {
            case SETTINGS_EASY_MODE:
                difficultyMoveMultiplier = 0.70f; // Slower on Easy
                break;
            case SETTINGS_HARD_MODE:
                difficultyMoveMultiplier = 1.30f; // Faster on Hard
                break;
            case SETTINGS_NORMAL_MODE:
            default:
                difficultyMoveMultiplier = 1.0f;
                break;
        }

        float moveSpeed = baseMoveSpeedFactor * difficultyMoveMultiplier * deltaTime;

        // Apply pursuit bonus (could also scale this bonus with difficulty if desired)
        float pursuitBonus = 1.5f; // Standard bonus when chasing
        if (creature->withinPlayerRange && distanceToPlayer > 1.0f) {
            moveSpeed *= pursuitBonus;
        }
        // --- End Movement Speed Adjustment ---

        Vector2 moveVector = {
            creature->moveDirection.x * moveSpeed,
            creature->moveDirection.y * moveSpeed
        };

        // Apply movement, collision, sliding (logic unchanged by difficulty)
        if (fabsf(moveVector.x) > 0.001f || fabsf(moveVector.y) > 0.001f) {
            Vector2 newPos = {
                creature->position.x + moveVector.x,
                creature->position.y + moveVector.y
            };

            // Check validity and move/slide
            if (IsPositionValid(newPos, ENTITY_CREATURE, creature)) {
                creature->position = newPos;
                creatureMoved = true;
                
                // Update facing direction based on actual moveVector
                if (fabsf(moveVector.x) > fabsf(moveVector.y)) {
                   creature->creature.anims.direction = (moveVector.x > 0) ? RIGHT : LEFT;
                } else {
                   creature->creature.anims.direction = (moveVector.y > 0) ? DOWN : UP;
                }
            }
            // Optional: Sliding logic
            else {
                // Try horizontal slide
                Vector2 slideH = {newPos.x, creature->position.y};
                if (fabsf(moveVector.x) > 0.001f && IsPositionValid(slideH, ENTITY_CREATURE, creature)) {
                   creature->position = slideH;
                   creatureMoved = true;
                   creature->creature.anims.direction = (moveVector.x > 0) ? RIGHT : LEFT;
                }
                // Try vertical slide
                else {
                    Vector2 slideV = {creature->position.x, newPos.y};
                    if (fabsf(moveVector.y) > 0.001f && IsPositionValid(slideV, ENTITY_CREATURE, creature)) {
                       creature->position = slideV;
                       creatureMoved = true;
                       creature->creature.anims.direction = (moveVector.y > 0) ? DOWN : UP;
                    }
                }
           }
        }

        // In the animation state update section
        if (creatureMoved) {
            // Set walking state - no changes needed here
            creature->creature.anims.movementState = WALKING;
            creature->creature.anims.isMoving = true;
            creature->creature.anims.isStopping = false;
            creature->animTracking.stopDelayCounter = 60;
            ApplySeparationBehavior(&creature->position, ENTITY_CREATURE, creature);
        } else {
            // Check if we're stopped because we're preparing to attack
            bool readyToAttack = creature->withinPlayerRange && 
                                creature->attackCooldown <= 0 &&
                                distanceToPlayer <= 1.2f;  // Add this variable
            
            if (readyToAttack) {
                // If ready to attack, stay in WALKING or combat-ready state
                creature->creature.anims.movementState = WALKING;
                // Use a specific frame that looks like combat stance if you have one
                creature->animTracking.currentFrame = 0;
                creature->creature.anims.isMoving = false;
                creature->creature.anims.isStopping = false;
            } else {
                // Normal idle transition logic for when not attacking
                if (creature->creature.anims.isMoving) {
                    creature->creature.anims.isMoving = false;
                    creature->creature.anims.isStopping = true;
                    creature->animTracking.stopDelayCounter = 60;
                } else if (creature->creature.anims.isStopping && creature->animTracking.stopDelayCounter > 0) {
                    creature->animTracking.stopDelayCounter--;
                } else if (creature->creature.anims.isStopping) {
                    creature->creature.anims.movementState = IDLE;
                    creature->creature.anims.isStopping = false;
                }
            }
        }

        // Update animation frames (unaffected by difficulty)
        UpdatePlayerAnimation(&creature->creature, &creature->animTracking);
    } // End of creature loop

    // Update player damage display (unaffected by difficulty)
    if (rtbsState->playerDamageTimer > 0) {
        rtbsState->playerDamageTimer--; // Use integer timer maybe?
        rtbsState->playerDamageOffsetY -= 0.5f;
    }

    // Update player damage display (unaffected by difficulty)
    for (int i = 0; i < followerCount; i++) {
        if (rtbsState->followerDamageTimer[i] > 0) {
            rtbsState->followerDamageTimer[i]--; // Use integer timer maybe?
            rtbsState->followerDamageOffsetY[i] -= 0.5f;
        }
    }

    // Update creature damage display (add after player and follower damage updates)
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (rtbsState->creatureDamageTimer[i] > 0) {
            rtbsState->creatureDamageTimer[i]--;
            rtbsState->creatureDamageOffsetY[i] -= 0.5f;
        }
    }

    // Update screen shake (unaffected by difficulty)
    if (rtbsState->shakeTimer > 0) {
        rtbsState->shakeTimer--;
    }
}


void DrawOverworldCreature(OverworldCreature* creature, Vector2 viewport) {
    if (!creature->active) return;
    
    // Calculate screen position
    int creatureScreenX = (creature->position.x - viewport.x) * TILE_SIZE;
    int creatureScreenY = (creature->position.y - viewport.y) * TILE_SIZE;
    
    // Skip if off-screen
    if (creatureScreenX < -TILE_SIZE || creatureScreenX > SCREEN_WIDTH ||
        creatureScreenY < -TILE_SIZE || creatureScreenY > SCREEN_HEIGHT) {
        return;
    }
    
    // Get the terrain at creature's position
    TerrainType currentTerrain;
    int terrainX = (int)creature->position.x;
    int terrainY = (int)creature->position.y;
    
    if (terrainX >= 0 && terrainX < mapWidth && terrainY >= 0 && terrainY < mapHeight) {
        currentTerrain = worldMap[terrainY][terrainX].type;
    } else {
        currentTerrain = TERRAIN_EMPTY;
    }
    
    // Try to use animation frames first - WITH SIZE CHECK
    Texture2D textureToDraw = {0};
    bool animationTextureFound = false;

    // First verify the correct texture dimensions for animation frames
    if (creature->creature.anims.movementState == IDLE && 
        creature->creature.anims.idleFrameCount > 0) {
        int frame = creature->animTracking.currentFrame % creature->creature.anims.idleFrameCount;
        if (creature->creature.anims.idleTextures[frame].id != 0) {
            textureToDraw = creature->creature.anims.idleTextures[frame];
            animationTextureFound = true;
        }
    } else if (creature->creature.anims.movementState == WALKING) {
        Direction dir = creature->creature.anims.direction;
        if (creature->creature.anims.walkFrameCount[dir] > 0) {
            int frame = creature->animTracking.currentFrame % creature->creature.anims.walkFrameCount[dir];
            if (creature->creature.anims.walkTextures[dir][frame].id != 0) {
                textureToDraw = creature->creature.anims.walkTextures[dir][frame];
                animationTextureFound = true;
            }
        }
    }

        // Draw damage number if applicable
    if (creature->targetPlayerDamage > 0) {
        // Calculate position for damage text (above creature)
        float damageX = creatureScreenX + TILE_SIZE/2;
        float damageY = creatureScreenY - 20;
        
        // Draw the damage number
        char damageText[16];
        sprintf(damageText, "%d", creature->targetPlayerDamage);
        int textWidth = MeasureText(damageText, 20);
        DrawText(damageText, damageX - textWidth/2, damageY, 20, RED);
        
        // Clear damage after displaying for one frame
        creature->targetPlayerDamage = 0;
    }
    

    // Fallback to static terrain textures if animation textures not found or invalid
    if (!animationTextureFound) {
        printf("No animation texture found... why?\n");
        if (creature->creature.anims.movementState == IDLE) {
            // Use idle animation frame
            int frame = creature->animTracking.currentFrame;
            if (frame >= creature->creature.anims.idleFrameCount) {
                frame = 0;
            }
            textureToDraw = creature->creature.anims.idleTextures[frame];
        } else {
            // Use walking animation frame for the appropriate direction
            Direction dir = creature->creature.anims.direction;
            int frame = creature->animTracking.currentFrame;
            if (frame >= creature->creature.anims.walkFrameCount[dir]) {
                frame = 0;
            }
            textureToDraw = creature->creature.anims.walkTextures[dir][frame];
        }
    }
    
    // CHECK WITH AI is this code duplicated?
    // Draw the sprite with CONSISTENT SIZE
    if (textureToDraw.id != 0) {
        // Always draw at TILE_SIZE dimensions regardless of actual texture size
        Rectangle source = {0, 0, textureToDraw.width, textureToDraw.height};
        Rectangle dest = {creatureScreenX, creatureScreenY, TILE_SIZE, TILE_SIZE};
        DrawTexturePro(textureToDraw, source, dest, (Vector2){0, 0}, 0, WHITE);
    } else {
        // Fallback placeholder
        DrawRectangle(creatureScreenX, creatureScreenY, TILE_SIZE, TILE_SIZE, RED);
        DrawText("?", creatureScreenX + TILE_SIZE/3, creatureScreenY + TILE_SIZE/3, 20, WHITE);
    }
    
    // Draw the creature
    // is this duplicated code?
    // DrawTexture(textureToDraw, creatureScreenX, creatureScreenY, WHITE);
    
    // Draw engagement radius (as in your original code)
    float radiusInPixels = creature->engagementRadius * TILE_SIZE;
    float creatureCenterX = creatureScreenX + (TILE_SIZE / 2.0f);
    float creatureCenterY = creatureScreenY + (TILE_SIZE / 2.0f);
    
    DrawCircleLines(creatureCenterX, creatureCenterY, radiusInPixels, Fade(creature->withinPlayerRange ? GREEN : RED, 0.6f));
    
    int barWidth = 40;
    int barHeight = 5;
    int barY = creatureScreenY - 15;

    // Draw creature name and HP bar (as in your original code)
    DrawText(creature->creature.name, creatureScreenX + (int)barWidth/2 - (int)MeasureText(creature->creature.name , 12) / 2, creatureScreenY - 30, 12, WHITE);
    
    DrawRectangle(creatureScreenX - (int)barHeight/2, barY - 2, barWidth + barHeight - 1, barHeight + 11, Fade(BLACK, 0.7f));
    
    DrawStatBar(creatureScreenX, barY, barWidth, barHeight, creature->creature.currentHP, creature->creature.maxHP, DARKGRAY, RED, false); // No text
    DrawRectangleLines(creatureScreenX, barY, barWidth, barHeight, WHITE);

    DrawStatBar(creatureScreenX, barY + barHeight + 2, barWidth, barHeight, creature->creature.currentMP, creature->creature.maxMP, DARKGRAY, BLUE, false); // No text
    DrawRectangleLines(creatureScreenX, barY + barHeight + 2, barWidth , barHeight, WHITE);

}

// Modified DrawRealTimeBattle to include creatures
void DrawRealTimeBattle(RealTimeBattleState* rtbsState, Camera2D camera, bool keysPressed[]) {
    // Apply screen shake if active
    Camera2D shakeCamera = camera;
    if (rtbsState->shakeTimer > 0) {
        // Add random screen shake
        shakeCamera.offset.x += (float)RANDOMS(-5, 5);
        shakeCamera.offset.y += (float)RANDOMS(-5, 5);
        BeginMode2D(shakeCamera);
    } else {
        BeginMode2D(camera);
    }

    // Draw map and characters
    Vector2 viewport = DrawMap();
    
    // Calculate player screen position
    int playerScreenX = (mainTeam[0].x - viewport.x) * TILE_SIZE;
    int playerScreenY = (mainTeam[0].y - viewport.y) * TILE_SIZE;
    
    // Draw player 
    DrawPlayer(playerScreenX, playerScreenY);

    // Draw overworld creatures
    DrawOverworldCreatures(rtbsState, viewport);
    
    // Draw followers
    DrawFollowers(viewport);

    DrawCommandQueue(rtbsState, viewport);

    // Draw overworld creatures with attack animations
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        OverworldCreature* creature = &rtbsState->creatures[i];
        
        if (!creature->active) continue;
        
        // Calculate screen position
        int creatureScreenX = (creature->position.x - viewport.x) * TILE_SIZE;
        int creatureScreenY = (creature->position.y - viewport.y) * TILE_SIZE;
        
        // Skip if off-screen 
        if (creatureScreenX < -TILE_SIZE || creatureScreenX > SCREEN_WIDTH ||
            creatureScreenY < -TILE_SIZE || creatureScreenY > SCREEN_HEIGHT) {
            continue;
        }

        if (rtbsState->creatures[i].active && rtbsState->creatureDamageTimer[i] > 0) {
            int creatureScreenX = (rtbsState->creatures[i].position.x - viewport.x) * TILE_SIZE;
            int creatureScreenY = (rtbsState->creatures[i].position.y - viewport.y) * TILE_SIZE;
            
            // Calculate alpha based on remaining time
            float alpha = (float)rtbsState->creatureDamageTimer[i] / DAMAGE_TIMER;
            
            // Position for damage text
            float damageX = creatureScreenX + rtbsState->creatureDamageRandomX[i];
            float damageY = creatureScreenY + rtbsState->creatureDamageOffsetY[i];
            
            // Choose color based on effect type
            Color damageColor = (rtbsState->creatureDamageEffectType[i] == EFFECT_OFFENSIVE) ? 
                                Fade(WHITE, alpha) : Fade(GREEN, alpha);
            
            // Draw the damage number
            char damageText[16];
            sprintf(damageText, "%d", rtbsState->creatureDamageDisplay[i]);
            int textWidth = MeasureText(damageText, 20);
            DrawTextWithOutline(damageText, damageX - textWidth/2, damageY, 20, damageColor, Fade(BLACK, alpha));
        }

        // Highlight selected creature in attack mode
        if (rtbsState->playerAttackMode && rtbsState->selectedCreatureIndex == i) {
            // Draw a more prominent selection highlight - use a circle instead of rectangle
            float creatureCenterX = creatureScreenX + (TILE_SIZE / 2.0f);
            float creatureCenterY = creatureScreenY + (TILE_SIZE / 2.0f);
            
            // Draw filled circle behind creature
            DrawCircle(creatureCenterX, creatureCenterY, TILE_SIZE * 0.7f, Fade(YELLOW, 0.3f));
            
            // Draw circle outline
            DrawCircleLines(creatureCenterX, creatureCenterY, TILE_SIZE * 0.7f, YELLOW);
            
            // Draw targeting arrow above creature
            // CHECK WITH AI other symbols that raylib acepts?
            DrawText("v", creatureScreenX + TILE_SIZE/2 - 5, creatureScreenY - 20, 26, YELLOW);
        }
    }
        

    // Modify the attack animation section in DrawRealTimeBattle
    // Find this section in your DrawRealTimeBattle function:
    if (rtbsState->attackAnimationActive && animations.attack.isAnimating) {
        float attackerX, attackerY, targetX, targetY;
        
        // Determine attacker position
        if (rtbsState->attackingCreatureIndex >= 0) {
            // Creature is attacking
            OverworldCreature* attacker = &rtbsState->creatures[rtbsState->attackingCreatureIndex];
            int attackerScreenX = (attacker->position.x - viewport.x) * TILE_SIZE;
            int attackerScreenY = (attacker->position.y - viewport.y) * TILE_SIZE;
            attackerX = attackerScreenX + (TILE_SIZE / 2.0f);
            attackerY = attackerScreenY + (TILE_SIZE / 2.0f);
            
            // For creature attacks, determine target based on damage timers
            if (rtbsState->playerDamageTimer > 0) {
                // Player is the target
                targetX = playerScreenX + (TILE_SIZE / 2.0f);
                targetY = playerScreenY + (TILE_SIZE / 2.0f);
            } else {
                // Check if any follower is the target
                bool foundTarget = false;
                for (int i = 0; i < followerCount; i++) {
                    if (rtbsState->followerDamageTimer[i] > 0) {
                        int followerScreenX = (followers[i].creature->x - viewport.x) * TILE_SIZE;
                        int followerScreenY = (followers[i].creature->y - viewport.y) * TILE_SIZE;
                        targetX = followerScreenX + (TILE_SIZE / 2.0f);
                        targetY = followerScreenY + (TILE_SIZE / 2.0f);
                        foundTarget = true;
                        break;
                    }
                }
                
                // Default to player if no target found
                if (!foundTarget) {
                    targetX = playerScreenX + (TILE_SIZE / 2.0f);
                    targetY = playerScreenY + (TILE_SIZE / 2.0f);
                }
            }
        } else if (rtbsState->attackingCreatureIndex == -2) {
            // NEW CODE: Follower is attacking (special case -2)
            int followerIndex = rtbsState->attackingFollowerIndex;
            
            if (followerIndex >= 0 && followerIndex < followerCount) {
                // Get follower position
                int followerScreenX = (followers[followerIndex].creature->x - viewport.x) * TILE_SIZE;
                int followerScreenY = (followers[followerIndex].creature->y - viewport.y) * TILE_SIZE;
                attackerX = followerScreenX + (TILE_SIZE / 2.0f);
                attackerY = followerScreenY + (TILE_SIZE / 2.0f);
                
                // Get target creature position
                if (rtbsState->attackAnimationTargetCreature >= 0) {
                    OverworldCreature* target = &rtbsState->creatures[rtbsState->attackAnimationTargetCreature];
                    int targetScreenX = (target->position.x - viewport.x) * TILE_SIZE;
                    int targetScreenY = (target->position.y - viewport.y) * TILE_SIZE;
                    targetX = targetScreenX + (TILE_SIZE / 2.0f);
                    targetY = targetScreenY + (TILE_SIZE / 2.0f);
                } else {
                    // Fallback if no valid target (shouldn't happen)
                    targetX = attackerX;
                    targetY = attackerY;
                }
            } else {
                // Fallback if follower index is invalid
                attackerX = playerScreenX + (TILE_SIZE / 2.0f);
                attackerY = playerScreenY + (TILE_SIZE / 2.0f);
                targetX = attackerX;
                targetY = attackerY;
            }
        } else {
            // Player is attacking (original -1 case)
            attackerX = playerScreenX + (TILE_SIZE / 2.0f);
            attackerY = playerScreenY + (TILE_SIZE / 2.0f);
            
            // Get target creature position
            if (rtbsState->attackAnimationTargetCreature >= 0) {
                OverworldCreature* target = &rtbsState->creatures[rtbsState->attackAnimationTargetCreature];
                int targetScreenX = (target->position.x - viewport.x) * TILE_SIZE;
                int targetScreenY = (target->position.y - viewport.y) * TILE_SIZE;
                targetX = targetScreenX + (TILE_SIZE / 2.0f);
                targetY = targetScreenY + (TILE_SIZE / 2.0f);
            } else {
                // Default to player position if no valid target
                targetX = attackerX;
                targetY = attackerY;
            }
        }
        
        // Calculate animation position (midpoint)
        float animX = (attackerX + targetX) / 2.0f;
        float animY = (attackerY + targetY) / 2.0f;
        
        // Draw the animation
        if (animations.attack.currentFrame < animations.attack.frameCount) {
            Texture2D animTexture = animations.attack.frames[animations.attack.currentFrame];
            if (animTexture.id != 0) {
                float scaleAttack = 1.25;
                DrawTextureEx(animTexture, 
                            (Vector2){animX - (scaleAttack*animTexture.width)/2, 
                                    animY - (scaleAttack*animTexture.height)/2}, 
                            0.0f, scaleAttack, WHITE);
            }
        }
    }

    // Draw player engagement radius
    float radiusInPixels = rtbsState->engagementRadius * TILE_SIZE;
    float playerCenterX = playerScreenX + (TILE_SIZE / 2.0f);
    float playerCenterY = playerScreenY + (TILE_SIZE / 2.0f);
    DrawCircleLines(playerCenterX, playerCenterY, radiusInPixels, Fade(RED, 0.6f));

    // Draw followers' engagement radius (to show the extended range)
    for (int i = 0; i < followerCount; i++) {
        
        if (followers[i].creature->currentHP <= 0) continue;

        int followerScreenX = (followers[i].creature->x - viewport.x) * TILE_SIZE;
        int followerScreenY = (followers[i].creature->y - viewport.y) * TILE_SIZE;
        
        float followerCenterX = followerScreenX + (TILE_SIZE / 2.0f);
        float followerCenterY = followerScreenY + (TILE_SIZE / 2.0f);
        
        DrawCircleLines(followerCenterX, followerCenterY, radiusInPixels, Fade(BLUE, 0.4f));

        if (rtbsState->followerDamageTimer[i] > 0) {
            int followerScreenX = (followers[i].creature->x - viewport.x) * TILE_SIZE;
            int followerScreenY = (followers[i].creature->y - viewport.y) * TILE_SIZE;
            
            // Calculate alpha based on remaining time
            float alpha = (float)rtbsState->followerDamageTimer[i] / DAMAGE_TIMER;
            
            // Position for damage text
            float damageX = followerScreenX + rtbsState->followerDamageRandomX[i];
            float damageY = followerScreenY + rtbsState->followerDamageOffsetY[i];

            // Choose color based on effect type
            Color damageColor = (rtbsState->followerDamageEffectType[i] == EFFECT_OFFENSIVE) ? Fade(WHITE, alpha) : Fade(GREEN, alpha);
            
            // Draw the damage number
            char damageText[16];
            sprintf(damageText, "%d", rtbsState->followerDamageDisplay[i]);
            int textWidth = MeasureText(damageText, 20);
            DrawTextWithOutline(damageText, damageX - textWidth/2, damageY, 20, damageColor, Fade(BLACK, alpha));
        }

        // Draw small HP/MP bars above follower
        if (followers[i].creature->name[0] != '\0') {
            int barWidth = 40;
            int barHeight = 5;
            int barY = followerScreenY - 15;

            DrawText(followers[i].creature->name, followerScreenX + (int)barWidth/2 - (int)MeasureText(followers[i].creature->name , 12) / 2, followerScreenY - 30, 12, WHITE);
            
            // Small dark background
            DrawRectangle(followerScreenX - (int)barHeight/2 , barY - 2, barWidth + barHeight - 1, barHeight + 11, Fade(BLACK, 0.7f));
            
            // HP bar (red)
            DrawStatBar(followerScreenX, barY, barWidth, barHeight, followers[i].creature->currentHP, followers[i].creature->maxHP, DARKGRAY, RED, false); // No text
            DrawRectangleLines(followerScreenX, barY, barWidth, barHeight, WHITE);
            
            // MP bar (blue)
            DrawStatBar(followerScreenX, barY + barHeight + 2, barWidth, barHeight, followers[i].creature->currentMP, followers[i].creature->maxMP, DARKGRAY, BLUE, false); // No text
            DrawRectangleLines(followerScreenX, barY + barHeight + 2, barWidth , barHeight, WHITE);
        }
    }

    // Draw small HP/MP bars above player
    if (mainTeam[0].name[0] != '\0') {
        int barWidth = 40;
        int barHeight = 5;
        // int barY = playerScreenY - 20;
        int barY = playerScreenY - 15;
        
        // Small dark background
        // DrawRectangle(playerScreenX - 5, barY - 5, barWidth + 10, 15, Fade(BLACK, 0.7f));
        DrawText(mainTeam[0].name, playerScreenX + (int)barWidth/2 - (int)MeasureText(mainTeam[0].name , 12) / 2, playerScreenY - 30, 12, WHITE);
        
        DrawRectangle(playerScreenX - (int)barHeight/2, barY - 2, barWidth + barHeight - 1, barHeight + 11, Fade(BLACK, 0.7f));
        
        // HP bar (red)
        DrawStatBar(playerScreenX, barY, barWidth, barHeight, mainTeam[0].currentHP, mainTeam[0].maxHP, DARKGRAY, RED, false); // No text
        DrawRectangleLines(playerScreenX, barY, barWidth, barHeight, WHITE);
        
        // MP bar (blue)
        DrawStatBar(playerScreenX, barY + barHeight + 2, barWidth, barHeight, mainTeam[0].currentMP, mainTeam[0].maxMP, DARKGRAY, BLUE, false); // No text
        DrawRectangleLines(playerScreenX, barY + barHeight + 2, barWidth , barHeight, WHITE);
    }
    

    // Draw combo text above player (between BeginMode2D and EndMode2D)
    if (rtbsState->comboCounter > 0) {
        // Convert player world position to screen coordinates
        int playerScreenX = (mainTeam[0].x - viewport.x) * TILE_SIZE;
        int playerScreenY = (mainTeam[0].y - viewport.y) * TILE_SIZE;
        
        // Position above player with bounce effect
        float bounce = sinf(GetTime() * 8.0f) * 5.0f;
        
        // Combo counter
        char comboText[50];
        sprintf(comboText, "COMBO x%d", rtbsState->comboCounter);
        int textWidth = MeasureText(comboText, 20);
        
        // Draw combo text with outline
        DrawTextWithOutline( comboText, playerScreenX + TILE_SIZE/2 - textWidth/2, playerScreenY - 50 + (int)bounce, 20, GOLD, BLACK);
        
        // Draw bonus indicators (smaller text below combo counter)
        char bonusText[50];
        sprintf(bonusText, "DMG x%.1f", rtbsState->comboMultiplier);
        int bonusWidth = MeasureText(bonusText, 14);
        
        // Draw bonus text with outline
        DrawTextWithOutline( bonusText,  playerScreenX + TILE_SIZE/2 - bonusWidth/2, playerScreenY - 60 + (int)bounce, 14, ORANGE, BLACK );
    }

    // Draw player damage display if active
    // this damage only affects the player not the enemies!
    if (rtbsState->playerDamageTimer > 0) {
        // Calculate alpha based on remaining time
        float alpha = (float)rtbsState->playerDamageTimer / DAMAGE_TIMER;
        
        // Position for damage text (above player, moving upward)
        // float damageX = playerScreenX + TILE_SIZE/2;
        // float damageX = playerScreenX + RANDOMS(0,TILE_SIZE);
        // CHECK WITH AI is it possible to set up a random and use it without it variating?
        float damageX = playerScreenX + rtbsState->playerDamageRandomX;
        float damageY = playerScreenY + rtbsState->playerDamageOffsetY;
        
        // Choose color based on effect type
        Color damageColor = (rtbsState->playerDamageEffectType == EFFECT_OFFENSIVE) ? Fade(WHITE, alpha) : Fade(GREEN, alpha);
        
        // Draw the damage number
        char damageText[16];
        sprintf(damageText, "%d", rtbsState->playerDamageDisplay);
        int textWidth = MeasureText(damageText, 20);
        DrawTextWithOutline(damageText, damageX - textWidth/2, damageY, 20, damageColor, Fade(BLACK, alpha));

    }

    // Standard UI elements
    EndMode2D();
    

    // Draw command energy bar at bottom of screen
    // int energyBarX = 20;
    // int energyBarY = SCREEN_HEIGHT - 30;
    // int energyBarWidth = 200;
    // int energyBarHeight = 20;
    
    // Draw background
    // DrawRectangle(energyBarX - 5, energyBarY - 5, energyBarWidth + 10, energyBarHeight + 10, Fade(BLACK, 0.7f));
    
    // Draw label
    // DrawText("COMMAND", energyBarX, energyBarY - 22, 16, YELLOW);
    
    // Draw energy bar
    // float energyRatio = rtbsState->commandEnergy / rtbsState->maxCommandEnergy;
    // int filledWidth = (int)(energyBarWidth * energyRatio);
    
    // // Draw background for whole bar
    // DrawRectangle(energyBarX, energyBarY, energyBarWidth, energyBarHeight, DARKGRAY);
    
    // // Draw filled portion
    // Color energyColor = rtbsState->canUseCommand ? GOLD : ORANGE;
    // DrawRectangle(energyBarX, energyBarY, filledWidth, energyBarHeight, energyColor);
    
    // For recently used energy, show drain animation
    // if (rtbsState->showEnergyDrain) {
    //     float prevEnergyRatio = rtbsState->lastEnergyLevel / rtbsState->maxCommandEnergy;
    //     int prevFilledWidth = (int)(energyBarWidth * prevEnergyRatio);
        
    //     float animProgress = 1.0f - (rtbsState->energyDrainTimer / 0.5f);
        
    //     DrawRectangle(
    //         energyBarX + filledWidth,
    //         energyBarY,
    //         prevFilledWidth - filledWidth,
    //         energyBarHeight,
    //         Fade(RED, 1.0f - animProgress)
    //     );
    // }
    
    // Draw border
    // DrawRectangleLines(energyBarX, energyBarY, energyBarWidth, energyBarHeight, WHITE);
    
    // Draw "CLAW MODE" indicator
    DrawRectangle(10, 10, 170, 25, Fade(BLACK, 0.7f));
    
    // Change prompt based on whether commands are available
    if (rtbsState->canUseCommand) {
        DrawText("CLAW MODE - Press Z", 15, 15, 16, RED);
    } else {
        DrawText("CLAW MODE - Charging...", 15, 15, 16, ORANGE);
    }

    // Add to your DrawRealTimeBattle function to better visualize selected targets
    if (rtbsState->playerAttackMode) {
        // Draw a message at the top of the screen
        DrawRectangle(0, 0, SCREEN_WIDTH, 40, Fade(BLACK, 0.7f));
        char selectedMsg[100];
        if (rtbsState->selectedCreatureIndex >= 0) {
            sprintf(selectedMsg, "Targeting: %s - Press Z to confirm, X to cancel", 
                    rtbsState->creatures[rtbsState->selectedCreatureIndex].creature.name);
        } else {
            sprintf(selectedMsg, "No target selected - Press X to cancel");
        }
        DrawText(selectedMsg, 10, 10, 20, WHITE);
        
        // Highlight all valid targets
        for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
            if (rtbsState->creatures[i].active && IsCreatureInPartyRange(rtbsState, i)) {
                // Calculate screen position
                int creatureScreenX = (rtbsState->creatures[i].position.x - viewport.x) * TILE_SIZE;
                int creatureScreenY = (rtbsState->creatures[i].position.y - viewport.y) * TILE_SIZE;
                
                // Draw a targeting outline
                Color outlineColor = (i == rtbsState->selectedCreatureIndex) ? YELLOW : GREEN;
                DrawCircleLines(creatureScreenX + TILE_SIZE/2, creatureScreenY + TILE_SIZE/2, 
                            TILE_SIZE * 0.7f, outlineColor);
            }
        }
    }

    // In DrawRealTimeBattle, add this to your UI elements
    // Show current AI behavior
    int aiIndicatorX = 20;
    int aiIndicatorY = 50; // Below your command energy bar
    DrawRectangle(aiIndicatorX - 5, aiIndicatorY - 5, 170, 25, Fade(BLACK, 0.7f));

    const char* modeText = (currentFollowerBehavior == FOLLOWER_BEHAVIOR_AGGRESSIVE) ? 
                        "FOLLOWERS: AGGRESSIVE" : "FOLLOWERS: FOLLOW";
    Color modeColor = (currentFollowerBehavior == FOLLOWER_BEHAVIOR_AGGRESSIVE) ? RED : BLUE;
    DrawText(modeText, aiIndicatorX, aiIndicatorY, 16, modeColor);
    
    // Standard UI elements
    DrawKeyRepresentation(keysPressed);
    DrawToolWheelIfActive();
    DrawOverworldMessages();
}

void UpdateRealTimeBattle(RealTimeBattleState* rtbsState) {
    // Basic input handling
    bool APressed = IsKeyPressed(KEY_Z);
    bool LPressed = IsKeyPressed(KEY_A);
    bool RPressed = IsKeyPressed(KEY_S);
    float deltaTime = GetFrameTime();
    
    // Time slowdown logic for command menu transition
    static float timeSlowdownFactor = 1.0f;
    if (currentGameState == GAME_STATE_COMMAND_MENU) {
        timeSlowdownFactor = approach(timeSlowdownFactor, 0.1f, 0.05f);
    } else {
        timeSlowdownFactor = approach(timeSlowdownFactor, 1.0f, 0.05f);
    }
    
    // Apply slowdown to movement speed
    moveIncrement *= timeSlowdownFactor;
    UpdateRealTimeBattleMovement(rtbsState);
    moveIncrement /= timeSlowdownFactor; // Restore original increment
    
    // Update animations and followers
    UpdatePlayerAnimation(&mainTeam[0], &frameTracking);
    UpdateFollowers(&mainTeam[0], &frameTracking);
    UpdateCommandQueue(rtbsState);
    
    // Update command energy system
    if (rtbsState->commandEnergy < rtbsState->maxCommandEnergy) {
        rtbsState->commandEnergy += rtbsState->rechargeRate * deltaTime;
        if (rtbsState->commandEnergy > rtbsState->maxCommandEnergy) {
            rtbsState->commandEnergy = rtbsState->maxCommandEnergy;
        }
        UpdateCommandAvailability();
    }
    
    // Update energy drain animation
    if (rtbsState->showEnergyDrain) {
        rtbsState->energyDrainTimer -= deltaTime;
        if (rtbsState->energyDrainTimer <= 0.0f) {
            rtbsState->showEnergyDrain = false;
        }
    }

    // Handle creature spawning
    if (IsKeyPressed(KEY_F)) { // Debug spawn
        TerrainType currentTerrain = worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].type;
        SpawnOverworldCreature(rtbsState, currentTerrain);
    }
    
    // Update spawn timer
    if (rtbsState->creatureCount == 0) {
        rtbsState->spawnTimer -= deltaTime * 2.0f; // Faster timer when no creatures
    } else {
        rtbsState->spawnTimer -= deltaTime;
    }
    
    // Check for random creature spawns
    if (rtbsState->spawnTimer <= 0) {
        RandomEncounterRTBS(rtbsState);
    }
    
    // Update existing creatures
    UpdateOverworldCreatures(rtbsState);
    
    // Standard overworld updates
    if (UpdateBombAnimation()) { return; }
    UpdateTerrainAnimations();
    UpdateToolWheel(LPressed, RPressed);

    // Add this first before any other debug prints
    if (APressed && !rtbsState->waitingForKeyRelease) {
        printf("DEBUG: Z key pressed. Initial state - playerAttackMode=%d, targetSelectionMode=%d, canUseCommand=%d\n",
                rtbsState->playerAttackMode, commandMenuState.targetSelectionMode, rtbsState->canUseCommand);
        
        if (rtbsState->canUseCommand && !rtbsState->playerAttackMode) {
            printf("DEBUG: Before transition: playerAttackMode=%d, targetSelectionMode=%d\n",
                    rtbsState->playerAttackMode, commandMenuState.targetSelectionMode);
                    
            printf("DEBUG: Command menu activated. targetSelectionMode before: %d\n",
                    commandMenuState.targetSelectionMode);
            
            // Open command menu
            // FULLY initialize command menu state
            commandMenuState.previousState = currentGameState;
            commandMenuState.justEnteredMenu = true;
            commandMenuState.targetSelectionMode = false;
            commandMenuState.inAttackSubmenu = false;  // Initialize new flag
            commandMenuState.selectedIndex = 0;
            commandMenuState.actingParticipant = &battleState.participants[0];
            commandMenuState.inAISubmenu = false;
            commandMenuState.waitingForKeyRelease = true;
            commandMenuState.wasXPressed = false;
            commandMenuState.xHoldTime = 0.0f;
            commandMenuState.xLongPressExecuted = false;
            
            currentGameState = GAME_STATE_COMMAND_MENU;
            rtbsState->waitingForKeyRelease = true;
            printf("DEBUG: waitingForKeyRelease set to: %d\n", rtbsState->waitingForKeyRelease);
        }
    }

    // Reset key release flag when Z is released
    if (!APressed && rtbsState->waitingForKeyRelease) {
        rtbsState->waitingForKeyRelease = false;
    }
    
    // Handle return from command menu - reset previous state
    if (currentGameState == GAME_STATE_REALTIME_BATTLE && 
        previousGameState == GAME_STATE_COMMAND_MENU) {
        previousGameState = GAME_STATE_REALTIME_BATTLE;
    }

    // Update attack animation
    if (animations.attack.isAnimating) {
        animations.attack.frameCounter++;
        
        if (animations.attack.frameCounter >= animations.attack.frameSpeed) {
            animations.attack.frameCounter = 0;
            animations.attack.currentFrame++;
            
            if (animations.attack.currentFrame >= animations.attack.frameCount) {
                animations.attack.isAnimating = false;
                animations.attack.currentFrame = 0;
                rtbsState->attackAnimationActive = false;
                
                // Replace multiple flag setting with:
                DisableTargeting(&commandMenuState, rtbsState);

                // Clear attacking state for all creatures
                for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
                    if (rtbsState->creatures[i].active) {
                        rtbsState->creatures[i].isAttacking = false;
                    }
                }
            }
        }
    }

    // Check for game over condition
    if (mainTeam[0].currentHP <= 0) {
        mainTeam[0].currentHP = 0;
        currentGameState = GAME_STATE_GAME_OVER;
        PlaySoundByName(&soundManager, "gameOver", false);
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        currentGameState = GAME_STATE_MENU;
        menu.currentMenu = MENU_MAIN;
        return;
    }

    // Debug information (only when TAB key is pressed)
    if (IsKeyPressed(KEY_TAB)) {
        printf("\n--- DEBUG STATE INFO ---\n");
        printf("Game State: %d\n", currentGameState);
        printf("Previous State: %d\n", previousGameState);
        printf("Command Energy: %.1f / %.1f\n", rtbsState->commandEnergy, rtbsState->maxCommandEnergy);
        printf("Can Use Command: %d\n", rtbsState->canUseCommand);
        printf("Player Attack Mode: %d\n", rtbsState->playerAttackMode);
        printf("Selected Creature: %d\n", rtbsState->selectedCreatureIndex);
        printf("Active Creatures: %d\n", rtbsState->creatureCount);
        
        // Print active creature info
        for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
            if (rtbsState->creatures[i].active) {
                OverworldCreature* c = &rtbsState->creatures[i];
                printf("- Creature #%d: %s, HP: %d/%d, Pos: (%.1f,%.1f), InRange: %d\n",
                    i, c->creature.name, c->creature.currentHP, c->creature.maxHP,
                    c->position.x, c->position.y, IsCreatureInPartyRange(rtbsState, i));
            }
        }
        printf("------------------------\n");
    }
}


void SpawnOverworldCreature(RealTimeBattleState* rtbsState, TerrainType terrain) {
    printf("Attempting to spawn creature...\n");
    Tool* selectedTool = &toolWheel.tools[toolWheel.selectedToolIndex];
    
    // Find an empty slot in the creatures array
    int slotIndex = -1;
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (!rtbsState->creatures[i].active) {
            slotIndex = i;
            break;
        }
    }
    
    if (slotIndex == -1) {
        printf("No free slots for creatures!\n");
        return; // No free slots
    }
    
    // Find a suitable spawn location VERY close to the player for testing
    float minDistance = 2.0f;  // Much closer to player (was 5.0f)
    float maxDistance = 4.0f;  // Much closer to player (was 10.0f)
    
    // Try multiple positions until we find a valid one
    Vector2 spawnPos = {0, 0};
    bool validPosition = false;
    int attempts = 0;
    
    while (!validPosition && attempts < 20) {
        attempts++;
        
        // Instead of spawning in a circle around the player, spawn within the visible viewport
        // Get current viewport boundaries
        float viewportLeft = mainTeam[0].x - (SCREEN_WIDTH / 2.0f / TILE_SIZE);
        float viewportRight = mainTeam[0].x + (SCREEN_WIDTH / 2.0f / TILE_SIZE);
        float viewportTop = mainTeam[0].y - (SCREEN_HEIGHT / 2.0f / TILE_SIZE);
        float viewportBottom = mainTeam[0].y + (SCREEN_HEIGHT / 2.0f / TILE_SIZE);
        
        // Adjust to ensure spawn is visible but not too close to player
        viewportLeft = fmax(viewportLeft + 2, 0);
        viewportRight = fmin(viewportRight - 2, mapWidth - 1);
        viewportTop = fmax(viewportTop + 2, 0);
        viewportBottom = fmin(viewportBottom - 2, mapHeight - 1);
        
        // Randomly select a position within the viewport
        spawnPos.x = viewportLeft + ((float)RANDOMS(0, 100) / 100.0f) * (viewportRight - viewportLeft);
        spawnPos.y = viewportTop + ((float)RANDOMS(0, 100) / 100.0f) * (viewportBottom - viewportTop);
        
        // Ensure position is integer coordinates for terrain check
        int tileX = (int)spawnPos.x;
        int tileY = (int)spawnPos.y;
        
        // Ensure within map boundaries
        if (tileX < 0) tileX = 0;
        if (tileY < 0) tileY = 0;
        if (tileX >= mapWidth) tileX = mapWidth - 1;
        if (tileY >= mapHeight) tileY = mapHeight - 1;
        
        // Get the actual terrain at this position
        TerrainType tileTerrain = worldMap[tileY][tileX].type;
        
        // Check if position is walkable and has a valid terrain type
        if (IsTileWalkable(tileX, tileY, selectedTool) && 
            (tileTerrain == TERRAIN_EMPTY || tileTerrain == TERRAIN_GRASS || 
            tileTerrain == TERRAIN_GROUND || tileTerrain == TERRAIN_WATER || 
            tileTerrain == TERRAIN_MOUNTAIN)) {
            
            // Set spawn position to exact tile center
            spawnPos.x = (float)tileX;
            spawnPos.y = (float)tileY;
            validPosition = true;
            
            // Save the actual terrain for creature selection
            terrain = tileTerrain;
            printf("Found valid spawn position at (%d,%d) with terrain: %c\n", 
                tileX, tileY, TerrainToSymbol(terrain));
            break;
        }
    }

    if (!validPosition) {
        // If all attempts failed, spawn closer to the player on a valid terrain
        int playerTileX = (int)mainTeam[0].x;
        int playerTileY = (int)mainTeam[0].y;
        
        // Try positions immediately adjacent to player
        int offsets[4][2] = {{1,0}, {0,1}, {-1,0}, {0,-1}};
        
        for (int i = 0; i < 4; i++) {
            int tileX = playerTileX + offsets[i][0];
            int tileY = playerTileY + offsets[i][1];
            
            // Check bounds
            if (tileX < 0 || tileX >= mapWidth || tileY < 0 || tileY >= mapHeight) {
                continue;
            }
            
            // Check if walkable
            if (IsTileWalkable(tileX, tileY, selectedTool)) {
                spawnPos.x = (float)tileX;
                spawnPos.y = (float)tileY;
                terrain = worldMap[tileY][tileX].type;
                validPosition = true;
                printf("Using fallback spawn position at (%d,%d) with terrain: %c\n", 
                    tileX, tileY, TerrainToSymbol(terrain));
                break;
            }
        }
    }

    // Ensure we have a valid position
    if (!validPosition) {
        printf("Failed to find a valid spawn position after all attempts!\n");
        return;
    }

    printf("Final spawn position: x=%.2f, y=%.2f in terrain %c\n", 
        spawnPos.x, spawnPos.y, TerrainToSymbol(terrain));

    // Find a creature that can appear on this terrain
    int validCreatures[MAX_ENEMY_TYPES];
    int validCount = 0;

    // First try to find creatures that match this terrain
    for (int i = 0; i < enemyDatabase.enemyCount; i++) {
        // Check if this creature can appear on our terrain
        for (int j = 0; j < enemyDatabase.creatures[i].terrainCount; j++) {
            if (enemyDatabase.creatures[i].terrains[j] == terrain) {
                // Also check that it has a valid sprite for this terrain
                if (enemyDatabase.creatures[i].sprites[terrain][PERFECT].id != 0) {
                    validCreatures[validCount++] = i;
                    break;
                }
            }
        }
    }

    // If no terrain-specific creatures found, fall back to any creature
    if (validCount == 0) {
        printf("No terrain-specific creatures found for terrain %c, using any creature.\n", 
            TerrainToSymbol(terrain));
        for (int i = 0; i < enemyDatabase.enemyCount; i++) {
            // Check if it has ANY valid sprite
            bool hasSprite = false;
            for (int t = 0; t < NUM_TERRAINS; t++) {
                if (enemyDatabase.creatures[i].sprites[t][PERFECT].id != 0) {
                    hasSprite = true;
                    break;
                }
            }
            
            if (hasSprite) {
                validCreatures[validCount++] = i;
            }
        }
    }

    if (validCount == 0) {
        printf("No valid creatures found with sprites!\n");
        return;
    }

    // Select a random creature from valid options
    int creatureIndex = validCreatures[RANDOMS(0, validCount - 1)];
    printf("Selected creature %s (index %d) from %d valid options\n", 
        enemyDatabase.creatures[creatureIndex].name, creatureIndex, validCount);

    // Initialize the overworld creature
    OverworldCreature* newCreature = &rtbsState->creatures[slotIndex];
    
    // First clear the creature memory before copying
    memset(newCreature, 0, sizeof(OverworldCreature));
    
    CopyCreature(&newCreature->creature, &enemyDatabase.creatures[creatureIndex]);
    
    // Make sure animation frames are properly loaded for the creature
    // LoadCreatureAnimations(&newCreature->creature, FOLDER_FOLLOWERS);
    
    newCreature->position = spawnPos;
    newCreature->active = true;
    newCreature->engagementRadius = 3.0f;         // Increased radius for testing
    newCreature->lifespan = 60.0f;               // Double lifespan (was 30.0f)
    newCreature->moveTimer = 0.0f;
    newCreature->moveDirection = (Vector2){ 0.0f, 0.0f };
    newCreature->withinPlayerRange = false;
    newCreature->despawnTimer = 30.0f;           // Triple despawn timer (was 10.0f)

        // Initialize attack-related fields
    newCreature->attackCooldown = 3.0f + ((float)RANDOMS(0, 200) / 100.0f); // Random initial cooldown (3-5 sec)
    newCreature->isAttacking = false;  // Use isAttacking instead of attackAnimationActive
    newCreature->targetPlayerDamage = 0;

        // Assuming the base Creature struct has CreatureAnimations called 'anims'
    newCreature->creature.anims.movementState = WALKING;
    newCreature->creature.anims.direction = DOWN; // Default direction
    newCreature->creature.anims.isMoving = false;
    newCreature->creature.anims.isStopping = false;

        // Reset all direction press counters to avoid any unexpected movement
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        newCreature->creature.anims.directionPressCounter[i] = 0;
    }


    // Initialize its frame tracker
    newCreature->animTracking.currentFrame = 0;
    newCreature->animTracking.frameCounter = 0;
    // *** You'll need to define default idle/walk speeds, maybe load from creature data ***
    newCreature->animTracking.idleFrameSpeed = 15; // Example
    newCreature->animTracking.walkFrameSpeed = 8;  // Example
    newCreature->animTracking.stopDelayCounter = 0; // Not strictly needed for simple AI yet

    
    // Increment creature count
    rtbsState->creatureCount++;
    
    // Play sound effect
    PlaySoundByName(&soundManager, "encounter", false);
    
    // Show message
    char message[100];
    sprintf(message, "A wild %s appeared at (%.1f, %.1f)!", 
           newCreature->creature.name, spawnPos.x, spawnPos.y);
    SetOverworldMessage(message);
    
    printf("Successfully spawned creature: %s\n", newCreature->creature.name);
}

// Modified InitializeRealTimeBattle function for debugging
void InitializeRealTimeBattle() {
    printf("Initializing Real-Time Battle System...\n");
    
    realTimeBattleState.isActive = false;
    realTimeBattleState.engagementRadius = DEFAULT_ENGAGEMENT_RADIUS;
    
    // Initialize command energy system
    realTimeBattleState.commandEnergy = 4.0f;
    realTimeBattleState.maxCommandEnergy = 4.0f;
    realTimeBattleState.rechargeRate = 0.5f;
    realTimeBattleState.canUseCommand = true;
    
    realTimeBattleState.showEnergyDrain = false;
    realTimeBattleState.energyDrainTimer = 0.0f;
    realTimeBattleState.lastEnergyLevel = 4.0f;
    
    // Initialize creature system
    realTimeBattleState.creatureCount = 0;
    realTimeBattleState.spawnTimer = 2.0f;      // Initial spawn delay (reduced for testing)
    realTimeBattleState.spawnInterval = 5.0f;   // Check for spawns every 5 seconds (reduced for testing)
    
    // Initialize damage display
    realTimeBattleState.playerDamageDisplay = 0;
    realTimeBattleState.playerDamageTimer = 0;
    realTimeBattleState.playerDamageOffsetY = 0.0f;
    realTimeBattleState.playerDamageRandomX = 0;
    realTimeBattleState.playerDamageEffectType = EFFECT_OFFENSIVE;
    
    // Initialize attack animation
    // realTimeBattleState.attackAnimation = animations.attack;
    // realTimeBattleState.attackAnimation.isAnimating = false;
    // realTimeBattleState.attackAnimation.currentFrame = 0;
        // Initialize attack animation tracking
    realTimeBattleState.attackAnimationActive = false;
    realTimeBattleState.attackAnimationTargetCreature = -1;
    
    // Initialize screen shake
    realTimeBattleState.shakeTimer = 0;

    realTimeBattleState.waitingForKeyRelease = false;
    
    
    // Initialize follower damage display
    for (int i = 0; i < MAX_FOLLOWERS; i++) {
        realTimeBattleState.followerDamageDisplay[i] = 0;
        realTimeBattleState.followerDamageTimer[i] = 0;
        realTimeBattleState.followerDamageOffsetY[i] = 0.0f;
        realTimeBattleState.followerDamageEffectType[i] = EFFECT_OFFENSIVE;
        realTimeBattleState.followerDamageRandomX[i] = 0;
    }

    // Initialize creature damage display
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        realTimeBattleState.creatureDamageDisplay[i] = 0;
        realTimeBattleState.creatureDamageTimer[i] = 0;
        realTimeBattleState.creatureDamageOffsetY[i] = 0.0f;
        realTimeBattleState.creatureDamageEffectType[i] = EFFECT_OFFENSIVE;
        realTimeBattleState.creatureDamageRandomX[i] = 0;
    }

    for (int i = 0; i < APPROACH_COUNT; i++) {
        realTimeBattleState.usedDirections[i] = false;
    }

    realTimeBattleState.lastApproachDirection = APPROACH_COUNT; // Initialize last direction


    // Initialize player attack mode
    realTimeBattleState.playerAttackMode = false;
    realTimeBattleState.selectedCreatureIndex = -1;

    // Initialize command queue execution variables
    realTimeBattleState.isExecutingCommandQueue = false;
    realTimeBattleState.currentQueueIndex = 0;
    realTimeBattleState.commandExecutionTimer = 0.0f;
    realTimeBattleState.commandCooldown = 0.8f; // Default time between attacks

    // Set MAX_RTBS_CREATURES definition if not already defined
    #ifndef MAX_RTBS_CREATURES
    #define MAX_RTBS_CREATURES 5
    #endif
    
    // Clear creature array
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        realTimeBattleState.creatures[i].active = false;
    }

    realTimeBattleState.attackingCreatureIndex = -1;  // No creature attacking initially
    realTimeBattleState.attackingFollowerIndex = -1;  // No follower attacking initially
    
    printf("RTBS Initialization complete!\n");
}

// Helper function to consume command energy
void ConsumeCommandEnergy(float amount) {
    printf("Consuming %.1f energy\n", amount);
    
    realTimeBattleState.commandEnergy -= amount;
    if (realTimeBattleState.commandEnergy < 0.0f) {
        realTimeBattleState.commandEnergy = 0.0f;
    }
    
    // Set visual feedback flags
    realTimeBattleState.showEnergyDrain = true;
    realTimeBattleState.energyDrainTimer = 0.5f; // Show drain animation for 0.5 seconds
    realTimeBattleState.lastEnergyLevel = realTimeBattleState.commandEnergy + amount;
    
    // Update whether we can use commands
    UpdateCommandAvailability();
}


// Helper function to set up a battle participant from an overworld creature
void SetupBattleParticipant(OverworldCreature* creature) {
    // Find an empty slot in the battle participants
    int slotIndex = -1;
    for (int i = 1; i < MAX_PARTICIPANTS; i++) { // Start at 1 to keep player at index 0
        if (!battleState.participants[i].isActive) {
            slotIndex = i;
            break;
        }
    }
    
    if (slotIndex == -1) {
        printf("No empty battle participant slot found!\n");
        return;
    }
    
    // Set up the battle participant
    BattleParticipant* participant = &battleState.participants[slotIndex];
    participant->creature = &creature->creature;
    participant->isPlayer = false;
    participant->isActive = true;
    participant->pendingDamage = 0;
    participant->damageTimer = 0;
    participant->damageOffsetY = 0.0f;
    participant->deathTimer = 0;
    participant->deathActivated = false;
    
    // Ensure player is set up as the first participant
    if (!battleState.participants[0].isActive) {
        battleState.participants[0].creature = &mainTeam[0];
        battleState.participants[0].isPlayer = true;
        battleState.participants[0].isActive = true;
    }
    
    // Update participant count
    battleState.participantCount = 2; // Player + this creature
    
    // Set command menu's acting participant
    commandMenuState.actingParticipant = &battleState.participants[0];
    
    printf("Battle participant set up for creature: %s\n", creature->creature.name);
}

// // Function to handle attack command on an overworld creature
// void AttackOverworldCreature(RealTimeBattleState* rtbsState) {
//     // Find the targeted creature
//     OverworldCreature* targetedCreature = NULL;
//     int targetIndex = -1;
    
//     for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
//         if (rtbsState->creatures[i].active && rtbsState->creatures[i].withinPlayerRange) {
//             targetedCreature = &rtbsState->creatures[i];
//             targetIndex = i;
//             break;
//         }
//     }
    
//     if (targetedCreature == NULL) {
//         SetOverworldMessage("No target in range!");
//         return;
//     }
    
//     // Set up the battle participants
//     BattleParticipant attacker = {0};
//     BattleParticipant defender = {0};
    
//     // Set up attacker (player)
//     attacker.creature = &mainTeam[0];
//     attacker.isPlayer = true;
//     attacker.isActive = true;
    
//     // Set up defender (creature)
//     defender.creature = &targetedCreature->creature;
//     defender.isPlayer = false;
//     defender.isActive = true;
    
//     // Calculate hit chance using the turn battle logic
//     bool didHit = WillItHit(attacker.creature, defender.creature);
    
//     if (!didHit) {
//         SetOverworldMessage("Your attack missed!");
//         return;
//     }
    
//     // Calculate damage using turn battle logic
//     int damage = attacker.creature->attack + attacker.creature->tempAttackBuff
//                - defender.creature->defense - defender.creature->tempDefenseBuff;
//     damage = MAX(damage, 1);  // Ensure at least 1 damage
    
//     // Apply damage to creature
//     targetedCreature->creature.currentHP -= damage;
//     targetedCreature->creature.currentHP = CLAMP(targetedCreature->creature.currentHP, 
//                                               0, targetedCreature->creature.maxHP);
    
//     // Set damage display on creature
//     targetedCreature->targetPlayerDamage = damage;
    
//     // Play attack animation on creature
//     // rtbsState->attackAnimation.isAnimating = true;
//     // rtbsState->attackAnimation.currentFrame = 0;
//     // rtbsState->attackAnimation.frameCounter = 0;
//         // Activate attack animation
//     rtbsState->attackAnimationActive = true;
//     animations.attack.isAnimating = true;
//     animations.attack.currentFrame = 0;
//     animations.attack.frameCounter = 0;
    
//     // Store target for animation positioning
//     rtbsState->attackAnimationTargetCreature = targetIndex;

//     // Apply screen shake
//     rtbsState->shakeTimer = 10;
    
//     // Display damage message
//     char message[100];
//     sprintf(message, "%s takes %d damage!", targetedCreature->creature.name, damage);
//     SetOverworldMessage(message);
    
//     // Play attack sound
//     PlaySoundByName(&soundManager, "attack", false);
    
//     // Check if creature is defeated
//     if (targetedCreature->creature.currentHP <= 0) {
//         // Creature is defeated
//         targetedCreature->creature.currentHP = 0;
        
//         // Calculate exp gain
//         int expGain = targetedCreature->creature.giveExp;
        
//         // Apply exp to player
//         mainTeam[0].exp += expGain;
        
//         // Check for level up (replace with your actual level logic)
//         if (mainTeam[0].exp >= GetExpForNextGridLevel(mainTeam[0].totalGridLevel)) {
//             mainTeam[0].exp -= GetExpForNextGridLevel(mainTeam[0].totalGridLevel);
//             mainTeam[0].gridLevel++;
//             mainTeam[0].totalGridLevel++;
            
//             // Apply stat increases
//             // GiveStatBonus(&mainTeam[0]);
            
//             SetOverworldMessage("Level up!");
//             PlaySoundByName(&soundManager, "levelUp", true);
//         }
        
//         // Remove the creature from the array
//         rtbsState->creatures[targetIndex].active = false;
//         rtbsState->creatureCount--;
        
//         char defeatMessage[100];
//         sprintf(defeatMessage, "%s defeated! Gained %d EXP.", 
//                 targetedCreature->creature.name, expGain);
//         SetOverworldMessage(defeatMessage);
//     }
// }


void UpdateCommandMenu(CommandMenuState *cmdState) {
    // If we just entered the menu, wait for Z key release
    if (cmdState->justEnteredMenu) {
        if (!IsKeyDown(KEY_Z)) {
            cmdState->justEnteredMenu = false;
            printf("DEBUG: Z key released, clearing justEnteredMenu flag\n");
        } else {
            printf("DEBUG: Waiting for Z key release\n");
            return; // Skip all processing until Z is released
        }
    }

    const char *commands[] = {"Attack", "Skill", "Magic", "Item", "Defense", "AI"};
    const int commandCount = sizeof(commands) / sizeof(commands[0]);
    const int columns = 3;
    const int rows = 2;

    // Reset target selection when entering menu from RTBS
    if (cmdState->previousState == GAME_STATE_REALTIME_BATTLE && 
        previousGameState == GAME_STATE_REALTIME_BATTLE) {
        // cmdState->targetSelectionMode = false;
        // cmdState->inAttackSubmenu = false;
        DisableTargeting(cmdState, &realTimeBattleState);
    }
    
    // Define key hold thresholds
    const float LONG_PRESS_THRESHOLD = 0.5f; // 500ms for long press
    float deltaTime = GetFrameTime();
        
    // Handle Z key holds/presses
    if (IsKeyDown(KEY_Z)) {
        if (!realTimeBattleState.isHoldingZ) {
            // Just started holding Z
            realTimeBattleState.isHoldingZ = true;
            realTimeBattleState.keyHoldTimer = 0;
            printf("DEBUG: Z hold started, timer reset\n");
        }
        else {
            // Continue holding Z
            realTimeBattleState.keyHoldTimer += deltaTime;
            printf("DEBUG: Z hold timer: %.2f/%.2f\n", realTimeBattleState.keyHoldTimer, LONG_PRESS_THRESHOLD);
            
            // Check BOTH flags, and make sure we have commands to execute
            if ((cmdState->targetSelectionMode || cmdState->inAttackSubmenu) && 
                realTimeBattleState.keyHoldTimer > LONG_PRESS_THRESHOLD &&
                realTimeBattleState.queueCount > 0) {
                
                printf("DEBUG: EXECUTING COMMANDS! Queue count: %d\n", realTimeBattleState.queueCount);
                ExecuteAvailableCommands(&realTimeBattleState);
                
                // Reset states and return to realtime battle
                // cmdState->targetSelectionMode = false;
                // cmdState->inAttackSubmenu = false;
                // realTimeBattleState.playerAttackMode = false;
                // realTimeBattleState.selectedCreatureIndex = -1;
                DisableTargeting(cmdState, &realTimeBattleState);
                currentGameState = cmdState->previousState;
                cmdState->waitingForKeyRelease = true;
                
                // Feedback message
                SetOverworldMessage("Executing all commands!");
                PlaySoundByName(&soundManager, "menuConfirm", false);
            }
        }
    }
    else {
        // Z key released
        realTimeBattleState.isHoldingZ = false;
        printf("DEBUG: Z key released, timer: %.2f\n", realTimeBattleState.keyHoldTimer);
    }
    
    // Current X button state
    bool isXPressed = IsKeyDown(KEY_X);
    
    // X just pressed this frame
    if (isXPressed && !cmdState->wasXPressed) {
        cmdState->xHoldTime = 0;
        cmdState->xLongPressExecuted = false;
        cmdState->wasXPressed = true;
    } 
    // X is being held
    else if (isXPressed && cmdState->wasXPressed) {
        cmdState->xHoldTime += deltaTime;
        
        // Check for long press threshold - only execute once
        if (cmdState->xHoldTime >= LONG_PRESS_THRESHOLD && !cmdState->xLongPressExecuted &&  realTimeBattleState.queueCount > 0) {
            // Long press detected!
            cmdState->xLongPressExecuted = true;
            
            // Handle long press - execute commands and exit to main game
            // if (cmdState->targetSelectionMode || cmdState->inAttackSubmenu) {
                // Execute all queued commands (add this line!)
            ExecuteAvailableCommands(&realTimeBattleState);
            
            // Reset states and return to realtime battle
            // cmdState->targetSelectionMode = false;
            // cmdState->inAttackSubmenu = false;
            // realTimeBattleState.playerAttackMode = false;
            // realTimeBattleState.selectedCreatureIndex = -1;
            DisableTargeting(cmdState, &realTimeBattleState);
            currentGameState = cmdState->previousState;
            
            SetOverworldMessage("Executing commands and returning to combat!");
            PlaySoundByName(&soundManager, "menuConfirm", false);
            // }
        }
    }
    
    // X was released this frame
    else if (!isXPressed && cmdState->wasXPressed) {
        // Only process short press if long press wasn't already executed
        if (cmdState->xHoldTime < LONG_PRESS_THRESHOLD || !cmdState->xLongPressExecuted) {
            // Then check target selection mode
            if (cmdState->inAttackSubmenu || cmdState->targetSelectionMode) {
                if (realTimeBattleState.queueCount > 0) {
                    // Remove last queued command
                    realTimeBattleState.queueCount--;
                    
                    char message[100];
                    sprintf(message, "Removed last command from queue (%d/%d)", realTimeBattleState.queueCount, MAX_COMMAND_QUEUE);
                    SetOverworldMessage(message);
                    PlaySoundByName(&soundManager, "menuBack", false);
                } else {
                    // No commands to remove, so return to main command menu
                    // cmdState->targetSelectionMode = false;
                    // cmdState->inAttackSubmenu = false;
                    // realTimeBattleState.playerAttackMode = false;
                    // realTimeBattleState.selectedCreatureIndex = -1;
                    DisableTargeting(cmdState, &realTimeBattleState);
                    PlaySoundByName(&soundManager, "menuBack", false);
                    SetOverworldMessage("Target selection canceled");
                }
            }
            // Then check AI submenu
            else if (cmdState->inAISubmenu) {
                cmdState->inAISubmenu = false;
                PlaySoundByName(&soundManager, "menuBack", false);
            }
            // Finally, handle main menu case
            else {
                // If in main command menu (no submenus active), exit to game
                currentGameState = cmdState->previousState;
                // cmdState->selectedIndex = -1;
                // cmdState->targetSelectionMode = false;
                // cmdState->inAttackSubmenu = false;
                DisableTargeting(cmdState, &realTimeBattleState);
                PlaySoundByName(&soundManager, "menuBack", false);
            }
        }
        // Reset the tracking
        cmdState->wasXPressed = false;
        cmdState->xHoldTime = 0;
        cmdState->xLongPressExecuted = false; // Reset this flag!
    }
    
    // Handle the different menu modes
    // ----- AI SUBMENU MODE -----
    if (cmdState->inAISubmenu) HandleAISubmenu(cmdState);

    // ----- ATTACK TARGETING SUBMENU -----
    else if (cmdState->inAttackSubmenu) HandleAttackSubmenu(cmdState);

    // Handle the command menu in its different modes
    else {
        // ----- COMMAND SELECTION MODE -----
        // Grid navigation for command selection
        bool selectionChanged = false;
        int currentRow = cmdState->selectedIndex / columns;
        int currentCol = cmdState->selectedIndex % columns;
        
        if (IsKeyPressed(KEY_DOWN)) {
            currentRow = (currentRow + 1) % rows;
            selectionChanged = true;
        }
        if (IsKeyPressed(KEY_UP)) {
            currentRow = (currentRow - 1 + rows) % rows;
            selectionChanged = true;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            currentCol = (currentCol + 1) % columns;
            selectionChanged = true;
        }
        if (IsKeyPressed(KEY_LEFT)) {
            currentCol = (currentCol - 1 + columns) % columns;
            selectionChanged = true;
        }
        
        if (selectionChanged) {
            cmdState->selectedIndex = currentRow * columns + currentCol;
            // Keep index within valid range
            if (cmdState->selectedIndex >= commandCount) {
                cmdState->selectedIndex = commandCount - 1;
            }
            PlaySoundByName(&soundManager, "menuSelect", false);
        }

        // Handle Z press to select a command
        if (IsKeyPressed(KEY_Z) && !cmdState->waitingForKeyRelease) {
            cmdState->waitingForKeyRelease = true;
            printf("DEBUG: Z pressed in main menu, set waitingForKeyRelease\n");
            PlaySoundByName(&soundManager, "menuConfirm", false);
            
            // Handle different commands
            switch (cmdState->selectedIndex) {
                case 0: // Attack - go to target selection
                    cmdState->inAttackSubmenu = true;
                    // Initialize target selection
                    realTimeBattleState.selectedCreatureIndex = -1;

                    // Find first valid target
                    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
                        if (realTimeBattleState.creatures[i].active && 
                            IsCreatureInPartyRange(&realTimeBattleState, i)) {
                            realTimeBattleState.selectedCreatureIndex = i;
                            break;
                        }
                    }

                    cmdState->selectedTargetIndex = realTimeBattleState.selectedCreatureIndex;
                    
                    if (realTimeBattleState.selectedCreatureIndex >= 0) {
                        SetOverworldMessage("Select target with arrows, Z to confirm, X to cancel");
                    } else {
                        SetOverworldMessage("No targets in range!");
                        cmdState->inAttackSubmenu = false; // Go back to main menu
                    }
                    break;
                    
                case 5: // AI
                    cmdState->inAISubmenu = true;
                    cmdState->aiSelection = currentFollowerBehavior; // Use current behavior as default
                    break;
                    
                default:
                    // For other commands, return to previous state
                    currentGameState = cmdState->previousState;
                    break;
            }
        }
        // Handle X to exit the entire command menu - only in main menu, not in submenus
        if (IsKeyPressed(KEY_X)) {
            currentGameState = cmdState->previousState;
            // cmdState->selectedIndex = -1;
            // cmdState->targetSelectionMode = false;
            // cmdState->inAttackSubmenu = false;
            DisableTargeting(cmdState, &realTimeBattleState);
            PlaySoundByName(&soundManager, "menuBack", false);
        }
    } 
    
    
    // Set visual targeting flag based on BOTH attack submenu and target selection modes
    realTimeBattleState.playerAttackMode = cmdState->inAttackSubmenu || cmdState->targetSelectionMode;

    // Reset Z key release flag when Z is released
    if (!IsKeyDown(KEY_Z) && cmdState->waitingForKeyRelease) {
        cmdState->waitingForKeyRelease = false;
        printf("DEBUG: Z released, cleared waitingForKeyRelease\n");
    }

    // Update X key state
    // bool isXPressed = IsKeyDown(KEY_X);
    if (!isXPressed) {
        cmdState->wasXPressed = false;
    }
}

// Updated DrawCommandMenu for a 2x3 grid layout at bottom left
void DrawCommandMenu(CommandMenuState *cmdState) {

    // If in AI submenu, draw that instead
    if (cmdState->inAISubmenu) {
        // AI submenu drawing
        const char *aiOptions[] = {"FOLLOW MODE", "AGGRESSIVE MODE"};
        int menuWidth = 200;
        int menuHeight = 100;
        int menuX = (SCREEN_WIDTH - menuWidth) / 2;
        int menuY = (SCREEN_HEIGHT - menuHeight) / 2;
        
        // Draw menu background
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
        DrawRectangle(menuX, menuY, menuWidth, menuHeight, Fade(DARKGRAY, 0.9f));
        DrawRectangleLines(menuX, menuY, menuWidth, menuHeight, WHITE);
        
        // Draw header
        DrawText("SELECT AI BEHAVIOR", menuX + 10, menuY + 10, 16, WHITE);
        
        // Draw options
        for (int i = 0; i < AI_SUBMENU_COUNT; i++) {
            int optionY = menuY + 40 + i * 25;
            bool isSelected = (i == cmdState->aiSelection);
            
            // Draw selection highlight
            if (isSelected) {
                DrawRectangle(menuX + 5, optionY - 2, menuWidth - 10, 20, RED);
            }
            
            // Draw option text
            DrawText(aiOptions[i], menuX + 15, optionY, 16, isSelected ? WHITE : LIGHTGRAY);
        }
        
        // Draw help text
        DrawText("Press UP/DOWN to navigate", menuX + 10, menuY + menuHeight - 30, 12, WHITE);
        DrawText("Press Z to confirm, X to cancel", menuX + 10, menuY + menuHeight - 15, 12, WHITE);
        
        return;
    }

    

        // If we're in target selection mode, draw the targets
    if (cmdState->targetSelectionMode) {
        // Draw a semi-transparent overlay 
        // DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
        
        // Draw a header
        DrawRectangle(0, 0, SCREEN_WIDTH, 40, Fade(DARKGRAY, 0.8f));
        DrawText("SELECT TARGET", 20, 10, 20, WHITE);
        DrawText("Press Z to confirm, X to cancel", SCREEN_WIDTH - 300, 15, 16, WHITE);
        
        // return;
    }


    const char *commands[] = {"Attack", "Skill", "Magic", "Item", "Defense", "AI"};
    const int commandCount = sizeof(commands) / sizeof(commands[0]);
    const int columns = 3;
    const int rows = 2;

    // Command menu layout
    int buttonWidth = 120;
    int buttonHeight = 40;
    int menuWidth = buttonWidth * columns + 20; // padding
    int menuHeight = buttonHeight * rows + 20; // padding
    int menuX = 20; // Position at bottom left
    int menuY = SCREEN_HEIGHT - menuHeight - 20; // with some margin
    
    // First, draw a semi-transparent overlay across the entire screen to signify time slowing down
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f));
    
    // Draw menu background
    DrawRectangle(menuX, menuY, menuWidth, menuHeight, Fade(DARKGRAY, 0.9f));
    DrawRectangleLines(menuX, menuY, menuWidth, menuHeight, WHITE);
    
    // Draw command grid (2 rows x 3 columns)
    for (int i = 0; i < commandCount; i++) {
        int row = i / columns;
        int col = i % columns;
        
        int buttonX = menuX + 10 + (col * buttonWidth);
        int buttonY = menuY + 10 + (row * buttonHeight);
        bool isSelected = (i == cmdState->selectedIndex);
        
        // Draw button background
        Color buttonColor = isSelected ? Fade(RED, 0.7f) : Fade(BLACK, 0.6f);
        DrawRectangle(buttonX, buttonY, buttonWidth - 10, buttonHeight - 10, buttonColor);
        DrawRectangleLines(buttonX, buttonY, buttonWidth - 10, buttonHeight - 10, WHITE);
        
        // Draw command text centered in button
        int textWidth = MeasureText(commands[i], 16);
        int textX = buttonX + ((buttonWidth - 10) - textWidth) / 2;
        int textY = buttonY + ((buttonHeight - 10) - 16) / 2;
        Color textColor = isSelected ? WHITE : LIGHTGRAY;
        DrawText(commands[i], textX, textY, 16, textColor);
    }
    
    // Display help text
    DrawText("Press UP, DOWN, LEFT, RIGHT to navigate", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 50, 12, WHITE);
    DrawText("Press Z to confirm, X to cancel", SCREEN_WIDTH - 300, SCREEN_HEIGHT - 30, 12, WHITE);
}


// Helper function to update command availability based on energy
void UpdateCommandAvailability() {
    // Require at least 1.0 energy to use commands (enough for a basic attack)
    realTimeBattleState.canUseCommand = (realTimeBattleState.commandEnergy >= 0.0f);
    //TODO CHANGE LATER
}


// Helper function for smooth value transitions
float approach(float current, float target, float step) {
    if (current < target) {
        return fmin(current + step, target);
    } else {
        return fmax(current - step, target);
    }
}


void UpdateRealTimeBattleMovement(RealTimeBattleState* rtbsState) {
    float deltaTime = GetFrameTime();
    float adjustedMoveIncrement = moveIncrement;
    
    // Apply time slowdown if needed (handled in parent function)
    
    // Get key states
    bool rightPressed = IsKeyDown(KEY_RIGHT);
    bool leftPressed = IsKeyDown(KEY_LEFT);
    bool downPressed = IsKeyDown(KEY_DOWN);
    bool upPressed = IsKeyDown(KEY_UP);
    bool BPressed = IsKeyDown(KEY_X); // Run button
    
    // Calculate movement direction
    Vector2 newPos = { mainTeam[0].x, mainTeam[0].y };
    bool isMoving = false;
    
    // Apply movement logic
    if (rightPressed) {
        mainTeam[0].anims.direction = RIGHT;
        mainTeam[0].anims.directionPressCounter[RIGHT]++;
        isMoving = true;
    } else {
        mainTeam[0].anims.directionPressCounter[RIGHT] = 0;
    }
    
    if (leftPressed) {
        mainTeam[0].anims.direction = LEFT;
        mainTeam[0].anims.directionPressCounter[LEFT]++;
        isMoving = true;
    } else {
        mainTeam[0].anims.directionPressCounter[LEFT] = 0;
    }
    
    if (downPressed) {
        mainTeam[0].anims.direction = DOWN;
        mainTeam[0].anims.directionPressCounter[DOWN]++;
        isMoving = true;
    } else {
        mainTeam[0].anims.directionPressCounter[DOWN] = 0;
    }
    
    if (upPressed) {
        mainTeam[0].anims.direction = UP;
        mainTeam[0].anims.directionPressCounter[UP]++;
        isMoving = true;
    } else {
        mainTeam[0].anims.directionPressCounter[UP] = 0;
    }
    
    // Update movement state and animation
    if (isMoving) {
        // Check if the key has been held long enough to start moving
        if (mainTeam[0].anims.directionPressCounter[mainTeam[0].anims.direction] > INERTIA_TO_MOTION) {
            // Calculate movement vector
            float moveSpeed = BPressed ? adjustedMoveIncrement * 2 : adjustedMoveIncrement;
            
            if (rightPressed) newPos.x += moveSpeed;
            if (leftPressed) newPos.x -= moveSpeed;
            if (downPressed) newPos.y += moveSpeed;
            if (upPressed) newPos.y -= moveSpeed;
            
            // Apply collision detection
            Tool* selectedTool = &toolWheel.tools[toolWheel.selectedToolIndex];
            
            // Check if new position is walkable and doesn't overlap with creatures
            if (IsTileWalkable((int)newPos.x, (int)newPos.y, selectedTool) && 
                !PlayerOverlapsCreature(rtbsState, newPos)) {

                if (newPos.x < 0) newPos.x = 0;
                if (newPos.y < 0) newPos.y = 0;
                if (newPos.x >= mapWidth) newPos.x = mapWidth - 1;
                if (newPos.y >= mapHeight) newPos.y = mapHeight - 1;
                
                // Update position
                mainTeam[0].x = newPos.x;
                mainTeam[0].y = newPos.y;

                // Apply separation force
                Vector2 playerPos = {mainTeam[0].x, mainTeam[0].y};
                ApplySeparationBehavior(&playerPos, ENTITY_PLAYER, &mainTeam[0]);
                mainTeam[0].x = playerPos.x;
                mainTeam[0].y = playerPos.y;
                
                // Update player history for followers
                historyIndex = (historyIndex + 1) % MAX_HISTORY;
                playerHistory[historyIndex].x = mainTeam[0].x;
                playerHistory[historyIndex].y = mainTeam[0].y;
                
                // Update step counter for the current terrain
                TerrainType currentTerrain = worldMap[(int)mainTeam[0].y][(int)mainTeam[0].x].type;
                mainTeam[0].steps[currentTerrain]++;
                
                // Set movement state
                mainTeam[0].anims.movementState = WALKING;
                mainTeam[0].anims.isMoving = true;
                mainTeam[0].anims.isStopping = false;
                frameTracking.stopDelayCounter = 120; // Reset delay counter
            }
        } else {
            // Player is pressing a direction but hasn't started moving yet
            mainTeam[0].anims.isMoving = false;
            mainTeam[0].anims.isStopping = true;
            mainTeam[0].anims.movementState = WALKING;
            frameTracking.currentFrame = 0;
            frameTracking.stopDelayCounter = 120;
        }
    } else if (mainTeam[0].anims.isMoving) {
        // Player just released keys, transition to stopping
        mainTeam[0].anims.isMoving = false;
        mainTeam[0].anims.isStopping = true;
        frameTracking.stopDelayCounter = 120;
    } else if (mainTeam[0].anims.isStopping && frameTracking.stopDelayCounter > 0) {
        // Count down the stop delay
        frameTracking.stopDelayCounter--;
    } else if (mainTeam[0].anims.isStopping && frameTracking.stopDelayCounter == 0) {
        // Transition to idle
        mainTeam[0].anims.isStopping = false;
        mainTeam[0].anims.movementState = IDLE;
    }
}


void DrawOverworldCreatures(RealTimeBattleState* rtbsState, Vector2 viewport) {
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        OverworldCreature* creature = &rtbsState->creatures[i];
        if (creature->active) {
            DrawOverworldCreature(creature, viewport);
        }
    }
}


// Check if a position would cause a creature to overlap with another
bool CreatureOverlap(RealTimeBattleState* rtbsState, Vector2 position, int excludeIndex) {
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        // Skip self and inactive creatures
        if (i == excludeIndex || !rtbsState->creatures[i].active) continue;
        
        // Calculate distance between creatures
        float dx = rtbsState->creatures[i].position.x - position.x;
        float dy = rtbsState->creatures[i].position.y - position.y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        // Check for overlap (using 1.0 as minimum distance, can adjust as needed)
        if (distance < 1.0f) {
            return true; // Overlap detected
        }
    }
    
    // Also check for overlap with player
    float dx = mainTeam[0].x - position.x;
    float dy = mainTeam[0].y - position.y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    // Player overlap check (smaller distance to allow creatures to get close for battle)
    if (distance < 0.8f) {
        return true;
    }

        // Check for overlap with followers
    for (int i = 0; i < followerCount; i++) {
        dx = followers[i].creature->x - position.x;
        dy = followers[i].creature->y - position.y;
        distance = sqrtf(dx * dx + dy * dy);
        
        if (distance < 0.8f) {
            return true;
        }
    }
    
    return false; // No overlap
}


// Check if a position would overlap with any active creatures
bool PlayerOverlapsCreature(RealTimeBattleState* rtbsState, Vector2 position) {
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (!rtbsState->creatures[i].active) continue;
        
        // Calculate distance to creature
        float dx = rtbsState->creatures[i].position.x - position.x;
        float dy = rtbsState->creatures[i].position.y - position.y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        // Check for overlap (using 0.8f to match the creature check)
        if (distance < 0.8f) {
            return true;
        }
    }
    
    return false;
}

// Simple helper function to determine if a creature can attack
bool CanCreatureAttack(OverworldCreature* creature) {
    // Calculate distance to player
    float dx = mainTeam[0].x - creature->position.x;
    float dy = mainTeam[0].y - creature->position.y;
    float distanceToPlayer = sqrtf(dx * dx + dy * dy);
    
    // Can attack if close enough (adjust the 1.0f value as needed)
    return distanceToPlayer <= 1.0f;
}

// Handle creature attack on player
// void CreatureAttackPlayer(RealTimeBattleState* rtbsState, OverworldCreature* creature, int creatureIndex) {
//     // Set up the battle participants
//     BattleParticipant attacker = {0};
//     BattleParticipant defender = {0};
    
//     // Set up attacker
//     attacker.creature = &creature->creature;
//     attacker.isPlayer = false;
//     attacker.isActive = true;
    
//     // Set up defender (player)
//     defender.creature = &mainTeam[0];
//     defender.isPlayer = true;
//     defender.isActive = true;
    
//     // Calculate hit chance
//     bool didHit = WillEnemyHit(attacker.creature, defender.creature);
    
//     // Debug logging
//     printf("\n%s attacks player - Hit: %d\n", creature->creature.name, didHit);
    
//     if (!didHit) {
//         char message[100];
//         sprintf(message, "%s's attack missed!", creature->creature.name);
//         SetOverworldMessage(message);
//         return;
//     }
    
//     // Calculate and apply damage
//     int damage = attacker.creature->attack + attacker.creature->tempAttackBuff
//                - defender.creature->defense - defender.creature->tempDefenseBuff;
//     damage = MAX(damage, 1);  // Ensure at least 1 damage
    
//     // Apply damage to player
//     mainTeam[0].currentHP -= damage;
//     mainTeam[0].currentHP = CLAMP(mainTeam[0].currentHP, 0, mainTeam[0].maxHP);
    
//     // Set up damage display for player
//     // We'll need to track this in the real-time battle state
//     rtbsState->playerDamageDisplay = damage;
//     rtbsState->playerDamageTimer = DAMAGE_TIMER;
//     rtbsState->playerDamageOffsetY = 0.0f;
//     rtbsState->playerDamageEffectType = EFFECT_OFFENSIVE;
//     rtbsState->playerDamageRandomX = RANDOMS(0, TILE_SIZE);
    
//     // Set attack animation for the creature
//     // creature->attackAnimationActive = true;
//     // creature->attackAnimationTimer = creature->animTracking.frameCounter;
    
//     // Set attack animation
//     rtbsState->attackAnimationActive = true;
//     animations.attack.isAnimating = true;
//     animations.attack.currentFrame = 0;
//     animations.attack.frameCounter = 0;
    
//     // Flag the creature as attacking
//     creature->isAttacking = true;

//     // Battle message
//     char message[128];
//     snprintf(message, sizeof(message), "%s dealt %d damage to you!",
//              creature->creature.name, damage);
//     SetOverworldMessage(message);
    
//     // Check if player was defeated
//     if (mainTeam[0].currentHP <= 0) {
//         mainTeam[0].currentHP = 0;
//         // Handle player defeat (could transition to game over)
//         SetOverworldMessage("You were defeated!");
//         // Could play defeat sound and trigger game over
//     }
    
//     // Add screen shake effect
//     rtbsState->shakeTimer = 10;
// }


// Update the attack animation handling
void UpdateAttackAnimations(RealTimeBattleState* rtbsState) {
    // Update the global attack animation
    if (animations.attack.isAnimating) {
        animations.attack.frameCounter++;
        
        if (animations.attack.frameCounter >= animations.attack.frameSpeed) {
            animations.attack.frameCounter = 0;
            animations.attack.currentFrame++;
            
            if (animations.attack.currentFrame >= animations.attack.frameCount) {
                animations.attack.isAnimating = false;
                animations.attack.currentFrame = 0;
                rtbsState->attackAnimationActive = false;

                // Reset targeting flags
                DisableTargeting(&commandMenuState, rtbsState);
                
                // Clear attacking state for all creatures
                for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
                    if (rtbsState->creatures[i].active) {
                        rtbsState->creatures[i].isAttacking = false;
                    }
                }
                
                // Clear attacking state for all followers
                for (int i = 0; i < followerCount; i++) {
                    followers[i].isAttacking = false;
                }
                
                // Reset attacking indices
                rtbsState->attackingCreatureIndex = -1;
                rtbsState->attackingFollowerIndex = -1;
            }
        }
    }
}


// Check if a creature can attack player or followers
bool CanCreatureAttackParty(RealTimeBattleState* rtbsState, OverworldCreature* creature, int* targetIndex) {
    // First check player
    float dx = mainTeam[0].x - creature->position.x;
    float dy = mainTeam[0].y - creature->position.y;
    float distanceToPlayer = sqrtf(dx * dx + dy * dy);
    
    if (distanceToPlayer <= 1.0f && mainTeam[0].currentHP > 0) {
        *targetIndex = -1; // -1 means target is the player
        return true;
    }
    
    // Check followers
    for (int i = 0; i < followerCount; i++) {
        dx = followers[i].creature->x - creature->position.x;
        dy = followers[i].creature->y - creature->position.y;
        float distanceToFollower = sqrtf(dx * dx + dy * dy);
        
        if (distanceToFollower <= 1.0f && followers[i].creature->currentHP > 0) {
            *targetIndex = i;
            return true;
        }
    }
    
    return false;
}


// Enhanced creature attack function
void CreatureAttackParty(RealTimeBattleState* rtbsState, OverworldCreature* creature, int creatureIndex) {
    int targetIndex;
    if (!CanCreatureAttackParty(rtbsState, creature, &targetIndex)) {
        return; // No valid target
    }
    
    // Set up attacker
    BattleParticipant attacker = {0};
    attacker.creature = &creature->creature;
    attacker.isPlayer = false;
    attacker.isActive = true;
    
    // Set up defender
    BattleParticipant defender = {0};
    if (targetIndex == -1) {
        // Target is player
        defender.creature = &mainTeam[0];
    } else {
        // Target is a follower
        defender.creature = followers[targetIndex].creature;
    }
    defender.isPlayer = true;
    defender.isActive = true;
    
    // Calculate hit chance using battle system logic
    bool didHit = WillEnemyHit(attacker.creature, defender.creature);
    
    // Debug logging
    printf("\n%s attacks %s - Hit: %d\n", creature->creature.name, defender.creature->name, didHit);
    
    if (!didHit) {
        char message[100];
        sprintf(message, "%s's attack missed %s!", creature->creature.name, 
                defender.creature->name);
        SetOverworldMessage(message);
        return;
    }
    
    // Calculate and apply damage
    int damage = attacker.creature->attack + attacker.creature->tempAttackBuff
               - defender.creature->defense - defender.creature->tempDefenseBuff;
    damage = MAX(damage, 1);  // Ensure at least 1 damage
    
    // Apply damage to target
    defender.creature->currentHP -= damage;
    defender.creature->currentHP = CLAMP(defender.creature->currentHP, 0, defender.creature->maxHP);

    // Immediately sync follower HP to main team if target is a follower
    if (targetIndex >= 0) SyncTeamStats();

    // Apply knockback to the hit target
    ApplyKnockbackToTarget(targetIndex, creature->creature.anims.direction);
    
    // Set up damage display
    if (targetIndex == -1) {
        // Player damage display
        rtbsState->playerDamageDisplay = damage;
        rtbsState->playerDamageTimer = DAMAGE_TIMER;
        rtbsState->playerDamageOffsetY = 0.0f;
        rtbsState->playerDamageRandomX = RANDOMS(0, TILE_SIZE);
        rtbsState->playerDamageEffectType = EFFECT_OFFENSIVE;
    } else {
        // Follower damage display
        rtbsState->followerDamageDisplay[targetIndex] = damage;
        rtbsState->followerDamageTimer[targetIndex] = DAMAGE_TIMER;
        rtbsState->followerDamageOffsetY[targetIndex] = 0.0f;
        rtbsState->followerDamageEffectType[targetIndex] = EFFECT_OFFENSIVE;
        rtbsState->followerDamageRandomX[targetIndex] = RANDOMS(0, TILE_SIZE);
    }
    
    // Set attack animation
    animations.attack.isAnimating = true;
    animations.attack.currentFrame = 0;
    animations.attack.frameCounter = 0;
    rtbsState->attackAnimationActive = true;
    rtbsState->attackingCreatureIndex = creatureIndex;  // Add this line
    
    // Flag the creature as attacking
    creature->isAttacking = true;
    
    // Battle message
    char message[128];
    snprintf(message, sizeof(message), "%s dealt %d damage to %s!",
             creature->creature.name, damage, defender.creature->name);
    SetOverworldMessage(message);
    
    // Play sound effect
    PlaySoundByName(&soundManager, "enemyAttack", false);
    
    // Check if target was defeated
    if (defender.creature->currentHP <= 0) {
        defender.creature->currentHP = 0;
        
        if (targetIndex == -1) {
            // Player was defeated
            SetOverworldMessage("You were defeated!");
            currentGameState = GAME_STATE_GAME_OVER;
        } else {
            // Follower was defeated
            char defeatMessage[100];
            sprintf(defeatMessage, "%s was defeated!", defender.creature->name);
            SetOverworldMessage(defeatMessage);
        }
    }
    
    // Add screen shake effect
    rtbsState->shakeTimer = 10;
}


// CORRECTED ApplyKnockbackToTarget function
void ApplyKnockbackToTarget(int targetIndex, int direction) {
    const float KNOCKBACK_DISTANCE = 0.5f; // Adjust as needed
    Vector2 knockbackVector = {0, 0};

    // Determine knockback direction based on attacker's facing direction
    switch (direction) {
        case UP:    knockbackVector.y = -KNOCKBACK_DISTANCE; break;
        case DOWN:  knockbackVector.y =  KNOCKBACK_DISTANCE; break;
        case LEFT:  knockbackVector.x = -KNOCKBACK_DISTANCE; break;
        case RIGHT: knockbackVector.x =  KNOCKBACK_DISTANCE; break;
    }

    // Apply knockback to appropriate target
    if (targetIndex == -1) { // CORRECTED: Check for -1 for player
        // Knockback player
        Vector2 newPos = {
            mainTeam[0].x + knockbackVector.x,
            mainTeam[0].y + knockbackVector.y
        };

        // Check if new position is valid before applying
        // Pass ENTITY_PLAYER and the player object to exclude player from self-collision check
        if (IsPositionValid(newPos, ENTITY_PLAYER, &mainTeam[0])) {
             printf("Knocking back player to (%.1f, %.1f)\n", newPos.x, newPos.y); // DEBUG
             mainTeam[0].x = newPos.x;
             mainTeam[0].y = newPos.y;
             // Update history if player moved significantly (maybe add threshold)
             historyIndex = (historyIndex + 1) % MAX_HISTORY;
             playerHistory[historyIndex].x = newPos.x;
             playerHistory[historyIndex].y = newPos.y;
        } else {
             printf("Player knockback to (%.1f, %.1f) invalid.\n", newPos.x, newPos.y); // DEBUG
        }
    } else if (targetIndex >= 0 && targetIndex < followerCount) { // CORRECTED: Check for 0 to followerCount-1
        // Knockback follower (targetIndex is the direct index now)
        int followerIndex = targetIndex; // CORRECTED: Direct index
        Vector2 newPos = {
            followers[followerIndex].creature->x + knockbackVector.x,
            followers[followerIndex].creature->y + knockbackVector.y
        };

        // Check if new position is valid before applying
        // Pass ENTITY_FOLLOWER and the specific follower to exclude from self-collision
        if (IsPositionValid(newPos, ENTITY_FOLLOWER, &followers[followerIndex])) {
            printf("Knocking back follower %d to (%.1f, %.1f)\n", followerIndex, newPos.x, newPos.y); // DEBUG
            followers[followerIndex].creature->x = newPos.x;
            followers[followerIndex].creature->y = newPos.y;
        } else {
             printf("Follower %d knockback to (%.1f, %.1f) invalid.\n", followerIndex, newPos.x, newPos.y); // DEBUG
        }
    } else {
         printf("ApplyKnockbackToTarget called with invalid targetIndex: %d\n", targetIndex); // DEBUG
    }
}


bool IsCreatureInPartyRange(RealTimeBattleState* rtbsState, int creatureIndex) {
    OverworldCreature* creature = &rtbsState->creatures[creatureIndex];

        // Apply range bonus to engagement radius
    float adjustedRadius = rtbsState->engagementRadius + rtbsState->rangeBonus;
    
    // Check if within player's range
    float dx = mainTeam[0].x - creature->position.x;
    float dy = mainTeam[0].y - creature->position.y;
    float distanceToPlayer = sqrtf(dx * dx + dy * dy);
    
    if (distanceToPlayer <= adjustedRadius) {
        return true;
    }
    
    // Check if within any follower's range
    for (int i = 0; i < followerCount; i++) {
        if (followers[i].creature->currentHP <= 0) continue;

        dx = followers[i].creature->x - creature->position.x;
        dy = followers[i].creature->y - creature->position.y;
        float distanceToFollower = sqrtf(dx * dx + dy * dy);
        
        if (distanceToFollower <= adjustedRadius) {
            return true;
        }
    }
    
    return false;
}



void MovePlayerTowardTarget(RealTimeBattleState* rtbsState, int targetCreatureIndex) {
    OverworldCreature* targetCreature = &rtbsState->creatures[targetCreatureIndex];
    
    // Calculate distance to target
    float dx = targetCreature->position.x - mainTeam[0].x;
    float dy = targetCreature->position.y - mainTeam[0].y;
    float distance = sqrtf(dx * dx + dy * dy);
    
    // If already close enough, no need to move
    if (distance <= 1.2f) {
        return;
    }
    
    // Calculate direction to target
    float dirX = dx / distance;
    float dirY = dy / distance;
    
    // Calculate position that's close enough to attack
    float targetX = targetCreature->position.x - dirX * 1.0f;
    float targetY = targetCreature->position.y - dirY * 1.0f;
    
    // Set player position (simplified - in a real game you'd animate this)
    mainTeam[0].x = targetX;
    mainTeam[0].y = targetY;
    
    // Update player direction
    if (fabsf(dx) > fabsf(dy)) {
        mainTeam[0].anims.direction = (dx > 0) ? RIGHT : LEFT;
    } else {
        mainTeam[0].anims.direction = (dy > 0) ? DOWN : UP;
    }
    
    // Update player history for followers
    historyIndex = (historyIndex + 1) % MAX_HISTORY;
    playerHistory[historyIndex].x = mainTeam[0].x;
    playerHistory[historyIndex].y = mainTeam[0].y;
}

int FindNextTarget(RealTimeBattleState* rtbsState, int currentIndex, bool inMenu) {
    // If no valid current target, find the first valid one
    if (currentIndex < 0) {
        for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
            if (rtbsState->creatures[i].active && IsCreatureInPartyRange(rtbsState, i)) {
                return i;
            }
        }
        return -1; // No valid targets
    }
    
    // Handle direction keys
    bool rightPressed = IsKeyPressed(KEY_RIGHT);
    bool leftPressed = IsKeyPressed(KEY_LEFT);
    bool upPressed = IsKeyPressed(KEY_UP);
    bool downPressed = IsKeyPressed(KEY_DOWN);
    
    // If no direction pressed, return current
    if (!rightPressed && !leftPressed && !upPressed && !downPressed) {
        return currentIndex;
    }
    
    OverworldCreature* currentTarget = &rtbsState->creatures[currentIndex];
    float currentX = currentTarget->position.x;
    float currentY = currentTarget->position.y;
    
    // Find best target in selected direction
    int bestIndex = -1;
    float bestScore = 999999.0f;
    
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (i == currentIndex || !rtbsState->creatures[i].active || 
            !IsCreatureInPartyRange(rtbsState, i)) {
            continue;
        }
        
        float dx = rtbsState->creatures[i].position.x - currentX;
        float dy = rtbsState->creatures[i].position.y - currentY;
        float dist = sqrtf(dx*dx + dy*dy);
        
        // Calculate a directional score based on which key was pressed
        float score = 999999.0f;
        
        if (rightPressed && dx > 0) score = dist - dx*3; // Prefer rightward
        else if (leftPressed && dx < 0) score = dist + dx*3; // Prefer leftward
        else if (downPressed && dy > 0) score = dist - dy*3; // Prefer downward
        else if (upPressed && dy < 0) score = dist + dy*3; // Prefer upward
        
        if (score < bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    
    return (bestIndex >= 0) ? bestIndex : currentIndex;
}

// Replace the FindNextTargetInDirection function with this improved version
int FindNextTargetInDirection(RealTimeBattleState* rtbsState) {
    return FindNextTarget(rtbsState, rtbsState->selectedCreatureIndex, false);
}

int FindNextTargetInMenuDirection(RealTimeBattleState* rtbsState, CommandMenuState* cmdState) {
    return FindNextTarget(rtbsState, cmdState->selectedTargetIndex, true);
}



// Find the nearest creature to the party (player or any follower)
int FindNearestCreatureToParty() {
    int nearestIndex = -1;
    float nearestDist = 999999.0f;
    
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (!realTimeBattleState.creatures[i].active) continue;
        
        // Check distance to player
        float dx = realTimeBattleState.creatures[i].position.x - mainTeam[0].x;
        float dy = realTimeBattleState.creatures[i].position.y - mainTeam[0].y;
        float dist = sqrtf(dx*dx + dy*dy);
        
        // If this creature is closer than any found so far
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestIndex = i;
        }
        
        // Also check distance to followers
        for (int j = 0; j < followerCount; j++) {
            dx = realTimeBattleState.creatures[i].position.x - followers[j].creature->x;
            dy = realTimeBattleState.creatures[i].position.y - followers[j].creature->y;
            dist = sqrtf(dx*dx + dy*dy);
            
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestIndex = i;
            }
        }
    }
    
    // Only return a valid index if creature is within a reasonable range
    if (nearestDist <= realTimeBattleState.engagementRadius * 1.5f) {
        return nearestIndex;
    }
    
    return -1;  // No suitable target found
}

// Move follower toward a specific creature
void MoveFollowerTowardCreature(Follower* follower, int creatureIndex) {
    OverworldCreature* target = &realTimeBattleState.creatures[creatureIndex];
    
    // Calculate direction to target
    float dx = target->position.x - follower->creature->x;
    float dy = target->position.y - follower->creature->y;
    float dist = sqrtf(dx*dx + dy*dy);
    
    // Don't move if already very close
    if (dist < 1.0f) return;
    
    // Normalize direction vector
    float dirX = dx / dist;
    float dirY = dy / dist;
    
    // Set follower direction based on movement
    if (fabsf(dirX) > fabsf(dirY)) {
        follower->creature->anims.direction = (dirX > 0) ? RIGHT : LEFT;
    } else {
        follower->creature->anims.direction = (dirY > 0) ? DOWN : UP;
    }
    
    // Calculate movement speed
    float moveSpeed = 1.0f * GetFrameTime();  // Slightly faster than normal following
    
    // Calculate new position
    float newX = follower->creature->x + dirX * moveSpeed;
    float newY = follower->creature->y + dirY * moveSpeed;
    
    // Check if new position is valid (walkable and not overlapping)
    if (IsPositionValid((Vector2){newX, newY}, ENTITY_FOLLOWER, follower)) {
        
        // Update position
        follower->creature->x = newX;
        follower->creature->y = newY;
        
        // Update animation state
        follower->creature->anims.isMoving = true;
        follower->creature->anims.isStopping = false;
        follower->creature->anims.movementState = WALKING;
    }
}

// Check if a follower is in attack range of a creature
bool IsFollowerInAttackRange(Follower* follower, int creatureIndex) {
    OverworldCreature* target = &realTimeBattleState.creatures[creatureIndex];
    
    // Calculate distance to target
    float dx = target->position.x - follower->creature->x;
    float dy = target->position.y - follower->creature->y;
    float dist = sqrtf(dx*dx + dy*dy);
    
    // Can attack if close enough (you can adjust the 1.5f value as needed)
    return dist <= 1.5f;
}



// Updated follower update function with animation state reset
void UpdateFollowers(Creature *c, FrameTracking *f) {
    float deltaTime = GetFrameTime();

    // Determine if we're in a battle state
    bool inBattleState = (currentGameState == GAME_STATE_REALTIME_BATTLE || 
                          currentGameState == GAME_STATE_COMMAND_MENU);

    for (int i = 0; i < followerCount; i++) {
        // Skip defeated followers
        if (followers[i].creature->currentHP <= 0) continue;
        
        // Update attack cooldown regardless of state
        if (followers[i].attackCooldown > 0) {
            followers[i].attackCooldown -= deltaTime;
        }

        // Check if attack animation is complete and reset isAttacking flag
        if (followers[i].isAttacking) {
            if (!animations.attack.isAnimating && !realTimeBattleState.attackAnimationActive) {
                followers[i].isAttacking = false;
            }
        }

        // Handle different behaviors based on game state
        if (inBattleState && followers[i].behavior == FOLLOWER_BEHAVIOR_AGGRESSIVE) {
            // Use enemy-like behavior in battle states when in aggressive mode
            UpdateFollowerAggressiveBehavior(&followers[i], i, deltaTime, f);
        }
        else {
            // Use standard following behavior for all other situations
            UpdateFollowerStandardBehavior(&followers[i], c, f, i);
        }
    }
}



// Standard following behavior function - unchanged
void UpdateFollowerStandardBehavior(Follower* follower, Creature* c, FrameTracking* f, int followerIndex) {
    // Original following code
    int followerHistoryIndex = (int)(historyIndex - (followerIndex + 1) * lag + MAX_HISTORY) % MAX_HISTORY;
    follower->creature->x = playerHistory[followerHistoryIndex].x;
    follower->creature->y = playerHistory[followerHistoryIndex].y;
    
    // Simple direction update: Copy the player's direction
    // This single line is all we need to fix the direction
    follower->lastDirection = c->anims.direction;
    
    // Update animation state based on player
    follower->creature->anims.movementState = c->anims.movementState;
    follower->creature->anims.isMoving = c->anims.isMoving;
    follower->creature->anims.isStopping = c->anims.isStopping;
    
    // Animation updates
    if (!c->anims.isStopping) {
        follower->followerFrameCounter++;
    }
   
    int followerFrameCounted = follower->creature->anims.walkFrameCount[follower->lastDirection];
    if (followerFrameCounted <= 0) followerFrameCounted = 1; // Safety check
    
    if (c->anims.movementState == WALKING) {
        // We removed the complex logic - just using player's direction
        
        if (follower->followerFrameCounter >= FOLLOWER_FRAME_SPEED) {
            follower->lastFrame = (follower->lastFrame + 1) % followerFrameCounted;
            follower->followerFrameCounter = 0;
        }
    }
    else if (c->anims.movementState == IDLE) {
        if (follower->followerFrameCounter >= f->idleFrameSpeed) {
            int maxIdleFrames = follower->creature->anims.idleFrameCount;
            if (maxIdleFrames <= 0) maxIdleFrames = 1;
            follower->lastFrame = rand() % maxIdleFrames;
            follower->followerFrameCounter = 0;
        }
    }
}

// Updated aggressive behavior function
void UpdateFollowerAggressiveBehavior(Follower* follower, int followerIndex, float deltaTime, FrameTracking* f) {
    // Access the global difficulty level - same as enemy code
    SettingsGameplayDifficultyType currentDifficulty = (SettingsGameplayDifficultyType)mainMenuState.difficultyLevel;

    // Check if follower can attack - exactly like enemy code
    if (follower->attackCooldown <= 0 && !follower->isAttacking) {
        int targetIndex = FindNearestCreatureToFollower(followerIndex);
        if (targetIndex >= 0 && IsFollowerInAttackRange(follower, targetIndex)) {
            // Execute attack using same pattern as enemy code
            PerformFollowerAttack(follower, followerIndex, targetIndex);

            // Set cooldown based on difficulty - same formula as enemy code
            float baseCooldown = 3.0f; // Base time for Normal
            float randomFactor = ((float)RANDOMS(0, 100) / 100.0f);

            switch (currentDifficulty) {
                case SETTINGS_EASY_MODE:
                    follower->attackCooldown = (baseCooldown * 1.5f) + randomFactor; 
                    break;
                case SETTINGS_HARD_MODE:
                    follower->attackCooldown = (baseCooldown * 0.6f) + randomFactor; 
                    break;
                case SETTINGS_NORMAL_MODE:
                default:
                    follower->attackCooldown = baseCooldown + randomFactor; 
                    break;
            }

            // Ensure minimum cooldown - same as enemy code
            if (follower->attackCooldown < 1.2f) {
                follower->attackCooldown = 1.2f;
            }
            
            // Continue with movement AI even after attacking
        }
    }

    // Movement AI section - mirrors the enemy code - now runs even when attacking
    int targetIndex = FindNearestCreatureToFollower(followerIndex);
    
    if (targetIndex >= 0) {
        OverworldCreature* target = &realTimeBattleState.creatures[targetIndex];
        
        // Calculate distance to target
        float dx = target->position.x - follower->creature->x;
        float dy = target->position.y - follower->creature->y;
        float distanceToTarget = sqrtf(dx * dx + dy * dy);
        
        // Check if already in range
        bool inAttackRange = (distanceToTarget <= 1.0f);
        
        if (distanceToTarget > 0.01f) { // Avoid division by zero - like enemy code
            if (!inAttackRange) {
                // Movement vectors - identical to enemy code
                float dirX = dx / distanceToTarget;
                float dirY = dy / distanceToTarget;
                
                // Movement speed with difficulty scaling - identical to enemy code
                float baseMoveSpeedFactor = 2.5f;
                float difficultyMoveMultiplier = 1.0f;

                switch (currentDifficulty) {
                    case SETTINGS_EASY_MODE:
                        difficultyMoveMultiplier = 0.70f;
                        break;
                    case SETTINGS_HARD_MODE:
                        difficultyMoveMultiplier = 1.30f;
                        break;
                    case SETTINGS_NORMAL_MODE:
                    default:
                        difficultyMoveMultiplier = 1.0f;
                        break;
                }

                float moveSpeed = baseMoveSpeedFactor * difficultyMoveMultiplier * deltaTime;

                // Pursuit bonus - identical to enemy code
                float pursuitBonus = 1.5f;
                if (distanceToTarget > 1.0f) {
                    moveSpeed *= pursuitBonus;
                }

                // Calculate move vector - identical to enemy code
                Vector2 moveVector = {
                    dirX * moveSpeed,
                    dirY * moveSpeed
                };

                // Apply movement with collision and sliding - identical to enemy code
                if (fabsf(moveVector.x) > 0.001f || fabsf(moveVector.y) > 0.001f) {
                    Vector2 newPos = {
                        follower->creature->x + moveVector.x,
                        follower->creature->y + moveVector.y
                    };

                    // Position validity check and movement - identical to enemy code
                    if (IsPositionValid(newPos, ENTITY_FOLLOWER, follower)) {
                        follower->creature->x = newPos.x;
                        follower->creature->y = newPos.y;

                        // Apply separation force
                        Vector2 followerPos = {follower->creature->x, follower->creature->y};
                        ApplySeparationBehavior(&followerPos, ENTITY_FOLLOWER, follower);
                        follower->creature->x = followerPos.x;
                        follower->creature->y = followerPos.y;
                        
                        // Direction update - identical to enemy code
                        if (fabsf(moveVector.x) > fabsf(moveVector.y)) {
                            follower->creature->anims.direction = (moveVector.x > 0) ? RIGHT : LEFT;
                            follower->lastDirection = follower->creature->anims.direction; // Add this
                        } else {
                            follower->creature->anims.direction = (moveVector.y > 0) ? DOWN : UP;
                            follower->lastDirection = follower->creature->anims.direction; // Add this
                        }
                        
                        // Animation state updates - identical to enemy code
                        follower->creature->anims.movementState = WALKING;
                        follower->creature->anims.isMoving = true;
                        follower->creature->anims.isStopping = false;
                        f->stopDelayCounter = 60; // Using global frameTracking
                    }
                    // Sliding logic - identical to enemy code
                    else {
                        // Try horizontal slide
                        Vector2 slideH = {newPos.x, follower->creature->y};
                        if (fabsf(moveVector.x) > 0.001f && IsPositionValid(slideH, ENTITY_FOLLOWER, follower)) {
                           follower->creature->x = slideH.x;
                           follower->creature->anims.direction = (moveVector.x > 0) ? RIGHT : LEFT;
                           follower->creature->anims.isMoving = true;
                        }
                        // Try vertical slide
                        else {
                            Vector2 slideV = {follower->creature->x, newPos.y};
                            if (fabsf(moveVector.y) > 0.001f && IsPositionValid(slideV, ENTITY_FOLLOWER, follower)) {
                               follower->creature->y = slideV.y;
                               follower->creature->anims.direction = (moveVector.y > 0) ? DOWN : UP;
                               follower->creature->anims.isMoving = true;
                            }
                            else {
                                // Face target when stuck - identical to enemy code
                                if (fabsf(dx) > fabsf(dy)) {
                                    follower->creature->anims.direction = (dx > 0) ? RIGHT : LEFT;
                                    follower->lastDirection = follower->creature->anims.direction; // Add this
                                } else {
                                    follower->creature->anims.direction = (dy > 0) ? DOWN : UP;
                                    follower->lastDirection = follower->creature->anims.direction; // Add this
                                }
                                follower->creature->anims.isMoving = false;
                            }
                        }
                    }
                }
            } else {
                // In attack range - stop moving and face creature (identical to enemy code)
                if (fabsf(dx) > fabsf(dy)) {
                    follower->creature->anims.direction = (dx > 0) ? RIGHT : LEFT;
                } else {
                    follower->creature->anims.direction = (dy > 0) ? DOWN : UP;
                }
                
                // Set animation state for combat stance - identical to enemy code
                follower->creature->anims.movementState = WALKING;
                follower->creature->anims.isMoving = false;
                follower->lastFrame = 0; // Use first frame as combat stance
            }
        }
        
        // Animation state management - identical to enemy code
        bool creatureMoved = (fabsf(dx) > 0.001f || fabsf(dy) > 0.001f);
        if (!creatureMoved) {
            // Check if ready to attack - identical to enemy code
            bool readyToAttack = (follower->attackCooldown <= 0 && distanceToTarget <= 1.2f);
            
            if (readyToAttack) {
                // Combat stance - identical to enemy code
                follower->creature->anims.movementState = WALKING;
                follower->lastFrame = 0;
                follower->creature->anims.isMoving = false;
                follower->creature->anims.isStopping = false;
            } else {
                // Stop animation transitions - identical to enemy code
                if (follower->creature->anims.isMoving) {
                    follower->creature->anims.isMoving = false;
                    follower->creature->anims.isStopping = true;
                    f->stopDelayCounter = 60; // Using global frameTracking
                } else if (follower->creature->anims.isStopping && 
                          f->stopDelayCounter > 0) {
                    f->stopDelayCounter--; // Using global frameTracking
                } else if (follower->creature->anims.isStopping) {
                    follower->creature->anims.movementState = IDLE;
                    follower->creature->anims.isStopping = false;
                }
            }
        }
    } 
    else {
        // No targets - fall back to following behavior
        int histIndex = (int)(historyIndex - (followerIndex + 1) * lag + MAX_HISTORY) % MAX_HISTORY;
        follower->creature->x = playerHistory[histIndex].x;
        follower->creature->y = playerHistory[histIndex].y;
        
        // Use player's direction and movement state
        follower->creature->anims.direction = mainTeam[0].anims.direction;
        follower->creature->anims.movementState = mainTeam[0].anims.movementState;
        follower->creature->anims.isMoving = mainTeam[0].anims.isMoving;
        
        // Add this line to sync lastDirection!
        follower->lastDirection = mainTeam[0].anims.direction;
    }
    
    // Animation frame updates
    UpdateFollowerAnimation(follower, f);
}

// Animation update helper
void UpdateFollowerAnimation(Follower* follower, FrameTracking* f) {
    if (follower->creature->anims.movementState == WALKING) {
        follower->followerFrameCounter++;
        
        int directionIndex = follower->creature->anims.direction;
        int maxFrames = follower->creature->anims.walkFrameCount[directionIndex];
        
        // Safety check
        if (maxFrames <= 0) maxFrames = 1;
        
        if (follower->followerFrameCounter >= FOLLOWER_FRAME_SPEED) {
            follower->lastFrame = (follower->lastFrame + 1) % maxFrames;
            follower->followerFrameCounter = 0;
        }
    } 
    else if (follower->creature->anims.movementState == IDLE) {
        follower->followerFrameCounter++;
        
        if (follower->followerFrameCounter >= f->idleFrameSpeed) { // Using the global frameTracking's idleFrameSpeed
            int maxFrames = follower->creature->anims.idleFrameCount;
            if (maxFrames <= 0) maxFrames = 1;
            
            follower->lastFrame = rand() % maxFrames;
            follower->followerFrameCounter = 0;
        }
    }
}

// Updated follower attack function with proper animation setup
void PerformFollowerAttack(Follower* follower, int followerIndex, int creatureIndex) {
    OverworldCreature* target = &realTimeBattleState.creatures[creatureIndex];
    
    // Face the target - identical to enemy code
    float dx = target->position.x - follower->creature->x;
    float dy = target->position.y - follower->creature->y;
    
    if (fabsf(dx) > fabsf(dy)) {
        follower->creature->anims.direction = (dx > 0) ? RIGHT : LEFT;
    } else {
        follower->creature->anims.direction = (dy > 0) ? DOWN : UP;
    }
    
    // Set follower as attacking
    follower->isAttacking = true;
    
    // Set up battle participants - identical to enemy code
    BattleParticipant attacker = {0};
    BattleParticipant defender = {0};
    
    attacker.creature = follower->creature;
    attacker.isPlayer = true;
    attacker.isActive = true;
    
    defender.creature = &target->creature;
    defender.isPlayer = false;
    defender.isActive = true;
    
    // Calculate hit chance - identical to enemy code
    bool didHit = WillItHit(attacker.creature, defender.creature);
    
    if (!didHit) {
        char message[100];
        sprintf(message, "%s's attack missed!", follower->creature->name);
        SetOverworldMessage(message);
        return;
    }
    
    // Calculate damage - identical to enemy code
    int baseDamage = attacker.creature->attack + attacker.creature->tempAttackBuff
                   - defender.creature->defense - defender.creature->tempDefenseBuff;
    baseDamage = MAX(baseDamage, 1);
    
    // Apply combo multiplier if active
    int damage = baseDamage;
    if (realTimeBattleState.comboCounter > 0) {
        damage = (int)(baseDamage * realTimeBattleState.comboMultiplier);
    }
    
    // Apply damage to target
    target->creature.currentHP -= damage;
    target->creature.currentHP = MAX(0, target->creature.currentHP);
    
    // Set damage display - identical to enemy code
    realTimeBattleState.creatureDamageDisplay[creatureIndex] = damage;
    realTimeBattleState.creatureDamageTimer[creatureIndex] = DAMAGE_TIMER;
    realTimeBattleState.creatureDamageOffsetY[creatureIndex] = 0.0f;
    realTimeBattleState.creatureDamageEffectType[creatureIndex] = EFFECT_OFFENSIVE;
    realTimeBattleState.creatureDamageRandomX[creatureIndex] = RANDOMS(0, TILE_SIZE);
    
    // Activate attack animation - but mark as follower attack by setting a special index
    realTimeBattleState.attackAnimationActive = true;
    animations.attack.isAnimating = true;
    animations.attack.currentFrame = 0;
    animations.attack.frameCounter = 0;
    realTimeBattleState.attackAnimationTargetCreature = creatureIndex;
    
    // IMPORTANT: Set attackingCreatureIndex to -2 to indicate a follower is attacking
    // -1 is used for player attacks, so we use -2 for follower attacks
    realTimeBattleState.attackingCreatureIndex = -2;
    
    // Store which follower is attacking to correctly position the animation
    realTimeBattleState.attackingFollowerIndex = followerIndex;
    
    // Apply screen shake - identical to enemy code
    realTimeBattleState.shakeTimer = 10;
    
    // Display message - identical to enemy code
    char message[100];
    sprintf(message, "%s dealt %d damage to %s!", 
            follower->creature->name, damage, target->creature.name);
    SetOverworldMessage(message);
    
    // Play attack sound - identical to enemy code
    PlaySoundByName(&soundManager, "attack", false);
    
    // Check if creature is defeated - identical to enemy code
    if (target->creature.currentHP <= 0) {
        // Calculate exp gain
        int expGain = target->creature.giveExp;
        
        // Distribute exp to party
        int playerExp = expGain / 2;
        int followerExp = expGain / (2 * followerCount);
        
        mainTeam[0].exp += playerExp;
        for (int i = 0; i < followerCount; i++) {
            followers[i].creature->exp += followerExp;
        }
        
        // Remove creature
        realTimeBattleState.creatures[creatureIndex].active = false;
        realTimeBattleState.creatureCount--;
        
        char defeatMessage[100];
        sprintf(defeatMessage, "%s defeated %s! Party gained %d EXP.",
                follower->creature->name, target->creature.name, expGain);
        SetOverworldMessage(defeatMessage);
    }
}



// Function to find the nearest creature to a specific follower
int FindNearestCreatureToFollower(int followerIndex) {
    if (followers[followerIndex].creature->currentHP <= 0) return -1;
    int nearestIndex = -1;
    float nearestDist = realTimeBattleState.engagementRadius * 1.5f; // Maximum search radius
    
    Follower* follower = &followers[followerIndex];
    
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (!realTimeBattleState.creatures[i].active) continue;
        
        // Calculate distance
        float dx = realTimeBattleState.creatures[i].position.x - follower->creature->x;
        float dy = realTimeBattleState.creatures[i].position.y - follower->creature->y;
        float dist = sqrtf(dx*dx + dy*dy);
        
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestIndex = i;
        }
    }
    
    return nearestIndex;
}

// Function to perform follower attack on a creature
// void PerformFollowerAttack(Follower* follower, int followerIndex, int creatureIndex) {
//     OverworldCreature* target = &realTimeBattleState.creatures[creatureIndex];
    
//     // Face the target
//     float dx = target->position.x - follower->creature->x;
//     float dy = target->position.y - follower->creature->y;
    
//     if (fabsf(dx) > fabsf(dy)) {
//         follower->creature->anims.direction = (dx > 0) ? RIGHT : LEFT;
//     } else {
//         follower->creature->anims.direction = (dy > 0) ? DOWN : UP;
//     }
    
//     // Set follower as attacking
//     follower->isAttacking = true;
    
//     // Calculate damage (simplified)
//     int damage = follower->creature->attack - target->creature.defense;
//     damage = MAX(damage, 1);
    
//     // Apply damage
//     target->creature.currentHP -= damage;
//     target->creature.currentHP = MAX(0, target->creature.currentHP);
    
//     // Display damage on creature
//     target->targetPlayerDamage = damage;
    
//     // Activate attack animation
//     realTimeBattleState.attackAnimationActive = true;
//     animations.attack.isAnimating = true;
//     animations.attack.currentFrame = 0;
//     animations.attack.frameCounter = 0;
//     realTimeBattleState.attackAnimationTargetCreature = creatureIndex;
    
//     // Apply screen shake
//     realTimeBattleState.shakeTimer = 5;
    
//     // Display message
//     char message[100];
//     sprintf(message, "%s dealt %d damage to %s!", 
//             follower->creature->name, damage, target->creature.name);
//     SetOverworldMessage(message);
    
//     // Play attack sound
//     PlaySoundByName(&soundManager, "attack", false);
    
//     // Check if creature is defeated
//     if (target->creature.currentHP <= 0) {
//         // Calculate exp gain
//         int expGain = target->creature.giveExp;
        
//         // Distribute exp to party
//         // Half to player, half split among followers
//         int playerExp = expGain / 2;
//         int followerExp = expGain / (2 * followerCount);
        
//         mainTeam[0].exp += playerExp;
//         for (int i = 0; i < followerCount; i++) {
//             followers[i].creature->exp += followerExp;
//         }
        
//         // Remove creature
//         realTimeBattleState.creatures[creatureIndex].active = false;
//         realTimeBattleState.creatureCount--;
        
//         char defeatMessage[100];
//         sprintf(defeatMessage, "%s defeated %s! Party gained %d EXP.",
//                 follower->creature->name, target->creature.name, expGain);
//         SetOverworldMessage(defeatMessage);
//     }
// }


void ApplyFollowerBehavior(AISubmenuOption behavior) {
    // Set current global behavior
    currentFollowerBehavior = behavior;
    
    // Apply to all followers
    for (int i = 0; i < followerCount; i++) {
        followers[i].behavior = currentFollowerBehavior;
    }
    
    // Show message about behavior change
    if (currentFollowerBehavior == FOLLOWER_BEHAVIOR_AGGRESSIVE) {
        SetOverworldMessage("Followers set to AGGRESSIVE mode!");
    } else {
        SetOverworldMessage("Followers set to FOLLOW mode!");
    }
    
    // Consume command energy when changing AI mode
    ConsumeCommandEnergy(0.5f); // Lower cost than attacks
}

// Unified function to check if a position is valid for movement
bool IsPositionValid(Vector2 position, EntityType excludeType, void* excludeEntity) {
    // 1. Check map boundaries
    if (position.x < 0 || position.x >= mapWidth || 
        position.y < 0 || position.y >= mapHeight) {
        return false;
    }
    
    // 2. Check if tile is walkable (use current tool)
    if (!IsTileWalkable((int)position.x, (int)position.y, 
                        &toolWheel.tools[toolWheel.selectedToolIndex])) {
        return false;
    }
    
    // 3. Check for entity overlaps
    // 3a. Check creatures (unless this is a creature)
    if (excludeType != ENTITY_CREATURE) {
        for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
            if (!realTimeBattleState.creatures[i].active) continue;
            if (excludeType == ENTITY_CREATURE && excludeEntity == &realTimeBattleState.creatures[i]) continue;
            
            float dx = realTimeBattleState.creatures[i].position.x - position.x;
            float dy = realTimeBattleState.creatures[i].position.y - position.y;
            float distance = sqrtf(dx * dx + dy * dy);
            
            if (distance < 0.8f) {
                return false;
            }
        }
    }
    
    // 3b. Check player (unless this is the player)
    if (excludeType != ENTITY_PLAYER) {
        float dx = mainTeam[0].x - position.x;
        float dy = mainTeam[0].y - position.y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance < 0.8f) {
            return false;
        }
    }
    
    // 3c. Check followers (unless this is a follower)
    if (excludeType != ENTITY_FOLLOWER) {
        for (int i = 0; i < followerCount; i++) {
            if (followers[i].creature->currentHP <= 0) continue;
            if (excludeType == ENTITY_FOLLOWER && excludeEntity == &followers[i]) continue;
            
            float dx = followers[i].creature->x - position.x;
            float dy = followers[i].creature->y - position.y;
            float distance = sqrtf(dx * dx + dy * dy);
            
            if (distance < 0.8f) {
                return false;
            }
        }
    }
    
    return true; // All checks passed
}


// Modify UpdateCommandQueue for better execution and queue removal
void UpdateCommandQueue(RealTimeBattleState* rtbsState) {
    float deltaTime = GetFrameTime();

    // Update energy recharge (same as before)
    float adjustedRechargeRate = rtbsState->rechargeRate * (1.0f + rtbsState->rechargeRateBonus);
    rtbsState->commandEnergy += adjustedRechargeRate * deltaTime;
    if (rtbsState->commandEnergy > rtbsState->maxCommandEnergy) {
        rtbsState->commandEnergy = rtbsState->maxCommandEnergy;
    }

    // If we are in the process of executing commands
    if (rtbsState->isExecutingCommandQueue) {
        // Update the timer for next command execution
        rtbsState->commandExecutionTimer -= deltaTime;

        // Check if it's time to execute AND if there are commands left in the queue
        if (rtbsState->commandExecutionTimer <= 0 && rtbsState->queueCount > 0) {
            // Get the command at the front of the queue
            QueuedCommand* cmd = &rtbsState->commandQueue[0];

            // Check if we have enough energy for this command
            if (rtbsState->commandEnergy >= cmd->energyCost) {
                // Choose an unused approach direction
                ApproachDirection direction = GetUnusedDirection(rtbsState);

                // Execute the command
                ExecuteCommandWithApproach(rtbsState, cmd, direction);

                // Mark the direction as used for this sequence
                rtbsState->usedDirections[direction] = true;

                // Consume energy upon execution
                rtbsState->commandEnergy -= cmd->energyCost;

                // --- Remove the executed command and shift the queue ---
                rtbsState->queueCount--; // Decrement count first
                if (rtbsState->queueCount > 0) {
                    // Shift remaining elements down using memmove
                    // memmove is safer than memcpy for overlapping memory regions
                    memmove(&rtbsState->commandQueue[0],        // Destination: start of the queue
                            &rtbsState->commandQueue[1],        // Source: element after the one executed
                            sizeof(QueuedCommand) * rtbsState->queueCount); // Size: remaining elements
                }
                // --- End of removal and shifting ---

                // Set cooldown for the *next* potential command
                // Use faster cooldown if more commands *were* queued before removal
                 if (rtbsState->queueCount > 0) { // Check if any commands remain
                    rtbsState->commandExecutionTimer = 0.4f; // Faster execution for subsequent commands
                 } else {
                    rtbsState->commandExecutionTimer = 0.6f; // Normal cooldown if this was the last one
                 }

            } else {
                // Not enough energy, wait and check again soon
                rtbsState->commandExecutionTimer = 0.1f;
                // Don't remove the command yet, we need to wait for energy
            }
        }

        // Check if the queue is now empty
        if (rtbsState->queueCount <= 0 && rtbsState->isExecutingCommandQueue) {
            // Finished executing all commands
            rtbsState->isExecutingCommandQueue = false;

            // Reset used directions for the next sequence
            for (int i = 0; i < APPROACH_COUNT; i++) {
                rtbsState->usedDirections[i] = false;
            }
            rtbsState->lastApproachDirection = APPROACH_COUNT; // Reset last direction here
             // Reset combo decay timer here if needed, or handle below
        }
    }
    // Not currently executing, check if we should start
    else if (rtbsState->queueCount > 0) { // Check if there are commands waiting
         // Check if we have energy for the *first* command in the queue
         if(rtbsState->commandEnergy >= rtbsState->commandQueue[0].energyCost) {
            // Start executing the sequence
            rtbsState->isExecutingCommandQueue = true;
            rtbsState->commandExecutionTimer = 0.1f; // Small delay before first attack

            // Reset direction tracking
            for (int i = 0; i < APPROACH_COUNT; i++) {
                rtbsState->usedDirections[i] = false;
            }
        }
    }
    // Handle combo decay when idle (no commands queued or executing)
    else if (rtbsState->comboCounter > 0) { // Only decay if queue is empty and not executing
        static float comboDecayTimer = 0.0f;
        comboDecayTimer += deltaTime;

        if (comboDecayTimer > 3.0f) { // 3 seconds of inactivity
            if (rtbsState->comboCounter > 0) { // Ensure it doesn't go negative
                 rtbsState->comboCounter--;
                 UpdateComboBonuses(rtbsState);
            }
            comboDecayTimer = 0.0f; // Reset timer after decay
        }
         // Reset timer if an attack just happened (handled elsewhere, e.g., in ExecuteCommand)
         // Or maybe reset comboDecayTimer = 0.0f whenever isExecutingCommandQueue becomes true?

    } else {
         // Ensure combo decay timer is reset if combo is 0
         // static float comboDecayTimer = 0.0f; // Declared above
         // comboDecayTimer = 0.0f; // Reset if combo is 0 and idle
    }
}


// Helper function to get an unused direction, strongly avoiding the last used one.
ApproachDirection GetUnusedDirection(RealTimeBattleState* rtbsState) {
    // --- Check if all directions have been used ---
    bool allUsed = true;
    int availableCount = 0;
    for (int i = 0; i < APPROACH_COUNT; i++) {
        if (!rtbsState->usedDirections[i]) {
            allUsed = false;
            availableCount++;
        }
    }

    // --- If all used, reset everything for the next cycle ---
    if (allUsed) {
        availableCount = APPROACH_COUNT; // All are now available
        for (int i = 0; i < APPROACH_COUNT; i++) {
            rtbsState->usedDirections[i] = false;
        }
        // Reset last direction since we are starting a fresh cycle
        // Keep lastApproachDirection as it was, so the *first* attack of the new cycle avoids it if possible.
        // Or reset it if you want the first attack of a new cycle to be truly random:
        // rtbsState->lastApproachDirection = APPROACH_COUNT;
        printf("All directions used, resetting cycle.\n"); // Debug log
    }

    // --- Build list of currently available (unused) directions ---
    ApproachDirection availableDirections[APPROACH_COUNT];
    int currentAvailableCount = 0;
    for (int i = 0; i < APPROACH_COUNT; i++) {
        if (!rtbsState->usedDirections[i]) {
            availableDirections[currentAvailableCount++] = (ApproachDirection)i;
        }
    }

    // --- Select the next direction ---
    ApproachDirection selectedDirection;

    if (currentAvailableCount == 0) {
        // Should not happen if reset logic above works
        printf("Warning: GetUnusedDirection called with no available directions!\n");
        selectedDirection = APPROACH_FRONT; // Fallback
    } else if (currentAvailableCount == 1) {
        // Only one option left, must choose it
        selectedDirection = availableDirections[0];
        printf("GetUnusedDirection: Only one option left: %d\n", selectedDirection); // Debug log
    } else {
        // Multiple options available. Try to pick one different from the last.
        int attempts = 0;
        do {
            int randomIndex = RANDOMS(0, currentAvailableCount - 1);
            selectedDirection = availableDirections[randomIndex];
            attempts++;
            // Keep picking if it's the same as the last one AND there are other options to try
        } while (selectedDirection == rtbsState->lastApproachDirection && attempts < 5); // Limit attempts to prevent infinite loop

        // If after 5 attempts we still only picked the last one, just use it.
        // This handles the edge case where randomness keeps hitting the same value,
        // although statistically unlikely with few options.
         printf("GetUnusedDirection: Picked %d (last was %d) after %d attempts.\n",
               selectedDirection, rtbsState->lastApproachDirection, attempts); // Debug log
    }

    // --- Update state before returning ---
    // Remember the selected direction as the last one used *for the next call*
    // Note: The 'usedDirections' flag for this selectedDirection is set later in UpdateCommandQueue after execution.
    rtbsState->lastApproachDirection = selectedDirection;

    return selectedDirection;
}



// Execute a single command
void ExecuteCommand(RealTimeBattleState* rtbsState, QueuedCommand* cmd) {
    switch (cmd->type) {
        case RTBS_COMMAND_ATTACK:
            if (cmd->targetIndex >= 0 && 
                cmd->targetIndex < MAX_RTBS_CREATURES && 
                rtbsState->creatures[cmd->targetIndex].active) {
                
                // Apply combo multiplier to damage (modify attack function)
                rtbsState->comboMultiplier = 1.0f + (rtbsState->comboCounter * 0.1f);
                if (rtbsState->comboMultiplier > 2.0f) rtbsState->comboMultiplier = 2.0f;
                
                // Perform attack with combo multiplier
                PerformPlayerAttack(rtbsState, cmd->targetIndex);
                
                // Increment combo counter
                rtbsState->comboCounter++;
                UpdateComboBonuses(rtbsState);
                
                char message[100];
                sprintf(message, "Combo x%d! Attack executed!", rtbsState->comboCounter);
                SetOverworldMessage(message);
            }
            break;
            
        // Add cases for other command types
        default:
            break;
    }
    
    // Play combo sound (gets more intense with higher combos)
    if (rtbsState->comboCounter <= 1) {
        PlaySoundByName(&soundManager, "attack", false);
    } else if (rtbsState->comboCounter <= 3) {
        PlaySoundByName(&soundManager, "comboAttack", false);
    } else {
        PlaySoundByName(&soundManager, "superComboAttack", false);
    }
}

void ExecuteAvailableCommands(RealTimeBattleState* rtbsState) {
    // Don't wait for energy, just start the sequence execution
    if (rtbsState->queueCount > 0) {
        rtbsState->isExecutingCommandQueue = true;
        rtbsState->currentQueueIndex = 0;
        rtbsState->commandExecutionTimer = 0.0f; // Execute first command immediately
        
        DisableTargeting(&commandMenuState, rtbsState);        
    }
}

// Update bonuses based on combo counter
void UpdateComboBonuses(RealTimeBattleState* rtbsState) {
    // Damage multiplier (capped at 2.0x)
    rtbsState->comboMultiplier = 1.0f + (rtbsState->comboCounter * 0.1f);
    if (rtbsState->comboMultiplier > 2.0f) {
        rtbsState->comboMultiplier = 2.0f;
    }
    
    // Range bonus (capped at +3 tiles)
    rtbsState->rangeBonus = rtbsState->comboCounter * 0.2f;
    if (rtbsState->rangeBonus > 3.0f) {
        rtbsState->rangeBonus = 3.0f;
    }
    
    // Recharge rate bonus (capped at +50%)
    rtbsState->rechargeRateBonus = rtbsState->comboCounter * 0.05f;
    if (rtbsState->rechargeRateBonus > 0.5f) {
        rtbsState->rechargeRateBonus = 0.5f;
    }
}

void DrawCommandQueue(RealTimeBattleState* rtbsState, Vector2 viewport) {
    int iconSize = 40;
    int padding = 5;
    int queueWidth = (iconSize + padding*2) * MAX_COMMAND_QUEUE + padding*2;
    int queueHeight = iconSize + padding*4 + 30; // Increased for help text
    
    // Position at bottom right with margin
    int queueX = SCREEN_WIDTH - queueWidth - 20; // 20px margin from right edge
    int queueY = SCREEN_HEIGHT - queueHeight - 20; // 20px margin from bottom edge
    
    // Draw main background
    DrawRectangle(queueX, queueY, queueWidth, queueHeight, Fade(BLACK, 0.7f));
    DrawRectangleLines(queueX, queueY, queueWidth, queueHeight, WHITE);
    
    // Draw header text (no combo info here - it will be above player)
    DrawText("COMMAND QUEUE", queueX + 10, queueY + 5, 16, YELLOW);
    
    // Draw command slots
    for (int i = 0; i < MAX_COMMAND_QUEUE; i++) {
        int slotX = queueX + padding*2 + i * (iconSize + padding*2);
        int slotY = queueY + 25;
        
        // Draw slot background 
        Color slotColor = (i < rtbsState->queueCount) ? Fade(DARKGRAY, 0.8f) : Fade(GRAY, 0.3f);
        DrawRectangle(slotX, slotY, iconSize, iconSize, slotColor);
        DrawRectangleLines(slotX, slotY, iconSize, iconSize, WHITE);
        
        // If slot has a command, draw command info
        if (i < rtbsState->queueCount) {
            QueuedCommand* cmd = &rtbsState->commandQueue[i];
            
            // Draw command icon/name
            Color cmdColor = WHITE;
            const char* cmdText = "?";
            
            switch (cmd->type) {
                case RTBS_COMMAND_ATTACK:
                    cmdColor = RED;
                    cmdText = "ATK";
                    break;
                default: break;
                // Add other command types
            }
            
            DrawText(cmdText, slotX + 8, slotY + 12, 16, cmdColor);
            
            // Highlight currently executing command
            if (rtbsState->isExecutingCommandQueue && i == rtbsState->currentQueueIndex) {
                // Draw highlight around active command
                DrawRectangleLines(slotX - 2, slotY - 2, iconSize + 4, iconSize + 4, GOLD);
                
                // Add a pulsing effect
                float pulse = sinf(GetTime() * 10.0f) * 0.3f + 0.7f;
                DrawRectangle(slotX, slotY, iconSize, iconSize, Fade(GOLD, 0.2f * pulse));
            }
        } else {
            // Empty slot
            DrawText("-", slotX + 15, slotY + 10, 20, GRAY);
        }
    }
    
    // Draw compact help text
    DrawText("Z:Queue | X:Remove | Hold-X:Keep | Hold-Z:Execute", 
             queueX + 10, queueY + queueHeight - padding - 12, 10, LIGHTGRAY);
    
    // Draw energy bar separately at bottom right
    int energyBarX = SCREEN_WIDTH - 220;
    int energyBarY = SCREEN_HEIGHT - 15;
    int energyBarWidth = 200;
    int energyBarHeight = 8;
    
    // Draw energy background
    DrawRectangle(energyBarX - 5, energyBarY - 5, energyBarWidth + 10, energyBarHeight + 10, Fade(BLACK, 0.7f));
    
    // Draw energy label
    DrawText("ENERGY", energyBarX - 55, energyBarY - 1, 12, YELLOW);
    
    // Draw background for whole bar
    DrawRectangle(energyBarX, energyBarY, energyBarWidth, energyBarHeight, DARKGRAY);
    
    // Draw filled portion
    float energyRatio = rtbsState->commandEnergy / rtbsState->maxCommandEnergy;
    int filledWidth = (int)(energyBarWidth * energyRatio);
    // Color energyColor = rtbsState->canUseCommand ? GOLD : ORANGE;
    Color energyColor;
    if (energyRatio < 0.25) energyColor = RED;
    else if (energyRatio == 1) energyColor = GREEN;
    else energyColor = GOLD;
    DrawRectangle(energyBarX, energyBarY, filledWidth, energyBarHeight, energyColor);
    
    // Draw segments showing command costs
    for (int i = 1; i < MAX_COMMAND_QUEUE; i++) {
        float segmentX = energyBarX + (energyBarWidth * i / MAX_COMMAND_QUEUE);
        DrawRectangleLines(segmentX, energyBarY, 1, energyBarHeight, WHITE);
    }
    
    // Draw numeric energy value
    char energyText[20];
    sprintf(energyText, "%.1f/%.1f", rtbsState->commandEnergy, rtbsState->maxCommandEnergy);
    DrawText(energyText, energyBarX + energyBarWidth + 10, energyBarY - 2, 12, YELLOW);
}



// New function to execute commands with different approach directions
void ExecuteCommandWithApproach(RealTimeBattleState* rtbsState, QueuedCommand* cmd, ApproachDirection direction) {
    // Add visual flash effect when attack connects
    rtbsState->flashEffectTimer = 0.2f;
    rtbsState->flashEffectColor = YELLOW;
    rtbsState->lastAttackTime = GetTime();

    // In DrawRealTimeBattle:
    // Draw attack flash effect
    if (rtbsState->flashEffectTimer > 0) {
        rtbsState->flashEffectTimer -= GetFrameTime();
        
        // Flash the whole screen briefly
        float alpha = rtbsState->flashEffectTimer / 0.2f * 0.2f;
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                    Fade(rtbsState->flashEffectColor, alpha));
    }
    
    if (cmd->type == RTBS_COMMAND_ATTACK && 
        cmd->targetIndex >= 0 && 
        cmd->targetIndex < MAX_RTBS_CREATURES && 
        rtbsState->creatures[cmd->targetIndex].active) {
        
        // Apply combo multiplier to damage
        rtbsState->comboMultiplier = 1.0f + (rtbsState->comboCounter * 0.1f);
        if (rtbsState->comboMultiplier > 2.0f) rtbsState->comboMultiplier = 2.0f;
        
        // Perform attack with combo multiplier and approach direction
        PerformPlayerAttackWithDirection(rtbsState, cmd->targetIndex, direction);
        
        // Increment combo counter
        rtbsState->comboCounter++;
        UpdateComboBonuses(rtbsState);
        
        char message[100];
        sprintf(message, "Combo x%d! Attack executed!", rtbsState->comboCounter);
        SetOverworldMessage(message);
        
        // Play combo sound based on combo level
        if (rtbsState->comboCounter <= 1) {
            PlaySoundByName(&soundManager, "attack", false);
        } else if (rtbsState->comboCounter <= 3) {
            PlaySoundByName(&soundManager, "comboAttack", false);
        } else {
            PlaySoundByName(&soundManager, "superComboAttack", false);
        }
    }
}


void PerformPlayerAttack(RealTimeBattleState* rtbsState, int targetCreatureIndex) {
    OverworldCreature* targetCreature = &rtbsState->creatures[targetCreatureIndex];
    
    // Move closer to target if needed
    MovePlayerTowardTarget(rtbsState, targetCreatureIndex);
    
    // Set up battle participants
    BattleParticipant attacker = {0};
    BattleParticipant defender = {0};
    
    // Set up attacker (player)
    attacker.creature = &mainTeam[0];
    attacker.isPlayer = true;
    attacker.isActive = true;
    
    // Set up defender (creature)
    defender.creature = &targetCreature->creature;
    defender.isPlayer = false;
    defender.isActive = true;
    
    // Calculate hit chance
    bool didHit = WillItHit(attacker.creature, defender.creature);
    
    if (!didHit) {
        rtbsState->comboCounter = 0;
        UpdateComboBonuses(rtbsState);
        SetOverworldMessage("Attack missed! Combo reset!");
        return;
    }
    
    // Calculate damage
    int baseDamage = attacker.creature->attack + attacker.creature->tempAttackBuff
              - defender.creature->defense - defender.creature->tempDefenseBuff;
    baseDamage = MAX(baseDamage, 1); // or 0?

    // Apply combo multiplier
    int damage = (int)(baseDamage * rtbsState->comboMultiplier);
    
    // Apply damage
    targetCreature->creature.currentHP -= damage;
    targetCreature->creature.currentHP = CLAMP(targetCreature->creature.currentHP, 
                                         0, targetCreature->creature.maxHP);
    
    // Set damage display
    targetCreature->targetPlayerDamage = damage;

    rtbsState->creatureDamageDisplay[targetCreatureIndex] = damage;
    rtbsState->creatureDamageTimer[targetCreatureIndex] = DAMAGE_TIMER;
    rtbsState->creatureDamageOffsetY[targetCreatureIndex] = 0.0f;
    rtbsState->creatureDamageEffectType[targetCreatureIndex] = EFFECT_OFFENSIVE;
    rtbsState->creatureDamageRandomX[targetCreatureIndex] = RANDOMS(0, TILE_SIZE);
    
    // Activate attack animation
    animations.attack.isAnimating = true;
    animations.attack.currentFrame = 0;
    animations.attack.frameCounter = 0;
    rtbsState->attackAnimationActive = true;
    rtbsState->attackAnimationTargetCreature = targetCreatureIndex;
    rtbsState->attackingCreatureIndex = -1;  // Add this line
    
    // Apply screen shake
    rtbsState->shakeTimer = 10;
    
    // Display message
    char message[100];
    sprintf(message, "You dealt %d damage to %s!", damage, targetCreature->creature.name);
    SetOverworldMessage(message);
    
    // Play sound
    PlaySoundByName(&soundManager, "attack", false);
    
    // Check for defeat
    if (targetCreature->creature.currentHP <= 0) {
        targetCreature->creature.currentHP = 0;
        
        // Process experience and removal
        int expGain = targetCreature->creature.giveExp;
        mainTeam[0].exp += expGain;
        
        // Track enemy defeated
        TrackEnemyDefeated(targetCreature->creature.id);
        
        // Check for level up (using updated system)
        int expForNextLevel = GetExpForNextGridLevel(mainTeam[0].totalGridLevel);
        if (mainTeam[0].exp >= expForNextLevel) {
            mainTeam[0].exp -= expForNextLevel;
            mainTeam[0].gridLevel++;
            mainTeam[0].totalGridLevel++;
            
            // Update available movement points in the grid
            FriendshipGrid* grid = GetCurrentGrid();
            if (grid) {
                grid->availablePoints = mainTeam[0].gridLevel;
            }
            
            SetOverworldMessage("Level up!");
            PlaySoundByName(&soundManager, "levelUp", true);
        }
        
        // Remove creature
        rtbsState->creatures[targetCreatureIndex].active = false;
        rtbsState->creatureCount--;
        
        char defeatMessage[100];
        sprintf(defeatMessage, "%s defeated! Gained %d AP.", 
                targetCreature->creature.name, expGain);
        SetOverworldMessage(defeatMessage);
    }
}

// Enhanced attack function with multiple level-up handling for all party members
void PerformPlayerAttackWithDirection(RealTimeBattleState* rtbsState, int targetCreatureIndex, ApproachDirection direction) {
    OverworldCreature* targetCreature = &rtbsState->creatures[targetCreatureIndex];
   
    // Move closer to target from the specified direction
    MovePlayerTowardTargetWithDirection(rtbsState, targetCreatureIndex, direction);
   
    // Handle the attack (similar to original PerformPlayerAttack)
    // Set up battle participants
    BattleParticipant attacker = {0};
    BattleParticipant defender = {0};
   
    // Setup attacker and defender
    attacker.creature = &mainTeam[0];
    attacker.isPlayer = true;
    attacker.isActive = true;
   
    defender.creature = &targetCreature->creature;
    defender.isPlayer = false;
    defender.isActive = true;
   
    // Calculate hit chance
    bool didHit = WillItHit(attacker.creature, defender.creature);
   
    if (!didHit) {
        // Break combo on miss
        rtbsState->comboCounter = 0;
        UpdateComboBonuses(rtbsState);
        SetOverworldMessage("Attack missed! Combo reset!");
        return;
    }
   
    // Calculate damage with combo multiplier
    int baseDamage = attacker.creature->attack + attacker.creature->tempAttackBuff
                   - defender.creature->defense - defender.creature->tempDefenseBuff;
    baseDamage = MAX(baseDamage, 1);
   
    // Apply combo multiplier
    int damage = (int)(baseDamage * rtbsState->comboMultiplier);
   
    // Apply damage
    targetCreature->creature.currentHP -= damage;
    targetCreature->creature.currentHP = CLAMP(targetCreature->creature.currentHP,
                                          0, targetCreature->creature.maxHP);
   
    // Set damage display
    targetCreature->targetPlayerDamage = damage;
    rtbsState->creatureDamageDisplay[targetCreatureIndex] = damage;
    rtbsState->creatureDamageTimer[targetCreatureIndex] = DAMAGE_TIMER;
    rtbsState->creatureDamageOffsetY[targetCreatureIndex] = 0.0f;
    rtbsState->creatureDamageEffectType[targetCreatureIndex] = EFFECT_OFFENSIVE;
    rtbsState->creatureDamageRandomX[targetCreatureIndex] = RANDOMS(0, TILE_SIZE);
   
    // Activate attack animation
    animations.attack.isAnimating = true;
    animations.attack.currentFrame = 0;
    animations.attack.frameCounter = 0;
    rtbsState->attackAnimationActive = true;
    rtbsState->attackAnimationTargetCreature = targetCreatureIndex;
   
    // Apply screen shake
    rtbsState->shakeTimer = 10;
    
    // Display message
    char message[100];
    sprintf(message, "You dealt %d damage to %s!", damage, targetCreature->creature.name);
    SetOverworldMessage(message);
    
    // Play sound
    PlaySoundByName(&soundManager, "attack", false);
   
    // Check for defeat
    if (targetCreature->creature.currentHP <= 0) {
        targetCreature->creature.currentHP = 0;
    
        // Process experience
        int expGain = targetCreature->creature.giveExp;
        
        // Count alive party members (mainTeam[0] + followers)
        int alivePartyMembers = 1; // Start with mainTeam[0]
        for (int i = 0; i < followerCount; i++) {
            if (followers[i].creature->currentHP > 0) {
                alivePartyMembers++;
            }
        }
        
        // Calculate EXP per party member
        int expPerMember = expGain / alivePartyMembers;
        
        // Award EXP to mainTeam[0]
        mainTeam[0].exp += expPerMember;
        
        // IMPROVED: Level up check loop for mainTeam[0]
        bool playerLeveledUp = false;
        while (true) {
            int expForNextLevel = GetExpForNextGridLevel(mainTeam[0].totalGridLevel);
            if (mainTeam[0].exp >= expForNextLevel) {
                mainTeam[0].exp -= expForNextLevel;
                mainTeam[0].gridLevel++;
                mainTeam[0].totalGridLevel++;
                playerLeveledUp = true;
                
                // Update available movement points in the grid
                FriendshipGrid* grid = GetCurrentGrid();
                if (grid) {
                    grid->availablePoints = mainTeam[0].gridLevel;
                }
            } else {
                break; // Exit loop when not enough EXP for next level
            }
        }
        
        // Show level up message only once if player leveled up
        if (playerLeveledUp) {
            SetOverworldMessage("Level up!");
            PlaySoundByName(&soundManager, "levelUp", true);
        }
        
        // IMPROVED: Award EXP to followers with level up loop for each
        for (int i = 0; i < followerCount; i++) {
            if (followers[i].creature->currentHP > 0) {
                followers[i].creature->exp += expPerMember;
                
                // Check for multiple follower level ups in a loop
                bool followerLeveledUp = false;
                while (true) {
                    int expForNextLevel = GetExpForNextGridLevel(followers[i].creature->totalGridLevel);
                    if (followers[i].creature->exp >= expForNextLevel) {
                        followers[i].creature->exp -= expForNextLevel;
                        followers[i].creature->gridLevel++;
                        followers[i].creature->totalGridLevel++;
                        followerLeveledUp = true;
                        
                        // Also need to update the corresponding mainTeam creature
                        for (int j = 1; j < MAX_PARTY_SIZE; j++) {
                            if (strcmp(followers[i].creature->name, mainTeam[j].name) == 0) {
                                mainTeam[j].exp = followers[i].creature->exp;
                                mainTeam[j].gridLevel = followers[i].creature->gridLevel;
                                mainTeam[j].totalGridLevel = followers[i].creature->totalGridLevel;
                                break;
                            }
                        }
                    } else {
                        break; // Exit loop when not enough EXP
                    }
                }
                
                // Only show follower level up message if they actually leveled up
                if (followerLeveledUp) {
                    char levelUpMsg[100];
                    sprintf(levelUpMsg, "%s leveled up!", followers[i].creature->name);
                    SetOverworldMessage(levelUpMsg);
                }
            }
        }
        
        // Track enemy defeated
        TrackEnemyDefeated(targetCreature->creature.id);
        
        // Sync stats between team and followers
        SyncTeamStats();
        
        // Remove creature
        rtbsState->creatures[targetCreatureIndex].active = false;
        rtbsState->creatureCount--;
        
        char defeatMessage[100];
        sprintf(defeatMessage, "%s defeated! Party gained %d AP.",
                targetCreature->creature.name, expGain);
        SetOverworldMessage(defeatMessage);
    }
}

// Helper to get the offset vector FROM creature TO desired player position
Vector2 GetApproachPositionOffset(ApproachDirection approachDir, Direction creatureFacingDir) {
    Vector2 offset = {0, 0};
    Vector2 creatureLeft = {0, 0}, creatureRight = {0, 0}, creatureBack = {0, 0}, creatureFront = {0,0};

    // Determine relative directions based on creature facing
    switch (creatureFacingDir) {
        // Assumes Screen Coords: +X Right, +Y Down
        case UP:    creatureLeft = (Vector2){-1, 0}; creatureRight = (Vector2){1, 0}; creatureBack = (Vector2){0, 1}; creatureFront = (Vector2){0,-1}; break;
        case DOWN:  creatureLeft = (Vector2){1, 0};  creatureRight = (Vector2){-1, 0}; creatureBack = (Vector2){0, -1}; creatureFront = (Vector2){0,1}; break;
        case LEFT:  creatureLeft = (Vector2){0, 1};  creatureRight = (Vector2){0, -1}; creatureBack = (Vector2){1, 0}; creatureFront = (Vector2){-1,0}; break;
        case RIGHT: creatureLeft = (Vector2){0, -1}; creatureRight = (Vector2){0, 1}; creatureBack = (Vector2){-1, 0}; creatureFront = (Vector2){1,0}; break;
    }

    // Determine the offset vector needed to reach the approach position
    switch (approachDir) {
        case APPROACH_FRONT: offset = creatureFront; break; // Player ends up in front of creature
        case APPROACH_BACK:  offset = creatureBack; break;  // Player ends up behind creature
        case APPROACH_LEFT:  offset = creatureLeft; break;  // Player ends up on creature's left
        case APPROACH_RIGHT: offset = creatureRight; break; // Player ends up on creature's right
        default: offset = creatureFront; break; // Default
    }
    // No need to normalize, should be unit vectors already
    return offset;
}

void MovePlayerTowardTargetWithDirection(RealTimeBattleState* rtbsState, int targetCreatureIndex, ApproachDirection direction) {
    OverworldCreature* targetCreature = &rtbsState->creatures[targetCreatureIndex];
    Vector2 targetPos = targetCreature->position;
    Direction creatureFacing = targetCreature->creature.anims.direction;

    Vector2 offset = {0, 0};

    // Special Case: APPROACH_FRONT remains relative to the player for a direct charge-in feel.
    if (direction == APPROACH_FRONT) {
        float dx = mainTeam[0].x - targetPos.x;
        float dy = mainTeam[0].y - targetPos.y;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.001f) {
            offset.x = dx / len;
            offset.y = dy / len;
        } else {
            offset = (Vector2){0, -1}; // Default if overlapping
        }
    } else {
        // BACK, LEFT, RIGHT are relative to the creature's facing direction
        offset = GetApproachPositionOffset(direction, creatureFacing);
    }

    // Calculate the desired final player position (1 unit away from target along offset)
    // The offset vector points FROM the creature TO the player's target spot
    float desiredX = targetPos.x + offset.x * 1.0f;
    float desiredY = targetPos.y + offset.y * 1.0f;

    // --- (Rest of the function: check validity, move player, update history) ---
    // Boundary checks
    if (desiredX < 0) desiredX = 0;
    if (desiredY < 0) desiredY = 0;
    if (desiredX >= mapWidth) desiredX = mapWidth - 1;
    if (desiredY >= mapHeight) desiredY = mapHeight - 1;

    // Simple Walkable Check (You might need more robust collision/pathing)
    if (!IsTileWalkable((int)desiredX, (int)desiredY, &toolWheel.tools[toolWheel.selectedToolIndex])) {
         // Fallback if desired spot is invalid: Try to find nearest valid adjacent tile to desired spot?
         // Or simpler: just don't move if target tile is bad.
         printf("Warning: Desired approach position (%f, %f) for dir %d is not walkable.\n", desiredX, desiredY, direction);
         desiredX = mainTeam[0].x; // Stay put
         desiredY = mainTeam[0].y;
    }

    // TODO: Add checks for collision with other entities at desiredX, desiredY?

    // --- Move Player (Instantaneous for now) ---
    if (fabsf(mainTeam[0].x - desiredX) > 0.1f || fabsf(mainTeam[0].y - desiredY) > 0.1f) {
        mainTeam[0].x = desiredX;
        mainTeam[0].y = desiredY;

        // Update player history for followers ONLY if player actually moved
        historyIndex = (historyIndex + 1) % MAX_HISTORY;
        playerHistory[historyIndex].x = mainTeam[0].x;
        playerHistory[historyIndex].y = mainTeam[0].y;
    }

    // --- Update Player Facing ---
    // Always face the target creature after moving
    float faceDx = targetPos.x - mainTeam[0].x;
    float faceDy = targetPos.y - mainTeam[0].y;
    if (fabsf(faceDx) > 0.01 || fabsf(faceDy) > 0.01) { // Check magnitude to avoid jitter
        if (fabsf(faceDx) > fabsf(faceDy)) {
            mainTeam[0].anims.direction = (faceDx > 0) ? RIGHT : LEFT;
        } else {
            mainTeam[0].anims.direction = (faceDy > 0) ? DOWN : UP;
        }
    }
}


// helper function to draw white text with black boarder
// Draw text with outline by rendering the same text in 8 directions
void DrawTextWithOutline(const char* text, int posX, int posY, int fontSize, Color textColor, Color outlineColor) {
    // Draw outline first (in 8 directions)
    DrawText(text, posX - 1, posY - 1, fontSize, outlineColor);
    DrawText(text, posX - 1, posY,     fontSize, outlineColor);
    DrawText(text, posX - 1, posY + 1, fontSize, outlineColor);
    DrawText(text, posX,     posY - 1, fontSize, outlineColor);
    DrawText(text, posX,     posY + 1, fontSize, outlineColor);
    DrawText(text, posX + 1, posY - 1, fontSize, outlineColor);
    DrawText(text, posX + 1, posY,     fontSize, outlineColor);
    DrawText(text, posX + 1, posY + 1, fontSize, outlineColor);
    
    // Draw text in the center
    DrawText(text, posX, posY, fontSize, textColor);
}


void SyncTeamStats(void) {
    // Debug info (keep your existing output)
    printf("Syncing stats between team and followers (count: %d)...\n", followerCount);
   
    for (int i = 0; i < followerCount; i++) {
        bool foundMatch = false;
        for (int j = 1; j < MAX_PARTY_SIZE; j++) {
            if (mainTeam[j].name[0] != '\0' &&
                strcmp(followers[i].creature->name, mainTeam[j].name) == 0) {
                // Match found!
                printf("Found match: follower %s -> mainTeam[%d]\n",
                      followers[i].creature->name, j);
                foundMatch = true;
               
                // Get current values before sync
                int followerHP = followers[i].creature->currentHP;
                int mainTeamHP = mainTeam[j].currentHP;
                int followerMP = followers[i].creature->currentMP;
                int mainTeamMP = mainTeam[j].currentMP;
                
                // Get EXP/level values
                int followerExp = followers[i].creature->exp;
                int mainTeamExp = mainTeam[j].exp;
                int followerLevel = followers[i].creature->gridLevel;
                int mainTeamLevel = mainTeam[j].gridLevel;
                int followerTotalLevel = followers[i].creature->totalGridLevel;
                int mainTeamTotalLevel = mainTeam[j].totalGridLevel;
                
                // True bidirectional sync - take minimum for HP/MP for safety
                int syncedHP = MIN(followerHP, mainTeamHP);
                int syncedMP = MIN(followerMP, mainTeamMP);
                
                // For experience, take the maximum
                int syncedExp = MAX(followerExp, mainTeamExp);
                int syncedLevel = MAX(followerLevel, mainTeamLevel);
                int syncedTotalLevel = MAX(followerTotalLevel, mainTeamTotalLevel);
                
                // Apply synced values to both copies
                followers[i].creature->currentHP = syncedHP;
                mainTeam[j].currentHP = syncedHP;
                followers[i].creature->currentMP = syncedMP;
                mainTeam[j].currentMP = syncedMP;
                
                followers[i].creature->exp = syncedExp;
                mainTeam[j].exp = syncedExp;
                followers[i].creature->gridLevel = syncedLevel;
                mainTeam[j].gridLevel = syncedLevel;
                followers[i].creature->totalGridLevel = syncedTotalLevel;
                mainTeam[j].totalGridLevel = syncedTotalLevel;
               
                printf("Synced HP: %d/%d, EXP: %d, Level: %d\n", 
                      syncedHP, mainTeam[j].maxHP, syncedExp, syncedLevel);
                break;
            }
        }
        if (!foundMatch) {
            printf("WARNING: No main team match for follower %s\n",
                  followers[i].creature->name);
        }
    }
}


const char* StateToString(GameState gamestate) {
    switch (gamestate) {
        case GAME_STATE_TITLE_SCREEN: return "GAME_STATE_TITLE_SCREEN";
        case GAME_STATE_MAIN_MENU: return "GAME_STATE_MAIN_MENU";
        case GAME_STATE_LOAD_SELECTION: return "GAME_STATE_LOAD_SELECTION";
        case GAME_STATE_SETTINGS: return "GAME_STATE_SETTINGS";
        case GAME_STATE_QUIT_CONFIRM: return "GAME_STATE_QUIT_CONFIRM";
        case GAME_STATE_OVERWORLD: return "GAME_STATE_OVERWORLD";
        case GAME_STATE_MENU: return "GAME_STATE_MENU";
        case GAME_STATE_FISHING_MINIGAME: return "GAME_STATE_FISHING_MINIGAME"; 
        case GAME_STATE_PICKAXE_MINIGAME: return "GAME_STATE_PICKAXE_MINIGAME";
        case GAME_STATE_CUTTING_MINIGAME: return "GAME_STATE_CUTTING_MINIGAME";
        case GAME_STATE_DIGGING_MINIGAME: return "GAME_STATE_DIGGING_MINIGAME";
        case GAME_STATE_EATING: return "GAME_STATE_EATING";
        case GAME_STATE_BATTLE_TRANSITION: return "GAME_STATE_BATTLE_TRANSITION"; 
        case GAME_STATE_BATTLE: return "GAME_STATE_BATTLE";
        case GAME_STATE_REALTIME_BATTLE: return "GAME_STATE_REALTIME_BATTLE";
        case GAME_STATE_COMMAND_MENU: return "GAME_STATE_COMMAND_MENU";
        case GAME_STATE_BEFRIEND_SWAP: return "GAME_STATE_BEFRIEND_SWAP";
        case GAME_STATE_CONFIRM_SWAP: return "GAME_STATE_CONFIRM_SWAP";
        case GAME_STATE_BOSS_BATTLE_TRANSITION: return "GAME_STATE_BOSS_BATTLE_TRANSITION";
        case GAME_STATE_GAME_OVER: return "GAME_STATE_GAME_OVER";
        default: return "-1";
    }
}


void HandleAttackSubmenu(CommandMenuState *cmdState) {
    // ----- TARGET SELECTION MODE -----
    // Keep the target selection logic that was working
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
        
        // Find next target in the pressed direction
        int currentIndex = realTimeBattleState.selectedCreatureIndex;
        
        // Direction-based search logic
        int bestIndex = -1;
        float bestScore = 999999.0f;
        
        // Get current target position
        OverworldCreature* currentTarget = &realTimeBattleState.creatures[currentIndex];
        float currentX = currentTarget->position.x;
        float currentY = currentTarget->position.y;
        
        // Search all creatures for best next target
        for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
            if (i == currentIndex || !realTimeBattleState.creatures[i].active || 
                !IsCreatureInPartyRange(&realTimeBattleState, i)) {
                continue;
            }
            
            float dx = realTimeBattleState.creatures[i].position.x - currentX;
            float dy = realTimeBattleState.creatures[i].position.y - currentY;
            float dist = sqrtf(dx*dx + dy*dy);
            
            // Calculate direction-based score
            float score = 999999.0f;
            
            if (IsKeyPressed(KEY_RIGHT) && dx > 0) score = dist - dx*3;
            else if (IsKeyPressed(KEY_LEFT) && dx < 0) score = dist + dx*3;
            else if (IsKeyPressed(KEY_DOWN) && dy > 0) score = dist - dy*3;
            else if (IsKeyPressed(KEY_UP) && dy < 0) score = dist + dy*3;
            
            if (score < bestScore) {
                bestScore = score;
                bestIndex = i;
            }
        }
        
        // After finding a new target and updating selectedCreatureIndex
        if (bestIndex >= 0) {
            EnableTargeting(cmdState, &realTimeBattleState, bestIndex);
            PlaySoundByName(&soundManager, "menuSelect", false);
        }
    }
    
    if (IsKeyPressed(KEY_Z) && !cmdState->waitingForKeyRelease) {
        cmdState->waitingForKeyRelease = true;
        printf("DEBUG: Target confirmed, set waitingForKeyRelease\n");
        if (realTimeBattleState.selectedCreatureIndex >= 0) {
            OverworldCreature* target = &realTimeBattleState.creatures[realTimeBattleState.selectedCreatureIndex];
            
            if (target->active) {
                // If queue isn't full and we have enough energy to queue this command
                if (realTimeBattleState.queueCount < MAX_COMMAND_QUEUE && 
                    realTimeBattleState.commandEnergy >= 1.0f) { // Check energy for queueing
                    
                    // Add to queue
                    QueuedCommand newCmd;
                    newCmd.type = RTBS_COMMAND_ATTACK;
                    newCmd.targetIndex = realTimeBattleState.selectedCreatureIndex;
                    newCmd.energyCost = 1.0f;
                    
                    realTimeBattleState.commandQueue[realTimeBattleState.queueCount] = newCmd;
                    realTimeBattleState.queueCount++;
                    
                    // Consume energy immediately upon queueing
                    // realTimeBattleState.commandEnergy -= 1.0f;
                    
                    // Play confirmation sound
                    PlaySoundByName(&soundManager, "menuConfirm", false);
                    
                    char message[100];
                    sprintf(message, "Attack on %s queued! (%d/%d)", 
                            target->creature.name,
                            realTimeBattleState.queueCount,
                            MAX_COMMAND_QUEUE);
                    SetOverworldMessage(message);
                    
                    // If queue is now full, return to combat
                    if (realTimeBattleState.queueCount >= MAX_COMMAND_QUEUE) {
                        // cmdState->targetSelectionMode = false;
                        DisableTargeting(cmdState, &realTimeBattleState);
                        currentGameState = cmdState->previousState;
                    }
                } else if (realTimeBattleState.queueCount >= MAX_COMMAND_QUEUE) {
                    // Queue is full
                    SetOverworldMessage("Command queue is full!");
                    PlaySoundByName(&soundManager, "menuError", false);
                } else {
                    // Not enough energy
                    SetOverworldMessage("Not enough energy to queue attack!");
                    PlaySoundByName(&soundManager, "menuError", false);
                }
            }
        }
    }
}

void HandleAISubmenu(CommandMenuState *cmdState) {
// ----- AI SUBMENU MODE -----
    // Navigation
    if (IsKeyPressed(KEY_DOWN)) {
        cmdState->aiSelection = (cmdState->aiSelection + 1) % AI_SUBMENU_COUNT;
        PlaySoundByName(&soundManager, "menuSelect", false);
    }
    if (IsKeyPressed(KEY_UP)) {
        cmdState->aiSelection = (cmdState->aiSelection - 1 + AI_SUBMENU_COUNT) % AI_SUBMENU_COUNT;
        PlaySoundByName(&soundManager, "menuSelect", false);
    }
    
    // Confirm selection
    if (IsKeyPressed(KEY_Z) && !cmdState->waitingForKeyRelease) {
        PlaySoundByName(&soundManager, "menuConfirm", false);
        ApplyFollowerBehavior(cmdState->aiSelection);
        cmdState->inAISubmenu = false;
        cmdState->waitingForKeyRelease = true;
        printf("DEBUG: AI option confirmed, set waitingForKeyRelease\n");
    }
}

// Enable targeting mode
void EnableTargeting(CommandMenuState* cmdState, RealTimeBattleState* rtbsState, int targetIndex) {
    cmdState->targetSelectionMode = true;
    cmdState->inAttackSubmenu = true;
    rtbsState->playerAttackMode = true;
    rtbsState->selectedCreatureIndex = targetIndex;
}

// Disable targeting mode
void DisableTargeting(CommandMenuState* cmdState, RealTimeBattleState* rtbsState) {
    cmdState->targetSelectionMode = false;
    cmdState->inAttackSubmenu = false;
    rtbsState->playerAttackMode = false;
    rtbsState->selectedCreatureIndex = -1;
}

// Add this function to apply separation force between entities
void ApplySeparationBehavior(Vector2* position, EntityType entityType, void* self) {
    const float MIN_SEPARATION = 0.8f;      // Minimum distance to maintain
    const float SEPARATION_FORCE = 0.04f;   // How quickly entities separate (adjust for desired speed)
    
    // Check against all creatures
    for (int i = 0; i < MAX_RTBS_CREATURES; i++) {
        if (!realTimeBattleState.creatures[i].active) continue;
        if (entityType == ENTITY_CREATURE && self == &realTimeBattleState.creatures[i]) continue;
        
        // Calculate distance between entities
        float dx = realTimeBattleState.creatures[i].position.x - position->x;
        float dy = realTimeBattleState.creatures[i].position.y - position->y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        // Apply separation force if too close
        if (distance < MIN_SEPARATION && distance > 0.01f) {
            // Calculate normalized direction vector away from other entity
            float repelX = -dx / distance;
            float repelY = -dy / distance;
            
            // Apply stronger force when closer (inverse to distance)
            float intensity = (MIN_SEPARATION - distance) / MIN_SEPARATION;
            float moveX = repelX * SEPARATION_FORCE * intensity;
            float moveY = repelY * SEPARATION_FORCE * intensity;
            
            // Apply the separation movement
            position->x += moveX;
            position->y += moveY;
        }
    }
    
    // Check against player (unless we are the player)
    if (entityType != ENTITY_PLAYER) {
        float dx = mainTeam[0].x - position->x;
        float dy = mainTeam[0].y - position->y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance < MIN_SEPARATION && distance > 0.01f) {
            float repelX = -dx / distance;
            float repelY = -dy / distance;
            float intensity = (MIN_SEPARATION - distance) / MIN_SEPARATION;
            
            position->x += repelX * SEPARATION_FORCE * intensity;
            position->y += repelY * SEPARATION_FORCE * intensity;
        }
    }
    
    // Check against followers
    for (int i = 0; i < followerCount; i++) {
        if (followers[i].creature->currentHP <= 0) continue;
        if (entityType == ENTITY_FOLLOWER && self == &followers[i]) continue;
        
        float dx = followers[i].creature->x - position->x;
        float dy = followers[i].creature->y - position->y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance < MIN_SEPARATION && distance > 0.01f) {
            float repelX = -dx / distance;
            float repelY = -dy / distance;
            float intensity = (MIN_SEPARATION - distance) / MIN_SEPARATION;
            
            position->x += repelX * SEPARATION_FORCE * intensity;
            position->y += repelY * SEPARATION_FORCE * intensity;
        }
    }
    
    // Ensure position stays within map bounds
    position->x = CLAMP(position->x, 0, mapWidth - 1);
    position->y = CLAMP(position->y, 0, mapHeight - 1);
}