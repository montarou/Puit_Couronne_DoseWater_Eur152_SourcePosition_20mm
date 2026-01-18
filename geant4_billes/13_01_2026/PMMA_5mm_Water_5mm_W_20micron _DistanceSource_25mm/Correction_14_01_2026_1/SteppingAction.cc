#include "SteppingAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "Logger.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include <cmath>
#include <sstream>

SteppingAction::SteppingAction(EventAction* eventAction, RunAction* runAction)
: G4UserSteppingAction(),
  fEventAction(eventAction),
  fRunAction(runAction),
  fVerbose(true),           // ACTIVÉ pour vérification
  fVerboseMaxEvents(10)     // Afficher les 10 premiers événements
{
    // Initialiser les noms des volumes d'eau pour identification rapide
    for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
        fWaterRingNames.insert(DetectorConstruction::GetWaterRingName(i) + "Log");
    }
    
    G4cout << "\n╔═══════════════════════════════════════════════════════════════╗" << G4endl;
    G4cout << "║  SteppingAction: Mode VERBOSE activé pour " << fVerboseMaxEvents << " événements     ║" << G4endl;
    G4cout << "║  Suivi par raie gamma Eu-152 ACTIVÉ                            ║" << G4endl;
    G4cout << "║  Comptage aux plans PreContainer et PostContainer ACTIVÉ       ║" << G4endl;
    G4cout << "║  *** CONFIGURATION SANS FILTRE ***                             ║" << G4endl;
    G4cout << "║  *** REMPLISSAGE HISTOGRAMMES ROOT ACTIVÉ ***                  ║" << G4endl;
    G4cout << "║  Diagnostics -> output.log                                     ║" << G4endl;
    G4cout << "╚═══════════════════════════════════════════════════════════════╝\n" << G4endl;
}

SteppingAction::~SteppingAction()
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    // ═══════════════════════════════════════════════════════════════
    // RÉCUPÉRATION DES INFORMATIONS DE BASE
    // ═══════════════════════════════════════════════════════════════

    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();

    // Vérifier que les volumes existent
    if (!preStepPoint->GetPhysicalVolume()) {
        return;
    }

    G4String preVolumeName = preStepPoint->GetPhysicalVolume()->GetName();
    
    G4String postVolumeName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postVolumeName = postStepPoint->GetPhysicalVolume()->GetName();
    }

    // Informations sur la trace
    G4Track* track = step->GetTrack();
    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();
    G4String particleName = track->GetDefinition()->GetParticleName();
    G4double kineticEnergy = preStepPoint->GetKineticEnergy();

    // ID de l'événement pour le debug
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // Direction du momentum
    G4ThreeVector momentum = step->GetTrack()->GetMomentumDirection();
    G4double pz = momentum.z();

    // Position
    G4ThreeVector pos = preStepPoint->GetPosition();
    G4double radius = std::sqrt(pos.x()*pos.x() + pos.y()*pos.y());

    // Noms des volumes logiques
    G4String logicalVolumeName = preStepPoint->GetTouchableHandle()
                                             ->GetVolume()
                                             ->GetLogicalVolume()
                                             ->GetName();
    
    G4String postLogVolName = "OutOfWorld";
    if (postStepPoint->GetPhysicalVolume()) {
        postLogVolName = postStepPoint->GetPhysicalVolume()->GetLogicalVolume()->GetName();
    }

    // ═══════════════════════════════════════════════════════════════
    // ENREGISTREMENT DU SPECTRE DES GAMMAS PRIMAIRES ÉMIS
    // (Premier step de chaque gamma primaire)
    // ═══════════════════════════════════════════════════════════════
    
    if (parentID == 0 && particleName == "gamma" && track->GetCurrentStepNumber() == 1) {
        // C'est le premier step d'un gamma primaire - enregistrer son énergie initiale
        G4double initialEnergy = track->GetVertexKineticEnergy();
        fRunAction->FillGammaEmittedSpectrum(initialEnergy / keV);
        
        // ═══════════════════════════════════════════════════════════════
        // CORRECTION BUG : Enregistrer le gamma primaire avec son VRAI trackID
        // (assigné par Geant4, pas une supposition basée sur l'ordre des vertex)
        // ═══════════════════════════════════════════════════════════════
        G4ThreeVector momDir = track->GetVertexMomentumDirection();
        G4double theta = std::acos(momDir.z());
        G4double phi = std::atan2(momDir.y(), momDir.x());
        fEventAction->RegisterPrimaryGamma(trackID, initialEnergy, theta, phi);
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DE L'ABSORPTION DES GAMMAS PRIMAIRES
    // ═══════════════════════════════════════════════════════════════
    
    // Si un gamma primaire est tué (absorbé), enregistrer où et par quel processus
    if (parentID == 0 && particleName == "gamma") {
        G4TrackStatus status = track->GetTrackStatus();
        if (status == fStopAndKill || status == fKillTrackAndSecondaries) {
            // Récupérer le processus qui a causé l'absorption
            G4String processName = "Unknown";
            const G4VProcess* process = postStepPoint->GetProcessDefinedStep();
            if (process) {
                processName = process->GetProcessName();
            }
            
            fEventAction->RecordGammaAbsorbed(trackID, logicalVolumeName, processName);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                std::stringstream ss;
                ss << "GAMMA_ABSORBED | Event " << eventID
                   << " | trackID=" << trackID
                   << " | in " << logicalVolumeName
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | process=" << processName;
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // DÉTECTION DANS LES ANNEAUX D'EAU AVEC SUIVI PAR RAIE
    // ═══════════════════════════════════════════════════════════════

    // Vérifier si on est dans un anneau d'eau
    if (fWaterRingNames.find(logicalVolumeName) != fWaterRingNames.end()) {
        G4double edep = step->GetTotalEnergyDeposit();
        if (edep > 0.) {
            // Extraire l'index de l'anneau depuis le nom du volume
            G4int ringIndex = -1;
            
            // Chercher quel anneau
            for (G4int i = 0; i < DetectorConstruction::kNbWaterRings; ++i) {
                G4String ringLogName = DetectorConstruction::GetWaterRingName(i) + "Log";
                if (logicalVolumeName == ringLogName) {
                    ringIndex = i;
                    break;
                }
            }
            
            if (ringIndex >= 0) {
                fEventAction->AddRingEnergy(ringIndex, edep);
                
                // ═══════════════════════════════════════════════════════════════
                // REMPLISSAGE DES HISTOGRAMMES ROOT
                // ═══════════════════════════════════════════════════════════════
                fRunAction->FillEdepWater(edep / keV);
                fRunAction->FillEdepRing(ringIndex, edep / keV);
                fRunAction->FillEdepXY(pos.x() / mm, pos.y() / mm, edep / keV);
                fRunAction->FillEdepRZ(radius / mm, pos.z() / mm, edep / keV);
                
                // Spectre des électrons secondaires
                if (particleName == "e-" && parentID != 0) {
                    fRunAction->FillElectronSpectrum(kineticEnergy / keV);
                }
                
                // Remplir le ntuple de steps (optionnel, peut être désactivé pour performance)
                const G4VProcess* proc = postStepPoint->GetProcessDefinedStep();
                G4String procName = proc ? proc->GetProcessName() : "Unknown";
                fRunAction->FillStepNtuple(eventID, pos.x()/mm, pos.y()/mm, pos.z()/mm,
                                           edep/keV, ringIndex, particleName, procName);
                
                // Suivi par raie gamma : identifier la raie du gamma parent
                // Pour les électrons secondaires, trouver le gamma primaire ancêtre
                G4int gammaLineIndex = -1;
                
                if (parentID == 0 && particleName == "gamma") {
                    // C'est un gamma primaire
                    gammaLineIndex = fEventAction->GetGammaLineForTrack(trackID);
                } else {
                    // Particule secondaire - essayer de trouver le gamma primaire parent
                    if (fEventAction->IsPrimaryTrack(parentID)) {
                        gammaLineIndex = fEventAction->GetGammaLineForTrack(parentID);
                    }
                }
                
                // Enregistrer le dépôt par raie si identifié
                if (gammaLineIndex >= 0) {
                    fEventAction->AddRingEnergyByLine(ringIndex, gammaLineIndex, edep);
                }
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    std::stringstream ss;
                    ss << "WATER_DEPOSIT | Event " << eventID
                       << " | Ring " << ringIndex
                       << " | " << particleName
                       << " | E_kin=" << kineticEnergy/keV << " keV"
                       << " | edep=" << edep/keV << " keV"
                       << " | r=" << radius/mm << " mm"
                       << " | z=" << pos.z()/mm << " mm";
                    if (gammaLineIndex >= 0) {
                        ss << " | Line=" << EventAction::GetGammaLineName(gammaLineIndex);
                    }
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // COMPTEURS DE VÉRIFICATION (toujours actifs)
    // ═══════════════════════════════════════════════════════════════
    
    // Entrée dans le container (gamma primaire)
    if ((postLogVolName == "ContainerWallLog" || postLogVolName == "ContainerTopLog") 
        && logicalVolumeName != "ContainerWallLog" && logicalVolumeName != "ContainerTopLog") {
        if (parentID == 0 && particleName == "gamma") {
            fRunAction->IncrementContainerEntry();
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector posPost = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "CONTAINER_ENTRY | Event " << eventID
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << posPost.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // ENTRÉE DANS L'EAU - CORRIGÉ : seulement primaires, pas de double-comptage
    // ═══════════════════════════════════════════════════════════════
    
    if (fWaterRingNames.find(postLogVolName) != fWaterRingNames.end() 
        && fWaterRingNames.find(logicalVolumeName) == fWaterRingNames.end()) {
        
        // CORRECTION : Ne compter que les gammas PRIMAIRES pour éviter le sur-comptage
        if (particleName == "gamma" && parentID == 0) {
            // Vérifier si ce gamma n'a pas déjà été compté (via EventAction)
            if (!fEventAction->HasEnteredWater(trackID)) {
                fRunAction->IncrementWaterEntry();
                fEventAction->RecordWaterEntry(trackID, kineticEnergy);
                
                // Remplir l'histogramme du spectre entrant dans l'eau
                fRunAction->FillGammaEnteringWater(kineticEnergy / keV);
            }
        }
        
        // Compter les électrons entrant dans l'eau (tous, pas seulement primaires)
        if (particleName == "e-") {
            fRunAction->IncrementElectronsInWater();
        }
        
        if (fVerbose && eventID < fVerboseMaxEvents) {
            G4ThreeVector posPost = postStepPoint->GetPosition();
            G4double radiusPost = std::sqrt(posPost.x()*posPost.x() + posPost.y()*posPost.y());
            G4int lineIdx = -1;
            if (parentID == 0 && particleName == "gamma") {
                lineIdx = fEventAction->GetGammaLineForTrack(trackID);
            }
            std::stringstream ss;
            ss << "WATER_ENTRY | Event " << eventID
               << " | " << particleName
               << " | trackID=" << trackID
               << " | parentID=" << parentID
               << " | E=" << kineticEnergy/keV << " keV";
            if (lineIdx >= 0) {
                ss << " | [" << EventAction::GetGammaLineName(lineIdx) << "]";
            }
            ss << " | r=" << radiusPost/mm << " mm"
               << " | z=" << posPost.z()/mm << " mm"
               << " | " << postLogVolName;
            Logger::GetInstance()->LogLine(ss.str());
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PLAN PRE-CONTAINER (dans eau, entrée)
    // Comptage des photons et électrons vers l'eau (+z)
    // ═══════════════════════════════════════════════════════════════
    
    if (postLogVolName == "PreContainerPlaneLog" && logicalVolumeName != "PreContainerPlaneLog") {
        // Photons vers l'eau (+z)
        if (particleName == "gamma" && pz > 0) {
            fRunAction->IncrementPreContainerPlane();
            fEventAction->AddPreContainerPhoton(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector posPost = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "PRE_CONTAINER_PLANE | Event " << eventID
                   << " | PHOTON +z"
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << posPost.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
        // Électrons vers l'eau (+z)
        if (particleName == "e-" && pz > 0) {
            fEventAction->AddPreContainerElectron(kineticEnergy);
            
            if (fVerbose && eventID < fVerboseMaxEvents) {
                G4ThreeVector posPost = postStepPoint->GetPosition();
                std::stringstream ss;
                ss << "PRE_CONTAINER_PLANE | Event " << eventID
                   << " | ELECTRON +z"
                   << " | trackID=" << trackID
                   << " | E=" << kineticEnergy/keV << " keV"
                   << " | z=" << posPost.z()/mm << " mm";
                Logger::GetInstance()->LogLine(ss.str());
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PLAN POST-CONTAINER (après eau, matériau eau)
    // Comptage des particules dans les deux sens
    // ═══════════════════════════════════════════════════════════════
    
    if (postLogVolName == "PostContainerPlaneLog" && logicalVolumeName != "PostContainerPlaneLog") {
        
        // --- PHOTONS ---
        if (particleName == "gamma") {
            if (pz > 0) {
                // Photons TRANSMIS vers la sortie (+z)
                fRunAction->IncrementPostContainerPlane();
                fEventAction->AddPostContainerPhotonFwd(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector posPost = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | PHOTON +z (transmis)"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << posPost.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            } else {
                // Photons RÉTRODIFFUSÉS depuis la sortie (-z)
                fEventAction->AddPostContainerPhotonBack(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector posPost = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | PHOTON -z (backscatter)"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << posPost.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
        
        // --- ÉLECTRONS ---
        if (particleName == "e-") {
            if (pz > 0) {
                // Électrons vers la sortie (+z)
                fEventAction->AddPostContainerElectronFwd(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector posPost = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | ELECTRON +z"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << posPost.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            } else {
                // Électrons depuis la sortie (-z)
                fEventAction->AddPostContainerElectronBack(kineticEnergy);
                
                if (fVerbose && eventID < fVerboseMaxEvents) {
                    G4ThreeVector posPost = postStepPoint->GetPosition();
                    std::stringstream ss;
                    ss << "POST_CONTAINER_PLANE | Event " << eventID
                       << " | ELECTRON -z (backscatter)"
                       << " | trackID=" << trackID
                       << " | E=" << kineticEnergy/keV << " keV"
                       << " | z=" << posPost.z()/mm << " mm";
                    Logger::GetInstance()->LogLine(ss.str());
                }
            }
        }
    }
}
