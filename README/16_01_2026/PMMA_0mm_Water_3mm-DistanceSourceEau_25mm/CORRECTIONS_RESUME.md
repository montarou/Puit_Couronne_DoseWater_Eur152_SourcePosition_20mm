# Corrections apportées aux fichiers Geant4

## Récapitulatif des volumes logiques définis dans DetectorConstruction.cc

| Volume Logique | Matériau | Position Z (mm) | Rôle |
|----------------|----------|-----------------|------|
| `PreContainerPlaneLog` | Air | 99-100 | Plan de diagnostic avant eau |
| `Water1Log` | Eau | 100-102 | Première couche d'eau (2mm) |
| `WaterRing_0Log` à `WaterRing_4Log` | Eau | 102-103 | Anneaux de mesure de dose (1mm) |
| `PostContainerPlaneLog` | Polystyrène | 103-104 | Fond boîte de Petri |
| `TungstenFoilLog` | Tungstène | 104-104.05 | Feuille de rétrodiffusion |

---

## Fichier 1 : PrimaryGeneratorAction.cc

### Correction apportée
**Ligne 16** : Position de la source corrigée pour être à exactement 25 mm de la surface de l'eau.

**Avant :**
```cpp
fSourcePosition(0., 0., 70.9*mm)  // Source à z = 70.9 mm (25 mm avant entrée eau)
```

**Après :**
```cpp
fSourcePosition(0., 0., 75.0*mm)  // Source à z = 75 mm (25 mm avant surface eau à z=100 mm)
```

### Impact géométrique
Avec la source à z = 75 mm et le cône de 45° :
- **Angle sous-tendant la surface eau** : arctan(25/25) = **45.00°** (exactement couvert)
- **Angle sous-tendant les anneaux** : arctan(25/27) = **42.80°**
- **100%** des gammas du cône atteignent la surface de l'eau (contre ~82% avant)

---

## Fichier 2 : RunAction.cc

### Corrections apportées

**Ligne 24** : Position de la source mise à jour
```cpp
fSourcePosZ(75.0*mm),  // Source à z = 75 mm (25 mm avant surface eau à z=100 mm)
```

**Lignes 378-379** : Libellés de sortie clarifiés
```
║  Gammas entrant Water1      : ...    (surface eau, z=100mm)
║  Gammas entrant anneaux     : ...    (anneaux mesure, z=102mm)
```

---

## Fichier 3 : SteppingAction.cc

### Bug corrigé
**Lignes 227-246** : Le code référençait des volumes `ContainerWallLog` et `ContainerTopLog` qui **n'existent pas** dans la géométrie.

**Avant :**
```cpp
if ((postLogVolName == "ContainerWallLog" || postLogVolName == "ContainerTopLog") 
    && logicalVolumeName != "ContainerWallLog" && logicalVolumeName != "ContainerTopLog") {
```

**Après :**
```cpp
if (postLogVolName == "Water1Log" && logicalVolumeName != "Water1Log") {
```

---

## Résumé de la géométrie corrigée

```
Z (mm)
  │
104.05 ┼─────────────────────────────────── TungstenFoilLog
104.00 ┼─────────────────────────────────── PostContainerPlaneLog  
103.00 ┼─────────────────────────────────── WaterRing_0..4Log (MESURE DOSE)
102.00 ┼─────────────────────────────────── Water1Log
100.00 ┼─────────────────────────────────── PreContainerPlaneLog (surface eau)
 99.00 ┼───────────────────────────────────
       │         ↑
       │     25 mm (exactement)
       │         ↓
 75.00 ┼     SOURCE Eu-152 ←── Cône 45° couvre exactement r=25mm à z=100mm
```

---

## Résultats attendus après correction

Pour 100 000 événements :
- **Gammas émis** : ~203 000
- **Gammas entrant Water1** (z=100mm) : ~203 000 (100% du cône)
- **Gammas entrant anneaux** (z=102mm) : ~185 000 (~91% - quelques pertes dans Water1)
- **Ratio anneaux/émis** : ~91% (au lieu de ~74% avant correction)

