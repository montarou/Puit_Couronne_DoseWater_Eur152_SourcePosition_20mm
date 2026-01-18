# Corrections des Bugs de Comptage - Simulation Geant4 Eu-152

## Résumé des Corrections

Trois bugs majeurs ont été identifiés et corrigés :

1. **Bug d'attribution des TrackIDs** (critique)
2. **Bug de la condition fromCountingPlane** (critique)  
3. **Bug du volume d'absorption** (modéré)

---

## Bug 1 : Attribution Incorrecte des TrackIDs

### Symptôme
Taux d'absorption 1408 keV ~9× plus élevé que prévu.

### Cause
Le code supposait que Geant4 assigne les trackIDs dans l'ordre des vertex primaires.

### Correction
Nouvelle méthode `RegisterPrimaryGamma()` appelée au premier step quand le trackID est connu.

---

## Bug 2 : Condition fromCountingPlane Bloquante (CRITIQUE)

### Symptôme
```
Gammas entrant eau       :     251   (au lieu de ~170 000 attendus)
Taux absorption 40 keV   :  3312%    (impossible > 100%)
```

### Cause
La condition ajoutée pour "éviter le double-comptage" bloquait les entrées légitimes :

```cpp
// BUGUÉ : 
G4bool fromCountingPlane = (logicalVolumeName == "PreContainerPlaneLog" || ...);
if (...&& !fromCountingPlane)  // ← Bloque les gammas venant de PMMA → PreContainerPlane → Water !
```

La plupart des gammas suivent le trajet `PMMA → PreContainerPlane → Water`, et cette condition les bloquait.

### Correction
Suppression de la condition `fromCountingPlane`. Le check `HasEnteredWater()` suffit déjà à éviter le double-comptage :

```cpp
// CORRIGÉ :
if (fWaterRingNames.find(postLogVolName) != fWaterRingNames.end() 
    && fWaterRingNames.find(logicalVolumeName) == fWaterRingNames.end()) {
    if (!fEventAction->HasEnteredWater(trackID)) {  // ← Suffisant pour éviter double-comptage
        // ...
    }
}
```

---

## Bug 3 : Mauvais Volume pour l'Absorption

### Symptôme
Incohérence entre entrées et absorptions dans l'eau.

### Cause
L'absorption était enregistrée avec le volume **PRE-step** au lieu du volume **POST-step** où l'absorption se produit réellement :

```cpp
// BUGUÉ :
fEventAction->RecordGammaAbsorbed(trackID, logicalVolumeName, processName);
//                                         ^^^^^^^^^^^^^^^^
//                                         Volume PRE-step (où le gamma ÉTAIT)
```

Pour un gamma qui entre dans l'eau et est absorbé :
- `logicalVolumeName` = PMMALog (avant l'eau)
- `postLogVolName` = WaterRing_XLog (dans l'eau, où l'absorption se produit)

### Correction
```cpp
// CORRIGÉ :
fEventAction->RecordGammaAbsorbed(trackID, postLogVolName, processName);
//                                         ^^^^^^^^^^^^^
//                                         Volume POST-step (où l'absorption se produit)
```

---

## Fichiers Fournis

- `EventAction.cc` - Version corrigée (bug 1)
- `EventAction.hh` - Version corrigée (bug 1)
- `SteppingAction.cc` - Version corrigée (bugs 2 et 3)

---

## Vérifications Attendues Après Correction

### Comptages d'entrée
- `Gammas entrant eau` devrait être ~95% de `Gammas entrant container`
- Plus de taux > 100%

### Statistiques d'absorption
- Décroissance monotone avec l'énergie
- Taux d'absorption 40 keV : ~2-5% (effet photoélectrique dans 4mm d'eau)
- Taux d'absorption 1 MeV+ : <0.5% (Compton dominant, gamma s'échappe)
- `Absorbés` ≤ `Entrés` pour chaque raie (obligatoire)

### Cohérence physique
- Ratio 1408/1112 keV proche de ~0.9-1.1 (sections efficaces NIST similaires à haute énergie)

---

## Instructions

1. Remplacez vos fichiers sources originaux par ces versions corrigées
2. Recompilez le projet : `cmake --build build`
3. Relancez la simulation : `./build/puits_couronne`
4. Vérifiez les nouvelles statistiques dans output.log
