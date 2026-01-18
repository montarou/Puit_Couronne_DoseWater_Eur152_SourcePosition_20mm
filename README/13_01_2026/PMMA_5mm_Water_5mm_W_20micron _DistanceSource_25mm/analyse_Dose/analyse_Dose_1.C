// =============================================================================
// SCRIPT D'ANALYSE - PUITS COURONNE (version SANS FILTRE - Eu-152)
// =============================================================================
// 
// Usage: root -l analyse_Dose.C
//    ou: root -l 'analyse_Dose.C("autre_fichier.root")'
//
// Compatible avec la nouvelle structure de fichier ROOT
// VERSION MODIFIÉE: Histogrammes de dose avec bins colorés
// =============================================================================

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TPaveStats.h>
#include <iostream>
#include <iomanip>

// Couleurs pour les anneaux
const int ringColors[] = {kRed, kOrange+1, kGreen+2, kBlue, kViolet};

// Noms des raies gamma Eu-152 (13 raies)
const char* gammaLineNames[] = {
    "40 keV (X)", "40 keV (X)", "122 keV", "245 keV", "344 keV", 
    "411 keV", "444 keV", "779 keV", "867 keV", "964 keV", 
    "1086 keV", "1112 keV", "1408 keV"
};
const int nGammaLines = 13;

void analyse_Dose(const char* filename = "output.root")
{
    // Configuration du style
    gStyle->SetOptStat(10);        // 10 = seulement Entries
    gStyle->SetHistLineWidth(2);   // Lignes 
    gStyle->SetTitleFont(62, "");  // Titre en gras
    gStyle->SetTitleFontSize(0.06);
    
    // ==========================================================================
    // OUVERTURE DU FICHIER
    // ==========================================================================
    
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "ERREUR: Impossible d'ouvrir " << filename << std::endl;
        std::cerr << "Verifiez que le fichier existe avec: ls -la *.root" << std::endl;
        return;
    }
    
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "     ANALYSE DU FICHIER: " << filename << "\n";
    std::cout << "     VERSION SANS FILTRE - Source Eu-152\n";
    std::cout << "================================================================\n\n";
    
    // Lister le contenu du fichier
    std::cout << "Contenu du fichier:\n";
    file->ls();
    std::cout << "\n";
    
    // ==========================================================================
    // 1. SPECTRE DES GAMMAS ÉMIS
    // ==========================================================================
    
    TH1D* hGammaEmitted = (TH1D*)file->Get("hGammaEmitted");
    if (hGammaEmitted && hGammaEmitted->GetEntries() > 0) {
        TCanvas* c_spectrum = new TCanvas("c_spectrum", "Spectre gamma Eu-152", 1000, 600);
        gPad->SetLogy();
        gPad->SetGridx();
        gPad->SetGridy();
        
        hGammaEmitted->SetLineColor(kRed+1);
        hGammaEmitted->SetLineWidth(2);
        hGammaEmitted->GetXaxis()->SetTitle("Energie [keV]");
        hGammaEmitted->GetYaxis()->SetTitle("Counts");
        hGammaEmitted->SetTitle("Spectre gamma Eu-152 emis");
        hGammaEmitted->Draw();
        
        // Annoter les raies principales
        TLatex* latex = new TLatex();
        latex->SetTextSize(0.025);
        latex->SetTextColor(kBlue+2);
        
        double energies[] = {40, 122, 245, 344, 779, 964, 1112, 1408};
        int nPeaks = sizeof(energies)/sizeof(energies[0]);
        for (int i = 0; i < nPeaks; i++) {
            int bin = hGammaEmitted->FindBin(energies[i]);
            double y = hGammaEmitted->GetBinContent(bin);
            if (y > 0) {
                latex->DrawLatex(energies[i], y*1.5, Form("%.0f", energies[i]));
            }
        }
        
        c_spectrum->SaveAs("spectre_gamma_emis.png");
        c_spectrum->SaveAs("spectre_gamma_emis.pdf");
        std::cout << "=> Sauvegarde: spectre_gamma_emis.png/pdf\n\n";
        
        std::cout << "Spectre gamma emis: " << hGammaEmitted->GetEntries() << " entries\n\n";
    } else {
        std::cout << "ATTENTION: Histogramme hGammaEmitted vide ou non trouve!\n\n";
    }
    
    // ==========================================================================
    // 2. SPECTRE DES GAMMAS ENTRANT DANS L'EAU
    // ==========================================================================
    
    TH1D* hGammaWater = (TH1D*)file->Get("hGammaEnteringWater");
    if (hGammaWater && hGammaWater->GetEntries() > 0) {
        TCanvas* c_water = new TCanvas("c_water", "Spectre gamma entrant eau", 1000, 600);
        gPad->SetLogy();
        gPad->SetGridx();
        gPad->SetGridy();
        
        hGammaWater->SetLineColor(kBlue+1);
        hGammaWater->SetLineWidth(2);
        hGammaWater->GetXaxis()->SetTitle("Energie [keV]");
        hGammaWater->GetYaxis()->SetTitle("Counts");
        hGammaWater->SetTitle("Spectre gamma entrant dans l'eau");
        hGammaWater->Draw();
        
        c_water->SaveAs("spectre_gamma_eau.png");
        std::cout << "=> Sauvegarde: spectre_gamma_eau.png\n";
        std::cout << "Spectre gamma eau: " << hGammaWater->GetEntries() << " entries\n\n";
    }
    
    // ==========================================================================
    // 3. HISTOGRAMMES DE DOSE PAR ANNEAU (nGy par événement)
    // VERSION MODIFIÉE: Représentation par BINS COLORÉS au lieu de symboles
    // ==========================================================================
    
    TCanvas* c_dose = new TCanvas("c_dose", "Dose par anneau", 1200, 800);
    c_dose->Divide(3, 2);
    
    TH1D* h_dose_ring[5];
    TH1D* h_dose_total = nullptr;
    
    // Parametres du pave de statistiques - coin haut droit du cadre
    double statX1 = 0.65, statX2 = 0.98;
    double statY1 = 0.85, statY2 = 0.98;
    double statTextSize = 0.055;
    
    bool hasHistos = false;
    
    for (int i = 0; i < 5; i++) {
        TString hname = Form("h_dose_ring%d", i);
        h_dose_ring[i] = (TH1D*)file->Get(hname);
        
        if (h_dose_ring[i] && h_dose_ring[i]->GetEntries() > 0) {
            hasHistos = true;
            c_dose->cd(i + 1);
            gPad->SetLogy();
            
            // ═══════════════════════════════════════════════════════════════
            // MODIFICATION: Représentation par bins colorés
            // ═══════════════════════════════════════════════════════════════
            h_dose_ring[i]->SetLineColor(ringColors[i]);
            h_dose_ring[i]->SetFillColor(ringColors[i]);
            h_dose_ring[i]->SetFillStyle(1001);  // Remplissage plein (au lieu de 3004 hachuré)
            h_dose_ring[i]->SetLineWidth(1);     // Ligne fine pour le contour
            
            h_dose_ring[i]->GetXaxis()->SetTitle("Dose [nGy]");
            h_dose_ring[i]->GetYaxis()->SetTitle("Counts");
            h_dose_ring[i]->GetXaxis()->SetTitleSize(0.05);
            h_dose_ring[i]->GetYaxis()->SetTitleSize(0.05);
            h_dose_ring[i]->GetXaxis()->SetLabelSize(0.05);
            h_dose_ring[i]->GetYaxis()->SetLabelSize(0.05);
            h_dose_ring[i]->SetTitle(Form("Anneau %d (r=%d-%d mm)", i, i*5, (i+1)*5));
            
            // MODIFICATION: Option "HIST" pour afficher des bins au lieu de "P" pour les points
            h_dose_ring[i]->Draw("HIST");
            
            gPad->Update();
            TPaveStats* stats = (TPaveStats*)h_dose_ring[i]->FindObject("stats");
            if (stats) {
                // Coin haut droit
                stats->SetX1NDC(statX1);
                stats->SetX2NDC(statX2);
                stats->SetY1NDC(statY1);
                stats->SetY2NDC(statY2);
                stats->SetTextSize(statTextSize);
                stats->SetTextFont(62);
            }
            gPad->Modified();
            gPad->Update();
            
            // Afficher les statistiques
            std::cout << "Anneau " << i << ": Entries=" << h_dose_ring[i]->GetEntries()
                      << ", Mean=" << h_dose_ring[i]->GetMean() << " nGy\n";
        }
    }

    // Dose totale
    h_dose_total = (TH1D*)file->Get("h_dose_total");
    if (h_dose_total && h_dose_total->GetEntries() > 0) {
        c_dose->cd(6);
        gPad->SetLogy();
        
        // ═══════════════════════════════════════════════════════════════
        // MODIFICATION: Représentation par bins colorés pour le total
        // ═══════════════════════════════════════════════════════════════
        h_dose_total->SetLineColor(kBlack);
        h_dose_total->SetFillColor(kGray);      // Gris pour le total
        h_dose_total->SetFillStyle(1001);       // Remplissage plein
        h_dose_total->SetLineWidth(1);
        
        h_dose_total->GetXaxis()->SetTitle("Dose totale [nGy]");
        h_dose_total->GetYaxis()->SetTitle("Counts");
        h_dose_total->GetXaxis()->SetTitleSize(0.05);
        h_dose_total->GetYaxis()->SetTitleSize(0.05);
        h_dose_total->GetXaxis()->SetLabelSize(0.05);
        h_dose_total->GetYaxis()->SetLabelSize(0.05);
        h_dose_total->SetTitle("TOTAL (5 anneaux)");
        
        // MODIFICATION: Option "HIST" pour afficher des bins
        h_dose_total->Draw("HIST");
        
        gPad->Update();
        TPaveStats* stats = (TPaveStats*)h_dose_total->FindObject("stats");
        if (stats) {
            stats->SetX1NDC(statX1);
            stats->SetX2NDC(statX2);
            stats->SetY1NDC(statY1);
            stats->SetY2NDC(statY2);
            stats->SetTextSize(statTextSize);
            stats->SetTextFont(62);
        }
        gPad->Modified();
        gPad->Update();
        
        std::cout << "TOTAL: Entries=" << h_dose_total->GetEntries()
                  << ", Mean=" << h_dose_total->GetMean() << " nGy\n\n";
    }
    
    if (hasHistos) {
        c_dose->Update();
        c_dose->SaveAs("dose_par_anneau.png");
        c_dose->SaveAs("dose_par_anneau.pdf");
        std::cout << "=> Sauvegarde: dose_par_anneau.png/pdf\n\n";
    }
    
    // ==========================================================================
    // 4. HISTOGRAMMES EDEP PAR STEP (keV par step)
    // VERSION MODIFIÉE: Représentation par BINS COLORÉS
    // ==========================================================================
    
    TCanvas* c_edep = new TCanvas("c_edep", "Edep par step", 1200, 800);
    c_edep->Divide(3, 2);
    
    TH1D* h_edep_ring[5];
    TH1D* h_edep_total = nullptr;
    
    bool hasEdep = false;
    
    for (int i = 0; i < 5; i++) {
        TString hname = Form("hEdepRing%d", i);
        h_edep_ring[i] = (TH1D*)file->Get(hname);
        
        if (h_edep_ring[i] && h_edep_ring[i]->GetEntries() > 0) {
            hasEdep = true;
            c_edep->cd(i + 1);
            gPad->SetLogy();
            
            // ═══════════════════════════════════════════════════════════════
            // MODIFICATION: Représentation par bins colorés
            // ═══════════════════════════════════════════════════════════════
            h_edep_ring[i]->SetLineColor(ringColors[i]);
            h_edep_ring[i]->SetFillColor(ringColors[i]);
            h_edep_ring[i]->SetFillStyle(1001);  // Remplissage plein
            h_edep_ring[i]->SetLineWidth(1);
            
            h_edep_ring[i]->GetXaxis()->SetTitle("Edep [keV]");
            h_edep_ring[i]->GetYaxis()->SetTitle("Counts");
            h_edep_ring[i]->GetXaxis()->SetTitleSize(0.05);
            h_edep_ring[i]->GetYaxis()->SetTitleSize(0.05);
            h_edep_ring[i]->GetXaxis()->SetLabelSize(0.05);
            h_edep_ring[i]->GetYaxis()->SetLabelSize(0.05);
            h_edep_ring[i]->SetTitle(Form("Edep/step Anneau %d", i));
            
            // MODIFICATION: Option "HIST" pour afficher des bins
            h_edep_ring[i]->Draw("HIST");
            
            gPad->Update();
            TPaveStats* stats = (TPaveStats*)h_edep_ring[i]->FindObject("stats");
            if (stats) {
                stats->SetX1NDC(statX1);
                stats->SetX2NDC(statX2);
                stats->SetY1NDC(statY1);
                stats->SetY2NDC(statY2);
                stats->SetTextSize(statTextSize);
                stats->SetTextFont(62);
            }
            gPad->Modified();
            gPad->Update();
            
            std::cout << "Edep Anneau " << i << ": " << h_edep_ring[i]->GetEntries() << " entries\n";
        }
    }
    
    // Edep total eau
    h_edep_total = (TH1D*)file->Get("hEdepWater");
    if (h_edep_total && h_edep_total->GetEntries() > 0) {
        c_edep->cd(6);
        gPad->SetLogy();
        
        // ═══════════════════════════════════════════════════════════════
        // MODIFICATION: Représentation par bins colorés
        // ═══════════════════════════════════════════════════════════════
        h_edep_total->SetLineColor(kBlack);
        h_edep_total->SetFillColor(kGray);
        h_edep_total->SetFillStyle(1001);
        h_edep_total->SetLineWidth(1);
        
        h_edep_total->GetXaxis()->SetTitle("Edep [keV]");
        h_edep_total->GetYaxis()->SetTitle("Counts");
        h_edep_total->GetXaxis()->SetTitleSize(0.05);
        h_edep_total->GetYaxis()->SetTitleSize(0.05);
        h_edep_total->GetXaxis()->SetLabelSize(0.05);
        h_edep_total->GetYaxis()->SetLabelSize(0.05);
        h_edep_total->SetTitle("Edep total eau");
        
        // MODIFICATION: Option "HIST" pour afficher des bins
        h_edep_total->Draw("HIST");
        
        gPad->Update();
        TPaveStats* stats = (TPaveStats*)h_edep_total->FindObject("stats");
        if (stats) {
            stats->SetX1NDC(statX1);
            stats->SetX2NDC(statX2);
            stats->SetY1NDC(statY1);
            stats->SetY2NDC(statY2);
            stats->SetTextSize(statTextSize);
            stats->SetTextFont(62);
        }
        gPad->Modified();
        gPad->Update();
        
        std::cout << "Edep Total: " << h_edep_total->GetEntries() << " entries\n\n";
    }
    
    if (hasEdep) {
        c_edep->Update();
        c_edep->SaveAs("edep_par_step.png");
        std::cout << "=> Sauvegarde: edep_par_step.png\n\n";
    }
    
    // ==========================================================================
    // 5. TAUX D'ABSORPTION PAR RAIE GAMMA
    // ==========================================================================
    
    TTree* gamma_lines = (TTree*)file->Get("gamma_lines");
    if (gamma_lines) {
        std::cout << "\n========== STATISTIQUES PAR RAIE GAMMA ==========\n";
        
        // Variables du ntuple
        Int_t lineIndex;
        Double_t energy_keV;
        Int_t emitted;
        Int_t enteredWater;
        Int_t absorbedWater;
        Double_t waterAbsRate;
        
        gamma_lines->SetBranchAddress("lineIndex", &lineIndex);
        gamma_lines->SetBranchAddress("energy_keV", &energy_keV);
        gamma_lines->SetBranchAddress("emitted", &emitted);
        gamma_lines->SetBranchAddress("enteredWater", &enteredWater);
        gamma_lines->SetBranchAddress("absorbedWater", &absorbedWater);
        gamma_lines->SetBranchAddress("waterAbsRate", &waterAbsRate);
        
        Long64_t nEntries = gamma_lines->GetEntries();
        
        // Creer l'histogramme du taux d'absorption
        TCanvas* c_abs = new TCanvas("c_abs", "Taux absorption", 1000, 600);
        gPad->SetLogy();
        gPad->SetBottomMargin(0.2);
        gPad->SetGridy();
        
        TH1D* h_abs_water = new TH1D("h_abs_water", 
            "Taux d'absorption dans l'eau (5 mm) par raie gamma Eu-152;Raie gamma;Taux d'absorption (%)",
            nEntries, 0, nEntries);
        
        std::cout << "┌────────┬────────────┬───────────┬─────────────┬────────────┬──────────────┐\n";
        std::cout << "│  Raie  │ Energie    │   Emis    │ Entre eau   │ Abs. eau   │ Taux abs (%) │\n";
        std::cout << "├────────┼────────────┼───────────┼─────────────┼────────────┼──────────────┤\n";
        
        double maxRate = 0;
        for (Long64_t i = 0; i < nEntries && i < nGammaLines; i++) {
            gamma_lines->GetEntry(i);
            h_abs_water->SetBinContent(lineIndex + 1, waterAbsRate);
            h_abs_water->GetXaxis()->SetBinLabel(lineIndex + 1, gammaLineNames[lineIndex]);
            
            if (waterAbsRate > maxRate) maxRate = waterAbsRate;
            
            std::cout << "│   " << std::setw(2) << lineIndex << "   │ "
                      << std::setw(8) << std::fixed << std::setprecision(1) << energy_keV << " keV │"
                      << std::setw(10) << emitted << " │"
                      << std::setw(12) << enteredWater << " │"
                      << std::setw(11) << absorbedWater << " │"
                      << std::setw(12) << std::setprecision(2) << waterAbsRate << " │\n";
        }
        std::cout << "└────────┴────────────┴───────────┴─────────────┴────────────┴──────────────┘\n\n";
        
        h_abs_water->SetFillColor(kAzure+1);
        h_abs_water->SetLineColor(kAzure+3);
        h_abs_water->SetLineWidth(3);
        h_abs_water->SetBarWidth(0.9);
        h_abs_water->SetBarOffset(0.05);
        h_abs_water->SetMinimum(0.001);
        
        if (maxRate < 5) {
            h_abs_water->SetMaximum(10);
        } else {
            h_abs_water->SetMaximum(maxRate * 2);
        }
        
        h_abs_water->GetXaxis()->SetLabelSize(0.05);
        h_abs_water->GetXaxis()->SetLabelOffset(0.02);
        h_abs_water->GetXaxis()->SetTitleOffset(2.2);
        h_abs_water->GetYaxis()->SetLabelSize(0.05);
        h_abs_water->GetYaxis()->SetTitleSize(0.055);
        h_abs_water->GetYaxis()->SetTitleOffset(0.85);
        
        h_abs_water->Draw("bar");
        
        TLatex* latex = new TLatex();
        latex->SetNDC();
        latex->SetTextSize(0.04);
        latex->DrawLatex(0.15, 0.85, "Source Eu-152 (42 kBq) - Eau 5 mm - SANS FILTRE");
        latex->DrawLatex(0.15, 0.80, Form("Absorption max: %.1f%% (raies X 40 keV)", maxRate));
        
        c_abs->Update();
        c_abs->SaveAs("taux_absorption_eau.png");
        c_abs->SaveAs("taux_absorption_eau.pdf");
        std::cout << "=> Sauvegarde: taux_absorption_eau.png/pdf\n\n";
    } else {
        std::cout << "TTree gamma_lines non trouve dans le fichier\n";
        std::cout << "Le fichier ROOT a ete genere avec une ancienne version.\n\n";
    }
    
    // ==========================================================================
    // 6. CARTES 2D DE DEPOT D'ENERGIE
    // ==========================================================================
    
    gStyle->SetOptStat(10);
    
    TH2D* hEdepXY = (TH2D*)file->Get("hEdepXY");
    TH2D* hEdepRZ = (TH2D*)file->Get("hEdepRZ");
    
    double stat2DX1 = 0.55, stat2DX2 = 0.82;
    double stat2DY1 = 0.85, stat2DY2 = 0.98;
    double stat2DTextSize = 0.06;
    
    if ((hEdepXY && hEdepXY->GetEntries() > 0) || (hEdepRZ && hEdepRZ->GetEntries() > 0)) {
        TCanvas* c_2d = new TCanvas("c_2d", "Cartes 2D", 1200, 500);
        c_2d->Divide(2, 1);
        
        if (hEdepXY && hEdepXY->GetEntries() > 0) {
            c_2d->cd(1);
            gPad->SetLogz();
            gPad->SetRightMargin(0.15);
            hEdepXY->SetTitle("Depot d'energie XY");
            hEdepXY->Draw("COLZ");
            
            gPad->Update();
            TPaveStats* stats = (TPaveStats*)hEdepXY->FindObject("stats");
            if (stats) {
                stats->SetX1NDC(stat2DX1);
                stats->SetX2NDC(stat2DX2);
                stats->SetY1NDC(stat2DY1);
                stats->SetY2NDC(stat2DY2);
                stats->SetTextSize(stat2DTextSize);
                stats->SetTextFont(62);
            }
            gPad->Modified();
            gPad->Update();
            
            std::cout << "hEdepXY: " << hEdepXY->GetEntries() << " entries\n";
        }
        
        if (hEdepRZ && hEdepRZ->GetEntries() > 0) {
            c_2d->cd(2);
            gPad->SetLogz();
            gPad->SetRightMargin(0.15);
            hEdepRZ->SetTitle("Depot d'energie RZ");
            hEdepRZ->Draw("COLZ");
            
            gPad->Update();
            TPaveStats* stats = (TPaveStats*)hEdepRZ->FindObject("stats");
            if (stats) {
                stats->SetX1NDC(stat2DX1);
                stats->SetX2NDC(stat2DX2);
                stats->SetY1NDC(stat2DY1);
                stats->SetY2NDC(stat2DY2);
                stats->SetTextSize(stat2DTextSize);
                stats->SetTextFont(62);
            }
            gPad->Modified();
            gPad->Update();
            
            std::cout << "hEdepRZ: " << hEdepRZ->GetEntries() << " entries\n";
        }
        
        c_2d->Update();
        c_2d->SaveAs("cartes_2d.png");
        std::cout << "=> Sauvegarde: cartes_2d.png\n\n";
    }
    
    // ==========================================================================
    // 7. SPECTRE DES ELECTRONS SECONDAIRES
    // ==========================================================================
    
    TH1D* hElectron = (TH1D*)file->Get("hElectronSpectrum");
    if (hElectron && hElectron->GetEntries() > 0) {
        TCanvas* c_elec = new TCanvas("c_elec", "Electrons secondaires", 800, 600);
        gPad->SetLogy();
        gPad->SetGridx();
        gPad->SetGridy();
        
        hElectron->SetLineColor(kGreen+2);
        hElectron->SetLineWidth(2);
        hElectron->SetTitle("Spectre des electrons secondaires dans l'eau");
        hElectron->Draw();
        
        c_elec->SaveAs("spectre_electrons.png");
        std::cout << "=> Sauvegarde: spectre_electrons.png\n";
        std::cout << "Electrons: " << hElectron->GetEntries() << " entries, Mean=" 
                  << hElectron->GetMean() << " keV\n\n";
    }
    
    // ==========================================================================
    // RESUME
    // ==========================================================================
    
    std::cout << "================================================================\n";
    std::cout << "                    ANALYSE TERMINEE                            \n";
    std::cout << "================================================================\n";
    std::cout << "  Fichiers generes:                                             \n";
    if (hGammaEmitted && hGammaEmitted->GetEntries() > 0)
        std::cout << "    - spectre_gamma_emis.png/pdf                               \n";
    if (hGammaWater && hGammaWater->GetEntries() > 0)
        std::cout << "    - spectre_gamma_eau.png                                    \n";
    if (hasHistos)
        std::cout << "    - dose_par_anneau.png/pdf                                  \n";
    if (hasEdep)
        std::cout << "    - edep_par_step.png                                        \n";
    if (gamma_lines)
        std::cout << "    - taux_absorption_eau.png/pdf                              \n";
    if ((hEdepXY && hEdepXY->GetEntries() > 0) || (hEdepRZ && hEdepRZ->GetEntries() > 0))
        std::cout << "    - cartes_2d.png                                            \n";
    if (hElectron && hElectron->GetEntries() > 0)
        std::cout << "    - spectre_electrons.png                                    \n";
    std::cout << "================================================================\n\n";
    
    // Ne pas fermer le fichier pour permettre l'exploration interactive
    // file->Close();
}
