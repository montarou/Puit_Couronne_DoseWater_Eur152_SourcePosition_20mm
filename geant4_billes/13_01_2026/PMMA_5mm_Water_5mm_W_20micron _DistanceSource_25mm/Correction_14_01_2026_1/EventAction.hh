#ifndef EventAction_h
#define EventAction_h 1

#include "G4UserEventAction.hh"
#include "DetectorConstruction.hh"
#include "globals.hh"
#include <vector>
#include <map>
#include <set>
#include <array>

class RunAction;
class G4Event;

/// @brief Gestion des événements avec suivi des primaires par raie gamma
/// VERSION SANS FILTRE - Eu-152
///
/// Cette classe :
/// - Enregistre tous les gammas primaires émis à chaque événement
/// - Suit leur passage dans les volumes de détection
/// - Accumule les dépôts d'énergie par anneau d'eau
/// - Identifie les raies gamma Eu-152 associées à chaque dépôt

class EventAction : public G4UserEventAction
{
public:
    EventAction(RunAction* runAction);
    virtual ~EventAction();
    
    virtual void BeginOfEventAction(const G4Event*);
    virtual void EndOfEventAction(const G4Event*);

    // ═══════════════════════════════════════════════════════════════
    // CONSTANTES POUR LES RAIES GAMMA Eu-152
    // ═══════════════════════════════════════════════════════════════
    
    static const G4int kNbGammaLines = 13;
    static const std::array<G4double, kNbGammaLines> kGammaLineEnergies;
    static const std::array<G4String, kNbGammaLines> kGammaLineNames;
    
    /// Retourne l'index de la raie gamma correspondant à cette énergie (-1 si non trouvé)
    static G4int GetGammaLineIndex(G4double energy);
    
    /// Retourne l'énergie de la raie gamma (keV)
    static G4double GetGammaLineEnergy(G4int lineIndex);
    
    /// Retourne le nom de la raie gamma
    static G4String GetGammaLineName(G4int lineIndex);

    // ═══════════════════════════════════════════════════════════════
    // CONSTANTES POUR LES PROCESSUS D'ABSORPTION
    // ═══════════════════════════════════════════════════════════════
    
    static const G4int kNbProcesses = 4;
    enum ProcessType { kPhotoelectric = 0, kCompton = 1, kPairProduction = 2, kOther = 3 };
    
    static G4int GetProcessIndex(const G4String& processName);
    static G4String GetProcessName(G4int processIndex);

    // ═══════════════════════════════════════════════════════════════
    // ENREGISTREMENT DES PASSAGES (appelé par SteppingAction)
    // ═══════════════════════════════════════════════════════════════
    
    /// Enregistre un gamma primaire avec son vrai trackID (appelé au premier step)
    void RegisterPrimaryGamma(G4int trackID, G4double energy, G4double theta, G4double phi);
    
    /// Enregistre l'entrée d'un gamma primaire dans l'eau
    void RecordWaterEntry(G4int trackID, G4double energy);
    
    /// Vérifie si un gamma primaire a déjà été compté comme entrant dans l'eau
    G4bool HasEnteredWater(G4int trackID) const;
    
    /// Enregistre l'absorption d'un gamma primaire
    void RecordGammaAbsorbed(G4int trackID, const G4String& volumeName, const G4String& processName);

    // ═══════════════════════════════════════════════════════════════
    // DOSE DANS LES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════
    
    /// Ajoute l'énergie déposée dans un anneau
    void AddRingEnergy(G4int ringIndex, G4double edep);
    
    /// Ajoute l'énergie déposée par raie gamma
    void AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep);
    
    /// Retourne l'énergie déposée dans un anneau
    G4double GetRingEnergy(G4int ringIndex) const;
    
    /// Retourne l'énergie totale déposée dans l'eau
    G4double GetTotalWaterEnergy() const;

    // ═══════════════════════════════════════════════════════════════
    // COMPTAGES AUX PLANS CONTAINER
    // ═══════════════════════════════════════════════════════════════
    
    void AddPreContainerPhoton(G4double energy);
    void AddPreContainerElectron(G4double energy);
    void AddPostContainerPhotonBack(G4double energy);
    void AddPostContainerElectronBack(G4double energy);
    void AddPostContainerPhotonFwd(G4double energy);
    void AddPostContainerElectronFwd(G4double energy);

    // ═══════════════════════════════════════════════════════════════
    // ACCESSEURS
    // ═══════════════════════════════════════════════════════════════
    
    G4int GetNumberTransmitted() const;
    G4int GetNumberAbsorbed() const;
    
    /// Vérifie si ce trackID est un gamma primaire
    G4bool IsPrimaryTrack(G4int trackID) const;
    
    /// Retourne l'index de raie gamma pour un trackID primaire
    G4int GetGammaLineForTrack(G4int trackID) const;
    
    void SetVerbose(G4int level) { fVerboseLevel = level; }

private:
    RunAction* fRunAction;
    
    // ═══════════════════════════════════════════════════════════════
    // STRUCTURE POUR LES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    
    struct PrimaryGammaInfo {
        G4int trackID;
        G4double energyInitial;
        G4int gammaLineIndex;      // Index de la raie Eu-152 (-1 si non identifiée)
        G4double theta;
        G4double phi;
        G4bool enteredWater;
        G4bool absorbedInWater;
        G4int absorptionProcess;   // Index du processus d'absorption
    };
    
    std::vector<PrimaryGammaInfo> fPrimaryGammas;
    std::map<G4int, size_t> fTrackIDtoIndex;  // trackID -> index dans fPrimaryGammas
    
    // Set pour éviter le double-comptage des entrées dans l'eau
    std::set<G4int> fGammasEnteredWater;

    // ═══════════════════════════════════════════════════════════════
    // DÉPÔTS D'ÉNERGIE PAR ANNEAU
    // ═══════════════════════════════════════════════════════════════
    
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingEnergyDeposit;
    std::array<std::array<G4double, kNbGammaLines>, DetectorConstruction::kNbWaterRings> fRingEnergyByLine;

    // ═══════════════════════════════════════════════════════════════
    // COMPTAGES AUX PLANS CONTAINER
    // ═══════════════════════════════════════════════════════════════
    
    G4int fPreContainerNPhotons;
    G4double fPreContainerSumEPhotons;
    G4int fPreContainerNElectrons;
    G4double fPreContainerSumEElectrons;
    
    G4int fPostContainerNPhotonsBack;
    G4double fPostContainerSumEPhotonsBack;
    G4int fPostContainerNElectronsBack;
    G4double fPostContainerSumEElectronsBack;
    
    G4int fPostContainerNPhotonsFwd;
    G4double fPostContainerSumEPhotonsFwd;
    G4int fPostContainerNElectronsFwd;
    G4double fPostContainerSumEElectronsFwd;

    G4int fVerboseLevel;
};

#endif
