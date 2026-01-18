// ═══════════════════════════════════════════════════════════════════════════════
// analyse_absorption_raies_v3.C
// Script ROOT pour analyser le taux d'absorption dans l'eau par raie gamma Eu-152
// VERSION 3 : Annotations corrigées pour éviter tout chevauchement
// - Traitement spécifique des deux raies X à 40 keV
// - Meilleur espacement pour les taux < 0.001%
// Usage: root -l analyse_absorption_raies_v3.C
//    ou: root -l 'analyse_absorption_raies_v3.C("autre_fichier.root")'
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

void analyse_absorption_raies_v3(const char* filename = "output.root")
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
                  << std::setw(18) << std::setprecision(4) << waterAbsRate << "  ║" << std::endl;
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
    // VERSION 3 : Annotations entièrement corrigées
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
    g_absVsE->SetMinimum(0.0002);  // Minimum ajusté pour voir les annotations basses
    g_absVsE->SetMaximum(10);
    
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
                xpos = energies[i] * 0.7;  // Décaler vers la gauche
                ypos = absRates[i] * 2.5;   // Au-dessus
                textAlign = 31;  // Aligné à droite
                break;
                
            case 1:  // Deuxième raie X à 40.12 keV - décaler vers la droite et le bas
                xpos = energies[i] * 1.4;  // Décaler vers la droite
                ypos = absRates[i] * 0.5;   // En dessous
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
                ypos = absRates[i] * 0.2;
                textAlign = 23;
                break;
                
            case 11: // 1112 keV - au-dessus
                xpos = energies[i];
                ypos = absRates[i] * 3.5;
                textAlign = 21;
                break;
                
            case 12: // 1408 keV - à droite
                xpos = energies[i] * 1.1;
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
    // HISTOGRAMME 3 & 4 : PROCESSUS D'ABSORPTION (si données disponibles)
    // ─────────────────────────────────────────────────────────────
    if (hasProcessData) {
        std::cout << "Données de processus disponibles - génération des figures 3 et 4" << std::endl;
        
        // Figure 3: Histogramme empilé des processus
        TCanvas* c3 = new TCanvas("c3", "Processus d'absorption", 1400, 600);
        c3->SetGrid();
        c3->SetLogy();
        c3->SetBottomMargin(0.15);
        c3->SetLeftMargin(0.10);
        c3->SetRightMargin(0.12);
        
        TH1D* h_phot = new TH1D("h_phot", "Photoelectrique", nEntries, 0, nEntries);
        TH1D* h_compt = new TH1D("h_compt", "Compton", nEntries, 0, nEntries);
        TH1D* h_conv = new TH1D("h_conv", "Creation paires", nEntries, 0, nEntries);
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
        h_other->SetFillColor(kOrange);
        
        THStack* hs = new THStack("hs", "Processus d'absorption par raie gamma;Raie gamma;Nombre d'absorptions");
        hs->Add(h_phot);
        hs->Add(h_compt);
        hs->Add(h_conv);
        hs->Add(h_other);
        hs->Draw("BAR");
        hs->GetXaxis()->SetLabelSize(0.04);
        
        TLegend* leg3 = new TLegend(0.88, 0.6, 0.99, 0.88);
        leg3->AddEntry(h_phot, "Photo#acute{e}lectrique", "f");
        leg3->AddEntry(h_compt, "Compton", "f");
        leg3->AddEntry(h_conv, "Paires", "f");
        leg3->AddEntry(h_other, "Autres", "f");
        leg3->Draw();
        
        c3->SaveAs("processus_absorption_par_raie.png");
        std::cout << "✓ processus_absorption_par_raie.png créé" << std::endl;
        
        // Figure 4: Fraction des processus par raie
        TCanvas* c4 = new TCanvas("c4", "Fraction des processus", 1200, 600);
        c4->SetGrid();
        c4->SetBottomMargin(0.15);
        c4->SetLeftMargin(0.10);
        c4->SetRightMargin(0.12);
        
        TGraph* g_phot = new TGraph(nEntries);
        TGraph* g_compt = new TGraph(nEntries);
        TGraph* g_conv = new TGraph(nEntries);
        
        for (Int_t i = 0; i < nEntries; i++) {
            int total = absorbedCounts[i];
            double fracPhot = (total > 0) ? 100.0 * photCounts[i] / total : 0;
            double fracCompt = (total > 0) ? 100.0 * comptCounts[i] / total : 0;
            double fracConv = (total > 0) ? 100.0 * convCounts[i] / total : 0;
            
            g_phot->SetPoint(i, energies[i], fracPhot);
            g_compt->SetPoint(i, energies[i], fracCompt);
            g_conv->SetPoint(i, energies[i], fracConv);
        }
        
        g_phot->SetTitle("Fraction des processus d'absorption vs #acute{E}nergie;#acute{E}nergie (keV);Fraction (%)");
        g_phot->SetMarkerStyle(21);
        g_phot->SetMarkerColor(kRed);
        g_phot->SetLineColor(kRed);
        g_phot->SetLineWidth(2);
        g_phot->SetMinimum(0);
        g_phot->SetMaximum(105);
        
        g_compt->SetMarkerStyle(22);
        g_compt->SetMarkerColor(kBlue);
        g_compt->SetLineColor(kBlue);
        g_compt->SetLineWidth(2);
        
        g_conv->SetMarkerStyle(23);
        g_conv->SetMarkerColor(kGreen+2);
        g_conv->SetLineColor(kGreen+2);
        g_conv->SetLineWidth(2);
        
        g_phot->Draw("APL");
        g_compt->Draw("PL SAME");
        g_conv->Draw("PL SAME");
        
        TLegend* leg4 = new TLegend(0.65, 0.65, 0.88, 0.88);
        leg4->AddEntry(g_phot, "Photo#acute{e}lectrique", "lp");
        leg4->AddEntry(g_compt, "Compton", "lp");
        leg4->AddEntry(g_conv, "Cr#acute{e}ation paires", "lp");
        leg4->Draw();
        
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
        std::cout << "║  3. processus_absorption_par_raie.png - Processus par raie (histogramme empilé)       ║" << std::endl;
        std::cout << "║  4. fraction_processus_par_raie.png   - Fraction des processus vs énergie             ║" << std::endl;
    }
    std::cout << "╚═══════════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    file->Close();
}
