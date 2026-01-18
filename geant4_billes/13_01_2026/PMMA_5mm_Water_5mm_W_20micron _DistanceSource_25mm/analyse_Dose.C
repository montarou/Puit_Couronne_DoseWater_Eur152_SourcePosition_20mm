// =============================================================================
// SCRIPT D'ANALYSE - PUITS COURONNE (version SANS FILTRE - Eu-152)
// =============================================================================
// 
// Usage: root -l analyse_Dose.C
//    ou: root -l 'analyse_Dose.C("autre_fichier.root")'
//
// =============================================================================

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
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
    gStyle->SetHistLineWidth(3);   // Lignes plus epaisses
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
    // 1. HISTOGRAMMES DE DOSE PAR ANNEAU (nGy)
    // ==========================================================================
    
    TCanvas* c_dose = new TCanvas("c_dose", "Dose par anneau", 1200, 800);
    c_dose->Divide(3, 2);
    
    TH1D* h_dose_ring[5];
    TH1D* h_dose_total = nullptr;
    
    // Parametres du pave de statistiques
    double statX1 = 0.60, statX2 = 0.90;
    double statY1 = 0.78, statY2 = 0.90;
    double statTextSize = 0.04;
    
    bool hasHistos = false;
    
    for (int i = 0; i < 5; i++) {
        TString hname = Form("h_dose_ring%d", i);
        h_dose_ring[i] = (TH1D*)file->Get(hname);
        
        if (h_dose_ring[i]) {
            hasHistos = true;
            c_dose->cd(i + 1);
            gPad->SetLogy();
            
            // Plage adaptee selon l'anneau
            if (i == 0) {
                h_dose_ring[i]->GetXaxis()->SetRangeUser(0., 1.);
            } else {
                h_dose_ring[i]->GetXaxis()->SetRangeUser(0., 0.3);
            }
            
            h_dose_ring[i]->SetLineColor(ringColors[i]);
            h_dose_ring[i]->SetFillColor(ringColors[i]);
            h_dose_ring[i]->SetFillStyle(3004);
            h_dose_ring[i]->SetLineWidth(3);
            h_dose_ring[i]->GetXaxis()->SetTitle("Dose [nGy]");
            h_dose_ring[i]->GetYaxis()->SetTitle("Counts");
            h_dose_ring[i]->GetXaxis()->SetTitleSize(0.05);
            h_dose_ring[i]->GetYaxis()->SetTitleSize(0.05);
            h_dose_ring[i]->GetXaxis()->SetLabelSize(0.05);
            h_dose_ring[i]->GetYaxis()->SetLabelSize(0.05);
            h_dose_ring[i]->SetTitle(Form("Anneau %d (r=%d-%d mm)", i, i*5, (i+1)*5));
            h_dose_ring[i]->Draw();
            
            gPad->Update();
            TPaveStats* stats = (TPaveStats*)h_dose_ring[i]->FindObject("stats");
            if (stats) {
                stats->SetX1NDC(statX1);
                stats->SetX2NDC(statX2);
                stats->SetY1NDC(statY1);
                stats->SetY2NDC(statY2);
                stats->SetTextSize(statTextSize);
                stats->SetTextFont(62);
                gPad->Modified();
            }
            
            // Afficher les statistiques
            std::cout << "Anneau " << i << ": Entries=" << h_dose_ring[i]->GetEntries()
                      << ", Mean=" << h_dose_ring[i]->GetMean() << " nGy\n";
        }
    }

    // Dose totale
    h_dose_total = (TH1D*)file->Get("h_dose_total");
    if (h_dose_total) {
        c_dose->cd(6);
        gPad->SetLogy();
        h_dose_total->GetXaxis()->SetRangeUser(0., 0.5);
        h_dose_total->SetLineColor(kBlack);
        h_dose_total->SetLineWidth(3);
        h_dose_total->GetXaxis()->SetTitle("Dose totale [nGy]");
        h_dose_total->GetYaxis()->SetTitle("Counts");
        h_dose_total->GetXaxis()->SetTitleSize(0.05);
        h_dose_total->GetYaxis()->SetTitleSize(0.05);
        h_dose_total->GetXaxis()->SetLabelSize(0.05);
        h_dose_total->GetYaxis()->SetLabelSize(0.05);
        h_dose_total->SetTitle("TOTAL (5 anneaux)");
        h_dose_total->Draw();
        
        gPad->Update();
        TPaveStats* stats = (TPaveStats*)h_dose_total->FindObject("stats");
        if (stats) {
            stats->SetX1NDC(statX1);
            stats->SetX2NDC(statX2);
            stats->SetY1NDC(statY1);
            stats->SetY2NDC(statY2);
            stats->SetTextSize(statTextSize);
            stats->SetTextFont(62);
            gPad->Modified();
        }
        
        std::cout << "TOTAL: Entries=" << h_dose_total->GetEntries()
                  << ", Mean=" << h_dose_total->GetMean() << " nGy\n\n";
    }
    
    if (hasHistos) {
        c_dose->Update();
        c_dose->SaveAs("dose_par_anneau.png");
        c_dose->SaveAs("dose_par_anneau.pdf");
        std::cout << "=> Sauvegarde: dose_par_anneau.png/pdf\n\n";
    } else {
        std::cout << "ATTENTION: Aucun histogramme h_dose_ringX trouve!\n\n";
    }
    
    // ==========================================================================
    // 2. TAUX D'ABSORPTION DANS L'EAU PAR RAIE GAMMA
    // ==========================================================================
    
    gStyle->SetOptStat(0);
    
    TTree* gamma_lines = (TTree*)file->Get("gamma_lines");
    if (gamma_lines) {
        std::cout << "Analyse du TTree gamma_lines...\n";
        gamma_lines->Print();
        
        Int_t lineIndex;
        Double_t waterAbsRate = 0;
        
        gamma_lines->SetBranchAddress("lineIndex", &lineIndex);
        
        // Chercher la branche pour le taux d'absorption dans l'eau
        if (gamma_lines->GetBranch("waterAbsRate")) {
            gamma_lines->SetBranchAddress("waterAbsRate", &waterAbsRate);
        } else if (gamma_lines->GetBranch("filterAbsRate")) {
            // Ancienne version avec filtre
            gamma_lines->SetBranchAddress("filterAbsRate", &waterAbsRate);
        } else {
            std::cout << "ATTENTION: Branche waterAbsRate/filterAbsRate non trouvee\n";
        }
        
        TCanvas* c_abs = new TCanvas("c_abs", "Taux d'absorption par raie", 1000, 600);
        c_abs->SetBottomMargin(0.18);
        c_abs->SetLeftMargin(0.12);
        c_abs->SetRightMargin(0.05);
        
        TH1D* h_abs_water = new TH1D("h_abs_water", 
            "Taux d'absorption dans l'eau (5 mm) par raie gamma Eu-152;Raie gamma;Taux d'absorption (%)", 
            nGammaLines, 0, nGammaLines);
        
        Long64_t nEntries = gamma_lines->GetEntries();
        std::cout << "Nombre d'entrees: " << nEntries << "\n";
        
        for (Long64_t i = 0; i < nEntries && i < nGammaLines; i++) {
            gamma_lines->GetEntry(i);
            h_abs_water->SetBinContent(lineIndex + 1, waterAbsRate);
            h_abs_water->GetXaxis()->SetBinLabel(lineIndex + 1, gammaLineNames[lineIndex]);
            std::cout << "  Raie " << lineIndex << " (" << gammaLineNames[lineIndex] 
                      << "): " << waterAbsRate << "%\n";
        }
        
        h_abs_water->SetFillColor(kAzure+1);
        h_abs_water->SetLineColor(kAzure+3);
        h_abs_water->SetLineWidth(2);
        h_abs_water->SetMinimum(0);
        
        // Adapter le maximum selon les donnees
        double maxRate = h_abs_water->GetMaximum();
        if (maxRate < 5) {
            h_abs_water->SetMaximum(5);  // Pour les faibles absorptions
        } else {
            h_abs_water->SetMaximum(maxRate * 1.2);
        }
        
        h_abs_water->GetXaxis()->SetLabelSize(0.04);
        h_abs_water->GetXaxis()->SetLabelOffset(0.02);
        h_abs_water->GetXaxis()->SetTitleOffset(2.2);
        h_abs_water->GetYaxis()->SetLabelSize(0.045);
        h_abs_water->GetYaxis()->SetTitleSize(0.05);
        h_abs_water->GetYaxis()->SetTitleOffset(0.9);
        
        h_abs_water->Draw("bar");
        
        TLatex* latex = new TLatex();
        latex->SetNDC();
        latex->SetTextSize(0.035);
        latex->DrawLatex(0.15, 0.85, "Source Eu-152 (42 kBq) - Eau 5 mm - SANS FILTRE");
        latex->DrawLatex(0.15, 0.80, Form("Absorption dominante: raies X 40 keV (%.1f%%)", maxRate));
        
        c_abs->SaveAs("taux_absorption_eau.png");
        c_abs->SaveAs("taux_absorption_eau.pdf");
        std::cout << "\n=> Sauvegarde: taux_absorption_eau.png/pdf\n\n";
    } else {
        std::cout << "TTree gamma_lines non trouve dans le fichier\n\n";
    }
    
    // ==========================================================================
    // 3. SPECTRE EN ENERGIE DES PRIMAIRES
    // ==========================================================================
    
    TH1D* h_spectrum = (TH1D*)file->Get("h_primary_spectrum");
    if (!h_spectrum) h_spectrum = (TH1D*)file->Get("h_spectrum");
    if (!h_spectrum) h_spectrum = (TH1D*)file->Get("h_energy");
    
    if (h_spectrum) {
        TCanvas* c_spec = new TCanvas("c_spec", "Spectre gamma", 900, 600);
        gPad->SetLogy();
        gPad->SetGridx();
        gPad->SetGridy();
        
        h_spectrum->SetLineColor(kRed+1);
        h_spectrum->SetLineWidth(2);
        h_spectrum->GetXaxis()->SetTitle("Energie [keV]");
        h_spectrum->GetYaxis()->SetTitle("Counts");
        h_spectrum->SetTitle("Spectre gamma Eu-152");
        h_spectrum->Draw();
        
        c_spec->SaveAs("spectre_gamma.png");
        std::cout << "=> Sauvegarde: spectre_gamma.png\n\n";
    }
    
    // ==========================================================================
    // RESUME
    // ==========================================================================
    
    std::cout << "================================================================\n";
    std::cout << "                    ANALYSE TERMINEE                            \n";
    std::cout << "================================================================\n";
    std::cout << "  Fichiers generes:                                             \n";
    std::cout << "    - dose_par_anneau.png/pdf                                   \n";
    std::cout << "    - taux_absorption_eau.png/pdf                               \n";
    if (h_spectrum) std::cout << "    - spectre_gamma.png                                       \n";
    std::cout << "================================================================\n\n";
    
    // Ne pas fermer le fichier pour permettre l'exploration interactive
    // file->Close();
}
