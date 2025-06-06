# CONFIGURATION DES BLOCKS

#### **A. PROPRIÉTÉS DE BASE**
- **`std::string path`** : Chemin vers le fichier texture
- **`GLuint textureID`** : ID OpenGL de la texture (généré automatiquement)
- **`int textureWidth/textureHeight`** : Dimensions de la texture

#### **B. SYSTÈME D'ANIMATION**
- **`TextureAnimationType animType`** : Type d'animation (`STATIC` ou `ANIMATED`)
- **`float animationSpeed`** : Vitesse d'animation en FPS (ex: 20.0f)
- **`int frameCount`** : Nombre total de frames dans le sprite sheet
- **`int frameHeight`** : Hauteur d'une frame (ex: 16px)
- **`bool animationStartRandomFrame`** : Démarrage aléatoire de l'animation
- **`float currentFrame`** : Frame actuelle (par défaut, remplacée par Block.currentFrame)

#### **C. SYSTÈME DE ROTATION**
- **`bool randomizedRotation`** : Rotation aléatoire activée/désactivée
- Les rotations possibles : 0°, 90°, 180°, 270°

#### **D. SYSTÈME DE TRANSFORMATION TEMPORELLE**
- **`bool hasTransformation`** : Transformation activée/désactivée
- **`BlockName transformBlockTo`** : Type de bloc cible de la transformation
- **`float transformBlockTimeIntervalStart/End`** : Intervalle de temps aléatoire pour la transformation
- **`bool savePreviousExistingBlock`** : Sauvegarde du bloc précédent
- **`bool transformBlockToPreviousExistingBlock`** : Retour au bloc précédent sauvegardé

### **2. STRUCTURE `Block` - Instance de Bloc**

Chaque bloc placé sur la carte possède ces propriétés :

#### **A. PROPRIÉTÉS SPATIALES**
- **`BlockName name`** : Type de bloc
- **`int x, y`** : Coordonnées sur la grille

#### **B. ANIMATION INDIVIDUELLE**
- **`float currentFrame`** : Frame actuelle de l'animation (spécifique à chaque instance)
- **`int rotationAngle`** : Angle de rotation (0, 90, 180, 270)

#### **C. TRANSFORMATION TEMPORELLE**
- **`float transformationTimer`** : Minuteur de transformation
- **`float transformationTarget`** : Temps cible pour la transformation
- **`bool hasBeenInitializedForTransformation`** : Flag d'initialisation





# CONFIGURATION DES ENTITEES

### 1. Identification et Rendu

```cpp
EntityName type;                    // Type unique de l'entité (PLAYER, ANTAGONIST, SHARK, etc.)
ElementName elementName;            // Nom de l'élément graphique associé
float scale;                        // Échelle de rendu (1.0 = taille normale)
```

### 2. Système de Santé et Dégâts

```cpp
int lifePoints = 100;               // Points de vie de l'entité
int damagePoints = 0;               // Points de dégâts infligés par l'entité
std::vector<BlockName> damageBlocks; // Blocs qui infligent des dégâts à l'entité
```

### 3. Configuration des Sprites et Animations

```cpp
// Configuration par défaut
int defaultSpriteSheetPhase;        // Phase de sprite par défaut
int defaultSpriteSheetFrame;        // Frame de sprite par défaut
float defaultAnimationSpeed;        // Vitesse d'animation par défaut

// Phases d'animation pour les directions de marche
int spritePhaseWalkUp;              // Phase sprite pour marcher vers le haut
int spritePhaseWalkDown;            // Phase sprite pour marcher vers le bas
int spritePhaseWalkLeft;            // Phase sprite pour marcher vers la gauche
int spritePhaseWalkRight;           // Phase sprite pour marcher vers la droite
```

### 4. Système de Mouvement

```cpp
// Vitesses de déplacement
float normalWalkingSpeed;           // Vitesse de marche normale
float normalWalkingAnimationSpeed;  // Vitesse d'animation en marche normale
float sprintWalkingSpeed;           // Vitesse de course
float sprintWalkingAnimationSpeed;  // Vitesse d'animation en course
```

### 5. Système de Collision Avancé

#### Collision de Base
```cpp
bool canCollide;                    // Active/désactive les collisions
std::vector<std::pair<float, float>> collisionShapePoints; // Points définissant la forme de collision
```

#### Collision Granulaire avec les Éléments
```cpp
std::vector<ElementName> avoidanceElements;  // Éléments évités en pathfinding
std::vector<ElementName> collisionElements;  // Éléments bloquant physiquement
```

#### Collision Granulaire avec les Blocs
```cpp
std::vector<BlockName> avoidanceBlocks;      // Blocs évités en pathfinding
std::vector<BlockName> collisionBlocks;      // Blocs bloquant physiquement
```

#### Collision Granulaire avec les Entités
```cpp
std::vector<EntityName> avoidanceEntities;   // Entités évitées en pathfinding
std::vector<EntityName> collisionEntities;   // Entités bloquant physiquement
```

### 6. Contrôle des Limites de Carte

```cpp
bool offMapAvoidance = true;        // Éviter les bords de carte en pathfinding
bool offMapCollision = true;        // Collisions avec les bords de carte
```

### 7. Système Comportemental Automatique

#### Configuration Générale
```cpp
bool automaticBehaviors = false;    // Active/désactive les comportements automatiques
```

#### État Passif (Exploration Aléatoire)
```cpp
bool passiveState = false;          // Active l'état passif
float passiveStateWalkingRadius = 10.0f;  // Rayon d'exploration
float passiveStateRandomWalkTriggerTimeIntervalMin = 2.0f;  // Temps min entre marches
float passiveStateRandomWalkTriggerTimeIntervalMax = 8.0f;  // Temps max entre marches
```

#### État d'Alerte
```cpp
bool alertState = false;            // Active l'état d'alerte
float alertStateStartRadius = 3.0f; // Rayon de début d'alerte
float alertStateEndRadius = 8.0f;   // Rayon de fin d'alerte
std::vector<EntityName> alertStateTriggerEntitiesList; // Entités déclenchant l'alerte
```

#### État de Fuite
```cpp
bool fleeState = false;             // Active l'état de fuite
bool fleeStateRunning = true;       // Courir pendant la fuite
float fleeStateStartRadius = 3.0f;  // Rayon de début de fuite
float fleeStateEndRadius = 6.0f;    // Rayon de fin de fuite
float fleeStateMinDistance = 8.0f;  // Distance minimale à maintenir
float fleeStateMaxDistance = 12.0f; // Distance maximale de fuite
std::vector<EntityName> fleeStateTriggerEntitiesList; // Entités déclenchant la fuite
```

#### État d'Attaque
```cpp
bool attackState = false;           // Active l'état d'attaque
bool attackStateRunning = true;     // Courir pendant l'attaque
float attackStateStartRadius = 5.0f; // Rayon de début d'attaque
float attackStateEndRadius = 10.0f; // Rayon de fin d'attaque
float attackStateWaitBeforeChargeMin = 1.0f; // Temps min avant charge
float attackStateWaitBeforeChargeMax = 3.0f; // Temps max avant charge
std::vector<EntityName> attackStateTriggerEntitiesList; // Entités déclenchant l'attaque
```





CONFIGURATION DES ELEMENTS

### 2. Type et Dimensions de Texture

```cpp
ElementTextureType type = ElementTextureType::STATIC; // Type : STATIC ou SPRITESHEET
int spriteWidth = 0;            // Largeur d'un sprite individuel (pour spritesheets)
int spriteHeight = 0;           // Hauteur d'un sprite individuel (pour spritesheets)
int totalWidth = 0;             // Largeur totale de la texture
int totalHeight = 0;            // Hauteur totale de la texture
GLuint textureID = 0;           // Handle OpenGL de la texture
```

### 3. Système de Point d'Ancrage

```cpp
AnchorPoint anchorPoint = AnchorPoint::CENTER; // Point d'ancrage par défaut
float anchorOffsetX = 0.0f;     // Décalage X depuis le point d'ancrage
float anchorOffsetY = 0.0f;     // Décalage Y depuis le point d'ancrage
```

### 4. Système de Collision

```cpp
bool hasCollision = false;      // Active/désactive la détection de collision
std::vector<std::pair<float, float>> collisionShapePoints; // Points définissant le polygone de collision
```

## Énumération AnchorPoint

Le système propose **7 points d'ancrage différents** :

```cpp
enum class AnchorPoint {
    CENTER,              // Centre de la texture (par défaut)
    TOP_LEFT_CORNER,     // Coin supérieur gauche
    TOP_RIGHT_CORNER,    // Coin supérieur droit
    BOTTOM_LEFT_CORNER,  // Coin inférieur gauche
    BOTTOM_RIGHT_CORNER, // Coin inférieur droit
    BOTTOM_CENTER,       // Centre inférieur (idéal pour personnages)
    USE_TEXTURE_DEFAULT  // Utilise le point d'ancrage par défaut de la texture
};
```

## Énumération ElementTextureType

```cpp
enum class ElementTextureType {
    STATIC,     // Texture statique simple
    SPRITESHEET // Feuille de sprites avec animations
};
```





# CONFIGURATION DES UIELEMENTS


### 2. Type et Dimensions de Texture

```cpp
UIElementTextureType type = UIElementTextureType::STATIC; // Type : STATIC ou SPRITESHEET
int spriteWidth = 0;            // Largeur d'un sprite individuel (pour spritesheets)
int spriteHeight = 0;           // Hauteur d'un sprite individuel (pour spritesheets)
```

### 3. Configuration des Sprites et Animations

```cpp
int defaultSpriteSheetPhase = 0;  // Phase de sprite par défaut (ligne)
int defaultSpriteSheetFrame = 0;  // Frame de sprite par défaut (colonne)
bool isAnimated = false;          // Animation automatique activée
float animationSpeed = 10.0f;     // Vitesse d'animation en FPS
```

### 4. Système de Marges pour Positionnement

```cpp
float marginTop = 0.0f;         // Marge supérieure en pixels
float marginBottom = 0.0f;      // Marge inférieure en pixels
float marginLeft = 0.0f;        // Marge gauche en pixels
float marginRight = 0.0f;       // Marge droite en pixels
```

## Énumérations du Système UI

### UIElementName

```cpp
enum class UIElementName {
    START_MENU,     // Menu de démarrage
    PAUSE_MENU,     // Menu de pause
    GAME_OVER,      // Écran de game over
    WIN_MENU,       // Menu de victoire
    HEALTH_BAR,     // Barre de vie
    SCORE_DISPLAY,  // Affichage du score
    BUTTON_START,   // Bouton démarrer
    BUTTON_QUIT,    // Bouton quitter
    COCONUTS,       // Compteur de noix de coco
    LOGO,           // Logo du jeu
    LOADER          // Écran de chargement
};
```

### UIElementTextureType

```cpp
enum class UIElementTextureType {
    STATIC,     // Texture statique simple
    SPRITESHEET // Feuille de sprites avec animations
};
```

### UIElementPosition

```cpp
enum class UIElementPosition {
    TOP_LEFT_CORNER,     // Coin supérieur gauche
    TOP_RIGHT_CORNER,    // Coin supérieur droit
    BOTTOM_LEFT_CORNER,  // Coin inférieur gauche
    BOTTOM_RIGHT_CORNER, // Coin inférieur droit
    CENTER               // Centre de l'écran
};
```





# CONFIGURATION DES REGLES DE GENERATION


#### **1. Configuration de Base du Spawn**
- `SpawnType spawnType` - Type de spawn (ELEMENT, ENTITY, BLOCK)
- `std::vector<ElementName> spawnElements` - Éléments à spawner (équiprobables si multiples)
- `std::vector<EntityName> spawnEntities` - Entités à spawner
- `std::vector<BlockName> spawnBlocks` - Types de blocs sur lesquels spawner

#### **2. Probabilité et Contraintes de Spawn**
- `int spawnChance` - Probabilité 1/spawnChance (ex: 50 = 1/50 chance)
- `int maxSpawns` - Nombre maximum total de spawns pour cette règle

#### **3. Contraintes de Distance**
- `float minDistanceFromSameRule` - Distance minimale entre spawns de la même règle
- `float maxDistanceFromBlocks` - Distance maximale des blocs spécifiés (0 = pas de contrainte)
- `std::vector<BlockName> proximityBlocks` - Types de blocs dont il faut être proche

#### **4. Spawn en Groupe**
- `bool spawnInGroup` - Activer le spawn en groupe
- `float groupRadius` - Rayon pour le spawn en groupe
- `int groupNumberMin/Max` - Nombre min/max d'éléments par groupe

#### **5. Propriétés des Éléments**
- `float scaleMin/Max` - Multiplicateurs d'échelle min/max
- `float baseScale` - Échelle de base avant variation aléatoire
- `float rotation` - Rotation en degrés (0 = pas de rotation, -1 = aléatoire)

#### **6. Propriétés des Spritesheets**
- `int defaultSpriteSheetPhase/Frame` - Phase et frame par défaut
- `bool randomDefaultSpriteSheetPhase` - Randomiser la phase pour les entités
- `bool isAnimated` - Élément animé
- `float animationSpeed` - Vitesse d'animation

#### **7. Ancrage et Positionnement**
- `AnchorPoint anchorPoint` - Point d'ancrage
- `float additionalXAnchorOffset/YAnchorOffset` - Offsets d'ancrage additionnels

#### **8. Stratégie de Placement**
- `bool randomPlacement` - Utiliser une sélection aléatoire au lieu de séquentielle
- `std::string ruleName` - Nom optionnel pour le débogage
