#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4UserLimits.hh"
#include "G4UnitsTable.hh"
#include <cmath>

DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fTungsten(nullptr),
  fWater(nullptr),
  fPolystyrene(nullptr),
  fContainerRadius(25.0*mm),              // Rayon : 25 mm = 2.5 cm
  fPolystyreneThickness(1.0*mm),          // Épaisseur paroi PS : 1 mm
  fWaterThickness1(2.0*mm),               // Première tranche d'eau : 2 mm
  fWaterThickness2(1.0*mm),               // Deuxième tranche d'eau : 1 mm (anneaux)
  fRingWidth(5.0*mm),                     // Largeur des anneaux : 5 mm
  fPreContainerPlaneThickness(1.0*mm),    // PreContainerPlane : 1 mm (AIR)
  fPreContainerPlaneRadius(25.0*mm),      // Rayon PreContainer : 25 mm = 2.5 cm
  fTungstenFoilThickness(50.0*um),        // Feuille W : 50 µm
  fTungstenFoilRadius(25.0*mm),           // Rayon feuille W : 25 mm
  fSourceToWaterDistance(25.0*mm)         // Distance source-eau : 25 mm
{
    fRingMasses.resize(kNbWaterRings, 0.);
}

DetectorConstruction::~DetectorConstruction()
{}

G4String DetectorConstruction::GetWaterRingName(G4int ringIndex)
{
    return "WaterRing_" + std::to_string(ringIndex);
}

G4double DetectorConstruction::GetRingInnerRadius(G4int ringIndex)
{
    return ringIndex * 5.0 * mm;
}

G4double DetectorConstruction::GetRingOuterRadius(G4int ringIndex)
{
    return (ringIndex + 1) * 5.0 * mm;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();
    
    // =============================================================================
    // MATÉRIAUX
    // =============================================================================

    G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
    fWater = nist->FindOrBuildMaterial("G4_WATER");
    fTungsten = nist->FindOrBuildMaterial("G4_W");
    fPolystyrene = nist->FindOrBuildMaterial("G4_POLYSTYRENE");

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║              MATÉRIAUX - CONFIGURATION OPTIMISÉE              ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Eau (G4_WATER)         : rho = " << G4BestUnit(fWater->GetDensity(), "Volumic Mass") << "               ║" << G4endl;
    G4cout << "║  Polystyrene (G4_PS)    : rho = " << G4BestUnit(fPolystyrene->GetDensity(), "Volumic Mass") << "               ║" << G4endl;
    G4cout << "║  Tungstene (G4_W)       : rho = " << G4BestUnit(fTungsten->GetDensity(), "Volumic Mass") << "               ║" << G4endl;
    G4cout << "║  Air (G4_AIR)           : rho = " << G4BestUnit(air->GetDensity(), "Volumic Mass") << "            ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // WORLD
    // =============================================================================
    G4double world_size = 50*cm;
    G4Box* solidWorld = new G4Box("World", world_size/2, world_size/2, world_size/2);
    G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, air, "World");
    
    G4VPhysicalVolume* physWorld = new G4PVPlacement(0,
                                G4ThreeVector(),
                                logicWorld,
                                "World",
                                0,
                                false,
                                0);

    logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    // =============================================================================
    // ENVELOPPE
    // =============================================================================
    G4Box* solidEnveloppe = new G4Box("Enveloppe", 20*cm, 20*cm, 20*cm);
    G4LogicalVolume* logicEnveloppe = new G4LogicalVolume(solidEnveloppe, air, "Enveloppe");

    new G4PVPlacement(nullptr,
                      G4ThreeVector(),
                      logicEnveloppe,
                      "Enveloppe",
                      logicWorld,
                      false,
                      0,
                      true);

    G4VisAttributes* enveloppeVis = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0, 0.05));
    enveloppeVis->SetVisibility(false);
    logicEnveloppe->SetVisAttributes(enveloppeVis);

    // =============================================================================
    // CALCUL DES POSITIONS Z
    // =============================================================================
    // Configuration (z croissant) :
    //   Source (75mm) -> Air -> PreContainer (air 1mm) -> Eau1 (2mm) 
    //   -> Eau2 (1mm) -> PostContainer=PS (1mm) -> W (50µm)
    // =============================================================================
    
    // Position de référence : surface de l'eau à z = 100 mm
    G4double waterSurfaceZ = 100.0*mm;
    G4double sourceZ = waterSurfaceZ - fSourceToWaterDistance;  // 75 mm
    
    // PreContainer Plane (AIR, 1 mm) : AVANT la surface de l'eau
    // z = 99 à 100 mm
    G4double preContainerTopZ = waterSurfaceZ;                                      // 100 mm
    G4double preContainerBottomZ = preContainerTopZ - fPreContainerPlaneThickness;  // 99 mm
    G4double preContainerCenterZ = (preContainerBottomZ + preContainerTopZ) / 2;    // 99.5 mm
    
    // Eau 1 (2 mm) : z = 100 à 102 mm
    G4double water1BottomZ = waterSurfaceZ;                                         // 100 mm
    G4double water1TopZ = water1BottomZ + fWaterThickness1;                         // 102 mm
    G4double water1CenterZ = (water1BottomZ + water1TopZ) / 2;                      // 101 mm
    
    // Eau 2 avec anneaux (1 mm) : z = 102 à 103 mm
    G4double water2BottomZ = water1TopZ;                                            // 102 mm
    G4double water2TopZ = water2BottomZ + fWaterThickness2;                         // 103 mm
    G4double water2CenterZ = (water2BottomZ + water2TopZ) / 2;                      // 102.5 mm
    
    // Polystyrène = PostContainer Plane (1 mm) : z = 103 à 104 mm
    G4double psBottomZ = water2TopZ;                                                // 103 mm
    G4double psTopZ = psBottomZ + fPolystyreneThickness;                            // 104 mm
    G4double psCenterZ = (psBottomZ + psTopZ) / 2;                                  // 103.5 mm
    
    // Tungstène (50 µm) : z = 104 à 104.05 mm
    G4double tungstenBottomZ = psTopZ;                                              // 104 mm
    G4double tungstenTopZ = tungstenBottomZ + fTungstenFoilThickness;               // 104.05 mm
    G4double tungstenCenterZ = (tungstenBottomZ + tungstenTopZ) / 2;                // 104.025 mm

    // =============================================================================
    // PRECONTAINER PLANE (1 mm) - AIR - AVANT la surface de l'eau
    // Matériau : AIR
    // Position : z = 99 à 100 mm
    // =============================================================================
    
    G4Tubs* solidPreContainerPlane = new G4Tubs("PreContainerPlane",
                                                 0.,
                                                 fPreContainerPlaneRadius,
                                                 fPreContainerPlaneThickness/2,
                                                 0.*deg, 360.*deg);
    
    // MATÉRIAU : AIR
    G4LogicalVolume* logicPreContainerPlane = new G4LogicalVolume(solidPreContainerPlane, 
                                                                   air, 
                                                                   "PreContainerPlaneLog");
    
    G4VisAttributes* preContainerVis = new G4VisAttributes(G4Colour(1.0, 1.0, 0.0, 0.3));  // Jaune transparent
    preContainerVis->SetForceSolid(true);
    logicPreContainerPlane->SetVisAttributes(preContainerVis);
    
    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, preContainerCenterZ),
                      logicPreContainerPlane,
                      "PreContainerPlane",
                      logicEnveloppe,
                      false, 0, true);

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║     PRECONTAINER PLANE - AVANT la surface de l'eau            ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Materiau   : AIR                                             ║" << G4endl;
    G4cout << "║  Epaisseur  : " << fPreContainerPlaneThickness/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Rayon      : " << fPreContainerPlaneRadius/mm << " mm (2.5 cm)                                ║" << G4endl;
    G4cout << "║  Z bas      : " << preContainerBottomZ/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Z haut     : " << preContainerTopZ/mm << " mm (= surface eau)                         ║" << G4endl;
    G4cout << "║  Z centre   : " << preContainerCenterZ/mm << " mm                                          ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // PREMIÈRE TRANCHE D'EAU (2 mm) - VOLUME UNIFORME
    // Position : z = 100 à 102 mm
    // =============================================================================

    G4UserLimits* waterLimits = new G4UserLimits(0.1*mm);

    G4Tubs* solidWater1 = new G4Tubs("Water1",
                                      0.,
                                      fContainerRadius,
                                      fWaterThickness1/2,
                                      0.*deg, 360.*deg);

    G4LogicalVolume* logicWater1 = new G4LogicalVolume(solidWater1, fWater, "Water1Log");
    logicWater1->SetUserLimits(waterLimits);

    G4VisAttributes* water1Vis = new G4VisAttributes(G4Colour(0.0, 0.5, 1.0, 0.4));  // Bleu clair
    water1Vis->SetForceSolid(true);
    logicWater1->SetVisAttributes(water1Vis);

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, water1CenterZ),
                      logicWater1,
                      "Water1",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4double water1Volume = M_PI * fContainerRadius * fContainerRadius * fWaterThickness1;
    G4double water1Mass = water1Volume * fWater->GetDensity();

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║     PREMIERE TRANCHE D'EAU (2 mm) - VOLUME UNIFORME           ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Epaisseur  : " << fWaterThickness1/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Rayon      : " << fContainerRadius/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Z bas      : " << water1BottomZ/mm << " mm (= surface eau)                         ║" << G4endl;
    G4cout << "║  Z haut     : " << water1TopZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Z centre   : " << water1CenterZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Masse      : " << water1Mass/g << " g                                        ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // DEUXIÈME TRANCHE D'EAU (1 mm) - ANNEAUX CONCENTRIQUES (mesure de dose)
    // Position : z = 102 à 103 mm
    // =============================================================================
    
    std::vector<G4Colour> ringColors = {
        G4Colour(0.0, 0.3, 1.0, 0.6),
        G4Colour(0.0, 0.4, 1.0, 0.6),
        G4Colour(0.0, 0.5, 1.0, 0.6),
        G4Colour(0.0, 0.6, 1.0, 0.6),
        G4Colour(0.0, 0.7, 1.0, 0.6)
    };

    fWaterRingLogicals.clear();
    G4double waterDensity = fWater->GetDensity();

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  DEUXIEME TRANCHE D'EAU (1 mm) - ANNEAUX CONCENTRIQUES        ║" << G4endl;
    G4cout << "║  >>> VOLUME DE MESURE DE DOSE <<<                             ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Epaisseur  : " << fWaterThickness2/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Z bas      : " << water2BottomZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Z haut     : " << water2TopZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Z centre   : " << water2CenterZ/mm << " mm                                         ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Index | R_in (mm) | R_out (mm) | Volume (mm3) | Masse (g)   ║" << G4endl;
    G4cout << "╠════════╪═══════════╪════════════╪══════════════╪═════════════╣" << G4endl;

    for (G4int i = 0; i < kNbWaterRings; ++i) {
        G4double rIn = GetRingInnerRadius(i);
        G4double rOut = GetRingOuterRadius(i);
        
        G4String ringName = GetWaterRingName(i);
        
        G4Tubs* solidRing = new G4Tubs(ringName,
                                        rIn,
                                        rOut,
                                        fWaterThickness2/2,
                                        0.*deg, 360.*deg);

        G4LogicalVolume* logicRing = new G4LogicalVolume(solidRing, fWater, ringName + "Log");
        logicRing->SetUserLimits(waterLimits);
        
        G4VisAttributes* ringVis = new G4VisAttributes(ringColors[i]);
        ringVis->SetForceSolid(true);
        logicRing->SetVisAttributes(ringVis);

        new G4PVPlacement(nullptr,
                          G4ThreeVector(0, 0, water2CenterZ),
                          logicRing,
                          ringName,
                          logicEnveloppe,
                          false,
                          i,
                          true);

        fWaterRingLogicals.push_back(logicRing);

        G4double ringVolume = M_PI * (rOut*rOut - rIn*rIn) * fWaterThickness2;
        G4double ringMass = ringVolume * waterDensity;

        fRingMasses[i] = ringMass;

        char buffer[100];
        sprintf(buffer, "║    %d   |   %5.1f   |    %5.1f   |   %8.2f   |   %7.4f   ║",
                i, rIn/mm, rOut/mm, ringVolume/mm3, ringMass/g);
        G4cout << buffer << G4endl;
    }

    G4double totalWaterMass = 0.;
    for (G4int i = 0; i < kNbWaterRings; ++i) {
        totalWaterMass += fRingMasses[i];
    }

    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Masse totale eau (anneaux) : " << totalWaterMass/g << " g                     ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // POSTCONTAINER PLANE = POLYSTYRÈNE (1 mm) - Fond de la boîte de Petri
    // Matériau : POLYSTYRÈNE
    // Position : z = 103 à 104 mm
    // Le PostContainerPlane EST ce volume de polystyrène
    // =============================================================================

    G4Tubs* solidPostContainer = new G4Tubs("PostContainerPlane",
                                             0.,
                                             fContainerRadius,
                                             fPolystyreneThickness/2,
                                             0.*deg, 360.*deg);

    // MATÉRIAU : POLYSTYRÈNE
    G4LogicalVolume* logicPostContainer = new G4LogicalVolume(solidPostContainer, 
                                                               fPolystyrene, 
                                                               "PostContainerPlaneLog");

    G4VisAttributes* psVis = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8, 0.6));  // Gris clair
    psVis->SetForceSolid(true);
    logicPostContainer->SetVisAttributes(psVis);

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, psCenterZ),
                      logicPostContainer,
                      "PostContainerPlane",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4double psVolume = M_PI * fContainerRadius * fContainerRadius * fPolystyreneThickness;
    G4double psMass = psVolume * fPolystyrene->GetDensity();

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║     POSTCONTAINER PLANE = POLYSTYRENE (1 mm)                  ║" << G4endl;
    G4cout << "║     (Fond boite de Petri)                                     ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Materiau   : POLYSTYRENE                                     ║" << G4endl;
    G4cout << "║  Epaisseur  : " << fPolystyreneThickness/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Rayon      : " << fContainerRadius/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Z bas      : " << psBottomZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Z haut     : " << psTopZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Z centre   : " << psCenterZ/mm << " mm                                         ║" << G4endl;
    G4cout << "║  Masse      : " << psMass/g << " g                                        ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // FEUILLE DE TUNGSTÈNE (50 µm) - Sous le polystyrène
    // Position : z = 104 à 104.05 mm
    // =============================================================================

    G4Tubs* solidTungstenFoil = new G4Tubs("TungstenFoil",
                                           0.,
                                           fTungstenFoilRadius,
                                           fTungstenFoilThickness/2,
                                           0.*deg, 360.*deg);

    G4LogicalVolume* logicTungstenFoil = new G4LogicalVolume(solidTungstenFoil, fTungsten, "TungstenFoilLog");

    G4VisAttributes* tungstenFoilVis = new G4VisAttributes(G4Colour(0.3, 0.3, 0.3, 0.9));
    tungstenFoilVis->SetForceSolid(true);
    logicTungstenFoil->SetVisAttributes(tungstenFoilVis);

    new G4PVPlacement(nullptr,
                      G4ThreeVector(0, 0, tungstenCenterZ),
                      logicTungstenFoil,
                      "TungstenFoil",
                      logicEnveloppe,
                      false,
                      0,
                      true);

    G4double tungstenVolume = M_PI * fTungstenFoilRadius * fTungstenFoilRadius * fTungstenFoilThickness;
    G4double tungstenMass = tungstenVolume * fTungsten->GetDensity();

    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║     FEUILLE DE TUNGSTENE (50 um)                              ║" << G4endl;
    G4cout << "╠═══════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║  Epaisseur  : " << fTungstenFoilThickness/um << " um                                          ║" << G4endl;
    G4cout << "║  Rayon      : " << fTungstenFoilRadius/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Z bas      : " << tungstenBottomZ/mm << " mm                                           ║" << G4endl;
    G4cout << "║  Z haut     : " << tungstenTopZ/mm << " mm                                       ║" << G4endl;
    G4cout << "║  Z centre   : " << tungstenCenterZ/mm << " mm                                      ║" << G4endl;
    G4cout << "║  Masse      : " << tungstenMass/g << " g                                        ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;

    // =============================================================================
    // AFFICHAGE RÉCAPITULATIF DE LA GÉOMÉTRIE
    // =============================================================================

    G4cout << "\n╔══════════════════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║      GEOMETRIE OPTIMISEE - CONFIGURATION FINALE                          ║" << G4endl;
    G4cout << "╠══════════════════════════════════════════════════════════════════════════╣" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "║  SOURCE Eu-152 : z = " << sourceZ/mm << " mm                                            ║" << G4endl;
    G4cout << "║  Distance source-eau : " << fSourceToWaterDistance/mm << " mm                                     ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  EMPILEMENT (direction +z) :                                             ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "║    1. PreContainer (AIR)    : z = " << preContainerBottomZ/mm << " - " << preContainerTopZ/mm << " mm  (1 mm)           ║" << G4endl;
    G4cout << "║       Materiau: AIR | Rayon: 25 mm | AVANT surface eau                   ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "║    2. Eau 1 (uniforme)      : z = " << water1BottomZ/mm << " - " << water1TopZ/mm << " mm  (2 mm)          ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "║    3. Eau 2 (anneaux)       : z = " << water2BottomZ/mm << " - " << water2TopZ/mm << " mm  (1 mm)          ║" << G4endl;
    G4cout << "║       >>> VOLUME DE MESURE DE DOSE <<<                                   ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "║    4. PostContainer (PS)    : z = " << psBottomZ/mm << " - " << psTopZ/mm << " mm  (1 mm)          ║" << G4endl;
    G4cout << "║       Materiau: POLYSTYRENE | Rayon: 25 mm                               ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "║    5. Tungstene             : z = " << tungstenBottomZ/mm << " - " << tungstenTopZ/mm << " mm  (50 um)       ║" << G4endl;
    G4cout << "║       Retrodiffusion electronique                                        ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "╟──────────────────────────────────────────────────────────────────────────╢" << G4endl;
    G4cout << "║  Rayon externe : " << fContainerRadius/mm << " mm (2.5 cm)                                    ║" << G4endl;
    G4cout << "║  Nombre d'anneaux : " << kNbWaterRings << "                                                   ║" << G4endl;
    G4cout << "║  Largeur anneaux : " << fRingWidth/mm << " mm                                               ║" << G4endl;
    G4cout << "║                                                                          ║" << G4endl;
    G4cout << "╚══════════════════════════════════════════════════════════════════════════╝\n" << G4endl;

    return physWorld;
}
