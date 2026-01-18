#ifndef DetectorConstruction_h
#define DetectorConstruction_h 1

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include <vector>

class G4VPhysicalVolume;
class G4LogicalVolume;
class G4Material;

/// @brief Construction du détecteur - CONFIGURATION OPTIMISÉE
///
/// Géométrie (dans le sens des z croissants) :
/// - Source Eu-152 à z = 75 mm (25 mm avant surface eau)
/// - PreContainer Plane (AIR, 1 mm, r=25mm) : z = 99-100 mm (AVANT surface eau)
/// - Première tranche d'eau (2 mm) : z = 100-102 mm
/// - Deuxième tranche d'eau (1 mm) : z = 102-103 mm (anneaux concentriques)
/// - PostContainer = Polystyrène (1 mm) : z = 103-104 mm (fond boîte de Petri)
/// - Feuille de tungstène (50 µm) : z = 104-104.05 mm (rétrodiffusion)

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();
    
    virtual G4VPhysicalVolume* Construct();

    // ═══════════════════════════════════════════════════════════════
    // ACCESSEURS POUR LES VOLUMES SENSIBLES (ANNEAUX D'EAU)
    // ═══════════════════════════════════════════════════════════════

    /// Nombre d'anneaux d'eau (incluant le disque central)
    static const G4int kNbWaterRings = 5;
    
    /// Retourne le nom du volume logique pour l'anneau i
    static G4String GetWaterRingName(G4int ringIndex);

    /// Retourne le rayon interne de l'anneau i (mm)
    static G4double GetRingInnerRadius(G4int ringIndex);

    /// Retourne le rayon externe de l'anneau i (mm)
    static G4double GetRingOuterRadius(G4int ringIndex);
    
    /// Retourne la masse de l'anneau i (g)
    G4double GetRingMass(G4int ringIndex) const { return fRingMasses[ringIndex]; }

private:

    // ═══════════════════════════════════════════════════════════════
    // MATÉRIAUX
    // ═══════════════════════════════════════════════════════════════
    G4Material* fTungsten;
    G4Material* fWater;
    G4Material* fPolystyrene;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES GÉOMÉTRIQUES GÉNÉRAUX
    // ═══════════════════════════════════════════════════════════════
    G4double fContainerRadius;          // Rayon : 25 mm (2.5 cm)
    G4double fPolystyreneThickness;     // Épaisseur paroi PS : 1 mm

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DES TRANCHES D'EAU
    // ═══════════════════════════════════════════════════════════════
    G4double fWaterThickness1;          // Première tranche : 2 mm (volume uniforme)
    G4double fWaterThickness2;          // Deuxième tranche : 1 mm (anneaux concentriques)
    G4double fRingWidth;                // Largeur des anneaux : 5 mm
    
    // Masses des anneaux (calculées dans Construct())
    std::vector<G4double> fRingMasses;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DU PRECONTAINER PLANE
    // AIR, AVANT la surface de l'eau
    // ═══════════════════════════════════════════════════════════════
    G4double fPreContainerPlaneThickness;  // Épaisseur : 1 mm
    G4double fPreContainerPlaneRadius;     // Rayon : 25 mm (2.5 cm)

    // ═══════════════════════════════════════════════════════════════
    // VOLUMES LOGIQUES DES ANNEAUX (pour identification dans SteppingAction)
    // ═══════════════════════════════════════════════════════════════
    std::vector<G4LogicalVolume*> fWaterRingLogicals;

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE LA FEUILLE DE TUNGSTÈNE
    // ═══════════════════════════════════════════════════════════════
    G4double fTungstenFoilThickness;    // Épaisseur feuille W : 50 µm
    G4double fTungstenFoilRadius;       // Rayon feuille W : 25 mm

    // ═══════════════════════════════════════════════════════════════
    // PARAMÈTRES DE POSITIONNEMENT
    // ═══════════════════════════════════════════════════════════════
    G4double fSourceToWaterDistance;    // Distance source-eau : 25 mm

};

#endif
