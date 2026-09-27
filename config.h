#pragma once
#include <cstdint>

// ============================================================================
// CONFIG.H - Estado global centralizado (ZB2 Menu / Zumbi Blocks 2)
// OBJETIVO: todas as flags do menu em um unico namespace, ordem PLAYER-VISUAL-MISC-SETTINGS.
// ORIGEM: briefing Pt.4/Pt.6/Pt.7/Pt.8; estrutura herdada de kiero-dx9-base/config.h.
// TESTES: alterar no menu -> reflete no overlay sem restart; configs JSON salvam/carregam.
// HISTORICO: v0.1.0 expande exemplo minimo para as 4 abas + preview drag-and-drop.
// ============================================================================

namespace Config {
    // ---- Global ----
    extern bool bMenuOpen;          // INSERT/DELETE
    extern int  iMenuKey;           // VK_INSERT por padrao
    extern bool bWatermark;         // watermark no canto
    extern bool bDebugOverlay;      // overlay de debug sempre ativo (briefing)
    extern bool bTooltips;          // tooltips em todos os controles

    // ---- Sobrevivencia (MISC): escrita direta, ver mono.cpp ----
    extern bool  bGodMode;        // trava HP local em 100 (healthFast+healthSlow)
    extern bool  bInfStamina;     // trava stamina local no maximo
    extern bool  bInfItems;       // trava stackCount=stackMax em pilhas por SubType
                                 // (municao/arremessavel/consumivel; resto fora)
    extern bool  bInfMoney;       // dinheiro infinito (3 moedas em 99999)
    // REMOVIDO 17/09: bAntiFlood (guardiao bania o proprio infinito).
    extern bool  bUnlockSlots;    // storage+misc desbloqueados (1x por sessao)
    extern bool  bUnlockLoadout;  // LoadoutSelector.UnlockAll (vendedor desbloqueado)
    // REMOVIDO 17/09: Spawn de Itens (bGiveItem/iGiveItem/iGiveQty/iGiveDest).

    // ---- PLAYER / Aimbot ----
    extern bool  bAimbot;
    extern int   iAimKey;           // VK_RBUTTON por padrao
    extern int   iAimMode;          // 0=Hold 1=Toggle
    extern bool  bAutoAim;
    extern bool  bSilentAim;
    extern bool  bAutoFire;
    extern bool  bTriggerbot;
    extern int   iAimBone;          // 0=Head 1=Neck 2=Chest 3=Pelvis
    extern int   iAimPriority;      // 0=Crosshair 1=LowestHP 2=Nearest
    extern float fSmoothing;        // 1-8 (1 = snap wohax)
    extern bool  bLimitFov;
    extern float fFovAngle;         // 1-360
    extern bool  b360Mode;
    extern bool  bDrawFov;
    extern float fEspDistance;      // 10-500m (VISUAL: ate onde o ESP desenha)
    extern float fAimDistance;      // 10-500m (PLAYER: ate onde o AIM mira)
    extern bool  bPrediction;
    extern float fLagComp;          // 0-200ms

    // ---- PLAYER / Weapon ----
    extern bool  bNoRecoil;
    extern bool  bNoSpread;
    extern bool  bNoSway;
    extern bool  bTightAim;      // mira fechada (crosshair junto: spread visual minimo)
    extern bool  bRapidFire;
    extern float fRapidMult;        // 1x-5x
    extern bool  bFastKnife;        // arma branca rapida (Duration curto em pa/pa/facao/faca/taco)
    extern float fKnifeMult;        // 1x-5x (divisor do Duration)
    extern bool  bInfAmmo;
    extern bool  bInstantReload;
    extern bool  bFullAuto;
    extern bool  bSaitama;          // 4.000.000 dano
    extern float fNadeTime;
    extern float fExplRadius;       // (HOST)
    extern float fExplDamage;       // (HOST)
    extern bool  bContactExpl;
    extern bool  bPowerDrop;        // velocidade 400

    // ---- PLAYER / Items ----
    extern char  szItemSearch[64];
    extern int   iItemAmount;       // 1-9999

    // ---- PLAYER / Movement ----
    extern bool  bSpeedHack;
    extern float fSpeedMult;        // 1x-5x
    extern bool  bSuperJump;
    extern float fJumpMult;         // 1x-10x
    extern bool  bRollSpeed;
    extern float fRollMult;

    // ---- VISUAL / Zumbis ----
    extern bool  bZombieEsp;        // master do ESP (worker + draw)
    extern bool  bZombieBoxShow;    // Box tem flag propria (desmarca so o box)
    extern int   iZombieBox;        // 0=2D 1=3D 2=Corners
    extern bool  bZombieName;
    extern bool  bZombieDist;
    extern bool  bZombieHp;         // barra de vida
    extern bool  bZombiePct;        // % arrastavel independente
    extern float fPctX, fPctY;      // offset do % em relacao a ancora
    extern bool  bZombieSkeleton;
    extern bool  bZombieSnap;
    extern bool  bZombieHeadDot;
    extern int   iSnapFrom;         // 0=Base 1=Topo 2=Centro (origem da snapline)
    extern float colZombieSkel[4];  // cor propria do Skeleton
    extern float colZombieSnap[4];  // cor propria da Linha
    extern float colZombieDot[4];   // cor propria do HeadDot
    extern float colZombieBox[4];   // cor do box
    extern bool  bBossColor;        // boss usa cor propria (default ON)
    extern float colBossBox[4];     // cor do boss (Assalto/Rainha/Ceifador)
    extern float colZombieName[4];  // cor do nome
    extern float colZombieDist[4];  // cor da distancia
    extern float colZombieHp[4];    // cor da vida (%)

    // ---- VISUAL / Aliados ----
    extern bool  bAllyEsp;
    extern bool bAllyBoxShow;
    extern bool bAllyPct;
    extern char szAllyLayout[512];
    extern int iAllySnapFrom,iAllyLayout;
    extern float colAllyName[4];
    extern float colAllyDist[4];
    extern float colAllyHp[4];
    extern float colAllySkel[4];
    extern float colAllyLine[4];
    extern float colAllyDot[4];

    extern float fAllyDistance;
    extern int iMagnetTargets;
    extern float fMagnetFront,fMagnetBoss;
    extern int   iAllyBox;
    extern bool  bAllyName;
    extern bool  bAllyDist;
    extern bool  bAllyHp;
    extern bool  bAllySkeleton;
    extern bool  bAllySnap;
    extern bool  bAllyHeadDot;
    extern float colAllyVis[4];     // azul por padrao (unica cor — aliado sempre visivel)

    // ---- VISUAL / Chams ----
    extern bool  bChams;
    extern float colChamsVis[4];
    extern float colChamsInv[4];

    // ---- VISUAL / Itens ----
    extern bool  bItemEsp;
    extern bool  bItemWeapons;
    extern bool  bItemRare;
    extern bool  bItemAmmo;
    extern bool  bItemSupply;
    extern float colItem[4];        // cor Armas (default laranja)
    extern float colItemRare[4];    // cor Raros (roxo)
    extern float colItemAmmo[4];    // cor Municao (cinza)
    extern float colItemSupply[4];  // cor Suprimento (verde)
    extern float fItemRadius;
    extern int iItemFilter0;
    extern int iItemFilter1;
    extern int iItemFilter2;
    extern int iItemFilter3;
    extern int iPoiFilter;
    extern bool bPoiGraves, bPoiPlayers;
    extern float fPoiRadius; // independent radius for static world points

    // ---- VISUAL / POI ----
    extern bool  bPoiEsp;           // master POI
    extern bool  bPoiHeli;
    extern bool  bPoiBoss;
    extern bool  bPoiMission;
    extern bool  bPoiWave;
    extern bool  bPoiLootFix;
    extern bool  bPoiBench;
    extern bool  bPoiFire;
    extern bool  bPoiShop;
    extern bool  bPoiRespawn;
    extern float colPoiHeli[4];
    extern float colPoiBoss[4];
    extern float colPoiMission[4];
    extern float colPoiWave[4];
    extern float colPoiLootFix[4];
    extern float colPoiBench[4];
    extern float colPoiFire[4];
    extern float colPoiShop[4];
    extern float colPoiRespawn[4];

    // ---- VISUAL / Preview interativo (drag-and-drop) ----
    // Fixos: Box, Skeleton, HeadDot, Snapline. Arrastaveis: Nome, Dist, Vida.
    // Ancoras do ESP (0=TL 1=TC 2=TR 3=ML 4=MC 5=MR 6=BL 7=BC 8=BR) + offsets px.
    extern int   iNameA, iDistA, iHpA, iPctA; // legado v2 (migracao)
    // Lado (0=topo 1=base 2=esq 3=dir) + alinhamento (0=ini 1=centro 2=fim).
    extern int   iSideN, iSideD, iSideH, iSideP;
    extern int   iAlinN, iAlinD, iAlinH, iAlinP;
    extern int   iCfgVer; // versao do schema (v2 = fabrica calibrada)
    extern float fPropN, fPropD, fPropH, fPropP; // proporcional do offset hibrido (Pilar 2)
    extern float fNameX, fNameY;    // offset do Nome em relacao a ancora
    extern float fDistX, fDistY;    // offset da Distancia em relacao a ancora
    extern float fHpX, fHpY;        // offset da Barra de vida em relacao a ancora
    extern int   iLayoutMode;       // 0=Personalizado 1=Ao lado 2=Topo/Base/Centro
    extern int   iLayoutSide;       // 0=Direita 1=Esquerda
    extern float fLayoutOffset;     // px
    extern float fLayoutSpacing;    // px
    extern bool  bSnapGrid;
    extern float fSnapSize;         // px
    extern bool  bShowGuides;
    extern bool  bAlignList;
    extern int   iListDir;          // 0=Coluna 1=Linha
    extern float fListSpacing;
    extern float fAlongN, fAlongD, fAlongH, fAlongP;
    extern float fGapN, fGapD, fGapH, fGapP;
    extern int iOrderN, iOrderD, iOrderH, iOrderP;
    extern float fBarLength, fBarThickness;
    extern float fPreviewHp;        // 0-100 simulado no preview

    // ---- MISC / Magnet ----
    extern bool  bEnemyMagnet;
    extern int   iMagnetKey;        // H
    extern int   iMagnetMode;       // 0=Frente 1=Centro
    extern float fMagnetRadius;     // 10-300m
    extern bool  bMagnetFreeze;
    extern bool  bKillOnSpawn;
    extern bool  bItemMagnet;
    extern int   iItemMagnetKey;    // J
    extern int   iItemMagnetType;   // 0=Armas 1=Municao 2=Loot 3=Caixas 4=Todos
    extern float fItemMagnetRadius; // 10-200m
    extern bool  bAutoCollect;

    // ---- MISC / Teleports ----
    extern float fSaveX, fSaveY, fSaveZ;

    // ---- MISC / Host ----
    extern float fDayHour;          // 0-24h (HOST)
    extern float fDaySpeed;         // 1x-10x (HOST)
    extern int   iSpawnCount;       // 1-100 (HOST)
    extern int   iSpawnBoss;        // 0=Riot 1=Queen 2=Reaper (HOST)

    // ---- MISC / Utilities ----
    extern float fCamFov;           // 60-120
    extern bool  bThirdPerson;
    extern float fThirdDist;        // 1-10m
    extern bool  bAntiAfk;
    extern bool  bNoClip;
    extern int iNoClipKey;
    extern float fNoClipSpeed;      // 1.3x padrao; Space sobe, Ctrl desce
    extern bool  bNoFall;
}
