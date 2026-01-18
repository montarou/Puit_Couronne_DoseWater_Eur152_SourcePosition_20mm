# Corrections des Bugs de Comptage - Simulation Geant4 Eu-152

## Résumé des Corrections

Deux bugs majeurs ont été identifiés et corrigés :

1. **Bug d'attribution des TrackIDs** (critique)
2. **Bug de double-comptage des entrées** (modéré)

---

## Bug 1 : Attribution Incorrecte des TrackIDs

### Fichier concerné
`EventAction.cc`, fonction `BeginOfEventAction()`

### Symptôme
Le taux d'absorption de la raie 1408 keV était 9× plus élevé que prévu (~2.92×10⁻⁵ vs ~3.28×10⁻⁶ pour 1112 keV), violant la décroissance monotone attendue de la section efficace avec l'énergie.

### Cause
Le code supposait que Geant4 assigne les trackIDs dans l'ordre des vertex primaires (1, 2, 3...). Cette hypothèse est **FAUSSE** - Geant4 assigne les trackIDs lors de la phase de tracking, pas lors de la création des vertex.

### Correction
- `BeginOfEventAction()` ne fait plus que réinitialiser les structures
- Nouvelle méthode `RegisterPrimaryGamma(trackID, energy, theta, phi)` 
- Appelée depuis `SteppingAction` au premier step de chaque gamma

---

## Bug 2 : Double-Comptage des Entrées

### Fichiers concernés
`SteppingAction.cc`, `EventAction.cc`, `EventAction.hh`

### Symptôme
```
Gammas entrant container : 172 850
Gammas entrant eau       : 193 127  ← Incohérent !
```

Le log montrait plusieurs messages "WATER_ENTRY" pour le même gamma :
```
WATER_ENTRY | Event 2 | trackID=2 | z=99.5 mm
WATER_ENTRY | Event 2 | trackID=2 | z=103.152 mm  ← Fausse entrée !
```

### Causes
1. **Plans de comptage** : Les volumes `PreContainerPlaneLog` et `PostContainerPlaneLog` étaient considérés comme "non-eau", causant de fausses détections de transition eau → non-eau → eau.

2. **Log mal placé** : Le message de log était affiché même quand le comptage était bloqué par `HasEnteredWater()`.

3. **Pas de protection pour container** : L'entrée dans le container n'avait aucune protection anti-double-comptage.

### Corrections

#### SteppingAction.cc - Entrée eau
```cpp
// Exclure les plans de comptage de la détection de transition
G4bool fromCountingPlane = (logicalVolumeName == "PreContainerPlaneLog" || 
                            logicalVolumeName == "PostContainerPlaneLog");

if (fWaterRingNames.find(postLogVolName) != fWaterRingNames.end() 
    && fWaterRingNames.find(logicalVolumeName) == fWaterRingNames.end()
    && !fromCountingPlane) {  // NOUVEAU
    
    if (particleName == "gamma" && parentID == 0) {
        if (!fEventAction->HasEnteredWater(trackID)) {
            // Comptage ET log à l'intérieur du même bloc
            fRunAction->IncrementWaterEntry();
            fEventAction->RecordWaterEntry(trackID, kineticEnergy);
            // Log ici...
        }
    }
}
```

#### SteppingAction.cc - Entrée container
```cpp
if (!fEventAction->HasEnteredContainer(trackID)) {
    fRunAction->IncrementContainerEntry();
    fEventAction->RecordContainerEntry(trackID);
    // Log ici...
}
```

#### EventAction.hh - Nouvelles méthodes
```cpp
void RecordContainerEntry(G4int trackID);
G4bool HasEnteredContainer(G4int trackID) const;
```

#### EventAction.cc - Nouveau set
```cpp
std::set<G4int> fGammasEnteredContainer;  // Anti-double-comptage
```

---

## Fichiers Fournis

- `EventAction.cc` - Version corrigée (bugs 1 et 2)
- `EventAction.hh` - Version corrigée (nouvelles déclarations)
- `SteppingAction.cc` - Version corrigée (bug 2)

---

## Vérifications Attendues

Après correction, vous devriez observer :

### Statistiques d'absorption
- Décroissance monotone avec l'énergie
- Ratio 1408/1112 keV proche de 1.12 (NIST)
- Pas d'anomalie sur la dernière raie

### Comptages d'entrée
- `Gammas entrant eau` ≤ `Gammas entrant container` + `Gammas absorbés eau`
- Plus de messages "WATER_ENTRY" dupliqués pour le même trackID
- Cohérence entre le total et la somme des stats par raie

---

## Instructions

1. Remplacez vos fichiers sources originaux par ces versions corrigées
2. Recompilez le projet
3. Relancez la simulation
4. Vérifiez les nouvelles statistiques dans output.log
