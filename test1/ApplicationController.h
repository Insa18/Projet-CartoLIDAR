#pragma once
#include "LidarScanner.h"
#include "LidarDataParser.h"
#include "Robot.h"
#include "LidarPanel.h"

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

namespace test1 {
    public ref class ApplicationController {
    private:
        LidarScanner^ scanner;
        LidarDataParser^ parser;
        Robot^ robot;
        List<LidarPoint^>^ currentPoints;
        LidarPanel^ lidarPanel;
        Action<String^>^ logCallback;
        Form^ parentForm;
        System::Windows::Forms::Timer^ refreshTimer;
        float lastRobotAngle;
        PointF lastRobotPosition;
        List<LidarPoint^>^ previousScanPoints;
        bool isFirstScan;

    public:
        ApplicationController(LidarPanel^ panel, Form^ form) {
            scanner = gcnew LidarScanner();
            parser = gcnew LidarDataParser();
            robot = gcnew Robot();
            currentPoints = gcnew List<LidarPoint^>();
            lidarPanel = panel;
            parentForm = form;
            previousScanPoints = gcnew List<LidarPoint^>();
            lastRobotAngle = 0.0f;
            lastRobotPosition = PointF(0, 0);
            isFirstScan = true;

            refreshTimer = gcnew System::Windows::Forms::Timer();
            refreshTimer->Interval = 100; 
            refreshTimer->Tick += gcnew EventHandler(this, &ApplicationController::OnTimerTick);
        }

        void SetLogCallback(Action<String^>^ callback) {
            logCallback = callback;
            scanner->SetLogCallback(callback);
            scanner->SetParentForm(parentForm);
        }

        void Initialize() {
            lidarPanel->Visualization->UpdateScale(lidarPanel->Width, lidarPanel->Height);
            lidarPanel->SetRobot(robot);
            refreshTimer->Start();
        }

        void Connect(String^ ip, int port) {
            try {
                scanner->Connect(ip, port);
                Log("Tentative de connexion à " + ip + ":" + port);
            }
            catch (Exception^ ex) {
                Log("Erreur de connexion: " + ex->Message);
                throw;
            }
        }

        void Disconnect() {
            scanner->Disconnect();
            Log("Deconnexion effectuee");
        }

        void StartScan() {
            scanner->StartScan();
            isFirstScan = true;
            lastRobotAngle = robot->Angle;
            lastRobotPosition = robot->Position;
            Log("Commande de demarrage du scan envoyee");
        }

        void StopScan() {
            scanner->StopScan();
            Log("Commande d'arrêt du scan envoyee");
        }

        void MoveRobotForward() {
            robot->MoveBackward(45.0f);
            robot->EnregistrerDeplacement("AV 1");  
            SendRobotCommand("AV 1");
            Log("Robot: Avancer");
            UpdateVisualization();
        }

        void MoveRobotBackward() {
            robot->MoveForward(45.0f);
            robot->EnregistrerDeplacement("AR 1");  
            SendRobotCommand("AR 1");
            Log("Robot: Reculer");
            UpdateVisualization();
        }

        void RotateRobotLeft() {
            lastRobotPosition = robot->Position;
            lastRobotAngle = robot->Angle;
            robot->RotateLeft(90.0f);
            robot->EnregistrerDeplacement("AG 9.5");  
            SendRobotCommand("AD 9.5");
            Log("Robot: Rotation à gauche");
            UpdateVisualization();
        }

        void RotateRobotRight() {
            lastRobotPosition = robot->Position;
            lastRobotAngle = robot->Angle;
            robot->RotateRight(90.0f);
            robot->EnregistrerDeplacement("AD 9.5");  
            SendRobotCommand("AG 9.5");
            Log("Robot: Rotation à droite");
            UpdateVisualization();
        }

        void StopRobot() {
            SendRobotCommand("ST");
            Log("Robot: Arrêt");
            UpdateVisualization();
        }

        void ClearMap() {
            if (lidarPanel != nullptr && lidarPanel->Visualization != nullptr) {
                lidarPanel->Visualization->ClearMap();
                lidarPanel->Invalidate();
                isFirstScan = true;
                previousScanPoints->Clear();
            }
        }

        void TestConnection() {
            SendRobotCommand("TEST");
            Log("Test de connexion envoye");
        }

        void RetourArriere() {
            if (robot->Deplacements->Count == 0) {
                Log("Aucun deplacement enregistre à annuler.");
                return;
            }
            PointF startPosition = robot->Position;
            float startAngle = robot->Angle;
            for (int i = robot->Deplacements->Count - 1; i >= 0; --i) {
                String^ deplacement = robot->Deplacements[i];
                array<String^>^ parts = deplacement->Split(' ');
                String^ commande = parts[0];
                String^ parametre = parts->Length > 1 ? parts[1] : "1";

                if (commande == "AV") {
                    robot->MoveForward(30.0f);
                    SendRobotCommand("AR " + parametre);
                    Log("Robot: Retour (inverse de Avancer)");
                    System::Threading::Thread::Sleep(1000);  
                }
                else if (commande == "AR") {
                    robot->MoveBackward(30.0f);
                    SendRobotCommand("AV " + parametre);
                    Log("Robot: Retour (inverse de Reculer)");
                    System::Threading::Thread::Sleep(1000);  
                }
                else if (commande == "AG") {
                    robot->RotateRight(90.0f);
                    SendRobotCommand("AG " + parametre);
                    Log("Robot: Retour (inverse de Gauche)");
                    System::Threading::Thread::Sleep(3000);
                }
                else if (commande == "AD") {
                    robot->RotateLeft(90.0f);
                    SendRobotCommand("AD " + parametre);
                    Log("Robot: Retour (inverse de Droite)");
                    System::Threading::Thread::Sleep(3000); 
                }
                UpdateVisualization();
            }
            robot->Deplacements->Clear();
            robot->Path->Clear();
            robot->Path->Add(startPosition);
            
            Log("Retour arrière termine.");
        }
        void ExecuteDroitesLIDAR() {
            try {
                System::Diagnostics::Process^ process = gcnew System::Diagnostics::Process();
                process->StartInfo->FileName = "python";  
                process->StartInfo->Arguments = "droitesLIDAR.py";
                process->StartInfo->UseShellExecute = false;
                process->StartInfo->RedirectStandardOutput = true;
                process->StartInfo->CreateNoWindow = true;
                
                Log("Lancement du script droitesLIDAR.py...");
                process->Start();
                String^ output = process->StandardOutput->ReadToEnd();
                if (!String::IsNullOrEmpty(output)) {
                    Log("Resultat de droitesLIDAR.py: " + output);
                }
                
                process->WaitForExit();
                Log("Script droitesLIDAR.py termine avec le code de sortie: " + process->ExitCode);
            }
            catch (Exception^ ex) {
                Log("Erreur lors de l'execution de droitesLIDAR.py: " + ex->Message);
            }
        }
    private:
        void SendRobotCommand(String^ command) {
            if (scanner->IsConnected) {
                try {
                    scanner->SendCommand(command);
                    Log("Commande envoyee: " + command);
                }
                catch (Exception^ ex) {
                    Log("Erreur lors de l'envoi de la commande: " + ex->Message);
                }
            }
            else {
                Log("Impossible d'envoyer la commande: non connecte");
            }
        }

        void Log(String^ message) {
            if (logCallback != nullptr) {
                logCallback(message);
            }
        }

        void OnTimerTick(Object^ sender, EventArgs^ e) {
            LoadLidarData();
        }
        void LoadLidarData() {
            String^ dataString = scanner->buffer_receiv_ladar;
            if (String::IsNullOrEmpty(dataString))
                return;
            scanner->buffer_receiv_ladar = String::Empty;
            if (dataString->Contains("A55A")) {
                int startIdx = dataString->IndexOf("A55A");
                if (startIdx > 0) {
                    dataString = dataString->Substring(startIdx);
                }
                List<LidarPoint^>^ newPoints = parser->ParseData(dataString);
                if (newPoints != nullptr && newPoints->Count > 0) {
                    bool rotationDetected = Math::Abs(robot->Angle - lastRobotAngle) > 1.0f;
                    if (isFirstScan) {
                        lastRobotAngle = robot->Angle;
                        lastRobotPosition = robot->Position;
                        isFirstScan = false;
                    }
                    for each (LidarPoint^ point in newPoints) {
                        TransformPointToRobotFrame(point, robot, rotationDetected);
                    }
                    Log("Points traites: " + newPoints->Count);
                    lidarPanel->AddPoints(newPoints);
                    previousScanPoints->Clear();
                    previousScanPoints->AddRange(newPoints);
                    currentPoints = newPoints;
                }
            }
        }
        void TransformPointToRobotFrame(LidarPoint^ point, Robot^ robot, bool rotationDetected) {
            float robotAngleRad = robot->Angle * (float)Math::PI / 180.0f;
            float originalX = point->relativeX;
            float originalY = point->relativeY;

            if (rotationDetected) {
                float lastAngleRad = lastRobotAngle * (float)Math::PI / 180.0f;
                float deltaAngleRad = robotAngleRad - lastAngleRad;
                Log(String::Format("Correction de rotation: {0:F2} degres", deltaAngleRad * 180.0f / (float)Math::PI));
                float correctedX = originalX * cos(-deltaAngleRad) - originalY * sin(-deltaAngleRad);
                float correctedY = originalX * sin(-deltaAngleRad) + originalY * cos(-deltaAngleRad);
                
                originalX = correctedX;
                originalY = correctedY;
            }

            // Calculer les coordonnées dans le repère monde
            float worldX = originalX * cos(robotAngleRad) - originalY * sin(robotAngleRad);
            float worldY = originalX * sin(robotAngleRad) + originalY * cos(robotAngleRad);

            // Ajouter la position du robot aux coordonnées relatives
            point->relativeX = worldX + robot->Position.X;
            point->relativeY = worldY + robot->Position.Y;

            // Créer un nouveau point avec les coordonnées du robot actuelles pour le CSV
            LidarPoint^ newPoint = gcnew LidarPoint(point->angle, point->distance, robot->Position.X, robot->Position.Y);
        }
        void UpdateVisualization() {
            if (lidarPanel != nullptr && lidarPanel->Visible) {
                lidarPanel->Invalidate();
            }
        }
    };
}