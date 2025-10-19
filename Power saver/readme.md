# Project Title
## Session Stats
- **Date & time:** <!-- e.g., 2024-04-20 14:00 -->
- **People/devices involved:** 
- **Current vibe / energy level:** <!-- relaxed, focused, chaotic, etc. -->
## Problem Statement
Quand j'afk dans mon serveur survie vanilla pve, j'ai peur de perdre mes ships par manque d'energie dh2 ou d'o2, mais je veux pouvoir afk tant que ce n'est pas dangereux.
Utiliser un timer block par grille est long, et nécessite de la configuration manuelle repetitive.
## Primary Goal For This Session
Définir des paliers de consommation de courant, et couper de manière safe petit a petit jusqu'a entrer en veile profonde, en identifiant de manière dynamique et sans intervention manuelle les devices a couper/maintenir en premier.
## Secondary Objectives
- Connecter a un lcd et/ou un cockpit
- Permettre de modifier l'ordre de priorité plus simplement qu'en utilisant le controle pannel.
## Constraints & Non-Negotiables
- Navigation Up, down, Apply
- Page de taille adapatée
## Context & References
- <!-- Link or path to previous work, specs, tickets, docs -->
- <!-- Reminder of stack versions, environments, commands -->
## Collaboration Notes For The AI
- **Tone & pacing:** <!-- keep it chill, move fast, go deep, etc. -->
- **Voice / naming / theme:** <!-- naming conventions, inside jokes, aesthetic -->
- **Prior art to respect:** <!-- files/modules/patterns to keep aligned with -->
- **May/not do:** <!-- things the AI should avoid or is encouraged to try -->
## Game Plan
0. Draft screens
1. MVP script surveille la consommation hydrogen
- 3 paliers
* Deep sleep
* Vital
* Production
2. Ajouter la gestion de l'elec
3. Ajouter la gestion de l'o2
4. Custom order

