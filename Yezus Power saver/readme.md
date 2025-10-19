# Yezus Power Saver

## Problem Statement
Quand j'AFK dans mon serveur survie vanilla PVE, j'ai peur de perdre mes ships par manque d'energie d'H2 ou d'O2, mais je veux pouvoir AFK tant que ce n'est pas dangereux.
Utiliser un timer block par grille est long et necessite une configuration manuelle repetitive.

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
   - Deep sleep (respawn)
   - Sleep (maintien vital)
   - Vital
   - Production
3. Ajouter la gestion de l'electricite (avec memes paliers).
4. Ajouter la gestion de l'O2.
5. Permettre de personnaliser l'ordre de priorite et les seuils.

## Detection automatique des paliers
Les blocs sont classes automatiquement sans groupe predefini. Le script s'appuie sur des whitelist de sous-types `MyDefinitionId` pour chaque interface (IMyMedicalRoom, IMyThrust, IMyAssembler, etc.) et couvre les variantes Vanilla et DLC (Economy, Sparks of the Future, Frostbite, Wasteland, Decorative 1&2, Heavy Industry, Warfare 1&2, Automatons). Chaque categorie ci-dessous est eevaluee sur grilles small et large.

### Deep Sleep - respawn uniquement
- Respawn (IMyMedicalRoom, IMyCryoChamber, IMySurvivalKit): Medical Room, Cryo Chamber, Survival Kit.
- Support vital minimum (IMyGasGenerator): O2/H2 Generator exclusivement pour alimenter le bloc de respawn.
- Energie critique (IMyBatteryBlock, IMyReactor): Battery Block (mode recharge auto), Small Reactor, Hydrogen Engine au ralenti.
- Atmosphere immediate (IMyAirVent): Air Vent pour pressuriser la zone du respawn si necessaire.
- Distribution directe (IMyConveyorTube): Conveyor Frames/Tubes reliant le point de respawn aux reserves d'H2/O2.
- Controle passif (IMyProgrammableBlock): Programmable Block hote du script pour que le systeme reste operationnel.

### Sleep - maintien vital
- Gaz & atmosphere (IMyGasTank, IMyOxygenTank, IMyOxygenFarm): Hydrogen Tank (standard, Industrial), Oxygen Tank (standard, Industrial), Oxygen Farm.
- Energie de base (IMyBatteryBlock, IMyReactor, IMyPowerProducer): Battery Block, Small Reactor, Hydrogen Engine en regime veille.
- Controle & scripts (IMyProgrammableBlock, IMyTimerBlock, IMySensorBlock, IMyCameraBlock, IMyShipController, IMyEventController): Programmable Block secondaires, Timer Block, Sensor, Camera, Cockpit, Flight Seat, Remote Control, Control Seat, Industrial Cockpit, Bridge Cockpit (Warfare), Event Controller.
- Ancrage & dock (IMyShipConnector, IMyShipMergeBlock, IMyLandingGear, IMyMechanicalConnectionBlock): Connector (standard, small, warfare), Merge Block, Landing Gear (standard, Wasteland), Magnetic Plate, Grapple Block (Automatons).
- Logistique passive (IMyConveyorTube, IMyCargoContainer, IMyConveyorSorter, IMyCollector, IMyShipConnector): Conveyor Tubes, Conveyor Frames, Conveyor Junction, Cargo Container (standard, Industrial), Conveyor Sorter en veille, Collector, Ejector.
- Communication & balises (IMyRadioAntenna, IMyLaserAntenna, IMyBeacon, IMyOreDetector, IMyStoreBlock, IMyContractBlock): Radio Antenna, Laser Antenna, Beacon, Ore Detector, Store Block, Contract Block, Vending Machine.
- Securite interne (IMyAirVent, IMyDoor, IMyAirtightHangarDoor, IMyGravityGenerator): Air Vent, Airtight Doors (Sliding, Offset, Blast), Airtight Hangar Door, Gravity Generator, Spherical Gravity Generator.

### Vital - manoeuvre et defense
- Propulsion & stabilisation (IMyThrust, IMyGyro, IMyMotorSuspension): Atmospheric Thruster (vanilla, Wasteland, Sparks), Ion Thruster (vanilla, Warfare), Hydrogen Thruster (vanilla, Warfare), Gyroscope, AI Flight Block (Automatons), Wheel Suspension 1x1/3x3/5x5 (vanilla, Wasteland).
- Energie active (IMyReactor, IMyPowerProducer, IMyBatteryBlock, IMyJumpDrive): Large Reactor, Hydrogen Engine, Wind Turbine, Solar Panel, Battery (mode Auto), Jump Drive.
- Defenses & contre-mesures (IMyLargeTurretBase, IMySmallGatlingGun, IMyUserControllableGun, IMySafeZoneBlock, IMyDecoy): Interior Turret, Gatling Turret, Missile Turret, Artillery Turret, Assault Cannon Turret, Autocannon Turret, Railgun, Sentry Gun (Warfare 2), Warfare Gatling Gun, Warfare Autocannon, Warfare Missile Launcher, Artillery Cannon, Assault Cannon, Railgun (statique), Tesla Coil (Automatons), Safe Zone Controller, Decoy.
- Mobilite avancee (IMyJumpDrive, IMyParachute, IMyGravityGenerator, IMyVirtualMass): Jump Drive, Parachute Hatch, combos Gravity Generator + Artificial Mass pour grav-drive.
- Maintenance & travaux (IMyShipDrill, IMyShipGrinder, IMyShipWelder, IMyPistonBase, IMyMotorBase, IMyHingeBase, IMyMechanicalConnectionBlock): Drill (standard, Warfare), Grinder (standard, Warfare), Welder (standard, Warfare), Pistons, Rotors, Advanced Rotors, Hinges, Mechanical Joint Block (Automatons).
- IA & automation (Automatons): AI Basic Interface, AI Task Interface, AI Offensive Interface, AI Defensive Interface, AI Recorder.

### Production - pleine capacite
- Industrialisation (IMyAssembler, IMyRefinery, IMySurvivalKit, IMyGasGenerator): Assembler, Basic Assembler, Industrial Assembler, Refinery, Basic Refinery, Industrial Refinery, Survival Kit (mode production), O2/H2 Generator (mode production continu).
- Modules (IMyProductionModule): Yield Module, Productivity Module, Power Efficiency Module, Speed Module.
- Traitement ressources & energie (IMyGasTank, IMyPowerProducer, IMyFunctionalBlock): Hydrogen Engine en banc, Hydrogen Tank (Industrial), Hydroponic Module (Frostbite).
- Utilitaires & confort (IMyTextPanel, IMyProjector, IMySoundBlock, IMyInteriorLight, IMyReflectorLight, IMyOreDetector, IMyVent): LCD Panel (standard, Wide, Corner, Industrial, Transparent), Text Panel, Projector (standard, Industrial), Sound Block, Air Vent (pressurisation), Heat Vent (Frostbite), Interior Light, Spotlight, Holo Projector.
- Logistique active (IMyConveyorSorter, IMyConveyorTube, IMyCollector, IMyShipConnector): Advanced Conveyor Sorter, Industrial Conveyor Junction, Collector, Ejector, boucles de Connectors actives.

## Heuristiques de classement
- Chaque bloc est scanne et mappe vers un palier via `Stage -> HashSet<MyDefinitionId>`. L'ordre de priorite est Deep > Sleep > Vital > Production.
- Les seuils proposes: `deep_threshold` (0.25 par defaut) force le mode Deep Sleep pour assurer le respawn; `sleep_threshold` (ex. 0.45) degrade en mode Sleep quand l'hydrogene devient critique mais reste suffisant pour operations vitales.
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
