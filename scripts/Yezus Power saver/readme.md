# Yezus Power Saver

## Problem Statement
Quand j'AFK dans mon serveur survie vanilla PVE, j'ai peur de perdre mes ships par manque d'energie d'H2 ou d'O2, mais je veux pouvoir AFK tant que ce n'est pas dangereux.
Utiliser un timer block par grille est long et necessite une configuration manuelle repetitive.

## Primary Goal
Definir des paliers de consommation de courant et couper de maniere progressive jusqu'a entrer en veille profonde, en identifiant de facon dynamique et sans intervention manuelle les devices a couper ou a maintenir en priorite.

## Secondary Objectives
- Connecter a un LCD et/ou un cockpit.
- Permettre de modifier l'ordre de priorite plus simplement qu'en utilisant le control panel.

## Constraints & Non-Negociables
- Navigation: Up, Down, Apply.
- Page de taille adaptable.

## Game Plan
1. Esquisser les ecrans.
2. Construire un MVP qui surveille la consommation d'hydrogene avec trois paliers:
   - Deep sleep
   - Vital
   - Production
3. Ajouter la gestion de l'electricite.
4. Ajouter la gestion de l'O2.
5. Permettre de personnaliser l'ordre de priorite.
