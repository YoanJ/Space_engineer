# Yezus Power Saver

## Problem Statement
Quand j'AFK dans mon serveur survie vanilla PVE, j'ai peur de perdre mes ships par manque d'energie d'H2 ou d'O2, mais je veux pouvoir AFK tant que ce n'est pas dangereux. Utiliser un timer block par grille est long et necessite une configuration manuelle repetitive.

## Primary Goal
Definir des paliers de consommation de courant et couper de maniere progressive jusqu'a entrer en veille profonde, en identifiant de facon dynamique et sans intervention manuelle les devices a couper ou a maintenir en priorite.

## Secondary Objectives
- Connecter a un LCD et/ou un cockpit.
- Permettre de modifier l'ordre de priorite plus simplement qu'en utilisant le control panel.
- Ajouter plus tard l'electricite et l'O2 dans la logique de sauvegarde.

## Constraints & Non-Negociables
- Affichage unique sur LCD/cockpit; pas de navigation utilisateur.
- Page de taille adaptable.
- Pas de groupes manuels: detection 100% script.

## Game Plan
1. Esquisser les ecrans.
2. Construire un MVP qui surveille la consommation d'hydrogene avec quatre paliers auto-detectes:
   - Deep sleep (respawn) | ratio < 0.10
   - Sleep (maintien vital) | 0.10 <= ratio < 0.50
   - Vital | 0.50 <= ratio < 0.75
   - Production | ratio >= 0.75
3. Ajouter la gestion de l'electricite (avec memes paliers).
4. Ajouter la gestion de l'O2.
5. Permettre de personnaliser l'ordre de priorite et les seuils.

## Detection automatique des paliers
Les blocs sont classes automatiquement sans groupe predefini. Le script s'appuie sur des whitelist de sous-types `MyDefinitionId` (IMyMedicalRoom, IMyThrust, IMyAssembler, etc.) et couvre les variantes Vanilla et DLC (Economy, Sparks of the Future, Frostbite, Wasteland, Decorative 1&2, Heavy Industry, Warfare 1&2, Automatons). Chaque palier herite automatiquement des niveaux inferieurs: tout bloc actif en Deep reste actif en Sleep, Vital puis Production.

### Deep Sleep (ratio < 0.10) - Respawn uniquement
- Respawn core: Medical Room, Cryo Chamber, Survival Kit (IMyMedicalRoom, IMyCryoChamber, IMySurvivalKit).
- Secours d'atmosphere: O2/H2 Generator dedie + Air Vent local (IMyGasGenerator, IMyAirVent).
- Energie critique: Battery Block, Small Reactor, Hydrogen Engine au ralenti (IMyBatteryBlock, IMyReactor).
- Tronc logistique: Conveyor Frame/Tubes reliant les reserves d'H2/O2 au bloc de respawn (IMyConveyorTube).
- Pilote du systeme: Programmable Block hote du script (IMyProgrammableBlock).

### Sleep (0.10 <= ratio < 0.50) - Maintien vital
- Reservoirs & culture: Hydrogen/Oxygen Tanks, Oxygen Farm (IMyGasTank, IMyOxygenFarm).
- Noyau de controle: Programmable Blocks secondaires, Timer, Sensors, Cameras, Ship Controllers, Event Controller (IMyProgrammableBlock, IMyTimerBlock, IMySensorBlock, IMyCameraBlock, IMyShipController, IMyEventController).
- Amarrage & support: Connectors, Merge Blocks, Landing Gear, Magnetic Plates, Grapple Blocks (IMyShipConnector, IMyShipMergeBlock, IMyLandingGear, IMyMechanicalConnectionBlock).
- Logistique passive: Conveyor Tubes/Frames/Junctions, Cargo Containers, Conveyor Sorters en veille, Collector/Ejector (IMyConveyorTube, IMyCargoContainer, IMyConveyorSorter, IMyCollector).
- Comms & services: Radio/Laser Antenna, Beacon, Ore Detector, Store Block, Contract Block, Vending Machine (IMyRadioAntenna, IMyLaserAntenna, IMyBeacon, IMyOreDetector, IMyStoreBlock, IMyContractBlock).
- Etancheite & gravite: Airtight Doors/Hangar Door, Gravity Generator (IMyDoor, IMyAirtightHangarDoor, IMyGravityGenerator).

### Vital (0.50 <= ratio < 0.75) - Manoeuvre & defense
- Propulsion & stabilisation: Atmos, Ion, Hydro thrusters (vanilla, Warfare, Sparks), Gyros, AI Flight Block, Wheel Suspensions (IMyThrust, IMyGyro, IMyMotorSuspension, IMyFlightBlock).
- Energie dynamique: Large Reactors, Hydrogen Engines en charge, Wind Turbines, Solar Panels, Jump Drives, Batteries en auto (IMyReactor, IMyPowerProducer, IMyBatteryBlock, IMyJumpDrive).
- Protection active: Turrets (Interior, Gatling, Missile, Artillery, Assault, Autocannon, Railgun, Warfare), Static Weaponry, Tesla Coil, Safe Zone Controller, Decoy (IMyLargeTurretBase, IMyUserControllableGun, IMySafeZoneBlock, IMyDecoy).
- Mobilite speciale: Parachute Hatch, combos Gravity Generator + Artificial Mass (IMyParachute, IMyGravityGenerator, IMyVirtualMass).
- Maintenance lourde: Drills, Grinders, Welders, Pistons, Rotors, Hinges, Mechanical Joint Block (IMyShipDrill, IMyShipGrinder, IMyShipWelder, IMyPistonBase, IMyMotorBase, IMyHingeBase, IMyMechanicalConnectionBlock).
- IA & automatons: AI Basic/Task/Offensive/Defensive, AI Recorder, modules Automaton critiques (Automatons DLC).

### Production (ratio >= 0.75) - Pleine capacite
- Industrie lourde: Assembler (standard, basic, industrial), Refinery (standard, basic, industrial), Survival Kit prod, O2/H2 Generator continu (IMyAssembler, IMyRefinery, IMySurvivalKit, IMyGasGenerator).
- Modules de prod: Yield, Productivity, Power Efficiency, Speed (IMyProductionModule).
- Traitement ressources: Hydrogen Engine banks, Industrial Hydrogen Tank, Hydroponic Module (IMyPowerProducer, IMyGasTank, IMyFunctionalBlock).
- Utilitaires & confort: LCD/Text Panels, Projectors, Sound Block, Heat Vent, Interior/Spot Lights, Holo Projector (IMyTextPanel, IMyProjector, IMySoundBlock, IMyInteriorLight, IMyReflectorLight, IMyVent).
- Logistique active: Advanced Conveyor Sorters, Industrial Conveyor Junctions, Collectors/Ejectors actifs, boucles de Connectors (IMyConveyorSorter, IMyConveyorTube, IMyCollector, IMyShipConnector).

## Heuristiques de classement
- Chaque bloc est scanne et mappe vers un palier via `Stage -> HashSet<MyDefinitionId>`. L'ordre de priorite est Deep > Sleep > Vital > Production.
- Ranges par defaut: `ratio < 0.10` => Deep, `0.10 <= ratio < 0.50` => Sleep, `0.50 <= ratio < 0.75` => Vital, `ratio >= 0.75` => Production.
- Les sous-types inconnus sont logs et rattaches par defaut au palier Production pour eviter de couper un bloc critique par accident.
- Un override manuel futur pourra utiliser des tags explicites `[YHP:DEEP]`, `[YHP:SLEEP]`, `[YHP:VITAL]` ou `[YHP:PROD]` dans le nom du bloc pour surclasser la detection automatique.

## Affichage MVP
```
YHP - <GRID_NAME> -
===
H2
[=====.....] 37.5%
CRIT LVL: 25%
CURRENT: ALL | PRODUCTION | VITAL | SLEEP | DEEP
```
Pas de navigation pour le moment: affichage unique sur LCD ou cockpit surface.
