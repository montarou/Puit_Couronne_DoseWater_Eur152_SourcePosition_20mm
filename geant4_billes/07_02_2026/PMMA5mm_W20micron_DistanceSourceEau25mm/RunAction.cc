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
  fSourcePosZ(73.5*mm),         // Source à z = 73.5 mm (20 mm avant PMMA)
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
  fOutputFileName("puits_couronne_sans_filtre.root")
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
// DÉBUT ET FIN DE RUN
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
    // CALCUL DIRECT DES MASSES DES ANNEAUX D'EAU (en grammes)
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
    
    std::ostringstream ossTot;
    ossTot << "  TOTAL : " << totalMass << " g";
    G4cout << ossTot.str() << G4endl;
    LOG(ossTot.str());
    G4cout << "================================\n" << G4endl;
    
    // Réinitialiser les compteurs
    fRingTotalEnergy.fill(0.);
    fRingTotalEnergy2.fill(0.);
    fRingEventCount.fill(0);
    
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
    
    // ═══════════════════════════════════════════════════════════════
    // CONFIGURATION DE L'ANALYSIS MANAGER
    // ═══════════════════════════════════════════════════════════════
    
    auto analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetDefaultFileType("root");
    analysisManager->SetVerboseLevel(1);
    
    analysisManager->OpenFile(fOutputFileName);
    
    // Histogrammes de dose en nGy
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4String name = "h_dose_ring" + std::to_string(i);
        G4String title = "Dose par désintégration - Anneau " + std::to_string(i) + " [nGy]";
        analysisManager->CreateH1(name, title, 200, 0., 0.5);
    }
    
    // Histogramme dose totale
    analysisManager->CreateH1("h_dose_total", "Dose totale par désintégration (somme anneaux) [nGy]", 200, 0., 0.5);
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4String name = "h_edep_line" + std::to_string(i);
        G4String title = "Énergie déposée - Raie " + EventAction::GetGammaLineName(i) + " [keV]";
        analysisManager->CreateH1(name, title, 200, 0., 100.);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 0 : doses
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("doses", "Doses par désintégration");
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4String name = "dose_nGy_ring" + std::to_string(i);
        analysisManager->CreateNtupleDColumn(name);
    }
    
    analysisManager->CreateNtupleDColumn("dose_nGy_total");
    analysisManager->CreateNtupleDColumn("edep_keV_total");
    analysisManager->CreateNtupleIColumn("nPrimaries");
    analysisManager->CreateNtupleIColumn("nTransmitted");
    analysisManager->CreateNtupleIColumn("nAbsorbed");
    
    analysisManager->FinishNtuple();
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 1 : gamma_lines (AVEC PROCESSUS D'ABSORPTION)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("gamma_lines", "Statistiques par raie gamma");
    analysisManager->CreateNtupleIColumn("lineIndex");
    analysisManager->CreateNtupleDColumn("energy_keV");
    analysisManager->CreateNtupleIColumn("emitted");
    analysisManager->CreateNtupleIColumn("enteredWater");
    analysisManager->CreateNtupleIColumn("absorbedWater");
    analysisManager->CreateNtupleDColumn("waterAbsRate");
    // NOUVEAU : colonnes pour les processus d'absorption
    analysisManager->CreateNtupleIColumn("nPhotoelectric");   // Effet photoélectrique
    analysisManager->CreateNtupleIColumn("nCompton");         // Diffusion Compton
    analysisManager->CreateNtupleIColumn("nPairProduction");  // Création de paires
    analysisManager->CreateNtupleIColumn("nOther");           // Autres processus
    analysisManager->FinishNtuple();
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 2 : precontainer (particules vers eau, +z)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("precontainer", "Plan PreContainer - particules vers eau (+z)");
    analysisManager->CreateNtupleIColumn("nPhotons");
    analysisManager->CreateNtupleDColumn("sumEPhotons_keV");
    analysisManager->CreateNtupleIColumn("nElectrons");
    analysisManager->CreateNtupleDColumn("sumEElectrons_keV");
    analysisManager->FinishNtuple();
    
    // ═══════════════════════════════════════════════════════════════
    // NTUPLE 3 : postcontainer (particules depuis et vers eau)
    // ═══════════════════════════════════════════════════════════════
    analysisManager->CreateNtuple("postcontainer", "Plan PostContainer - particules depuis/vers eau");
    analysisManager->CreateNtupleIColumn("nPhotons_back");
    analysisManager->CreateNtupleDColumn("sumEPhotons_back_keV");
    analysisManager->CreateNtupleIColumn("nElectrons_back");
    analysisManager->CreateNtupleDColumn("sumEElectrons_back_keV");
    analysisManager->CreateNtupleIColumn("nPhotons_fwd");
    analysisManager->CreateNtupleDColumn("sumEPhotons_fwd_keV");
    analysisManager->CreateNtupleIColumn("nElectrons_fwd");
    analysisManager->CreateNtupleDColumn("sumEElectrons_fwd_keV");
    analysisManager->FinishNtuple();
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4int nEvents = run->GetNumberOfEvent();
    if (nEvents == 0) return;
    
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Remplir le ntuple des raies gamma (AVEC PROCESSUS)
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        analysisManager->FillNtupleIColumn(1, 0, i);
        analysisManager->FillNtupleDColumn(1, 1, EventAction::GetGammaLineEnergy(i));
        analysisManager->FillNtupleIColumn(1, 2, fLineEmitted[i]);
        analysisManager->FillNtupleIColumn(1, 3, fLineEnteredWater[i]);
        analysisManager->FillNtupleIColumn(1, 4, fLineAbsorbedWater[i]);
        analysisManager->FillNtupleDColumn(1, 5, GetLineWaterAbsorptionRate(i));
        // Colonnes processus
        analysisManager->FillNtupleIColumn(1, 6, fLineAbsorbedByProcess[i][EventAction::kPhotoelectric]);
        analysisManager->FillNtupleIColumn(1, 7, fLineAbsorbedByProcess[i][EventAction::kCompton]);
        analysisManager->FillNtupleIColumn(1, 8, fLineAbsorbedByProcess[i][EventAction::kPairProduction]);
        analysisManager->FillNtupleIColumn(1, 9, fLineAbsorbedByProcess[i][EventAction::kOther]);
        analysisManager->AddNtupleRow(1);
    }
    
    analysisManager->Write();
    analysisManager->CloseFile();
    
    // Affichage des résultats (console + log)
    PrintResults(nEvents);
    PrintDoseTable();
    PrintAbsorptionTable();
    PrintDoseRateTable(nEvents);  // NOUVEAU : calcul du débit de dose
    
    Logger::GetInstance()->LogHeader("Fin du Run - Résultats");
    Logger::GetInstance()->Close();
}

// ═══════════════════════════════════════════════════════════════
// AFFICHAGE DES RÉSULTATS
// ═══════════════════════════════════════════════════════════════

void RunAction::PrintResults(G4int nEvents) const
{
    std::ostringstream oss;
    
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                     RÉSULTATS DU RUN (SANS FILTRE)                            ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  Événements simulés: " << std::setw(10) << nEvents << "                                          ║\n";
    oss << "║  Primaires générés:  " << std::setw(10) << fTotalPrimariesGenerated << "                                          ║\n";
    oss << "║  Transmis:           " << std::setw(10) << fTotalTransmitted << "                                          ║\n";
    oss << "║  Absorbés:           " << std::setw(10) << fTotalAbsorbed << "                                          ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  COMPTEURS DE PASSAGE (SANS FILTRE) :                                         ║\n";
    oss << "║    Pre-container:    " << std::setw(10) << fGammasPreContainerPlane << "  (dans eau, entrée)                     ║\n";
    oss << "║    Post-container:   " << std::setw(10) << fGammasPostContainerPlane << "  (dans eau, sortie)                     ║\n";
    oss << "║    Gammas -> eau:    " << std::setw(10) << fGammasEnteringWater << "                                          ║\n";
    oss << "║    Électrons -> eau: " << std::setw(10) << fElectronsInWater << "                                          ║\n";
    oss << "╚═══════════════════════════════════════════════════════════════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
}

void RunAction::PrintDoseTable() const
{
    std::ostringstream oss;
    
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                                  DOSES PAR ANNEAU (nGy)                                       ║\n";
    oss << "╠══════════╦═══════════════╦═══════════════╦═══════════════╦═══════════════╦════════════════════╣\n";
    oss << "║  Anneau  ║   Masse (g)   ║ E_dep (MeV)   ║  Dose (nGy)   ║ Dose/evt(nGy) ║  Événements        ║\n";
    oss << "╠══════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════╬════════════════════╣\n";
    
    G4double totalDose_nGy = 0.;
    G4double totalMass = 0.;
    G4int totalEvents = 0;
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double mass_g = fRingMasses[i];
        G4double energy_MeV = fRingTotalEnergy[i] / MeV;
        G4double dose_nGy = EnergyToNanoGray(energy_MeV, mass_g);
        G4double dosePerEvt = (fRingEventCount[i] > 0) ? dose_nGy / fRingEventCount[i] : 0.;
        
        oss << "║    " << std::setw(2) << i << "    ║"
            << std::setw(13) << std::fixed << std::setprecision(4) << mass_g << "  ║"
            << std::setw(13) << std::scientific << std::setprecision(3) << energy_MeV << "  ║"
            << std::setw(13) << std::scientific << std::setprecision(3) << dose_nGy << "  ║"
            << std::setw(13) << std::scientific << std::setprecision(3) << dosePerEvt << "  ║"
            << std::setw(18) << std::fixed << std::setprecision(0) << fRingEventCount[i] << "  ║\n";
        
        totalDose_nGy += dose_nGy;
        totalMass += mass_g;
        totalEvents = std::max(totalEvents, fRingEventCount[i]);
    }
    
    G4double totalDosePerEvt = (fTotalEvents > 0) ? totalDose_nGy / fTotalEvents : 0.;
    
    oss << "╠══════════╬═══════════════╬═══════════════╬═══════════════╬═══════════════╬════════════════════╣\n";
    oss << "║  TOTAL   ║"
        << std::setw(13) << std::fixed << std::setprecision(4) << totalMass << "  ║"
        << std::setw(13) << std::scientific << std::setprecision(3) << fTotalWaterEnergy/MeV << "  ║"
        << std::setw(13) << std::scientific << std::setprecision(3) << totalDose_nGy << "  ║"
        << std::setw(13) << std::scientific << std::setprecision(3) << totalDosePerEvt << "  ║"
        << std::setw(18) << std::fixed << std::setprecision(0) << fTotalEvents << "  ║\n";
    oss << "╚══════════╩═══════════════╩═══════════════╩═══════════════╩═══════════════╩════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
}

void RunAction::PrintAbsorptionTable() const
{
    std::ostringstream oss;
    
    oss << "\n";
    oss << "╔════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║              TAUX D'ABSORPTION PAR RAIE GAMMA Eu-152 (SANS FILTRE)             ║\n";
    oss << "╠═══════════════╦═══════════╦═══════════════╦═══════════════╦════════════════════╣\n";
    oss << "║     Raie      ║   Émis    ║ Entrés eau    ║   Abs. eau    ║  Taux abs. eau (%) ║\n";
    oss << "╠═══════════════╬═══════════╬═══════════════╬═══════════════╬════════════════════╣\n";
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4double waterRate = GetLineWaterAbsorptionRate(i);
        
        oss << "║ " << std::setw(13) << std::left << EventAction::GetGammaLineName(i) << " ║"
            << std::setw(10) << std::right << fLineEmitted[i] << " ║"
            << std::setw(14) << fLineEnteredWater[i] << " ║"
            << std::setw(14) << fLineAbsorbedWater[i] << " ║"
            << std::setw(18) << std::fixed << std::setprecision(2) << waterRate << " ║\n";
    }
    
    G4int totalEmitted = 0, totalEntered = 0, totalAbsWater = 0;
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        totalEmitted += fLineEmitted[i];
        totalEntered += fLineEnteredWater[i];
        totalAbsWater += fLineAbsorbedWater[i];
    }
    
    G4double totalWaterRate = (totalEntered > 0) ? 100. * totalAbsWater / totalEntered : 0.;
    
    oss << "╠═══════════════╬═══════════╬═══════════════╬═══════════════╬════════════════════╣\n";
    oss << "║     TOTAL     ║"
        << std::setw(10) << totalEmitted << " ║"
        << std::setw(14) << totalEntered << " ║"
        << std::setw(14) << totalAbsWater << " ║"
        << std::setw(18) << std::fixed << std::setprecision(2) << totalWaterRate << " ║\n";
    oss << "╚═══════════════╩═══════════╩═══════════════╩═══════════════╩════════════════════╝\n";
    
    // ─────────────────────────────────────────────────────────────
    // NOUVEAU TABLEAU : PROCESSUS D'ABSORPTION PAR RAIE
    // ─────────────────────────────────────────────────────────────
    oss << "\n";
    oss << "╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                              PROCESSUS D'ABSORPTION DANS L'EAU PAR RAIE                                          ║\n";
    oss << "╠═══════════════╦═══════════════╦═══════════════════╦═══════════════════╦═══════════════════╦══════════════════════╣\n";
    oss << "║     Raie      ║   Abs. eau    ║   Photoélectrique ║      Compton      ║  Création paires  ║       Autres         ║\n";
    oss << "╠═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═══════════════════╬══════════════════════╣\n";
    
    // Totaux par processus
    G4int totalPhot = 0, totalCompt = 0, totalConv = 0, totalOther = 0;
    
    for (G4int i = 0; i < EventAction::kNbGammaLines; ++i) {
        G4int absTotal = fLineAbsorbedWater[i];
        G4int phot = fLineAbsorbedByProcess[i][EventAction::kPhotoelectric];
        G4int compt = fLineAbsorbedByProcess[i][EventAction::kCompton];
        G4int conv = fLineAbsorbedByProcess[i][EventAction::kPairProduction];
        G4int other = fLineAbsorbedByProcess[i][EventAction::kOther];
        
        totalPhot += phot;
        totalCompt += compt;
        totalConv += conv;
        totalOther += other;
        
        // Calculer les pourcentages
        G4double photPct = (absTotal > 0) ? 100.0 * phot / absTotal : 0.;
        G4double comptPct = (absTotal > 0) ? 100.0 * compt / absTotal : 0.;
        G4double convPct = (absTotal > 0) ? 100.0 * conv / absTotal : 0.;
        G4double otherPct = (absTotal > 0) ? 100.0 * other / absTotal : 0.;
        
        oss << "║ " << std::setw(13) << std::left << EventAction::GetGammaLineName(i) << " ║"
            << std::setw(14) << std::right << absTotal << " ║"
            << std::setw(10) << phot << " (" << std::setw(5) << std::fixed << std::setprecision(1) << photPct << "%) ║"
            << std::setw(10) << compt << " (" << std::setw(5) << comptPct << "%) ║"
            << std::setw(10) << conv << " (" << std::setw(5) << convPct << "%) ║"
            << std::setw(10) << other << " (" << std::setw(5) << otherPct << "%)  ║\n";
    }
    
    // Ligne TOTAL
    G4double totalPhotPct = (totalAbsWater > 0) ? 100.0 * totalPhot / totalAbsWater : 0.;
    G4double totalComptPct = (totalAbsWater > 0) ? 100.0 * totalCompt / totalAbsWater : 0.;
    G4double totalConvPct = (totalAbsWater > 0) ? 100.0 * totalConv / totalAbsWater : 0.;
    G4double totalOtherPct = (totalAbsWater > 0) ? 100.0 * totalOther / totalAbsWater : 0.;
    
    oss << "╠═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═══════════════════╬══════════════════════╣\n";
    oss << "║     TOTAL     ║"
        << std::setw(14) << totalAbsWater << " ║"
        << std::setw(10) << totalPhot << " (" << std::setw(5) << std::fixed << std::setprecision(1) << totalPhotPct << "%) ║"
        << std::setw(10) << totalCompt << " (" << std::setw(5) << totalComptPct << "%) ║"
        << std::setw(10) << totalConv << " (" << std::setw(5) << totalConvPct << "%) ║"
        << std::setw(10) << totalOther << " (" << std::setw(5) << totalOtherPct << "%)  ║\n";
    oss << "╚═══════════════╩═══════════════╩═══════════════════╩═══════════════════╩═══════════════════╩══════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
}

// ═══════════════════════════════════════════════════════════════
// ACCUMULATION DES DONNÉES
// ═══════════════════════════════════════════════════════════════

void RunAction::AddRingEnergy(G4int ringIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings) {
        fRingTotalEnergy[ringIndex] += edep;
        fRingTotalEnergy2[ringIndex] += edep * edep;
        fRingEventCount[ringIndex]++;
        
        fTotalWaterEnergy += edep;
        fTotalWaterEventCount++;
        
        // Remplir l'histogramme de dose par anneau
        auto analysisManager = G4AnalysisManager::Instance();
        G4double dose_nGy = EnergyToNanoGray(edep / MeV, fRingMasses[ringIndex]);
        analysisManager->FillH1(ringIndex, dose_nGy);
    }
}

void RunAction::AddRingEnergyByLine(G4int ringIndex, G4int lineIndex, G4double edep)
{
    if (ringIndex >= 0 && ringIndex < DetectorConstruction::kNbWaterRings &&
        lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        fRingEnergyByLine[ringIndex][lineIndex] += edep;
        
        auto analysisManager = G4AnalysisManager::Instance();
        analysisManager->FillH1(6 + lineIndex, edep / keV);
    }
}

void RunAction::RecordEventStatistics(G4int nPrimaries, 
                                      const std::vector<G4double>& primaryEnergies,
                                      G4int nTransmitted,
                                      G4int nAbsorbed,
                                      G4double totalDeposit,
                                      const std::array<G4double, 5>& ringEnergies)
{
    fTotalEvents++;
    fTotalPrimariesGenerated += nPrimaries;
    fTotalTransmitted += nTransmitted;
    fTotalAbsorbed += nAbsorbed;
    
    if (nPrimaries == 0) {
        fTotalEventsWithZeroGamma++;
    }
    
    auto analysisManager = G4AnalysisManager::Instance();
    
    G4double totalDose_nGy = 0.;
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double dose_nGy = EnergyToNanoGray(ringEnergies[i] / MeV, fRingMasses[i]);
        analysisManager->FillNtupleDColumn(0, i, dose_nGy);
        totalDose_nGy += dose_nGy;
    }
    
    analysisManager->FillNtupleDColumn(0, DetectorConstruction::kNbWaterRings, totalDose_nGy);
    analysisManager->FillNtupleDColumn(0, DetectorConstruction::kNbWaterRings + 1, totalDeposit / keV);
    analysisManager->FillNtupleIColumn(0, DetectorConstruction::kNbWaterRings + 2, nPrimaries);
    analysisManager->FillNtupleIColumn(0, DetectorConstruction::kNbWaterRings + 3, nTransmitted);
    analysisManager->FillNtupleIColumn(0, DetectorConstruction::kNbWaterRings + 4, nAbsorbed);
    analysisManager->AddNtupleRow(0);
    
    if (totalDeposit > 0.) {
        analysisManager->FillH1(5, totalDose_nGy);
    }
}

void RunAction::RecordContainerPlaneStatistics(
    G4int preNPhotons, G4double preSumEPhotons,
    G4int preNElectrons, G4double preSumEElectrons,
    G4int postNPhotonsBack, G4double postSumEPhotonsBack,
    G4int postNElectronsBack, G4double postSumEElectronsBack,
    G4int postNPhotonsFwd, G4double postSumEPhotonsFwd,
    G4int postNElectronsFwd, G4double postSumEElectronsFwd)
{
    auto analysisManager = G4AnalysisManager::Instance();
    
    // Ntuple 2 : precontainer
    analysisManager->FillNtupleIColumn(2, 0, preNPhotons);
    analysisManager->FillNtupleDColumn(2, 1, preSumEPhotons / keV);
    analysisManager->FillNtupleIColumn(2, 2, preNElectrons);
    analysisManager->FillNtupleDColumn(2, 3, preSumEElectrons / keV);
    analysisManager->AddNtupleRow(2);
    
    // Ntuple 3 : postcontainer
    analysisManager->FillNtupleIColumn(3, 0, postNPhotonsBack);
    analysisManager->FillNtupleDColumn(3, 1, postSumEPhotonsBack / keV);
    analysisManager->FillNtupleIColumn(3, 2, postNElectronsBack);
    analysisManager->FillNtupleDColumn(3, 3, postSumEElectronsBack / keV);
    analysisManager->FillNtupleIColumn(3, 4, postNPhotonsFwd);
    analysisManager->FillNtupleDColumn(3, 5, postSumEPhotonsFwd / keV);
    analysisManager->FillNtupleIColumn(3, 6, postNElectronsFwd);
    analysisManager->FillNtupleDColumn(3, 7, postSumEElectronsFwd / keV);
    analysisManager->AddNtupleRow(3);
}

void RunAction::RecordGammaLineStatistics(G4int lineIndex,
                                          G4bool enteredWater,
                                          G4bool absorbedInWater,
                                          G4int absorptionProcess)
{
    if (lineIndex < 0 || lineIndex >= EventAction::kNbGammaLines) return;
    
    fLineEmitted[lineIndex]++;
    
    if (enteredWater) fLineEnteredWater[lineIndex]++;
    if (absorbedInWater) {
        fLineAbsorbedWater[lineIndex]++;
        
        // Enregistrer le processus d'absorption
        if (absorptionProcess >= 0 && absorptionProcess < EventAction::kNbProcesses) {
            fLineAbsorbedByProcess[lineIndex][absorptionProcess]++;
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// ACCESSEURS POUR LES STATISTIQUES PAR RAIE
// ═══════════════════════════════════════════════════════════════

G4int RunAction::GetLineEmitted(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        return fLineEmitted[lineIndex];
    }
    return 0;
}

G4int RunAction::GetLineAbsorbedWater(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        return fLineAbsorbedWater[lineIndex];
    }
    return 0;
}

G4double RunAction::GetLineWaterAbsorptionRate(G4int lineIndex) const
{
    if (lineIndex >= 0 && lineIndex < EventAction::kNbGammaLines) {
        if (fLineEnteredWater[lineIndex] > 0) {
            return static_cast<G4double>(fLineAbsorbedWater[lineIndex]) / 
                   static_cast<G4double>(fLineEnteredWater[lineIndex]) * 100.;
        }
    }
    return 0.;
}

// ═══════════════════════════════════════════════════════════════
// CALCUL DU DÉBIT DE DOSE EN FONCTION DE LA DISTANCE
// ═══════════════════════════════════════════════════════════════

void RunAction::PrintDoseRateTable(G4int nEvents) const
{
    std::ostringstream oss;
    
    // ─────────────────────────────────────────────────────────────
    // PARAMÈTRES GÉOMÉTRIQUES
    // ─────────────────────────────────────────────────────────────
    G4double distance = fWaterBottomZ - fSourcePosZ;  // Distance source -> eau
    G4double R = fWaterRadius;
    
    // Angle solide du disque d'eau vu depuis la source
    // Ω = 2π(1 - d/√(d² + R²))
    G4double solidAngle = 2.0 * CLHEP::pi * (1.0 - distance / std::sqrt(distance*distance + R*R));
    G4double solidAngleFraction = solidAngle / (4.0 * CLHEP::pi);
    
    // Angle θ du cône qui intercepte le disque
    G4double thetaWater = std::atan(R / distance);
    
    // ─────────────────────────────────────────────────────────────
    // ACTIVITÉ EFFECTIVE
    // ─────────────────────────────────────────────────────────────
    G4double activityEffective = fActivity4pi * solidAngleFraction;  // Bq
    
    // ─────────────────────────────────────────────────────────────
    // DOSE TOTALE
    // ─────────────────────────────────────────────────────────────
    G4double totalMass = 0.;
    for (const auto& m : fRingMasses) totalMass += m;
    
    G4double totalEnergy_MeV = fTotalWaterEnergy / MeV;
    G4double dosePerDecay_nGy = EnergyToNanoGray(totalEnergy_MeV, totalMass) / nEvents;
    G4double dosePerDecay_Gy = dosePerDecay_nGy * 1e-9;
    
    G4double doseRate_Gy_s = dosePerDecay_Gy * activityEffective;
    G4double doseRate_nGy_h = doseRate_Gy_s * 1e9 * 3600.0;
    G4double doseRate_uGy_h = doseRate_nGy_h / 1000.0;
    G4double doseRate_mGy_h = doseRate_uGy_h / 1000.0;
    
    // ─────────────────────────────────────────────────────────────
    // AFFICHAGE GÉOMÉTRIE ET SOURCE
    // ─────────────────────────────────────────────────────────────
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                        DÉBIT DE DOSE ET GÉOMÉTRIE                                     ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  GÉOMÉTRIE :                                                                          ║\n";
    oss << "║    Position source (z)      : " << std::setw(10) << std::fixed << std::setprecision(2) << fSourcePosZ/mm << " mm                                    ║\n";
    oss << "║    Position eau (z)         : " << std::setw(10) << fWaterBottomZ/mm << " mm                                    ║\n";
    oss << "║    Distance source → eau    : " << std::setw(10) << distance/mm << " mm                                    ║\n";
    oss << "║    Rayon de l'eau           : " << std::setw(10) << R/mm << " mm                                    ║\n";
    oss << "║    Demi-angle θ (eau)       : " << std::setw(10) << thetaWater/deg << " deg                                   ║\n";
    oss << "║    Angle solide Ω           : " << std::setw(10) << std::setprecision(4) << solidAngle << " sr                                    ║\n";
    oss << "║    Fraction Ω/(4π)          : " << std::setw(10) << std::setprecision(4) << solidAngleFraction << "                                        ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  SOURCE :                                                                             ║\n";
    oss << "║    Activité 4π              : " << std::setw(10) << std::scientific << std::setprecision(2) << fActivity4pi << " Bq (" << std::fixed << std::setprecision(1) << fActivity4pi/1000.0 << " kBq)                  ║\n";
    oss << "║    Activité effective       : " << std::setw(10) << std::scientific << std::setprecision(2) << activityEffective << " Bq (dans Ω)                          ║\n";
    oss << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n";
    
    // ─────────────────────────────────────────────────────────────
    // TABLEAU DÉBIT DE DOSE PAR ANNEAU
    // ─────────────────────────────────────────────────────────────
    oss << "\n";
    oss << "╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                                              DÉBIT DE DOSE PAR ANNEAU                                                                    ║\n";
    oss << "╠═════════╦═══════════════╦═══════════════╦═══════════════════╦═══════════════════╦═════════════════╦═════════════════╦═════════════════════╣\n";
    oss << "║ Anneau  ║  r_int-r_ext  ║   Masse (g)   ║ Dose/evt_sim(nGy) ║   Débit (µGy/h)   ║ t(20cGy) heures ║ t(50cGy) heures ║   t(20cGy) jours    ║\n";
    oss << "╠═════════╬═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═════════════════╬═════════════════╬═════════════════════╣\n";
    
    G4double totalDoseRate_nGy_h = 0.;
    
    // Stocker les débits par anneau pour le résumé
    std::array<G4double, 5> ringDoseRate_uGy_h_arr;
    
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        G4double rIn = DetectorConstruction::GetRingInnerRadius(i);
        G4double rOut = DetectorConstruction::GetRingOuterRadius(i);
        G4double mass_g = fRingMasses[i];
        
        // Dose par désintégration pour cet anneau
        G4double energy_MeV = fRingTotalEnergy[i] / MeV;
        G4double ringDosePerDecay_nGy = EnergyToNanoGray(energy_MeV, mass_g) / nEvents;
        
        // Débit de dose pour cet anneau
        G4double ringDosePerDecay_Gy = ringDosePerDecay_nGy * 1e-9;
        G4double ringDoseRate_Gy_s = ringDosePerDecay_Gy * activityEffective;
        G4double ringDoseRate_nGy_h = ringDoseRate_Gy_s * 1e9 * 3600.0;
        G4double ringDoseRate_uGy_h = ringDoseRate_nGy_h / 1000.0;
        G4double ringDoseRate_mGy_h = ringDoseRate_uGy_h / 1000.0;
        
        ringDoseRate_uGy_h_arr[i] = ringDoseRate_uGy_h;
        totalDoseRate_nGy_h += ringDoseRate_nGy_h;
        
        // Temps pour atteindre 20 cGy et 50 cGy (cGy -> mGy : ×10)
        G4double timeFor20cGy_h = (20.0 * 10.0) / ringDoseRate_mGy_h;  // 20 cGy = 200 mGy
        G4double timeFor50cGy_h = (50.0 * 10.0) / ringDoseRate_mGy_h;  // 50 cGy = 500 mGy
        G4double timeFor20cGy_days = timeFor20cGy_h / 24.0;
        
        oss << "║    " << std::setw(1) << i << "    ║   "
            << std::setw(2) << std::fixed << std::setprecision(0) << rIn/mm << " - " 
            << std::setw(2) << rOut/mm << " mm   ║"
            << std::setw(13) << std::setprecision(4) << mass_g << "  ║"
            << std::setw(17) << std::scientific << std::setprecision(3) << ringDosePerDecay_nGy << "  ║"
            << std::setw(17) << ringDoseRate_uGy_h << "  ║"
            << std::setw(15) << std::fixed << std::setprecision(1) << timeFor20cGy_h << "  ║"
            << std::setw(15) << timeFor50cGy_h << "  ║"
            << std::setw(19) << std::setprecision(2) << timeFor20cGy_days << "  ║\n";
    }
    
    G4double totalDoseRate_uGy_h = totalDoseRate_nGy_h / 1000.0;
    G4double totalDoseRate_mGy_h = totalDoseRate_uGy_h / 1000.0;
    
    // Temps totaux
    G4double totalTimeFor20cGy_h = (20.0 * 10.0) / totalDoseRate_mGy_h;
    G4double totalTimeFor50cGy_h = (50.0 * 10.0) / totalDoseRate_mGy_h;
    G4double totalTimeFor20cGy_days = totalTimeFor20cGy_h / 24.0;
    
    oss << "╠═════════╬═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═════════════════╬═════════════════╬═════════════════════╣\n";
    oss << "║  TOTAL  ║               ║"
        << std::setw(13) << std::fixed << std::setprecision(4) << totalMass << "  ║"
        << std::setw(17) << std::scientific << std::setprecision(3) << dosePerDecay_nGy << "  ║"
        << std::setw(17) << totalDoseRate_uGy_h << "  ║"
        << std::setw(15) << std::fixed << std::setprecision(1) << totalTimeFor20cGy_h << "  ║"
        << std::setw(15) << totalTimeFor50cGy_h << "  ║"
        << std::setw(19) << std::setprecision(2) << totalTimeFor20cGy_days << "  ║\n";
    oss << "╚═════════╩═══════════════╩═══════════════╩═══════════════════╩═══════════════════╩═════════════════╩═════════════════╩═════════════════════╝\n";
    
    // ─────────────────────────────────────────────────────────────
    // TEMPS D'IRRADIATION
    // ─────────────────────────────────────────────────────────────
    G4double timeFor10cGy_h = (10.0 * 10.0) / totalDoseRate_mGy_h;
    G4double timeFor20cGy_total_h = (20.0 * 10.0) / totalDoseRate_mGy_h;
    G4double timeFor50cGy_total_h = (50.0 * 10.0) / totalDoseRate_mGy_h;
    
    oss << "\n";
    oss << "╔═══════════════════════════════════════════════════════════════════════════════════════╗\n";
    oss << "║                         RÉSUMÉ TEMPS D'IRRADIATION (dose totale)                      ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  Débit total                : " << std::setw(12) << std::scientific << std::setprecision(3) << totalDoseRate_mGy_h << " mGy/h                              ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  Pour 10 cGy (100 mGy)      : " << std::setw(12) << std::fixed << std::setprecision(1) << timeFor10cGy_h << " h  = " << std::setw(8) << std::setprecision(2) << timeFor10cGy_h/24.0 << " jours               ║\n";
    oss << "║  Pour 20 cGy (200 mGy)      : " << std::setw(12) << std::setprecision(1) << timeFor20cGy_total_h << " h  = " << std::setw(8) << std::setprecision(2) << timeFor20cGy_total_h/24.0 << " jours               ║\n";
    oss << "║  Pour 50 cGy (500 mGy)      : " << std::setw(12) << timeFor50cGy_total_h << " h  = " << std::setw(8) << std::setprecision(2) << timeFor50cGy_total_h/24.0 << " jours               ║\n";
    oss << "╠═══════════════════════════════════════════════════════════════════════════════════════╣\n";
    oss << "║  NOTE : Dose/evt_sim = dose par événement simulation (tir dans le cône)              ║\n";
    oss << "║         Débit = Dose/evt_sim × A_effective = Dose/evt_sim × A × f_Ω                 ║\n";
    oss << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n";
    
    G4cout << oss.str();
    if (Logger::GetInstance()->IsOpen()) {
        Logger::GetInstance()->GetStream() << oss.str();
    }
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
