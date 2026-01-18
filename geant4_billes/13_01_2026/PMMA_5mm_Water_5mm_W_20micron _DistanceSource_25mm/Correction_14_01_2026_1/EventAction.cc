#include "EventAction.hh"
#include "RunAction.hh"
#include "Logger.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include <sstream>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
// DÉFINITION DES RAIES GAMMA Eu-152 (énergies en keV)
// ═══════════════════════════════════════════════════════════════
const std::array<G4double, EventAction::kNbGammaLines> EventAction::kGammaLineEnergies = {
    39.52,    // 0: 20.8% (raie X)
    40.12,    // 1: 37.7% (raie X)
    121.78,   // 2: 28.41%
    244.70,   // 3: 7.53%
    344.28,   // 4: 26.59%
    411.12,   // 5: 2.24%
    443.97,   // 6: 2.83%
    778.90,   // 7: 12.97%
    867.38,   // 8: 4.24%
    964.08,   // 9: 14.63%
    1085.87,  // 10: 10.21%
    1112.07,  // 11: 13.64%
    1408.01   // 12: 21.01%
};

const std::array<G4String, EventAction::kNbGammaLines> EventAction::kGammaLineNames = {
    "40 keV (X)",
    "40 keV (X)",
    "122 keV",
    "245 keV",
    "344 keV",
    "411 keV",
    "444 keV",
    "779 keV",
    "867 keV",
    "964 keV",
    "1086 keV",
    "1112 keV",
    "1408 keV"
};

// ═══════════════════════════════════════════════════════════════
// DÉFINITION DES NOMS DE PROCESSUS
// ═══════════════════════════════════════════════════════════════
const std::array<G4String, EventAction::kNbProcesses> kProcessNames = {
    "Photoelectric",
    "Compton",
    "Pair Production",
    "Other"
};

G4int EventAction::GetProcessIndex(const G4String& processName)
{
    if (processName == "phot") return kPhotoelectric;
    if (processName == "compt") return kCompton;
    if (processName == "conv") return kPairProduction;
    return kOther;
}

G4String EventAction::GetProcessName(G4int processIndex)
{
    if (processIndex >= 0 && processIndex < kNbProcesses) {
        return kProcessNames[processIndex];
    }
    return "Unknown";
}

EventAction::EventAction(RunAction* runAction)
: G4UserEventAction(),
  fRunAction(runAction),
  fPreContainerNPhotons(0),
  fPreContainerSumEPhotons(0.),
  fPreContainerNElectrons(0),
  fPreContainerSumEElectrons(0.),
  fPostContainerNPhotonsBack(0),
  fPostContainerSumEPhotonsBack(0.),
  fPostContainerNElectronsBack(0),
  fPostContainerSumEElectronsBack(0.),
  fPostContainerNPhotonsFwd(0),
  fPostContainerSumEPhotonsFwd(0.),
  fPostContainerNElectronsFwd(0),
  fPostContainerSumEElectronsFwd(0.),
  fVerboseLevel(1)
{
    // Initialisation des tableaux de dépôt d'énergie
    fRingEnergyDeposit.fill(0.);
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
}

EventAction::~EventAction()
{}

// ═══════════════════════════════════════════════════════════════
// IDENTIFICATION DES RAIES GAMMA
// ═══════════════════════════════════════════════════════════════

G4int EventAction::GetGammaLineIndex(G4double energy)
{
    // Tolérance de 0.5 keV pour l'identification
    const G4double tolerance = 0.5 * keV;
    
    for (G4int i = 0; i < kNbGammaLines; ++i) {
        if (std::abs(energy - kGammaLineEnergies[i] * keV) < tolerance) {
            return i;
        }
    }
    return -1;  // Raie non identifiée
}

G4double EventAction::GetGammaLineEnergy(G4int lineIndex)
{
    if (lineIndex >= 0 && lineIndex < kNbGammaLines) {
        return kGammaLineEnergies[lineIndex];
    }
    return 0.;
}

G4String EventAction::GetGammaLineName(G4int lineIndex)
{
    if (lineIndex >= 0 && lineIndex < kNbGammaLines) {
        return kGammaLineNames[lineIndex];
    }
    return "Unknown";
}

// ═══════════════════════════════════════════════════════════════
// DÉBUT ET FIN D'ÉVÉNEMENT
// ═══════════════════════════════════════════════════════════════

void EventAction::BeginOfEventAction(const G4Event* /*event*/)
{
    // Réinitialiser les structures pour le nouvel événement
    fPrimaryGammas.clear();
    fTrackIDtoIndex.clear();
    fGammasEnteredWater.clear();  // Réinitialiser le set anti-double-comptage
    
    // Réinitialiser les dépôts d'énergie
    fRingEnergyDeposit.fill(0.);
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    
    // Réinitialiser les comptages aux plans container
    fPreContainerNPhotons = 0;
    fPreContainerSumEPhotons = 0.;
    fPreContainerNElectrons = 0;
    fPreContainerSumEElectrons = 0.;
    
    fPostContainerNPhotonsBack = 0;
    fPostContainerSumEPhotonsBack = 0.;
    fPostContainerNElectronsBack = 0;
    fPostContainerSumEElectronsBack = 0.;
    
    fPostContainerNPhotonsFwd = 0;
    fPostContainerSumEPhotonsFwd = 0.;
    fPostContainerNElectronsFwd = 0;
    fPostContainerSumEElectronsFwd = 0.;
    
    // ═══════════════════════════════════════════════════════════════
    // CORRECTION BUG : Ne PAS enregistrer les primaires ici !
    // Les trackIDs ne sont pas encore assignés par Geant4.
    // L'enregistrement se fait maintenant dans RegisterPrimaryGamma()
    // appelé depuis SteppingAction au premier step de chaque gamma.
    // ═══════════════════════════════════════════════════════════════
}

// ═══════════════════════════════════════════════════════════════
// NOUVELLE MÉTHODE : Enregistrement d'un gamma primaire
// Appelée depuis SteppingAction au premier step (trackID maintenant connu)
// ═══════════════════════════════════════════════════════════════
void EventAction::RegisterPrimaryGamma(G4int trackID, G4double energy, 
                                        G4double theta, G4double phi)
{
    // Vérifier si ce trackID n'est pas déjà enregistré
    if (fTrackIDtoIndex.find(trackID) != fTrackIDtoIndex.end()) {
        return;  // Déjà enregistré
    }
    
    PrimaryGammaInfo info;
    info.trackID = trackID;
    info.energyInitial = energy;
    info.gammaLineIndex = GetGammaLineIndex(energy);
    info.theta = theta;
    info.phi = phi;
    info.absorbedInWater = false;
    info.enteredWater = false;
    info.absorptionProcess = -1;
    
    fTrackIDtoIndex[trackID] = fPrimaryGammas.size();
    fPrimaryGammas.push_back(info);
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    G4int eventID = event->GetEventID();
    
    // Collecter les statistiques pour chaque raie
    std::vector<G4double> primaryEnergies;
    for (const auto& gamma : fPrimaryGammas) {
        primaryEnergies.push_back(gamma.energyInitial);
        
        // Enregistrer les statistiques par raie (SANS FILTRE)
        if (gamma.gammaLineIndex >= 0) {
            fRunAction->RecordGammaLineStatistics(
                gamma.gammaLineIndex,
                gamma.enteredWater,
                gamma.absorbedInWater,
                gamma.absorptionProcess  // processus d'absorption
            );
        }
    }
    
    // Transférer les dépôts d'énergie vers RunAction
    G4double totalDeposit = 0.;
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        if (fRingEnergyDeposit[i] > 0.) {
            fRunAction->AddRingEnergy(i, fRingEnergyDeposit[i]);
            totalDeposit += fRingEnergyDeposit[i];
            
            // Transférer aussi les dépôts par raie
            for (G4int j = 0; j < kNbGammaLines; ++j) {
                if (fRingEnergyByLine[i][j] > 0.) {
                    fRunAction->AddRingEnergyByLine(i, j, fRingEnergyByLine[i][j]);
                }
            }
        }
    }
    
    // Enregistrer les statistiques globales de l'événement
    fRunAction->RecordEventStatistics(
        fPrimaryGammas.size(),
        primaryEnergies,
        GetNumberTransmitted(),
        GetNumberAbsorbed(),
        totalDeposit,
        fRingEnergyDeposit
    );
    
    // Enregistrer les comptages aux plans container
    fRunAction->RecordContainerPlaneStatistics(
        fPreContainerNPhotons, fPreContainerSumEPhotons,
        fPreContainerNElectrons, fPreContainerSumEElectrons,
        fPostContainerNPhotonsBack, fPostContainerSumEPhotonsBack,
        fPostContainerNElectronsBack, fPostContainerSumEElectronsBack,
        fPostContainerNPhotonsFwd, fPostContainerSumEPhotonsFwd,
        fPostContainerNElectronsFwd, fPostContainerSumEElectronsFwd
    );
    
    // ─────────────────────────────────────────────────────────────
    // REMPLISSAGE DES NTUPLES PRECONTAINER ET POSTCONTAINER
    // ─────────────────────────────────────────────────────────────
    fRunAction->FillPreContainerNtuple(
        eventID,
        fPreContainerNPhotons,
        fPreContainerSumEPhotons / keV,
        fPreContainerNElectrons,
        fPreContainerSumEElectrons / keV
    );
    
    fRunAction->FillPostContainerNtuple(
        eventID,
        fPostContainerNPhotonsFwd,
        fPostContainerSumEPhotonsFwd / keV,
        fPostContainerNPhotonsBack,
        fPostContainerSumEPhotonsBack / keV,
        fPostContainerNElectronsFwd,
        fPostContainerSumEElectronsFwd / keV,
        fPostContainerNElectronsBack,
        fPostContainerSumEElectronsBack / keV
    );
    
    // ─────────────────────────────────────────────────────────────
    // REMPLISSAGE DU NTUPLE DOSES (pour analyse_dose_anneaux.C)
    // ─────────────────────────────────────────────────────────────
    fRunAction->FillDosesNtuple(
        eventID,
        fRingEnergyDeposit,
        totalDeposit,
        fPrimaryGammas.size(),
        GetNumberTransmitted(),
        GetNumberAbsorbed()
    );
    
    // Debug pour les premiers événements
    if (fVerboseLevel > 0 && eventID < 10) {
        std::stringstream ss;
        ss << "EVENT " << eventID << " SUMMARY (SANS FILTRE):";
        ss << " Primaries=" << fPrimaryGammas.size();
        ss << " EnteredWater=" << fGammasEnteredWater.size();
        ss << " Transmitted=" << GetNumberTransmitted();
        ss << " Absorbed=" << GetNumberAbsorbed();
        ss << " TotalDeposit=" << totalDeposit/keV << " keV";
        Logger::GetInstance()->LogLine(ss.str());
        
        // Résumé des plans container
        std::stringstream cs;
        cs << "  PreContainer: nPhotons=" << fPreContainerNPhotons 
           << " sumE=" << fPreContainerSumEPhotons/keV << " keV"
           << " | nElec=" << fPreContainerNElectrons 
           << " sumE=" << fPreContainerSumEElectrons/keV << " keV";
        Logger::GetInstance()->LogLine(cs.str());
        
        std::stringstream ps;
        ps << "  PostContainer: nPhotons_back=" << fPostContainerNPhotonsBack 
           << " sumE_back=" << fPostContainerSumEPhotonsBack/keV << " keV"
           << " | nPhotons_fwd=" << fPostContainerNPhotonsFwd 
           << " sumE_fwd=" << fPostContainerSumEPhotonsFwd/keV << " keV";
        Logger::GetInstance()->LogLine(ps.str());
    }
}

// ═══════════════════════════════════════════════════════════════
// ENREGISTREMENT DES PASSAGES
// ═══════════════════════════════════════════════════════════════

void EventAction::RecordWaterEntry(G4int trackID, G4double energy)
{
    // Ajouter au set pour éviter le double-comptage
    fGammasEnteredWater.insert(trackID);
    
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        fPrimaryGammas[it->second].enteredWater = true;
    }
}

G4bool EventAction::HasEnteredWater(G4int trackID) const
{
    return fGammasEnteredWater.find(trackID) != fGammasEnteredWater.end();
}

void EventAction::RecordGammaAbsorbed(G4int trackID, const G4String& volumeName, const G4String& processName)
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        // Enregistrer le processus d'absorption
        G4int processIndex = GetProcessIndex(processName);
        fPrimaryGammas[it->second].absorptionProcess = processIndex;
        
        if (volumeName.find("Water") != std::string::npos) {
            fPrimaryGammas[it->second].absorbedInWater = true;
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// DOSE DANS LES ANNEAUX D'EAU
// ═══════════════════════════════════════════════════════════════

void EventAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingEnergyDeposit[ringIndex] += edep;
    }
}

void EventAction::AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings &&
        lineIndex >= 0 && lineIndex < kNbGammaLines) {
        fRingEnergyByLine[ringIndex][lineIndex] += edep;
    }
}

G4double EventAction::GetRingEnergy(G4int ringIndex) const
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        return fRingEnergyDeposit[ringIndex];
    }
    return 0.;
}

G4double EventAction::GetTotalWaterEnergy() const
{
    G4double total = 0.;
    for (const auto& edep : fRingEnergyDeposit) {
        total += edep;
    }
    return total;
}

// ═══════════════════════════════════════════════════════════════
// COMPTAGES AUX PLANS CONTAINER
// ═══════════════════════════════════════════════════════════════

void EventAction::AddPreContainerPhoton(G4double energy)
{
    fPreContainerNPhotons++;
    fPreContainerSumEPhotons += energy;
}

void EventAction::AddPreContainerElectron(G4double energy)
{
    fPreContainerNElectrons++;
    fPreContainerSumEElectrons += energy;
}

void EventAction::AddPostContainerPhotonBack(G4double energy)
{
    fPostContainerNPhotonsBack++;
    fPostContainerSumEPhotonsBack += energy;
}

void EventAction::AddPostContainerElectronBack(G4double energy)
{
    fPostContainerNElectronsBack++;
    fPostContainerSumEElectronsBack += energy;
}

void EventAction::AddPostContainerPhotonFwd(G4double energy)
{
    fPostContainerNPhotonsFwd++;
    fPostContainerSumEPhotonsFwd += energy;
}

void EventAction::AddPostContainerElectronFwd(G4double energy)
{
    fPostContainerNElectronsFwd++;
    fPostContainerSumEElectronsFwd += energy;
}

// ═══════════════════════════════════════════════════════════════
// STATISTIQUES
// ═══════════════════════════════════════════════════════════════

G4int EventAction::GetNumberTransmitted() const
{
    // Sans filtre, on compte les gammas qui ont traversé l'eau sans être absorbés
    G4int count = 0;
    for (const auto& gamma : fPrimaryGammas) {
        if (gamma.enteredWater && !gamma.absorbedInWater) count++;
    }
    return count;
}

G4int EventAction::GetNumberAbsorbed() const
{
    G4int count = 0;
    for (const auto& gamma : fPrimaryGammas) {
        if (gamma.absorbedInWater) count++;
    }
    return count;
}

G4bool EventAction::IsPrimaryTrack(G4int trackID) const
{
    return fTrackIDtoIndex.find(trackID) != fTrackIDtoIndex.end();
}

G4int EventAction::GetGammaLineForTrack(G4int trackID) const
{
    auto it = fTrackIDtoIndex.find(trackID);
    if (it != fTrackIDtoIndex.end()) {
        return fPrimaryGammas[it->second].gammaLineIndex;
    }
    return -1;
}
