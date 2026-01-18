#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Logger.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh"
#include <iomanip>
#include <sstream>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
// CONSTANTES POUR LA CONVERSION EN DOSE
// ═══════════════════════════════════════════════════════════════

static const G4double kMeVtoJoule = 1.60218e-13;
static const G4double kNanoGrayFactor = 0.160218;  // nGy per MeV per gram

RunAction::RunAction()
: G4UserRunAction(),
  fActivity4pi(4.2e4),          // 42 kBq (source réelle)
  fConeAngle(45.*deg),
  fSourcePosZ(75.0*mm),           // Source à z = 75 mm (25 mm avant surface eau à z=100 mm)
  fMeanGammasPerDecay(2.03),    // Mis à jour avec 13 raies
  fWaterRadius(25.0*mm),        // Rayon de l'eau
  fWaterBottomZ(98.5*mm),       // Position Z du bas de l'eau
  fTotalPrimariesGenerated(0),
  fTotalEventsWithZeroGamma(0),
  fTotalTransmitted(0),
  fTotalAbsorbed(0),
  fTotalEvents(0),
  fTotalWaterEnergy(0.),
  fTotalWaterEventCount(0),
  fGammasEnteringContainer(0),
  fGammasEnteringWater(0),
  fElectronsInWater(0),
  fGammasPreContainerPlane(0),
  fGammasPostContainerPlane(0),
  fOutputFileName("output.root")
{
    fRingTotalEnergy.fill(0.);
    fRingTotalEnergy2.fill(0.);
    fRingEventCount.fill(0);
    fRingMasses.fill(0.);
    
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    
    fLineEmitted.fill(0);
    fLineEnteredWater.fill(0);
    fLineAbsorbedWater.fill(0);
    
    // Initialiser les compteurs par processus
    for (auto& arr : fLineAbsorbedByProcess) {
        arr.fill(0);
    }
}

RunAction::~RunAction()
{}

// ═══════════════════════════════════════════════════════════════
// CONVERSION D'UNITÉS
// ═══════════════════════════════════════════════════════════════

G4double RunAction::EnergyToNanoGray(G4double energy_MeV, G4double mass_g)
{
    if (mass_g <= 0.) return 0.;
    return energy_MeV * kNanoGrayFactor / mass_g;
}

G4double RunAction::ConvertToNanoGray(G4double energy, G4double mass)
{
    G4double energy_MeV = energy / MeV;
    G4double mass_g = mass / g;
    return EnergyToNanoGray(energy_MeV, mass_g);
}

// ═══════════════════════════════════════════════════════════════
// FRACTION D'ANGLE SOLIDE
// ═══════════════════════════════════════════════════════════════

G4double RunAction::GetSolidAngleFraction() const
{
    // Fraction de l'angle solide 4π couverte par le cône
    return (1.0 - std::cos(fConeAngle)) / 2.0;
}

// ═══════════════════════════════════════════════════════════════
// DÉBUT DE RUN - AVEC CRÉATION DU FICHIER ROOT
// ═══════════════════════════════════════════════════════════════

void RunAction::BeginOfRunAction(const G4Run* run)
{
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  DÉBUT DU RUN " << run->GetRunID() << " - CONFIGURATION SANS FILTRE              ║" << G4endl;
    G4cout << "║  Source à z = " << fSourcePosZ/mm << " mm                                        ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;
    
    Logger::GetInstance()->Open("output.log");
    Logger::GetInstance()->LogHeader("Démarrage du Run " + std::to_string(run->GetRunID()) + " - SANS FILTRE");
    
    // ═══════════════════════════════════════════════════════════════
    // CRÉATION DU FICHIER ROOT ET DES HISTOGRAMMES
    // ═══════════════════════════════════════════════════════════════
    
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Configuration du manager
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);
    analysisManager->SetNtupleMerging(true);  // Pour le multithreading
    
    // Ouvrir le fichier ROOT
    G4bool fileOpen = analysisManager->OpenFile(fOutputFileName);
    if (!fileOpen) {
        G4cerr << "*** ERREUR: Impossible d'ouvrir le fichier " << fOutputFileName << G4endl;
        return;
    }
    G4cout << ">>> Fichier ROOT ouvert: " << fOutputFileName << G4endl;
    
    // ─────────────────────────────────────────────────────────────
    // CRÉATION DES HISTOGRAMMES 1D
    // ─────────────────────────────────────────────────────────────
    
    // H1 ID=0: Spectre d'énergie des gammas émis
    analysisManager->CreateH1("hGammaEmitted", "Spectre gamma emis;Energie (keV);Counts", 
                              1000, 0., 2000.);
    
    // H1 ID=1: Spectre d'énergie des gammas entrant dans l'eau
    analysisManager->CreateH1("hGammaEnteringWater", "Gamma entrant dans eau;Energie (keV);Counts", 
                              1000, 0., 2000.);
    
    // H1 ID=2: Énergie déposée dans l'eau (par step)
    analysisManager->CreateH1("hEdepWater", "Energie deposee dans eau;Energie (keV);Counts", 
                              500, 0., 250.);
    
    // H1 ID=3-7: Énergie déposée par anneau (par step)
    analysisManager->CreateH1("hEdepRing0", "Edep Anneau 0 (0-5mm);Energie (keV);Counts", 200, 0., 200.);
    analysisManager->CreateH1("hEdepRing1", "Edep Anneau 1 (5-10mm);Energie (keV);Counts", 200, 0., 200.);
    analysisManager->CreateH1("hEdepRing2", "Edep Anneau 2 (10-15mm);Energie (keV);Counts", 200, 0., 200.);
    analysisManager->CreateH1("hEdepRing3", "Edep Anneau 3 (15-20mm);Energie (keV);Counts", 200, 0., 200.);
    analysisManager->CreateH1("hEdepRing4", "Edep Anneau 4 (20-25mm);Energie (keV);Counts", 200, 0., 200.);
    
    // H1 ID=8: Profil radial de dose
    analysisManager->CreateH1("hRadialDose", "Profil radial dose;Rayon (mm);Dose (nGy)", 25, 0., 25.);
    
    // H1 ID=9: Spectre des électrons secondaires dans l'eau
    analysisManager->CreateH1("hElectronSpectrum", "Electrons secondaires;Energie (keV);Counts", 500, 0., 1500.);
    
    // ─────────────────────────────────────────────────────────────
    // NOUVEAUX HISTOGRAMMES DE DOSE PAR ÉVÉNEMENT (en nGy)
    // ─────────────────────────────────────────────────────────────
    
    // H1 ID=10-14: Dose par anneau par événement
    analysisManager->CreateH1("h_dose_ring0", "Dose Anneau 0 (0-5mm);Dose (nGy);Counts", 500, 0., 0.5);
    analysisManager->CreateH1("h_dose_ring1", "Dose Anneau 1 (5-10mm);Dose (nGy);Counts", 400, 0., 0.2);
    analysisManager->CreateH1("h_dose_ring2", "Dose Anneau 2 (10-15mm);Dose (nGy);Counts", 100, 0., 0.1);
    analysisManager->CreateH1("h_dose_ring3", "Dose Anneau 3 (15-20mm);Dose (nGy);Counts", 100, 0., 0.1);
    analysisManager->CreateH1("h_dose_ring4", "Dose Anneau 4 (20-25mm);Dose (nGy);Counts", 500, 0., 0.1);
    
    // H1 ID=15: Dose totale par événement
    analysisManager->CreateH1("h_dose_total", "Dose totale eau;Dose (nGy);Counts", 500, 0., 0.03);
    
    // ─────────────────────────────────────────────────────────────
    // CRÉATION DES HISTOGRAMMES 2D
    // ─────────────────────────────────────────────────────────────
    
    analysisManager->CreateH2("hEdepXY", "Edep position XY;X (mm);Y (mm)", 100, -30., 30., 100, -30., 30.);
    analysisManager->CreateH2("hEdepRZ", "Edep position RZ;R (mm);Z (mm)", 50, 0., 30., 50, 98., 108.);
    
    // ─────────────────────────────────────────────────────────────
    // CRÉATION DES NTUPLES
    // ─────────────────────────────────────────────────────────────
    
    // Ntuple 0: Données par événement
    analysisManager->CreateNtuple("EventData", "Donnees par evenement");
    analysisManager->CreateNtupleIColumn("EventID");
    analysisManager->CreateNtupleDColumn("EdepTotal");
    analysisManager->CreateNtupleDColumn("EdepRing0");
    analysisManager->CreateNtupleDColumn("EdepRing1");
    analysisManager->CreateNtupleDColumn("EdepRing2");
    analysisManager->CreateNtupleDColumn("EdepRing3");
    analysisManager->CreateNtupleDColumn("EdepRing4");
    analysisManager->CreateNtupleIColumn("NGammaEmitted");
    analysisManager->CreateNtupleIColumn("NGammaWater");
    analysisManager->FinishNtuple();
    
    // Ntuple 1: Données par step
    analysisManager->CreateNtuple("StepData", "Donnees par step dans eau");
    analysisManager->CreateNtupleIColumn("EventID");
    analysisManager->CreateNtupleDColumn("X");
    analysisManager->CreateNtupleDColumn("Y");
    analysisManager->CreateNtupleDColumn("Z");
    analysisManager->CreateNtupleDColumn("Edep");
    analysisManager->CreateNtupleIColumn("RingID");
    analysisManager->CreateNtupleSColumn("ParticleName");
    analysisManager->CreateNtupleSColumn("ProcessName");
    analysisManager->FinishNtuple();
    
    // Ntuple 2: Données des gammas primaires
    analysisManager->CreateNtuple("GammaData", "Donnees gammas primaires");
    analysisManager->CreateNtupleIColumn("EventID");
    analysisManager->CreateNtupleDColumn("Energy");
    analysisManager->CreateNtupleIColumn("LineID");
    analysisManager->CreateNtupleIColumn("ReachedWater");
    analysisManager->CreateNtupleIColumn("Absorbed");
    analysisManager->FinishNtuple();
    
    // Ntuple 3: Statistiques par raie gamma (rempli à la fin du run)
    analysisManager->CreateNtuple("gamma_lines", "Statistiques par raie gamma");
    analysisManager->CreateNtupleIColumn("lineIndex");
    analysisManager->CreateNtupleDColumn("energy_keV");
    analysisManager->CreateNtupleIColumn("emitted");
    analysisManager->CreateNtupleIColumn("enteredWater");
    analysisManager->CreateNtupleIColumn("absorbedWater");
    analysisManager->CreateNtupleDColumn("waterAbsRate");
    analysisManager->CreateNtupleDColumn("waterEntryRate");
    analysisManager->FinishNtuple();
    
    // ─────────────────────────────────────────────────────────────
    // Ntuple 4: PreContainerPlane - particules entrant dans l'eau
    // ─────────────────────────────────────────────────────────────
    analysisManager->CreateNtuple("precontainer", "Particules entrant dans eau");
    analysisManager->CreateNtupleIColumn("eventID");           // Col 0
    analysisManager->CreateNtupleIColumn("nPhotons");          // Col 1
    analysisManager->CreateNtupleDColumn("sumEPhotons_keV");   // Col 2
    analysisManager->CreateNtupleIColumn("nElectrons");        // Col 3
    analysisManager->CreateNtupleDColumn("sumEElectrons_keV"); // Col 4
    analysisManager->FinishNtuple();
    
    // ─────────────────────────────────────────────────────────────
    // Ntuple 5: PostContainerPlane - particules sortant de l'eau
    // ─────────────────────────────────────────────────────────────
    analysisManager->CreateNtuple("postcontainer", "Particules sortant de eau");
    analysisManager->CreateNtupleIColumn("eventID");               // Col 0
    analysisManager->CreateNtupleIColumn("nPhotons_fwd");          // Col 1
    analysisManager->CreateNtupleDColumn("sumEPhotons_fwd_keV");   // Col 2
    analysisManager->CreateNtupleIColumn("nPhotons_back");         // Col 3
    analysisManager->CreateNtupleDColumn("sumEPhotons_back_keV");  // Col 4
    analysisManager->CreateNtupleIColumn("nElectrons_fwd");        // Col 5
    analysisManager->CreateNtupleDColumn("sumEElectrons_fwd_keV"); // Col 6
    analysisManager->CreateNtupleIColumn("nElectrons_back");       // Col 7
    analysisManager->CreateNtupleDColumn("sumEElectrons_back_keV");// Col 8
    analysisManager->FinishNtuple();
    
    // ─────────────────────────────────────────────────────────────
    // Ntuple 6: doses - Doses par anneau par événement (pour analyse_dose_anneaux.C)
    // ─────────────────────────────────────────────────────────────
    analysisManager->CreateNtuple("doses", "Doses par anneau par evenement");
    analysisManager->CreateNtupleIColumn("eventID");          // Col 0
    analysisManager->CreateNtupleDColumn("dose_nGy_ring0");   // Col 1
    analysisManager->CreateNtupleDColumn("dose_nGy_ring1");   // Col 2
    analysisManager->CreateNtupleDColumn("dose_nGy_ring2");   // Col 3
    analysisManager->CreateNtupleDColumn("dose_nGy_ring3");   // Col 4
    analysisManager->CreateNtupleDColumn("dose_nGy_ring4");   // Col 5
    analysisManager->CreateNtupleDColumn("dose_nGy_total");   // Col 6
    analysisManager->CreateNtupleDColumn("edep_keV_total");   // Col 7
    analysisManager->CreateNtupleIColumn("nPrimaries");       // Col 8
    analysisManager->CreateNtupleIColumn("nTransmitted");     // Col 9
    analysisManager->CreateNtupleIColumn("nAbsorbed");        // Col 10
    analysisManager->FinishNtuple();
    
    G4cout << ">>> Histogrammes et Ntuples créés" << G4endl;
    
    // ═══════════════════════════════════════════════════════════════
    // CALCUL DES MASSES DES ANNEAUX D'EAU
    // ═══════════════════════════════════════════════════════════════
    
    const G4double waterThickness = 5.0 * mm;
    const G4double ringWidth = 5.0 * mm;
    const G4double waterDensity = 1.0 * g/cm3;
    const G4double pi = CLHEP::pi;
    
    G4cout << "\n=== MASSES DES ANNEAUX D'EAU ===" << G4endl;
    LOG("=== MASSES DES ANNEAUX D'EAU ===");
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double rInner = i * ringWidth;
        G4double rOuter = (i + 1) * ringWidth;
        G4double volume = pi * (rOuter*rOuter - rInner*rInner) * waterThickness;
        fRingMasses[i] = (volume * waterDensity) / g;
        
        std::ostringstream oss;
        oss << "  Anneau " << i 
            << " : r=[" << rInner/mm << "-" << rOuter/mm << "] mm"
            << " | V=" << std::fixed << std::setprecision(4) << volume/cm3 << " cm³"
            << " | m=" << fRingMasses[i] << " g";
        G4cout << oss.str() << G4endl;
        LOG(oss.str());
    }
    
    G4double totalMass = 0.;
    for (const auto& m : fRingMasses) totalMass += m;
    G4cout << "  TOTAL : " << totalMass << " g" << G4endl;
    G4cout << "================================\n" << G4endl;
    
    // Réinitialiser tous les compteurs
    fRingTotalEnergy.fill(0.);
    fRingTotalEnergy2.fill(0.);
    fRingEventCount.fill(0);
    
    for (auto& arr : fRingEnergyByLine) {
        arr.fill(0.);
    }
    
    fLineEmitted.fill(0);
    fLineEnteredWater.fill(0);
    fLineAbsorbedWater.fill(0);
    
    for (auto& arr : fLineAbsorbedByProcess) {
        arr.fill(0);
    }
    
    fTotalPrimariesGenerated = 0;
    fTotalEventsWithZeroGamma = 0;
    fTotalTransmitted = 0;
    fTotalAbsorbed = 0;
    fTotalEvents = 0;
    fTotalWaterEnergy = 0.;
    fTotalWaterEventCount = 0;
    
    fGammasEnteringContainer = 0;
    fGammasEnteringWater = 0;
    fElectronsInWater = 0;
    fGammasPreContainerPlane = 0;
    fGammasPostContainerPlane = 0;
}

// ═══════════════════════════════════════════════════════════════
// FIN DE RUN
// ═══════════════════════════════════════════════════════════════

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nEvents = run->GetNumberOfEvent();
    if (nEvents == 0) return;
    
    auto analysisManager = G4AnalysisManager::Instance();
    
    // ═══════════════════════════════════════════════════════════════
    // REMPLIR LE NTUPLE gamma_lines AVEC LES STATISTIQUES PAR RAIE
    // ═══════════════════════════════════════════════════════════════
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4double energy_keV = EventAction::GetGammaLineEnergy(i);
        G4double waterAbsRate = (fLineEnteredWater[i] > 0) ? 
            100.0 * fLineAbsorbedWater[i] / fLineEnteredWater[i] : 0.0;
        G4double waterEntryRate = (fLineEmitted[i] > 0) ?
            100.0 * fLineEnteredWater[i] / fLineEmitted[i] : 0.0;
        
        analysisManager->FillNtupleIColumn(3, 0, i);
        analysisManager->FillNtupleDColumn(3, 1, energy_keV);
        analysisManager->FillNtupleIColumn(3, 2, fLineEmitted[i]);
        analysisManager->FillNtupleIColumn(3, 3, fLineEnteredWater[i]);
        analysisManager->FillNtupleIColumn(3, 4, fLineAbsorbedWater[i]);
        analysisManager->FillNtupleDColumn(3, 5, waterAbsRate);
        analysisManager->FillNtupleDColumn(3, 6, waterEntryRate);
        analysisManager->AddNtupleRow(3);
    }
    
    // Écrire et fermer le fichier ROOT
    analysisManager->Write();
    analysisManager->CloseFile();
    
    G4cout << "\n>>> Fichier ROOT fermé: " << fOutputFileName << G4endl;
    
    // Affichage des statistiques
    std::ostringstream oss;
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                              FIN DU RUN " << std::setw(6) << run->GetRunID() << "                                        ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  Événements simulés         : " << std::setw(12) << nEvents << "                                    ║\n";
    oss << "║  Gammas primaires générés   : " << std::setw(12) << fTotalPrimariesGenerated << "                                    ║\n";
    oss << "║  Gammas entrant Water1      : " << std::setw(12) << fGammasEnteringContainer << "                                    ║\n";
    oss << "║  Gammas entrant anneaux     : " << std::setw(12) << fGammasEnteringWater << "                                    ║\n";
    oss << "║  Gammas absorbés eau        : " << std::setw(12) << fTotalAbsorbed << "                                    ║\n";
    oss << "║  Électrons dans eau         : " << std::setw(12) << fElectronsInWater << "                                    ║\n";
    oss << "║  Énergie totale eau (MeV)   : " << std::setw(12) << std::scientific << std::setprecision(4) << fTotalWaterEnergy/MeV << "                                ║\n";
    oss << "║  Fichier ROOT               : " << std::setw(20) << fOutputFileName << "                        ║\n";
    oss << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n";
    
    // ═══════════════════════════════════════════════════════════════
    // TABLEAU DES STATISTIQUES PAR RAIE GAMMA
    // ═══════════════════════════════════════════════════════════════
    
    oss << "\n╔═══════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                        STATISTIQUES PAR RAIE GAMMA Eu-152                             ║\n";
    oss << "╠════════╦════════════╦═══════════╦═══════════════╦══════════════╦══════════════════════╣\n";
    oss << "║  Raie  ║ Energie    ║   Émis    ║  Entré eau    ║  Absorbé eau ║   Taux abs. eau (%)  ║\n";
    oss << "╠════════╬════════════╬═══════════╬═══════════════╬══════════════╬══════════════════════╣\n";
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4double absRate = (fLineEnteredWater[i] > 0) ? 
            100.0 * fLineAbsorbedWater[i] / fLineEnteredWater[i] : 0.0;
        
        oss << "║   " << std::setw(2) << i << "   ║"
            << std::setw(8) << std::fixed << std::setprecision(1) << EventAction::GetGammaLineEnergy(i) << " keV║"
            << std::setw(10) << fLineEmitted[i] << " ║"
            << std::setw(14) << fLineEnteredWater[i] << " ║"
            << std::setw(13) << fLineAbsorbedWater[i] << " ║"
            << std::setw(20) << std::setprecision(2) << absRate << " ║\n";
    }
    oss << "╚════════╩════════════╩═══════════╩═══════════════╩══════════════╩══════════════════════╝\n";
    
    // Tableau des doses par anneau
    oss << "\n╔═══════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                           DOSE PAR ANNEAU D'EAU                                       ║\n";
    oss << "╠═════════╦═══════════════╦═══════════════╦═══════════════════╦═════════════════════════╣\n";
    oss << "║ Anneau  ║  r_int-r_ext  ║   Masse (g)   ║   Energie (MeV)   ║     Dose (nGy/evt)      ║\n";
    oss << "╠═════════╬═══════════════╬═══════════════╬═══════════════════╬═════════════════════════╣\n";
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double rIn = i * 5.0;
        G4double rOut = (i + 1) * 5.0;
        G4double mass_g = fRingMasses[i];
        G4double energy_MeV = fRingTotalEnergy[i] / MeV;
        G4double dosePerEvt_nGy = EnergyToNanoGray(energy_MeV, mass_g) / nEvents;
        
        oss << "║    " << i << "    ║   "
            << std::setw(2) << std::fixed << std::setprecision(0) << rIn << " - " 
            << std::setw(2) << rOut << " mm   ║"
            << std::setw(13) << std::setprecision(4) << mass_g << "  ║"
            << std::setw(17) << std::scientific << std::setprecision(3) << energy_MeV << "  ║"
            << std::setw(23) << dosePerEvt_nGy << "  ║\n";
    }
    oss << "╚═════════╩═══════════════╩═══════════════╩═══════════════════╩═════════════════════════╝\n";
    
    G4cout << oss.str();
    
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
        Logger::GetInstance()->Close();
    }
}

// ═══════════════════════════════════════════════════════════════
// ACCUMULATION DES STATISTIQUES (appelées par EventAction)
// ═══════════════════════════════════════════════════════════════

void RunAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingTotalEnergy[ringIndex] += edep;
        fRingTotalEnergy2[ringIndex] += edep * edep;
        fRingEventCount[ringIndex]++;
        fTotalWaterEnergy += edep;
    }
}

void RunAction::AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings &&
        lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        fRingEnergyByLine[ringIndex][lineIndex] += edep;
    }
}

void RunAction::RecordGammaLineStatistics(G4int lineIndex, G4bool enteredWater, 
                                           G4bool absorbedInWater, G4int absorptionProcess)
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        fLineEmitted[lineIndex]++;
        
        if (enteredWater) {
            fLineEnteredWater[lineIndex]++;
        }
        
        if (absorbedInWater) {
            fLineAbsorbedWater[lineIndex]++;
            
            if (absorptionProcess >= 0 && absorptionProcess < EventAction::kNbProcesses) {
                fLineAbsorbedByProcess[lineIndex][absorptionProcess]++;
            }
        }
    }
}

void RunAction::RecordEventStatistics(G4int nPrimaries, 
                                       const std::vector<G4double>& /*primaryEnergies*/,
                                       G4int nTransmitted, G4int nAbsorbed,
                                       G4double totalDeposit,
                                       const std::array<G4double, DetectorConstruction::kNbWaterRings>& ringDeposits)
{
    fTotalEvents++;
    fTotalPrimariesGenerated += nPrimaries;
    fTotalTransmitted += nTransmitted;
    fTotalAbsorbed += nAbsorbed;
    
    if (nPrimaries == 0) {
        fTotalEventsWithZeroGamma++;
    }
    
    if (totalDeposit > 0.) {
        fTotalWaterEventCount++;
        
        // ═══════════════════════════════════════════════════════════════
        // REMPLIR LES HISTOGRAMMES DE DOSE PAR ÉVÉNEMENT
        // ═══════════════════════════════════════════════════════════════
        
        auto analysisManager = G4AnalysisManager::Instance();
        
        G4double totalDose_nGy = 0.;
        G4double totalMass = 0.;
        for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
            totalMass += fRingMasses[i];
        }
        
        for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
            if (ringDeposits[i] > 0. && fRingMasses[i] > 0.) {
                G4double dose_nGy = EnergyToNanoGray(ringDeposits[i] / MeV, fRingMasses[i]);
                analysisManager->FillH1(10 + i, dose_nGy);
                totalDose_nGy += ringDeposits[i] / MeV * kNanoGrayFactor;  // Non pondéré par masse pour le total
            }
        }
        
        // Dose totale pondérée par les masses
        if (totalMass > 0.) {
            G4double totalDoseWeighted = EnergyToNanoGray(totalDeposit / MeV, totalMass);
            analysisManager->FillH1(15, totalDoseWeighted);
        }
    }
}

void RunAction::RecordContainerPlaneStatistics(
    G4int /*preNPhotons*/, G4double /*preSumEPhotons*/,
    G4int /*preNElectrons*/, G4double /*preSumEElectrons*/,
    G4int /*postNPhotonsBack*/, G4double /*postSumEPhotonsBack*/,
    G4int /*postNElectronsBack*/, G4double /*postSumEElectronsBack*/,
    G4int /*postNPhotonsFwd*/, G4double /*postSumEPhotonsFwd*/,
    G4int /*postNElectronsFwd*/, G4double /*postSumEElectronsFwd*/)
{
    // Statistiques additionnelles si nécessaire
    // Les compteurs principaux sont incrémentés via IncrementXXX()
}

// ═══════════════════════════════════════════════════════════════
// MÉTHODES POUR REMPLIR LES HISTOGRAMMES ROOT
// ═══════════════════════════════════════════════════════════════

void RunAction::FillGammaEmittedSpectrum(G4double energy_keV)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH1(0, energy_keV);
}

void RunAction::FillGammaEnteringWater(G4double energy_keV)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH1(1, energy_keV);
}

void RunAction::FillEdepWater(G4double edep_keV)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH1(2, edep_keV);
}

void RunAction::FillEdepRing(G4int ringID, G4double edep_keV)
{
    if (ringID < 0 || ringID > 4) return;
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH1(3 + ringID, edep_keV);
}

void RunAction::FillElectronSpectrum(G4double energy_keV)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH1(9, energy_keV);
}

void RunAction::FillEdepXY(G4double x_mm, G4double y_mm, G4double weight)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH2(0, x_mm, y_mm, weight);
}

void RunAction::FillEdepRZ(G4double r_mm, G4double z_mm, G4double weight)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillH2(1, r_mm, z_mm, weight);
}

void RunAction::FillStepNtuple(G4int eventID, G4double x, G4double y, G4double z, 
                               G4double edep, G4int ringID, 
                               const G4String& particleName, const G4String& processName)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleIColumn(1, 0, eventID);
    analysisManager->FillNtupleDColumn(1, 1, x);
    analysisManager->FillNtupleDColumn(1, 2, y);
    analysisManager->FillNtupleDColumn(1, 3, z);
    analysisManager->FillNtupleDColumn(1, 4, edep);
    analysisManager->FillNtupleIColumn(1, 5, ringID);
    analysisManager->FillNtupleSColumn(1, 6, particleName);
    analysisManager->FillNtupleSColumn(1, 7, processName);
    analysisManager->AddNtupleRow(1);
}

// ═══════════════════════════════════════════════════════════════
// MÉTHODES POUR REMPLIR LES NTUPLES PRECONTAINER/POSTCONTAINER
// ═══════════════════════════════════════════════════════════════

void RunAction::FillPreContainerNtuple(G4int eventID, 
                                        G4int nPhotons, G4double sumEPhotons_keV,
                                        G4int nElectrons, G4double sumEElectrons_keV)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleIColumn(4, 0, eventID);
    analysisManager->FillNtupleIColumn(4, 1, nPhotons);
    analysisManager->FillNtupleDColumn(4, 2, sumEPhotons_keV);
    analysisManager->FillNtupleIColumn(4, 3, nElectrons);
    analysisManager->FillNtupleDColumn(4, 4, sumEElectrons_keV);
    analysisManager->AddNtupleRow(4);
}

void RunAction::FillPostContainerNtuple(G4int eventID,
                                         G4int nPhotons_fwd, G4double sumEPhotons_fwd_keV,
                                         G4int nPhotons_back, G4double sumEPhotons_back_keV,
                                         G4int nElectrons_fwd, G4double sumEElectrons_fwd_keV,
                                         G4int nElectrons_back, G4double sumEElectrons_back_keV)
{
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->FillNtupleIColumn(5, 0, eventID);
    analysisManager->FillNtupleIColumn(5, 1, nPhotons_fwd);
    analysisManager->FillNtupleDColumn(5, 2, sumEPhotons_fwd_keV);
    analysisManager->FillNtupleIColumn(5, 3, nPhotons_back);
    analysisManager->FillNtupleDColumn(5, 4, sumEPhotons_back_keV);
    analysisManager->FillNtupleIColumn(5, 5, nElectrons_fwd);
    analysisManager->FillNtupleDColumn(5, 6, sumEElectrons_fwd_keV);
    analysisManager->FillNtupleIColumn(5, 7, nElectrons_back);
    analysisManager->FillNtupleDColumn(5, 8, sumEElectrons_back_keV);
    analysisManager->AddNtupleRow(5);
}

void RunAction::FillDosesNtuple(G4int eventID,
                                 const std::array<G4double, DetectorConstruction::kNbWaterRings>& ringDeposits,
                                 G4double totalDeposit,
                                 G4int nPrimaries, G4int nTransmitted, G4int nAbsorbed)
{
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Calculer les doses en nGy pour chaque anneau
    G4double dose_nGy[DetectorConstruction::kNbWaterRings];
    G4double totalDose_nGy = 0.;
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        if (ringDeposits[i] > 0. && fRingMasses[i] > 0.) {
            dose_nGy[i] = EnergyToNanoGray(ringDeposits[i] / MeV, fRingMasses[i]);
        } else {
            dose_nGy[i] = 0.;
        }
        totalDose_nGy += dose_nGy[i];
    }
    
    // Remplir le ntuple (ID = 6)
    analysisManager->FillNtupleIColumn(6, 0, eventID);
    analysisManager->FillNtupleDColumn(6, 1, dose_nGy[0]);
    analysisManager->FillNtupleDColumn(6, 2, dose_nGy[1]);
    analysisManager->FillNtupleDColumn(6, 3, dose_nGy[2]);
    analysisManager->FillNtupleDColumn(6, 4, dose_nGy[3]);
    analysisManager->FillNtupleDColumn(6, 5, dose_nGy[4]);
    analysisManager->FillNtupleDColumn(6, 6, totalDose_nGy);
    analysisManager->FillNtupleDColumn(6, 7, totalDeposit / keV);
    analysisManager->FillNtupleIColumn(6, 8, nPrimaries);
    analysisManager->FillNtupleIColumn(6, 9, nTransmitted);
    analysisManager->FillNtupleIColumn(6, 10, nAbsorbed);
    analysisManager->AddNtupleRow(6);
}

// ═══════════════════════════════════════════════════════════════
// CALCULS DE NORMALISATION
// ═══════════════════════════════════════════════════════════════

G4double RunAction::CalculateIrradiationTime(G4int nEvents) const
{
    G4double activityInCone = fActivity4pi * GetSolidAngleFraction();
    if (activityInCone > 0.) {
        return static_cast<G4double>(nEvents) / activityInCone;
    }
    return 0.;
}

G4double RunAction::CalculateDoseRate(G4double totalDose_Gy, G4int nEvents) const
{
    G4double time = CalculateIrradiationTime(nEvents);
    if (time > 0.) {
        return totalDose_Gy / time;
    }
    return 0.;
}
