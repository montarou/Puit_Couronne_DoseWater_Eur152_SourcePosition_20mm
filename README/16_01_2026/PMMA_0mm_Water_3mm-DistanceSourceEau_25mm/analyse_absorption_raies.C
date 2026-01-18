// ═══════════════════════════════════════════════════════════════════════════════
// analyse_absorption_raies.C
// Script ROOT pour analyser le taux d'absorption dans l'eau par raie gamma Eu-152
// Inclut l'analyse des processus d'absorption (photoélectrique, Compton, paires)
// Usage: root -l analyse_absorption_raies.C
//    ou: root -l 'analyse_absorption_raies.C("autre_fichier.root")'
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

void analyse_absorption_raies(const char* filename = "output.root")
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
    c1->SetLogy();  // Axe Y en echelle logarithmique
    
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
    h_absRate->SetMinimum(0.0001);  // Minimum > 0 pour echelle log
    h_absRate->SetMaximum(h_absRate->GetMaximum() * 2);  // Plus d'espace en haut pour les labels
    
    h_absRate->Draw("BAR");
    
    // Afficher les valeurs sur les barres (adapté pour échelle log)
    TLatex latex;
    latex.SetTextSize(0.03);
    latex.SetTextAlign(21);
    for (Int_t i = 0; i < nEntries; i++) {
        double ypos = (absRates[i] > 0.01) ? absRates[i] * 1.3 : 0.015;  // Position adaptée pour log
        latex.DrawLatex(i+0.5, ypos, Form("%.4f%%", absRates[i]));
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
    c2->SetLogy();  // Axe Y en echelle logarithmique
    c2->SetLeftMargin(0.12);
    c2->SetRightMargin(0.05);

    TGraph* g_absVsE = new TGraph(nEntries);
    for (Int_t i = 0; i < nEntries; i++) {
        g_absVsE->SetPoint(i, energies[i], absRates[i]);
    }
    
    g_absVsE->SetTitle("Taux d'absorption dans l'eau vs Energie gamma;Energie (keV);Taux d'absorption (%)");
    g_absVsE->SetMarkerStyle(21);
    g_absVsE->SetMarkerSize(1.5);
    g_absVsE->SetMarkerColor(kRed);
    g_absVsE->SetLineColor(kRed);
    g_absVsE->SetLineWidth(2);
    g_absVsE->GetXaxis()->SetTitleOffset(1.4);
    g_absVsE->GetYaxis()->SetTitleOffset(1.2);
    g_absVsE->SetMinimum(0.0001);  // Minimum pour echelle log
    g_absVsE->SetMaximum(10);     // Maximum pour echelle log
    
    g_absVsE->Draw("APL");
    
    // ═══════════════════════════════════════════════════════════════
    // ANNOTATIONS VERSION 3 - Positions individuelles optimisées
    // ═══════════════════════════════════════════════════════════════
    TLatex latex2;
    latex2.SetTextSize(0.022);

    for (Int_t i = 0; i < nEntries; i++) {
        double ypos;
        double xpos;
        int textAlign = 21;  // Centré horizontal, bas vertical par défaut

        // Formater l'affichage selon la valeur
        TString label;
        if (absRates[i] >= 0.01) {
            label = Form("%.2f%%", absRates[i]);
        } else if (absRates[i] >= 0.001) {
            label = Form("%.3f%%", absRates[i]);
        } else {
            label = Form("%.2e%%", absRates[i]);
        }

        // ═══════════════════════════════════════════════════════════════
        // TRAITEMENT SPÉCIFIQUE PAR INDEX DE RAIE
        // ═══════════════════════════════════════════════════════════════

        switch(i) {
            case 0:  // Première raie X à 39.52 keV - décaler vers la gauche et le haut
                xpos = energies[i] * 1.0;  // Décaler vers la gauche
                ypos = absRates[i] * 2.;   // Au-dessus
                textAlign = 31;  // Aligné à droite
                break;

            case 1:  // Deuxième raie X à 40.12 keV - décaler vers la droite et le bas
                xpos = energies[i] * 1.1;  // Décaler vers la droite
                ypos = absRates[i] * 0.3;   // En dessous
                textAlign = 11;  // Aligné à gauche
                break;

            case 2:  // 122 keV - au-dessus à droite
                xpos = energies[i] * 1.15;
                ypos = absRates[i] * 1.8;
                textAlign = 11;
                break;

            case 3:  // 245 keV - au-dessus à droite
                xpos = energies[i] * 1.15;
                ypos = absRates[i] * 2.2;
                textAlign = 11;
                break;

            case 4:  // 344 keV - au-dessus
                xpos = energies[i];
                ypos = absRates[i] * 2.5;
                textAlign = 21;
                break;

            case 5:  // 411 keV - au-dessus à droite
                xpos = energies[i] * 1.1;
                ypos = absRates[i] * 2.5;
                textAlign = 11;
                break;

            case 6:  // 444 keV - en dessous
                xpos = energies[i];
                ypos = absRates[i] * 0.35;
                textAlign = 23;
                break;

            case 7:  // 779 keV - au-dessus
                xpos = energies[i];
                ypos = absRates[i] * 3.0;
                textAlign = 21;
                break;

            case 8:  // 867 keV - en dessous (très faible)
                xpos = energies[i];
                ypos = absRates[i] * 0.25;
                textAlign = 23;
                break;

            case 9:  // 964 keV - au-dessus haut
                xpos = energies[i];
                ypos = absRates[i] * 4.0;
                textAlign = 21;
                break;

            case 10: // 1086 keV - en dessous très bas
                xpos = energies[i];
                ypos = absRates[i] * 0.4;
                textAlign = 23;
                break;

            case 11: // 1112 keV - au-dessus
                xpos = energies[i];
                ypos = absRates[i] * 3.1;
                textAlign = 21;
                break;

            case 12: // 1408 keV - à droite
                xpos = energies[i] * 1.;
                ypos = absRates[i] * 1.5;
                textAlign = 11;
                break;

            default:
                xpos = energies[i];
                ypos = absRates[i] * 2.0;
                textAlign = 21;
                break;
        }

        latex2.SetTextAlign(textAlign);
        latex2.DrawLatex(xpos, ypos, label);
    }

    
    c2->SaveAs("absorption_vs_energie.png");
    std::cout << "✓ absorption_vs_energie.png créé" << std::endl;
    

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
