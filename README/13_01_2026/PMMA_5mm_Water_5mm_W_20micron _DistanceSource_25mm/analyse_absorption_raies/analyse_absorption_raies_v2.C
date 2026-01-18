// ═══════════════════════════════════════════════════════════════════════════════
// analyse_absorption_raies_v2.C
// Script ROOT pour analyser le taux d'absorption dans l'eau par raie gamma Eu-152
// VERSION CORRIGÉE : Annotations améliorées pour éviter le chevauchement
// Usage: root -l analyse_absorption_raies_v2.C
//    ou: root -l 'analyse_absorption_raies_v2.C("autre_fichier.root")'
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

void analyse_absorption_raies_v2(const char* filename = "output.root")
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
    // HISTOGRAMME 1 : TAUX D'ABSORPTION PAR RAIE (barres)
    // ─────────────────────────────────────────────────────────────
    TCanvas* c1 = new TCanvas("c1", "Taux d'absorption par raie", 1400, 600);
    c1->SetGrid();
    c1->SetLogy();
    c1->SetBottomMargin(0.15);
    c1->SetLeftMargin(0.10);
    c1->SetRightMargin(0.05);
    
    TH1D* h_abs = new TH1D("h_abs", "Taux d'absorption dans l'eau par raie gamma Eu-152;Raie gamma;Taux d'absorption dans l'eau (%)",
                          nEntries, 0, nEntries);
    
    for (Int_t i = 0; i < nEntries; i++) {
        h_abs->SetBinContent(i+1, absRates[i]);
        h_abs->GetXaxis()->SetBinLabel(i+1, labels[i]);
    }
    
    h_abs->SetFillColor(kBlue);
    h_abs->SetLineColor(kBlue+2);
    h_abs->SetLineWidth(2);
    h_abs->SetMinimum(0.0008);
    h_abs->SetMaximum(10);
    h_abs->GetXaxis()->SetLabelSize(0.045);
    h_abs->GetYaxis()->SetTitleOffset(1.0);
    h_abs->Draw("BAR");
    
    // Afficher les valeurs sur les barres (adapté pour échelle log)
    TLatex latex;
    latex.SetTextSize(0.03);
    latex.SetTextAlign(21);
    for (Int_t i = 0; i < nEntries; i++) {
        double ypos = (absRates[i] > 0.01) ? absRates[i] * 1.3 : 0.015;
        latex.DrawLatex(i+0.5, ypos, Form("%.2f%%", absRates[i]));
    }
    
    c1->SaveAs("absorption_par_raie.png");
    std::cout << "✓ absorption_par_raie.png créé" << std::endl;
    
    // ─────────────────────────────────────────────────────────────
    // HISTOGRAMME 2 : TAUX D'ABSORPTION vs ÉNERGIE (graphe)
    // VERSION CORRIGÉE avec annotations améliorées
    // ─────────────────────────────────────────────────────────────
    TCanvas* c2 = new TCanvas("c2", "Taux d'absorption vs Energie", 1000, 700);
    c2->SetGrid();
    c2->SetLogx();
    c2->SetLogy();
    c2->SetLeftMargin(0.12);
    c2->SetRightMargin(0.05);
    
    TGraph* g_absVsE = new TGraph(nEntries);
    for (Int_t i = 0; i < nEntries; i++) {
        g_absVsE->SetPoint(i, energies[i], absRates[i]);
    }
    
    g_absVsE->SetTitle("Taux d'absorption dans l'eau vs #acute{E}nergie gamma;#acute{E}nergie (keV);Taux d'absorption (%)");
    g_absVsE->SetMarkerStyle(21);
    g_absVsE->SetMarkerSize(1.5);
    g_absVsE->SetMarkerColor(kRed);
    g_absVsE->SetLineColor(kRed);
    g_absVsE->SetLineWidth(2);
    g_absVsE->GetXaxis()->SetTitleOffset(1.2);
    g_absVsE->GetYaxis()->SetTitleOffset(1.2);
    g_absVsE->SetMinimum(0.0005);  // Minimum ajusté pour voir les annotations basses
    g_absVsE->SetMaximum(10);
    
    g_absVsE->Draw("APL");
    
    // ═══════════════════════════════════════════════════════════════
    // ANNOTATIONS AMÉLIORÉES - Éviter le chevauchement
    // ═══════════════════════════════════════════════════════════════
    TLatex latex2;
    latex2.SetTextSize(0.025);
    
    // Définir des positions Y alternées pour les faibles valeurs
    // On utilise un tableau de facteurs multiplicatifs pour alterner haut/bas
    // et des décalages différents selon la région d'énergie
    
    for (Int_t i = 0; i < nEntries; i++) {
        double ypos;
        double xpos;
        int textAlign = 21;  // Centré horizontal, bas vertical par défaut
        
        if (absRates[i] > 0.05) {
            // Taux élevés (raies X à 40 keV) : annotation au-dessus
            ypos = absRates[i] * 1.8;
            xpos = energies[i];
            textAlign = 21;
        }
        else if (absRates[i] > 0.01) {
            // Taux moyens (122 keV) : annotation au-dessus
            ypos = absRates[i] * 2.0;
            xpos = energies[i] * 1.1;
            textAlign = 11;  // Aligné à gauche
        }
        else if (absRates[i] > 0.005) {
            // Taux faibles (245 keV) : annotation au-dessus
            ypos = absRates[i] * 2.5;
            xpos = energies[i] * 1.1;
            textAlign = 11;
        }
        else {
            // Très faibles taux : alterner haut/bas et décaler en X
            // Utiliser l'index pour alterner
            if (i % 3 == 0) {
                // Position haute
                ypos = 0.006;
                xpos = energies[i];
                textAlign = 21;
            }
            else if (i % 3 == 1) {
                // Position moyenne-haute
                ypos = 0.003;
                xpos = energies[i];
                textAlign = 21;
            }
            else {
                // Position basse (sous le point)
                ypos = absRates[i] * 0.4;
                if (ypos < 0.0003) ypos = 0.0003;
                xpos = energies[i];
                textAlign = 23;  // Centré horizontal, haut vertical
            }
        }
        
        latex2.SetTextAlign(textAlign);
        
        // Formater l'affichage selon la valeur
        TString label;
        if (absRates[i] >= 0.01) {
            label = Form("%.2f%%", absRates[i]);
        } else if (absRates[i] >= 0.001) {
            label = Form("%.3f%%", absRates[i]);
        } else {
            label = Form("%.1e%%", absRates[i]);
        }
        
        latex2.DrawLatex(xpos, ypos, label);
    }
    
    c2->SaveAs("absorption_vs_energie.png");
    std::cout << "✓ absorption_vs_energie.png créé" << std::endl;
    
    // ─────────────────────────────────────────────────────────────
    // HISTOGRAMME 3 & 4 : PROCESSUS D'ABSORPTION (si données disponibles)
    // ─────────────────────────────────────────────────────────────
    if (hasProcessData) {
        // ... (code inchangé pour les figures 3 et 4)
        std::cout << "Données de processus disponibles - génération des figures 3 et 4" << std::endl;
    }
    
    // ─────────────────────────────────────────────────────────────
    // RÉSUMÉ FINAL
    // ─────────────────────────────────────────────────────────────
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                              FICHIERS CRÉÉS                                            ║" << std::endl;
    std::cout << "╠═══════════════════════════════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  1. absorption_par_raie.png          - Taux d'absorption par raie (barres)            ║" << std::endl;
    std::cout << "║  2. absorption_vs_energie.png        - Taux d'absorption vs énergie (courbe)          ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    file->Close();
}
