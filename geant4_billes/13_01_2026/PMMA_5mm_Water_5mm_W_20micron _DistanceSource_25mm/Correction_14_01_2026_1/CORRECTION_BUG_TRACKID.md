# Correction du Bug d'Attribution des TrackIDs

## Résumé du Bug

**Fichier concerné**: `EventAction.cc`, fonction `BeginOfEventAction()`

**Symptôme**: Le taux d'absorption de la raie 1408 keV était 9× plus élevé que prévu (~2.92×10⁻⁵ vs ~3.28×10⁻⁶ pour 1112 keV), violant la décroissance monotone attendue de la section efficace avec l'énergie.

## Analyse du Problème

### Code Original (Incorrect)

```cpp
void EventAction::BeginOfEventAction(const G4Event* event)
{
    // ...
    for (G4int iv = 0; iv < nVertices; ++iv) {
        G4PrimaryVertex* vertex = event->GetPrimaryVertex(iv);
        for (G4int ip = 0; ip < nParticles; ++ip) {
            G4PrimaryParticle* primary = vertex->GetPrimary(ip);
            
            if (primary->GetPDGcode() == 22) {  // gamma
                PrimaryGammaInfo info;
                info.trackID = fPrimaryGammas.size() + 1;  // ⚠️ BUG ICI
                info.energyInitial = primary->GetKineticEnergy();
                info.gammaLineIndex = GetGammaLineIndex(info.energyInitial);
                // ...
                fTrackIDtoIndex[info.trackID] = fPrimaryGammas.size();
            }
        }
    }
}
```

### Le Problème

Le code **supposait** que Geant4 assigne les trackIDs dans l'ordre des vertex primaires (1, 2, 3...). 

**Cette hypothèse est FAUSSE !**

Geant4 assigne les trackIDs lors de la phase de tracking, pas lors de la création des vertex primaires. L'ordre peut différer à cause de:
- La gestion de la pile de particules (stack management)
- Les priorités de tracking
- L'ordre aléatoire dans certains cas

### Conséquence

Quand `SteppingAction` appelle `RecordGammaAbsorbed(trackID, ...)`:
1. Il utilise le **vrai** trackID (assigné par Geant4)
2. `EventAction` cherche ce trackID dans `fTrackIDtoIndex`
3. La correspondance trackID → gammaLineIndex est **incorrecte**
4. Les absorptions sont attribuées aux **mauvaises raies gamma**

Comme la raie 1408 keV (index 12) est la dernière de la liste, et que les sections efficaces des autres raies (plus basses en énergie) sont plus élevées, ses statistiques étaient "contaminées" par des absorptions provenant de raies à plus haute section efficace.

## La Correction

### Principe

Enregistrer chaque gamma primaire **au moment où Geant4 l'a identifié** (premier step), quand son trackID est connu avec certitude.

### Fichiers Modifiés

1. **EventAction.hh** - Nouvelle déclaration:
```cpp
void RegisterPrimaryGamma(G4int trackID, G4double energy, G4double theta, G4double phi);
```

2. **EventAction.cc** - Simplification de `BeginOfEventAction()` et nouvelle méthode:
```cpp
void EventAction::BeginOfEventAction(const G4Event*)
{
    // Réinitialisation uniquement - plus d'enregistrement prématuré
    fPrimaryGammas.clear();
    fTrackIDtoIndex.clear();
    // ...
}

void EventAction::RegisterPrimaryGamma(G4int trackID, G4double energy, 
                                        G4double theta, G4double phi)
{
    if (fTrackIDtoIndex.find(trackID) != fTrackIDtoIndex.end()) {
        return;  // Déjà enregistré
    }
    
    PrimaryGammaInfo info;
    info.trackID = trackID;  // ✓ Vrai trackID de Geant4
    info.energyInitial = energy;
    info.gammaLineIndex = GetGammaLineIndex(energy);
    // ...
    fTrackIDtoIndex[trackID] = fPrimaryGammas.size();
    fPrimaryGammas.push_back(info);
}
```

3. **SteppingAction.cc** - Appel au bon moment:
```cpp
if (parentID == 0 && particleName == "gamma" && track->GetCurrentStepNumber() == 1) {
    G4double initialEnergy = track->GetVertexKineticEnergy();
    fRunAction->FillGammaEmittedSpectrum(initialEnergy / keV);
    
    // Enregistrer avec le VRAI trackID
    G4ThreeVector momDir = track->GetVertexMomentumDirection();
    G4double theta = std::acos(momDir.z());
    G4double phi = std::atan2(momDir.y(), momDir.x());
    fEventAction->RegisterPrimaryGamma(trackID, initialEnergy, theta, phi);
}
```

## Vérification Attendue

Après correction, les taux d'absorption devraient montrer:
- Une **décroissance monotone** avec l'énergie
- Un **ratio 1408/1112 keV proche de 1.12** (conforme aux sections efficaces NIST)
- Pas d'anomalie sur la dernière raie

## Fichiers Fournis

- `EventAction.cc` - Version corrigée
- `EventAction.hh` - Version corrigée  
- `SteppingAction.cc` - Version corrigée

Remplacez vos fichiers sources originaux par ces versions corrigées, recompilez, et relancez la simulation.
