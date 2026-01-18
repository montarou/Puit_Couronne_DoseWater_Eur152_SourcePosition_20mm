// ═══════════════════════════════════════════════════════════════════════════════
// analyse_absorption_raies.C
// Script ROOT pour analyser le taux d'absorption dans l'eau par raie gamma Eu-152
// Inclut l'analyse des processus d'absorption (photoélectrique, Compton, paires)
// Usage: root -l 'analyse_absorption_raies.C("puits_couronne_sans_filtre.root")'
// ═══════════════════════════════════════════════════════════════════════════════

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <THStack.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TLine.h>
#include <TGraph.h>
#include <TAxis.h>
#include <iostream>
#include <vector>
#include <iomanip>

void analyse_absorption_raies(const char* filename = "puits_couronne_sans_filtre.root")
{
    // ─────────────────────────────────────────────────────────────
    // CONFIGURATION
    // ─────────────────────────────────────────────────────────────
    gStyle->SetOptStat(0);
    gStyle->SetPaintTextFormat(".2f");
    
    // Ouvrir le fichier
    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Erreur: impossible d'ouvrir " << filename << std::endl;
        return;
    }
    
    // Récupérer le ntuple gamma_lines
    TTree* tree = (TTree*)file->Get("gamma_lines");
    if (!tree) {
        std::cerr << "Erreur: ntuple 'gamma_lines' non trouvé!" << std::endl;
        file->Close();
        return;
    }
    
    // ─────────────────────────────────────────────────────────────
    // VARIABLES DU NTUPLE
    // ─────────────────────────────────────────────────────────────
    Int_t lineIndex;
    Double_t energy_keV;
    Int_t emitted;
    Int_t enteredWater;
    Int_t absorbedWater;
    Double_t waterAbsRate;
    Int_t nPhotoelectric = 0;
    Int_t nCompton = 0;
    Int_t nPairProduction = 0;
    Int_t nOther = 0;
    
    tree->SetBranchAddress("lineIndex", &lineIndex);
    tree->SetBranchAddress("energy_keV", &energy_keV);
    tree->SetBranchAddress("emitted", &emitted);
    tree->SetBranchAddress("enteredWater", &enteredWater);
    tree->SetBranchAddress("absorbedWater", &absorbedWater);
    tree->SetBranchAddress("waterAbsRate", &waterAbsRate);
    
    // Nouvelles branches pour les processus (peuvent ne pas exister dans anciens fichiers)
    if (tree->GetBranch("nPhotoelectric")) tree->SetBranchAddress("nPhotoelectric", &nPhotoelectric);
    if (tree->GetBranch("nCompton")) tree->SetBranchAddress("nCompton", &nCompton);
    if (tree->GetBranch("nPairProduction")) tree->SetBranchAddress("nPairProduction", &nPairProduction);
    if (tree->GetBranch("nOther")) tree->SetBranchAddress("nOther", &nOther);
    
    bool hasProcessData = (tree->GetBranch("nPhotoelectric") != nullptr);
    
    // ─────────────────────────────────────────────────────────────
    // LECTURE DES DONNÉES
    // ─────────────────────────────────────────────────────────────
    Int_t nEntries = tree->GetEntries();
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    ANALYSE DU TAUX D'ABSORPTION PAR RAIE                              ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  Fichier: " << std::setw(74) << std::left << filename << " ║" << std::endl;
    std::cout << "║  Nombre de raies: " << std::setw(66) << std::left << nEntries << " ║" << std::endl;
    std::cout << "║  Données processus: " << std::setw(64) << std::left << (hasProcessData ? "OUI" : "NON") << " ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    // Vecteurs pour stocker les données
    std::vector<double> energies;
    std::vector<double> absRates;
    std::vector<int> emittedCounts;
    std::vector<int> enteredCounts;
    std::vector<int> absorbedCounts;
    std::vector<int> photCounts, comptCounts, convCounts, otherCounts;
    std::vector<TString> labels;
    
    // Noms des raies
    const char* raieNames[] = {
        "40 keV (X)", "40 keV (X)", "122 keV", "245 keV", "344 keV",
        "411 keV", "444 keV", "779 keV", "867 keV", "964 keV",
        "1086 keV", "1112 keV", "1408 keV"
    };
    
    // Lecture
    std::cout << "╔═══════╦════════════════╦════════════╦══════════════╦══════════════╦════════════════════╗" << std::endl;
    std::cout << "║ Index ║  Énergie (keV) ║    Émis    ║ Entrés eau   ║  Abs. eau    ║  Taux abs. (%)     ║" << std::endl;
    std::cout << "╠═══════╬════════════════╬════════════╬══════════════╬══════════════╬════════════════════╣" << std::endl;
    
    for (Int_t i = 0; i < nEntries; i++) {
        tree->GetEntry(i);
        
        energies.push_back(energy_keV);
        absRates.push_back(waterAbsRate);
        emittedCounts.push_back(emitted);
        enteredCounts.push_back(enteredWater);
        absorbedCounts.push_back(absorbedWater);
        photCounts.push_back(nPhotoelectric);
        comptCounts.push_back(nCompton);
        convCounts.push_back(nPairProduction);
        otherCounts.push_back(nOther);
        
        if (lineIndex < 13) {
            labels.push_back(raieNames[lineIndex]);
        } else {
            labels.push_back(Form("%.0f keV", energy_keV));
        }
        
        std::cout << "║   " << std::setw(2) << lineIndex << "  ║"
                  << std::setw(14) << std::fixed << std::setprecision(2) << energy_keV << "  ║"
                  << std::setw(10) << emitted << "  ║"
                  << std::setw(12) << enteredWater << "  ║"
                  << std::setw(12) << absorbedWater << "  ║"
                  << std::setw(18) << std::setprecision(2) << waterAbsRate << "  ║" << std::endl;
    }
    std::cout << "╚═══════╩════════════════╩════════════╩══════════════╩══════════════╩════════════════════╝\n" << std::endl;
    
    // ─────────────────────────────────────────────────────────────
    // TABLEAU DES PROCESSUS
    // ─────────────────────────────────────────────────────────────
    if (hasProcessData) {
        std::cout << "╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                              PROCESSUS D'ABSORPTION DANS L'EAU                                                   ║" << std::endl;
        std::cout << "╠═══════════════╦═══════════════╦═══════════════════╦═══════════════════╦═══════════════════╦══════════════════════╣" << std::endl;
        std::cout << "║     Raie      ║   Abs. eau    ║   Photoélectrique ║      Compton      ║  Création paires  ║       Autres         ║" << std::endl;
        std::cout << "╠═══════════════╬═══════════════╬═══════════════════╬═══════════════════╬═══════════════════╬══════════════════════╣" << std::endl;
        
        for (Int_t i = 0; i < nEntries; i++) {
            int total = absorbedCounts[i];
            double photPct = (total > 0) ? 100.0 * photCounts[i] / total : 0;
            double comptPct = (total > 0) ? 100.0 * comptCounts[i] / total : 0;
            double convPct = (total > 0) ? 100.0 * convCounts[i] / total : 0;
            double otherPct = (total > 0) ? 100.0 * otherCounts[i] / total : 0;
            
            std::cout << "║ " << std::setw(13) << std::left << labels[i] << " ║"
                      << std::setw(14) << std::right << total << " ║"
                      << std::setw(8) << photCounts[i] << " (" << std::setw(5) << std::fixed << std::setprecision(1) << photPct << "%) ║"
                      << std::setw(8) << comptCounts[i] << " (" << std::setw(5) << comptPct << "%) ║"
                      << std::setw(8) << convCounts[i] << " (" << std::setw(5) << convPct << "%) ║"
                      << std::setw(8) << otherCounts[i] << " (" << std::setw(5) << otherPct << "%)  ║" << std::endl;
        }
        std::cout << "╚═══════════════╩═══════════════╩═══════════════════╩═══════════════════╩═══════════════════╩══════════════════════╝\n" << std::endl;
    }
    
    // ─────────────────────────────────────────────────────────────
    // HISTOGRAMME 1 : TAUX D'ABSORPTION PAR RAIE (barres)
    // ─────────────────────────────────────────────────────────────
    TCanvas* c1 = new TCanvas("c1", "Taux d'absorption par raie", 1400, 600);
    c1->SetGrid();
    c1->SetBottomMargin(0.15);
    
    TH1D* h_absRate = new TH1D("h_absRate", "Taux d'absorption dans l'eau par raie gamma Eu-152", 
                               nEntries, 0, nEntries);
    
    for (Int_t i = 0; i < nEntries; i++) {
        h_absRate->SetBinContent(i+1, absRates[i]);
        h_absRate->GetXaxis()->SetBinLabel(i+1, labels[i]);
    }
    
    h_absRate->SetFillColor(kBlue);
    h_absRate->SetFillStyle(1001);
    h_absRate->SetLineColor(kBlue+2);
    h_absRate->SetLineWidth(2);
    h_absRate->GetXaxis()->SetTitle("Raie gamma");
    h_absRate->GetYaxis()->SetTitle("Taux d'absorption dans l'eau (%)");
    h_absRate->GetXaxis()->SetLabelSize(0.045);
    h_absRate->GetXaxis()->SetLabelOffset(0.01);
    h_absRate->GetYaxis()->SetTitleOffset(0.8);
    h_absRate->SetMinimum(0);
    
    h_absRate->Draw("BAR");
    
    // Afficher les valeurs sur les barres
    TLatex latex;
    latex.SetTextSize(0.03);
    latex.SetTextAlign(21);
    for (Int_t i = 0; i < nEntries; i++) {
        latex.DrawLatex(i+0.5, absRates[i] + h_absRate->GetMaximum()*0.02, 
                       Form("%.2f%%", absRates[i]));
    }
    
    c1->SaveAs("absorption_par_raie.png");
    std::cout << "✓ absorption_par_raie.png créé" << std::endl;
    
    // ─────────────────────────────────────────────────────────────
    // HISTOGRAMME 2 : TAUX D'ABSORPTION vs ÉNERGIE (graphe)
    // ─────────────────────────────────────────────────────────────
    TCanvas* c2 = new TCanvas("c2", "Taux d'absorption vs Energie", 1000, 700);
    c2->SetGrid();
    c2->SetLogx();
    c2->SetLeftMargin(0.12);
    
    TGraph* g_absVsE = new TGraph(nEntries);
    for (Int_t i = 0; i < nEntries; i++) {
        g_absVsE->SetPoint(i, energies[i], absRates[i]);
    }
    
    g_absVsE->SetTitle("Taux d'absorption dans l'eau vs Énergie gamma;Énergie (keV);Taux d'absorption (%)");
    g_absVsE->SetMarkerStyle(21);
    g_absVsE->SetMarkerSize(1.5);
    g_absVsE->SetMarkerColor(kRed);
    g_absVsE->SetLineColor(kRed);
    g_absVsE->SetLineWidth(2);
    g_absVsE->GetXaxis()->SetTitleOffset(1.2);
    g_absVsE->GetYaxis()->SetTitleOffset(1.2);
    
    g_absVsE->Draw("APL");
    
    // Annotations pour chaque point
    TLatex latex2;
    latex2.SetTextSize(0.025);
    for (Int_t i = 0; i < nEntries; i++) {
        latex2.DrawLatex(energies[i]*1.05, absRates[i], Form("%.1f%%", absRates[i]));
    }
    
    c2->SaveAs("absorption_vs_energie.png");
    std::cout << "✓ absorption_vs_energie.png créé" << std::endl;
    
    // ─────────────────────────────────────────────────────────────
    // HISTOGRAMME 3 : PROCESSUS D'ABSORPTION PAR RAIE (stacked)
    // ─────────────────────────────────────────────────────────────
    if (hasProcessData) {
        TCanvas* c3 = new TCanvas("c3", "Processus d'absorption par raie", 1400, 700);
        c3->SetGrid();
        c3->SetBottomMargin(0.15);
        
        TH1D* h_phot = new TH1D("h_phot", "Photoélectrique", nEntries, 0, nEntries);
        TH1D* h_compt = new TH1D("h_compt", "Compton", nEntries, 0, nEntries);
        TH1D* h_conv = new TH1D("h_conv", "Création paires", nEntries, 0, nEntries);
        TH1D* h_other = new TH1D("h_other", "Autres", nEntries, 0, nEntries);
        
        for (Int_t i = 0; i < nEntries; i++) {
            h_phot->SetBinContent(i+1, photCounts[i]);
            h_compt->SetBinContent(i+1, comptCounts[i]);
            h_conv->SetBinContent(i+1, convCounts[i]);
            h_other->SetBinContent(i+1, otherCounts[i]);
            h_phot->GetXaxis()->SetBinLabel(i+1, labels[i]);
        }
        
        h_phot->SetFillColor(kRed);
        h_compt->SetFillColor(kBlue);
        h_conv->SetFillColor(kGreen+2);
        h_other->SetFillColor(kGray);
        
        THStack* hs = new THStack("hs", "Processus d'absorption par raie gamma Eu-152;Raie gamma;Nombre de gammas absorbés");
        hs->Add(h_phot);
        hs->Add(h_compt);
        hs->Add(h_conv);
        hs->Add(h_other);
        
        hs->Draw("BAR");
        hs->GetXaxis()->SetLabelSize(0.045);
        
        TLegend* leg = new TLegend(0.65, 0.70, 0.88, 0.88);
        leg->AddEntry(h_phot, "Photoélectrique", "f");
        leg->AddEntry(h_compt, "Compton", "f");
        leg->AddEntry(h_conv, "Création paires", "f");
        leg->AddEntry(h_other, "Autres", "f");
        leg->Draw();
        
        c3->SaveAs("processus_absorption_par_raie.png");
        std::cout << "✓ processus_absorption_par_raie.png créé" << std::endl;
        
        // ─────────────────────────────────────────────────────────────
        // HISTOGRAMME 4 : FRACTION DES PROCESSUS PAR RAIE
        // ─────────────────────────────────────────────────────────────
        TCanvas* c4 = new TCanvas("c4", "Fraction des processus", 1400, 700);
        c4->SetGrid();
        c4->SetBottomMargin(0.15);
        
        TH1D* h_phot_pct = new TH1D("h_phot_pct", "", nEntries, 0, nEntries);
        TH1D* h_compt_pct = new TH1D("h_compt_pct", "", nEntries, 0, nEntries);
        TH1D* h_conv_pct = new TH1D("h_conv_pct", "", nEntries, 0, nEntries);
        
        for (Int_t i = 0; i < nEntries; i++) {
            int total = absorbedCounts[i];
            if (total > 0) {
                h_phot_pct->SetBinContent(i+1, 100.0 * photCounts[i] / total);
                h_compt_pct->SetBinContent(i+1, 100.0 * comptCounts[i] / total);
                h_conv_pct->SetBinContent(i+1, 100.0 * convCounts[i] / total);
            }
            h_phot_pct->GetXaxis()->SetBinLabel(i+1, labels[i]);
        }
        
        h_phot_pct->SetLineColor(kRed);
        h_phot_pct->SetLineWidth(3);
        h_phot_pct->SetMarkerStyle(20);
        h_phot_pct->SetMarkerColor(kRed);
        
        h_compt_pct->SetLineColor(kBlue);
        h_compt_pct->SetLineWidth(3);
        h_compt_pct->SetMarkerStyle(21);
        h_compt_pct->SetMarkerColor(kBlue);
        
        h_conv_pct->SetLineColor(kGreen+2);
        h_conv_pct->SetLineWidth(3);
        h_conv_pct->SetMarkerStyle(22);
        h_conv_pct->SetMarkerColor(kGreen+2);
        
        h_phot_pct->SetTitle("Fraction des processus d'absorption par raie;Raie gamma;Fraction (%)");
        h_phot_pct->GetXaxis()->SetLabelSize(0.045);
        h_phot_pct->SetMinimum(0);
        h_phot_pct->SetMaximum(105);
        
        h_phot_pct->Draw("LP");
        h_compt_pct->Draw("LP SAME");
        h_conv_pct->Draw("LP SAME");
        
        TLegend* leg2 = new TLegend(0.65, 0.70, 0.88, 0.88);
        leg2->AddEntry(h_phot_pct, "Photoélectrique", "lp");
        leg2->AddEntry(h_compt_pct, "Compton", "lp");
        leg2->AddEntry(h_conv_pct, "Création paires", "lp");
        leg2->Draw();
        
        c4->SaveAs("fraction_processus_par_raie.png");
        std::cout << "✓ fraction_processus_par_raie.png créé" << std::endl;
    }
    
    // ─────────────────────────────────────────────────────────────
    // RÉSUMÉ FINAL
    // ─────────────────────────────────────────────────────────────
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                              FICHIERS CRÉÉS                                            ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  1. absorption_par_raie.png          - Taux d'absorption par raie (barres)            ║" << std::endl;
    std::cout << "║  2. absorption_vs_energie.png        - Taux d'absorption vs énergie (courbe)          ║" << std::endl;
    if (hasProcessData) {
        std::cout << "║  3. processus_absorption_par_raie.png - Processus par raie (stacked)                  ║" << std::endl;
        std::cout << "║  4. fraction_processus_par_raie.png  - Fraction processus par raie (%)                ║" << std::endl;
    }
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    file->Close();
}
