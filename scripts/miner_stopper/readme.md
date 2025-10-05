# QG_Storage Monitor

Script Space Engineers pour surveiller automatiquement la capacité de stockage d’une base et contrôler les drones **PAM miners** en conséquence.

## 📘 Fonction

Le script vérifie régulièrement le taux de remplissage total des **conteneurs contenant `[QG]`**.  
Il exécute automatiquement les commandes suivantes sur les **Programmable Blocks** des mineurs configurés :

- `STOP` → lorsque la capacité totale atteint ou dépasse **75 %**
- `CONT` → lorsque la capacité repasse en dessous du seuil

Le script inclut un mécanisme de **3 tentatives automatiques** (avec délai) pour gérer les cas où la grille du mineur n’est pas encore fusionnée juste après un **lock de connecteur**.

---

## ⚙️ Configuration

### Variables principales
```csharp
const string TAG_QG = "[QG]";              // Tag à rechercher dans le nom des conteneurs
const double THRESHOLD_PERCENT = 75.0;     // Seuil de remplissage déclenchant l'arrêt
const int RETRY_COUNT = 3;                 // Nombre maximal de tentatives
const int RETRY_DELAY_TICKS = 60;          // Délai entre tentatives (~1s à 60TPS)

string[] MINER_BLOCKS = {
    "[HMiner-1] Programmable Block",
    "[HMiner-2] Programmable Block"
};
