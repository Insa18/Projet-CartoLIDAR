#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include "LidarPanel.h"
#include "ApplicationController.h"
#include "LidarDroit.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace test1 {
    public ref class MainForm : public Form {
    private:
        ApplicationController^ controller;
        Panel^ headerPanel;
        Button^ btnConnect;
        Button^ btnStart;
        Button^ btnStop;
        Button^ btnAvancer;
        Button^ btnReculer;
        Button^ btnGauche;
        Button^ btnDroite;
        Button^ btnClear;
        Button^ btnTest;  
        Button^ btnRetourArriere; 
        Button^ btnDroitesLIDAR;
        TextBox^ textBoxLog;
        LidarPanel^ panelLidar;
        
        // Nouveaux contrôles pour les paramètres RANSAC
        TrackBar^ trackDistanceThreshold;
        TrackBar^ trackMaxIterations;
        TrackBar^ trackMinPoints;
        TrackBar^ trackMinLength;
        Label^ lblDistanceThreshold;
        Label^ lblMaxIterations;
        Label^ lblMinPoints;
        Label^ lblMinLength;
        LidarDroit^ lidarDroit;
    
    public:
        MainForm() {
            panelLidar = gcnew LidarPanel();
            InitializeComponents();
            controller = gcnew ApplicationController(panelLidar, this);
            controller->Initialize();
            controller->SetLogCallback(gcnew Action<String^>(this, &MainForm::ThreadSafeLog));
            lidarDroit = gcnew LidarDroit();

            // Vérifier si lidar_data.csv est vide ou ne contient que l'en-tête au lancement
            try {
                String^ filePath = "lidar_data.csv";
                if (System::IO::File::Exists(filePath)) {
                    System::IO::File::WriteAllText(filePath, "");
                    ThreadSafeLog("Le fichier lidar_data.csv a été vidé.");
                } else {
                    System::IO::File::WriteAllText(filePath, "");
                    ThreadSafeLog("Le fichier lidar_data.csv a été créé vide.");
                }
            }
            catch (Exception^ ex) {
                ThreadSafeLog("Erreur lors de la gestion de lidar_data.csv : " + ex->Message);
            }
        }

        void ThreadSafeLog(String^ message) {
            if (textBoxLog->Lines->Length > 100) {
                textBoxLog->Clear();
            }
            textBoxLog->AppendText(DateTime::Now.ToString("[HH:mm:ss] ") + message + Environment::NewLine);
            textBoxLog->ScrollToCaret();
        }

    private:
        void InitializeComponents() {
            this->Text = "Affichage RPLIDAR A1";
            this->Size = Drawing::Size(1200, 1000);

            // Panel d'en-tête
            headerPanel = gcnew Panel();
            headerPanel->Dock = DockStyle::Top;
            headerPanel->Height = 40;
            headerPanel->BackColor = Color::FromArgb(240, 240, 240);

            // Panel LIDAR
            panelLidar->Dock = DockStyle::Fill;
            panelLidar->BackColor = Color::Black;
            panelLidar->Size = Drawing::Size(800, 800);
            panelLidar->Location = Point(0, headerPanel->Height);

            // Boutons de contrôle
            btnConnect = gcnew Button();
            btnConnect->Text = "Connecter";
            btnConnect->Location = Point(10, 5);
            btnConnect->Click += gcnew EventHandler(this, &MainForm::btnConnect_Click);

            btnStart = gcnew Button();
            btnStart->Text = "Démarrer Scan";
            btnStart->Location = Point(120, 5);
            btnStart->Enabled = false;
            btnStart->Click += gcnew EventHandler(this, &MainForm::btnStart_Click);

            btnStop = gcnew Button();
            btnStop->Text = "Arrêter Scan";
            btnStop->Location = Point(230, 5);
            btnStop->Enabled = false;
            btnStop->Click += gcnew EventHandler(this, &MainForm::btnStop_Click);

            btnAvancer = gcnew Button();
            btnAvancer->Text = "Avancer";
            btnAvancer->Location = Point(340, 5);
            btnAvancer->Click += gcnew EventHandler(this, &MainForm::btnAvancer_Click);

            btnReculer = gcnew Button();
            btnReculer->Text = "Reculer";
            btnReculer->Location = Point(430, 5);
            btnReculer->Click += gcnew EventHandler(this, &MainForm::btnReculer_Click);

            btnGauche = gcnew Button();
            btnGauche->Text = "Gauche";
            btnGauche->Location = Point(520, 5);
            btnGauche->Click += gcnew EventHandler(this, &MainForm::btnGauche_Click);

            btnDroite = gcnew Button();
            btnDroite->Text = "Droite";
            btnDroite->Location = Point(610, 5);
            btnDroite->Click += gcnew EventHandler(this, &MainForm::btnDroite_Click);

            btnClear = gcnew Button();
            btnClear->Text = "Effacer Carte";
            btnClear->Location = Point(700, 5);
            btnClear->Click += gcnew EventHandler(this, &MainForm::btnClear_Click);

            btnTest = gcnew Button();
            btnTest->Text = "Test";
            btnTest->Location = Point(790, 5);
            btnTest->Click += gcnew EventHandler(this, &MainForm::btnTest_Click);

            btnRetourArriere = gcnew Button();
            btnRetourArriere->Text = "Retour arrière";
            btnRetourArriere->Location = Point(880, 5);
            btnRetourArriere->Click += gcnew EventHandler(this, &MainForm::btnRetourArriere_Click);

            btnDroitesLIDAR = gcnew Button();
            btnDroitesLIDAR->Text = "Droites LIDAR";
            btnDroitesLIDAR->Location = Point(980, 5);
            btnDroitesLIDAR->Click += gcnew EventHandler(this, &MainForm::btnDroitesLIDAR_Click);

            // Ajouter les boutons au panel d'en-tête
            headerPanel->Controls->Add(btnConnect);
            headerPanel->Controls->Add(btnStart);
            headerPanel->Controls->Add(btnStop);
            headerPanel->Controls->Add(btnAvancer);
            headerPanel->Controls->Add(btnReculer);
            headerPanel->Controls->Add(btnGauche);
            headerPanel->Controls->Add(btnDroite);
            headerPanel->Controls->Add(btnClear);
            headerPanel->Controls->Add(btnTest);
            headerPanel->Controls->Add(btnRetourArriere);
            headerPanel->Controls->Add(btnDroitesLIDAR);

            // Zone de log
            textBoxLog = gcnew TextBox();
            textBoxLog->Multiline = true;
            textBoxLog->Dock = DockStyle::Bottom;
            textBoxLog->Height = 150;
            textBoxLog->ReadOnly = true;
            textBoxLog->ScrollBars = ScrollBars::Vertical;

            // Panel pour les paramètres RANSAC
            Panel^ paramPanel = gcnew Panel();
            paramPanel->Dock = DockStyle::Right;
            paramPanel->Width = 200;
            paramPanel->Height = 450;  // Hauteur ajustée pour tous les contrôles
            paramPanel->BackColor = Color::FromArgb(240, 240, 240);

            // Titre du panel des paramètres
            Label^ lblTitle = gcnew Label();
            lblTitle->Text = "Paramètres RANSAC\n"
                "Ajustez ces paramètres pour optimiser\n"
                "la détection des murs";
            lblTitle->Location = Point(10, 10);
            lblTitle->AutoSize = true;
            lblTitle->Font = gcnew System::Drawing::Font("Arial", 10, FontStyle::Bold);
            lblTitle->Height = 40;

            // Labels et trackbars pour les paramètres avec descriptions détaillées
            lblDistanceThreshold = gcnew Label();
            lblDistanceThreshold->Text = "Distance Threshold (cm): 5\n"
                "Distance maximale entre un point et une droite\n"
                "pour être considéré comme appartenant à cette droite";
            lblDistanceThreshold->Location = Point(10, 50);
            lblDistanceThreshold->AutoSize = true;
            lblDistanceThreshold->Height = 60;

            trackDistanceThreshold = gcnew TrackBar();
            trackDistanceThreshold->Location = Point(10, 110);
            trackDistanceThreshold->Width = 180;
            trackDistanceThreshold->Minimum = 1;
            trackDistanceThreshold->Maximum = 20;
            trackDistanceThreshold->Value = 5;
            trackDistanceThreshold->TickFrequency = 1;
            trackDistanceThreshold->ValueChanged += gcnew EventHandler(this, &MainForm::OnDistanceThresholdChanged);

            lblMaxIterations = gcnew Label();
            lblMaxIterations->Text = "Max Iterations: 1000\n"
                "Nombre maximum d'essais pour trouver\n"
                "la meilleure droite dans RANSAC";
            lblMaxIterations->Location = Point(10, 140);
            lblMaxIterations->AutoSize = true;
            lblMaxIterations->Height = 60;

            trackMaxIterations = gcnew TrackBar();
            trackMaxIterations->Location = Point(10, 200);
            trackMaxIterations->Width = 180;
            trackMaxIterations->Minimum = 100;
            trackMaxIterations->Maximum = 2000;
            trackMaxIterations->Value = 1000;
            trackMaxIterations->TickFrequency = 100;
            trackMaxIterations->ValueChanged += gcnew EventHandler(this, &MainForm::OnMaxIterationsChanged);

            lblMinPoints = gcnew Label();
            lblMinPoints->Text = "Min Points: 5\n"
                "Nombre minimum de points alignés\n"
                "pour former un segment de mur";
            lblMinPoints->Location = Point(10, 230);
            lblMinPoints->AutoSize = true;
            lblMinPoints->Height = 60;

            trackMinPoints = gcnew TrackBar();
            trackMinPoints->Location = Point(10, 290);
            trackMinPoints->Width = 180;
            trackMinPoints->Minimum = 3;
            trackMinPoints->Maximum = 20;
            trackMinPoints->Value = 5;
            trackMinPoints->TickFrequency = 1;
            trackMinPoints->ValueChanged += gcnew EventHandler(this, &MainForm::OnMinPointsChanged);

            lblMinLength = gcnew Label();
            lblMinLength->Text = "Min Length (cm): 20\n"
                "Longueur minimale d'un segment de mur\n"
                "pour être considéré comme valide";
            lblMinLength->Location = Point(10, 320);
            lblMinLength->AutoSize = true;
            lblMinLength->Height = 60;

            trackMinLength = gcnew TrackBar();
            trackMinLength->Location = Point(10, 380);
            trackMinLength->Width = 180;
            trackMinLength->Minimum = 5;
            trackMinLength->Maximum = 50;
            trackMinLength->Value = 20;
            trackMinLength->TickFrequency = 5;
            trackMinLength->ValueChanged += gcnew EventHandler(this, &MainForm::OnMinLengthChanged);

            // Ajouter les contrôles au panel des paramètres
            paramPanel->Controls->Add(lblTitle);
            paramPanel->Controls->Add(lblDistanceThreshold);
            paramPanel->Controls->Add(trackDistanceThreshold);
            paramPanel->Controls->Add(lblMaxIterations);
            paramPanel->Controls->Add(trackMaxIterations);
            paramPanel->Controls->Add(lblMinPoints);
            paramPanel->Controls->Add(trackMinPoints);
            paramPanel->Controls->Add(lblMinLength);
            paramPanel->Controls->Add(trackMinLength);

            // Ajouter tous les contrôles à la forme
            this->Controls->Add(panelLidar);
            this->Controls->Add(headerPanel);
            this->Controls->Add(textBoxLog);
            this->Controls->Add(paramPanel);
        }

        // Gestionnaires d'événements pour les paramètres RANSAC
        void OnDistanceThresholdChanged(Object^ sender, EventArgs^ e) {
            float value = trackDistanceThreshold->Value / 100.0f;
            String^ currentText = lblDistanceThreshold->Text;
            array<String^>^ lines = currentText->Split('\n');
            lblDistanceThreshold->Text = String::Format("Distance Threshold (cm): {0}\n{1}\n{2}", 
                trackDistanceThreshold->Value, lines[1], lines[2]);
            lidarDroit->SetDistanceThreshold(value);
            UpdateDetection();
        }

        void OnMaxIterationsChanged(Object^ sender, EventArgs^ e) {
            int value = trackMaxIterations->Value;
            String^ currentText = lblMaxIterations->Text;
            array<String^>^ lines = currentText->Split('\n');
            lblMaxIterations->Text = String::Format("Max Iterations: {0}\n{1}\n{2}", 
                value, lines[1], lines[2]);
            lidarDroit->SetMaxIterations(value);
            UpdateDetection();
        }

        void OnMinPointsChanged(Object^ sender, EventArgs^ e) {
            int value = trackMinPoints->Value;
            String^ currentText = lblMinPoints->Text;
            array<String^>^ lines = currentText->Split('\n');
            lblMinPoints->Text = String::Format("Min Points: {0}\n{1}\n{2}", 
                value, lines[1], lines[2]);
            lidarDroit->SetMinPointsForLine(value);
            UpdateDetection();
        }

        void OnMinLengthChanged(Object^ sender, EventArgs^ e) {
            float value = trackMinLength->Value / 100.0f;
            String^ currentText = lblMinLength->Text;
            array<String^>^ lines = currentText->Split('\n');
            lblMinLength->Text = String::Format("Min Length (cm): {0}\n{1}\n{2}", 
                trackMinLength->Value, lines[1], lines[2]);
            lidarDroit->SetMinSegmentLength(value);
            UpdateDetection();
        }

        void UpdateDetection() {
            if (panelLidar->Visualization->StoredPoints->Count > 0) {
                List<WallSegment^>^ walls = lidarDroit->DetectWalls(panelLidar->Visualization->StoredPoints);
                List<PointF>^ intersections = lidarDroit->FindIntersections(walls);
                panelLidar->SetWallSegments(walls);
                panelLidar->SetIntersections(intersections);
                ThreadSafeLog(String::Format("Murs détectés: {0}, Intersections: {1}", walls->Count, intersections->Count));
            }
        }

        // Gestionnaires d'événements existants
        void btnConnect_Click(Object^ sender, EventArgs^ e) {
            try {
                if (btnConnect->Text == "Connecter") {
                    controller->Connect("192.168.1.60", 1500);
                    btnConnect->Text = "Déconnecter";
                    btnStart->Enabled = true;
                    ThreadSafeLog("Connecté au serveur LIDAR");
                }
                else {
                    controller->Disconnect();
                    btnConnect->Text = "Connecter";
                    btnStart->Enabled = false;
                    btnStop->Enabled = false;
                    ThreadSafeLog("Déconnecté du serveur");
                }
            }
            catch (Exception^ ex) {
                ThreadSafeLog("Erreur: " + ex->Message);
            }
        }

        void btnStart_Click(Object^ sender, EventArgs^ e) {
            controller->StartScan();
            btnStop->Enabled = true;
            btnStart->Enabled = false;
            ThreadSafeLog("Scan démarré");
        }

        void btnStop_Click(Object^ sender, EventArgs^ e) {
            controller->StopScan();
            btnStart->Enabled = true;
            btnStop->Enabled = false;
            ThreadSafeLog("Scan arrêté");
        }

        void btnAvancer_Click(Object^ sender, EventArgs^ e) {
            controller->MoveRobotForward();
            ThreadSafeLog("Bouton Avancer pressé");
        }

        void btnReculer_Click(Object^ sender, EventArgs^ e) {
            controller->MoveRobotBackward();
            ThreadSafeLog("Bouton Reculer pressé");
        }

        void btnGauche_Click(Object^ sender, EventArgs^ e) {
            controller->RotateRobotLeft();
            ThreadSafeLog("Bouton Gauche pressé");
        }

        void btnDroite_Click(Object^ sender, EventArgs^ e) {
            controller->RotateRobotRight();
            ThreadSafeLog("Bouton Droite pressé");
        }

        void btnClear_Click(Object^ sender, EventArgs^ e) {
            controller->ClearMap();
            ThreadSafeLog("Carte effacée");
        }

        void btnTest_Click(Object^ sender, EventArgs^ e) {
            controller->TestConnection();
            ThreadSafeLog("Bouton Test pressé");
        }

        void btnRetourArriere_Click(Object^ sender, EventArgs^ e) {
            controller->RetourArriere();
            ThreadSafeLog("Retour arrière effectué");
        }

        void btnDroitesLIDAR_Click(Object^ sender, EventArgs^ e) {
            ThreadSafeLog("Lancement de la detection des droites LIDAR...");
            btnDroitesLIDAR->Enabled = false;

            Thread^ detectionThread = gcnew Thread(gcnew ThreadStart(this, &MainForm::DetectWallsThread));
            detectionThread->IsBackground = true;
            detectionThread->Start();
        }

        void DetectWallsThread() {
            try {
                List<LidarPoint^>^ points = lidarDroit->LoadPointsFromCSV();
                this->Invoke(gcnew Action<String^>(this, &MainForm::ThreadSafeLog), 
                    "Nombre de points chargés depuis le CSV : " + points->Count);

                if (points->Count > 0) {
                    List<WallSegment^>^ walls = lidarDroit->DetectWalls(points);
                    List<PointF>^ intersections = lidarDroit->FindIntersections(walls);

                    this->Invoke(gcnew Action<String^>(this, &MainForm::ThreadSafeLog), 
                        "Nombre de murs détectés : " + walls->Count);
                    this->Invoke(gcnew Action<String^>(this, &MainForm::ThreadSafeLog), 
                        "Nombre d'intersections trouvées : " + intersections->Count);

                    this->Invoke(gcnew Action<List<WallSegment^>^>(panelLidar, &LidarPanel::SetWallSegments), walls);
                    this->Invoke(gcnew Action<List<PointF>^>(panelLidar, &LidarPanel::SetIntersections), intersections);
                    this->Invoke(gcnew Action(panelLidar, &LidarPanel::Invalidate));
                }
            }
            catch (Exception^ ex) {
                this->Invoke(gcnew Action<String^>(this, &MainForm::ThreadSafeLog), 
                    "Erreur lors de la detection des droites : " + ex->Message);
            }
            finally {
                this->Invoke(gcnew Action<bool>(btnDroitesLIDAR, &Button::Enabled::set), true);
            }
        }
    };
}