#ifndef RunAction_h
#define RunAction_h 1

#include "G4UserRunAction.hh"
#include "DetectorConstruction.hh"
#include "EventAction.hh"
#include "globals.hh"
#include <array>
#include <vector>

class G4Run;

/// @brief Gestion du run avec sortie ROOT et statistiques de dose
/// VERSION SANS FILTRE - Avec output ROOT
///
/// Cette classe gère :
/// - L'accumulation des statistiques sur le run entier
/// - Le calcul des débits de dose
/// - La création et remplissage des histogrammes ROOT
/// - Les statistiques par raie gamma Eu-152

class RunAction : public G4UserRunAction
{
public:
    RunAction();
    virtual ~RunAction();
    
    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);

    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR REMPLIR LES HISTOGRAMMES ROOT
    // (appelées depuis SteppingAction ou EventAction)
    // ═══════════════════════════════════════════════════════════════
    
    void FillGammaEmittedSpectrum(G4double energy_keV);
    void FillGammaEnteringWater(G4double energy_keV);
    void FillEdepWater(G4double edep_keV);
    void FillEdepRing(G4int ringID, G4double edep_keV);
    void FillElectronSpectrum(G4double energy_keV);
    void FillEdepXY(G4double x_mm, G4double y_mm, G4double weight = 1.0);
    void FillEdepRZ(G4double r_mm, G4double z_mm, G4double weight = 1.0);
    void FillStepNtuple(G4int eventID, G4double x, G4double y, G4double z, 
                        G4double edep, G4int ringID, 
                        const G4String& particleName, const G4String& processName);

    // ═══════════════════════════════════════════════════════════════
    // MÉTHODES POUR REMPLIR LES NTUPLES PRECONTAINER/POSTCONTAINER
    // ═══════════════════════════════════════════════════════════════
    
    void FillPreContainerNtuple(G4int eventID, 
                                 G4int nPhotons, G4double sumEPhotons_keV,
                                 G4int nElectrons, G4double sumEElectrons_keV);
    
    void FillPostContainerNtuple(G4int eventID,
                                  G4int nPhotons_fwd, G4double sumEPhotons_fwd_keV,
                                  G4int nPhotons_back, G4double sumEPhotons_back_keV,
                                  G4int nElectrons_fwd, G4double sumEElectrons_fwd_keV,
                                  G4int nElectrons_back, G4double sumEElectrons_back_keV);
    
    void FillDosesNtuple(G4int eventID,
                          const std::array<G4double, DetectorConstruction::kNbWaterRings>& ringDeposits,
                          G4double totalDeposit,
                          G4int nPrimaries, G4int nTransmitted, G4int nAbsorbed);

    // ═══════════════════════════════════════════════════════════════
    // CONVERSION D'UNITÉS
    // ═══════════════════════════════════════════════════════════════
    
    /// Convertit énergie (MeV) et masse (g) en nGy
    static G4double EnergyToNanoGray(G4double energy_MeV, G4double mass_g);
    
    /// Convertit avec unités Geant4
    static G4double ConvertToNanoGray(G4double energy, G4double mass);

    // ═══════════════════════════════════════════════════════════════
    // ACCUMULATION DES STATISTIQUES (appelées par EventAction)
    // ═══════════════════════════════════════════════════════════════
    
    /// Ajoute l'énergie déposée dans un anneau
    void AddRingEnergy(G4int ringIndex, G4double edep);
    
    /// Ajoute l'énergie déposée par raie gamma
    void AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep);
    
    /// Enregistre les statistiques par raie gamma
    void RecordGammaLineStatistics(G4int lineIndex, G4bool enteredWater, 
                                    G4bool absorbedInWater, G4int absorptionProcess);
    
    /// Enregistre les statistiques globales de l'événement
    void RecordEventStatistics(G4int nPrimaries, 
                               const std::vector<G4double>& primaryEnergies,
                               G4int nTransmitted, G4int nAbsorbed,
                               G4double totalDeposit,
                               const std::array<G4double, DetectorConstruction::kNbWaterRings>& ringDeposits);
    
    /// Enregistre les comptages aux plans container
    void RecordContainerPlaneStatistics(
        G4int preNPhotons, G4double preSumEPhotons,
        G4int preNElectrons, G4double preSumEElectrons,
        G4int postNPhotonsBack, G4double postSumEPhotonsBack,
        G4int postNElectronsBack, G4double postSumEElectronsBack,
        G4int postNPhotonsFwd, G4double postSumEPhotonsFwd,
        G4int postNElectronsFwd, G4double postSumEElectronsFwd);

    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE VÉRIFICATION (appelées par SteppingAction)
    // ═══════════════════════════════════════════════════════════════
    
    void IncrementContainerEntry() { fGammasEnteringContainer++; }
    void IncrementWaterEntry() { fGammasEnteringWater++; }
    void IncrementElectronsInWater() { fElectronsInWater++; }
    void IncrementPreContainerPlane() { fGammasPreContainerPlane++; }
    void IncrementPostContainerPlane() { fGammasPostContainerPlane++; }

    // ═══════════════════════════════════════════════════════════════
    // ACCESSEURS
    // ═══════════════════════════════════════════════════════════════
    
    /// Retourne le nom du fichier ROOT de sortie
    const G4String& GetOutputFileName() const { return fOutputFileName; }
    
    /// Retourne la fraction d'angle solide interceptée par le cône
    G4double GetSolidAngleFraction() const;
    
    /// Retourne la masse d'un anneau (en grammes)
    G4double GetRingMass(G4int ringIndex) const { 
        return (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) 
               ? fRingMasses[ringIndex] : 0.; 
    }
    
    /// Retourne l'énergie totale déposée dans un anneau
    G4double GetRingTotalEnergy(G4int ringIndex) const {
        return (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings)
               ? fRingTotalEnergy[ringIndex] : 0.;
    }
    
    // Paramètres géométriques
    G4double GetActivity4pi() const { return fActivity4pi; }
    G4double GetConeAngle() const { return fConeAngle; }
    G4double GetSourcePosZ() const { return fSourcePosZ; }
    G4double GetWaterRadius() const { return fWaterRadius; }
    G4double GetWaterBottomZ() const { return fWaterBottomZ; }

    // ═══════════════════════════════════════════════════════════════
    // CALCULS DE NORMALISATION
    // ═══════════════════════════════════════════════════════════════
    
    /// Calcule le temps d'irradiation correspondant à nEvents désintégrations
    G4double CalculateIrradiationTime(G4int nEvents) const;
    
    /// Calcule le débit de dose à partir de la dose totale et du nombre d'événements
    G4double CalculateDoseRate(G4double totalDose_Gy, G4int nEvents) const;

private:
    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE LA SOURCE
    // ═══════════════════════════════════════════════════════════════
    G4double fActivity4pi;          // Activité 4π de la source (Bq)
    G4double fConeAngle;            // Demi-angle du cône d'émission
    G4double fSourcePosZ;           // Position Z de la source
    G4double fMeanGammasPerDecay;   // Nombre moyen de gammas par désintégration
    G4double fWaterRadius;          // Rayon de la zone d'eau
    G4double fWaterBottomZ;         // Position Z du bas de l'eau

    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS GLOBAUX
    // ═══════════════════════════════════════════════════════════════
    G4int fTotalPrimariesGenerated;
    G4int fTotalEventsWithZeroGamma;
    G4int fTotalTransmitted;
    G4int fTotalAbsorbed;
    G4int fTotalEvents;
    G4double fTotalWaterEnergy;
    G4int fTotalWaterEventCount;

    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE VÉRIFICATION
    // ═══════════════════════════════════════════════════════════════
    G4int fGammasEnteringContainer;
    G4int fGammasEnteringWater;
    G4int fElectronsInWater;
    G4int fGammasPreContainerPlane;
    G4int fGammasPostContainerPlane;

    // ═══════════════════════════════════════════════════════════════
    // STATISTIQUES PAR ANNEAU D'EAU
    // ═══════════════════════════════════════════════════════════════
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingTotalEnergy;
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingTotalEnergy2;  // Pour variance
    std::array<G4int, DetectorConstruction::kNbWaterRings> fRingEventCount;
    std::array<G4double, DetectorConstruction::kNbWaterRings> fRingMasses;
    
    // Énergie par anneau ET par raie gamma
    std::array<std::array<G4double, EventAction::kNbGammaLines>, DetectorConstruction::kNbWaterRings> fRingEnergyByLine;

    // ═══════════════════════════════════════════════════════════════
    // STATISTIQUES PAR RAIE GAMMA Eu-152
    // ═══════════════════════════════════════════════════════════════
    std::array<G4int, EventAction::kNbGammaLines> fLineEmitted;
    std::array<G4int, EventAction::kNbGammaLines> fLineEnteredWater;
    std::array<G4int, EventAction::kNbGammaLines> fLineAbsorbedWater;
    
    // Comptage par processus d'absorption pour chaque raie
    std::array<std::array<G4int, EventAction::kNbProcesses>, EventAction::kNbGammaLines> fLineAbsorbedByProcess;

    // ═══════════════════════════════════════════════════════════════
    // FICHIER DE SORTIE ROOT
    // ═══════════════════════════════════════════════════════════════
    G4String fOutputFileName;
};

#endif
