//drawn ok 

#pragma once
#include <string>
#include <cmath>
#include <iostream>
#include <msclr/marshal_cppstd.h>
#include <algorithm>

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;
using namespace System::Collections::Generic;
using namespace System::Drawing::Drawing2D;

using namespace System::Net::Sockets;
using namespace System::Text;
using namespace System::Threading;

namespace test1 {
    public ref class LidarPoint {
    public:
        float angle;
        float distance;

        LidarPoint(float a, float d) : angle(a), distance(d) {}
    };

    public ref class DoubleBufferedPanel : public Panel {
    public:
        DoubleBufferedPanel() {
            this->DoubleBuffered = true;
        }
    };

    public ref class MyForm : public Form {
    private:
        Panel^ panelLidar;
        Panel^ headerPanel;
        System::Windows::Forms::Timer^ timer;
        List<LidarPoint^>^ lidarPoints;
        Bitmap^ persistentImage;
        Graphics^ persistentGraphics;
        Button^ btnConnect;
        Button^ btnStart;
        Button^ btnStop;
        TextBox^ textBoxLog;
        Pen^ linePen;
        SolidBrush^ pointBrush;
        Pen^ outlinePen;
        PointF origin;
        float scaleFactor;
        bool isConnected;
        bool isScanRunning;
        List<PointF>^ mapPoints;

        // Ajout des boutons de contrôle
        Button^ btnAvancer;
        Button^ btnReculer;
        Button^ btnGauche;
        Button^ btnDroite;

        TcpClient^ tcpClient;
        NetworkStream^ networkStream;
        Thread^ receiveThread;
        String^ buffer_receiv_ladar;
        String^ fullReceivedData;

        // Ajouter ces variables pour le zoom
        float zoomLevel;
        float minZoom;
        float maxZoom;
        PointF panOffset;
        float robotAngle; // Angle actuel du robot en degres
        PointF robotPosition; // Position actuelle du robot

        // AJOUT : Liste pour stocker le chemin du robot
        List<PointF>^ robotPath;

        bool isPanning;
        Point lastMousePos;

        PointF CalculatePoint(LidarPoint^ point) {
            float rad = point->angle * (float)Math::PI / 180.0f;
            float scaledDist = point->distance * scaleFactor;
            return PointF(
                scaledDist * cos(rad) ,
                scaledDist * sin(rad)
                
            );
        }
        void SendCommand(String^ command) {
            array<Byte>^ bytes = Encoding::ASCII->GetBytes(command);
            networkStream->Write(bytes, 0, bytes->Length);
        }
        void ReceiveData() {
            array<Byte>^ buffer = gcnew array<Byte>(8096);

            while (isConnected) {
                if (networkStream->DataAvailable) {
                    int bytesRead = networkStream->Read(buffer, 0, buffer->Length);
                    String^ receivedData = Encoding::ASCII->GetString(buffer, 0, bytesRead);

                    buffer_receiv_ladar += receivedData;
                    fullReceivedData += receivedData;

                    this->Invoke(gcnew Action<String^>(this, &MyForm::ThreadSafeLog), receivedData);
                }
                Thread::Sleep(10);
            }
        }
        void DrawPoint(Graphics^ graphics, LidarPoint^ point) {
            PointF p = CalculatePoint(point);
            graphics->FillEllipse(pointBrush, p.X - 2.0f, p.Y - 2.0f, 4.0f, 4.0f);
            graphics->DrawEllipse(outlinePen, p.X - 2.0f, p.Y - 2.0f, 4.0f, 4.0f);
        }

        void InitializeDrawingTools() {
            linePen = gcnew Pen(Color::FromArgb(200, 255, 0, 0), 1.5f);
            pointBrush = gcnew SolidBrush(Color::FromArgb(200, 255, 255, 255));
            outlinePen = gcnew Pen(Color::FromArgb(200, 255, 0, 0), 0.8f);
        }

        static int CompareLidarPoints(LidarPoint^ a, LidarPoint^ b) {
            return a->angle.CompareTo(b->angle);
        }

        void LogMessage(String^ message) {
            if (textBoxLog->Lines->Length > 100) {
                textBoxLog->Clear();
            }
            textBoxLog->AppendText(message + Environment::NewLine);
            textBoxLog->ScrollToCaret();
        }
        void btnConnect_Click(Object^ sender, EventArgs^ e) {
            if (!isConnected) {
                try {
                    tcpClient = gcnew TcpClient();
                    tcpClient->Connect("192.168.1.60", 1500);
                    if (tcpClient->Connected) {
                        networkStream = tcpClient->GetStream();
                        isConnected = true;
                        receiveThread = gcnew Thread(gcnew ThreadStart(this, &MyForm::ReceiveData));
                        receiveThread->IsBackground = true;
                        receiveThread->Start();

                        btnConnect->Text = "Deconnecter";
                        btnStart->Enabled = true;
                        LogMessage("Connecte au serveur LIDAR");
                    }
                }
                catch (Exception^ ex) {
                    LogMessage("Erreur de connexion: " + ex->Message);
                }
            }
            else {
                isConnected = false;
                if (networkStream != nullptr) networkStream->Close();
                if (tcpClient != nullptr) tcpClient->Close();
                if (receiveThread != nullptr && receiveThread->IsAlive) receiveThread->Join();

                btnConnect->Text = "Connecter";
                btnStart->Enabled = false;
                btnStop->Enabled = false;
                LogMessage("Deconnecte du serveur");
            }
        }

        void btnStart_Click(Object^ sender, EventArgs^ e) {
            if (isConnected) {
                SendCommand("AX");
                btnStop->Enabled = true;
                btnStart->Enabled = false;
                LogMessage("Scan demarre");
            }
        }

        void btnStop_Click(Object^ sender, EventArgs^ e) {
            if (isConnected) {
                SendCommand("AZ");
                btnStart->Enabled = true;
                btnStop->Enabled = false;
                LogMessage("Scan arrête");
            }
        }

        void btnAvancer_Click(Object^ sender, EventArgs^ e) {
            // Calculer le deplacement en fonction de l'angle
            float angleRad = robotAngle * (float)Math::PI / 180.0f;
            float moveDistance = 20.0f; // Distance de deplacement en pixels
            
            // Mettre à jour la position du robot
            robotPosition.X += moveDistance * cos(angleRad);
            robotPosition.Y += moveDistance * sin(angleRad);

            // AJOUT : Ajouter la nouvelle position au chemin
            robotPath->Add(robotPosition);

            RedrawPersistentImage();
            panelLidar->Invalidate();
            LogMessage("Avancer");
            if (isConnected) {
                SendCommand("AR 1");
            }
        }

        void btnReculer_Click(Object^ sender, EventArgs^ e) {
            // Calculer le deplacement en fonction de l'angle
            float angleRad = robotAngle * (float)Math::PI / 180.0f;
            float moveDistance = 20.0f; // Distance de deplacement en pixels
            
            // Mettre à jour la position du robot (dans la direction opposee)
            robotPosition.X -= moveDistance * cos(angleRad);
            robotPosition.Y -= moveDistance * sin(angleRad);

            // AJOUT : Ajouter la nouvelle position au chemin
            robotPath->Add(robotPosition);

            RedrawPersistentImage();
            panelLidar->Invalidate();
            LogMessage("Reculer");
            if (isConnected) {
                SendCommand("AV 1");
            }
        }

        void btnGauche_Click(Object^ sender, EventArgs^ e) {
            robotAngle -= 90.0f; // Tourner de 90 degres à gauche

            // AJOUT : Ajouter la position courante au chemin (pour garder la trace même lors des rotations)
            robotPath->Add(robotPosition);

            RedrawPersistentImage();
            panelLidar->Invalidate();
            LogMessage("Rotation à gauche");
            if (isConnected) {
                SendCommand("AG 10");
            }
        }

        void btnDroite_Click(Object^ sender, EventArgs^ e) {
            robotAngle += 90.0f; // Tourner de 90 degres à droite

            // AJOUT : Ajouter la position courante au chemin (pour garder la trace même lors des rotations)
            robotPath->Add(robotPosition);

            RedrawPersistentImage();
            panelLidar->Invalidate();
            LogMessage("Rotation à droite");
            if (isConnected) {
                SendCommand("AD 10");
            }
        }

        void UpdateScale() {
            if (panelLidar->Width > 0 && panelLidar->Height > 0) {
                origin = PointF(panelLidar->Width / 2.0f, panelLidar->Height / 2.0f);
                scaleFactor = Math::Min(panelLidar->Width, panelLidar->Height) / 2000.0f;
            }
        }

        void InitializeComponents() {
            this->Text = "Affichage RPLIDAR A1";
            this->Size = Drawing::Size(1200, 1000);

            headerPanel = gcnew Panel();
            headerPanel->Dock = DockStyle::Top;
            headerPanel->Height = 40;
            headerPanel->BackColor = Color::FromArgb(240, 240, 240);

            btnConnect = gcnew Button();
            btnConnect->Text = "Connecter";
            btnConnect->Location = Point(10, 5);
            btnConnect->Click += gcnew EventHandler(this, &MyForm::btnConnect_Click);

            btnStart = gcnew Button();
            btnStart->Text = "Demarrer Scan";
            btnStart->Location = Point(120, 5);
            btnStart->Enabled = false;
            btnStart->Click += gcnew EventHandler(this, &MyForm::btnStart_Click);

            btnStop = gcnew Button();
            btnStop->Text = "Arreter Scan";
            btnStop->Location = Point(230, 5);
            btnStop->Enabled = false;
            btnStop->Click += gcnew EventHandler(this, &MyForm::btnStop_Click);

            // Ajout des boutons de contrôle
            btnAvancer = gcnew Button();
            btnAvancer->Text = "Avancer";
            btnAvancer->Location = Point(340, 5);
            btnAvancer->Click += gcnew EventHandler(this, &MyForm::btnAvancer_Click);

            btnReculer = gcnew Button();
            btnReculer->Text = "Reculer";
            btnReculer->Location = Point(430, 5);
            btnReculer->Click += gcnew EventHandler(this, &MyForm::btnReculer_Click);

            btnGauche = gcnew Button();
            btnGauche->Text = "Gauche";
            btnGauche->Location = Point(520, 5);
            btnGauche->Click += gcnew EventHandler(this, &MyForm::btnGauche_Click);

            btnDroite = gcnew Button();
            btnDroite->Text = "Droite";
            btnDroite->Location = Point(610, 5);
            btnDroite->Click += gcnew EventHandler(this, &MyForm::btnDroite_Click);

            headerPanel->Controls->Add(btnConnect);
            headerPanel->Controls->Add(btnStart);
            headerPanel->Controls->Add(btnStop);
            headerPanel->Controls->Add(btnAvancer);
            headerPanel->Controls->Add(btnReculer);
            headerPanel->Controls->Add(btnGauche);
            headerPanel->Controls->Add(btnDroite);

            panelLidar = gcnew Panel();
            panelLidar->Dock = DockStyle::Fill;
            panelLidar->BackColor = Color::Black;
            panelLidar->Paint += gcnew PaintEventHandler(this, &MyForm::OnPaint);
            panelLidar->Resize += gcnew EventHandler(this, &MyForm::OnPanelResize);

            textBoxLog = gcnew TextBox();
            textBoxLog->Multiline = true;
            textBoxLog->Dock = DockStyle::Bottom;
            textBoxLog->Height = 150;
            textBoxLog->ReadOnly = true;
            textBoxLog->ScrollBars = ScrollBars::Vertical;

            this->Controls->Add(panelLidar);
            this->Controls->Add(headerPanel);
            this->Controls->Add(textBoxLog);

            // Ajouter l'evenement de la molette
            panelLidar->MouseWheel += gcnew MouseEventHandler(this, &MyForm::OnMouseWheel);
            
            // Initialiser les valeurs de zoom
            zoomLevel = 1.0f;
            minZoom = 0.1f;
            maxZoom = 5.0f;
            panOffset = PointF(0, 0);

            panelLidar->MouseDown += gcnew MouseEventHandler(this, &MyForm::OnMouseDown);
            panelLidar->MouseMove += gcnew MouseEventHandler(this, &MyForm::OnMouseMove);
            panelLidar->MouseUp   += gcnew MouseEventHandler(this, &MyForm::OnMouseUp);
        }

        void OnPanelResize(Object^ sender, EventArgs^ e) {
            if (panelLidar->Width > 0 && panelLidar->Height > 0) {
                persistentImage = gcnew Bitmap(panelLidar->Width, panelLidar->Height);
                UpdateScale();
                RedrawPersistentImage();
            }
        }

        void RedrawPersistentImage() {
            if (persistentImage == nullptr) return;

            Graphics^ g = Graphics::FromImage(persistentImage);
            g->SmoothingMode = SmoothingMode::AntiAlias;
            g->Clear(panelLidar->BackColor);

            if (mapPoints != nullptr && mapPoints->Count > 0) {
                DrawPointsOnPersistentImage(g, mapPoints);
            }

            delete g;
        }

        void LoadLidarData() {
            String^ dataString = fullReceivedData;
            List<LidarPoint^>^ newPoints = ParseLidarData(dataString);

            if (newPoints->Count > 0) {
            // Effacer les anciens points avant d'ajouter les nouveaux
                mapPoints->Clear();  // <- Ajouter cette ligne
        
         // Pour chaque point, calcule sa position absolue sur la carte
                for each (LidarPoint^ point in newPoints) {
                    float rad = (point->angle + robotAngle) * (float)Math::PI / 180.0f;
                    float x = robotPosition.X + point->distance * cos(rad);
                    float y = robotPosition.Y + point->distance * sin(rad);
                    mapPoints->Add(PointF(x, y));
                }
            lidarPoints = newPoints;
            RedrawPersistentImage();
            }
        }

        void ThreadSafeLog(String^ message) {
            textBoxLog->AppendText(DateTime::Now.ToString("[HH:mm:ss] ") + message + Environment::NewLine);
        }
        void DrawPointsOnPersistentImage(Graphics^ g, List<PointF>^ points) {
            if (points->Count < 2) return;
            
            float centerX = panelLidar->Width / 2.0f;
            float centerY = panelLidar->Height / 2.0f;
            float panelWidth = (float)panelLidar->Width;
            float panelHeight = (float)panelLidar->Height;
            
            // Dessiner la grille avec le zoom
            Pen^ gridPen = gcnew Pen(Color::FromArgb(50, 128, 128, 128));
            float gridSpacing = 20.0f * scaleFactor * zoomLevel;
            int numLines = 500; // Pour aller jusqu'à 1000cm (500 carreaux de chaque côte)
            
            // Definir la zone de la grille
            float gridMinX = centerX + (-numLines * gridSpacing) + panOffset.X;
            float gridMaxX = centerX + (numLines * gridSpacing) + panOffset.X;
            float gridMinY = centerY + (-numLines * gridSpacing) + panOffset.Y;
            float gridMaxY = centerY + (numLines * gridSpacing) + panOffset.Y;

            // Lignes verticales (limitees à la zone de la grille)
            for (int i = -numLines; i <= numLines; i++) {
                float x = centerX + (i * gridSpacing) + panOffset.X;
                g->DrawLine(gridPen, x, gridMinY, x, gridMaxY);
            }
            
            // Lignes horizontales (limitees à la zone de la grille)
            for (int i = -numLines; i <= numLines; i++) {
                float y = centerY + (i * gridSpacing) + panOffset.Y;
                g->DrawLine(gridPen, gridMinX, y, gridMaxX, y);
            }
            
            // Dessiner les axes
            Pen^ axisPen = gcnew Pen(Color::Red, 2.0f);
            g->DrawLine(axisPen, centerX + panOffset.X, 0.0f, centerX + panOffset.X, panelHeight);
            g->DrawLine(axisPen, 0.0f, centerY + panOffset.Y, panelWidth, centerY + panOffset.Y);
            
            // Dessiner un point blanc au centre (0,0)
            SolidBrush^ centerPointBrush = gcnew SolidBrush(Color::White);
            float centerPointSize = 2.0f;
            g->FillEllipse(centerPointBrush, 
                          centerX + panOffset.X - centerPointSize/2, 
                          centerY + panOffset.Y - centerPointSize/2, 
                          centerPointSize, centerPointSize);
            delete centerPointBrush;
            
            // Dessiner les points LIDAR avec le zoom
            if (points != nullptr && points->Count > 0) {
                for each (PointF p in points) {
                    float screenX = centerX + (p.X * scaleFactor * zoomLevel) + panOffset.X;
                    float screenY = centerY - (p.Y * scaleFactor * zoomLevel) + panOffset.Y;
                    SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
                    g->FillEllipse(pointBrush, screenX - 2.0f, screenY - 2.0f, 4.0f, 4.0f);
                    delete pointBrush;
                }
            }
            
            // Position du robot (cercle jaune)
            SolidBrush^ robotBrush = gcnew SolidBrush(Color::Yellow);
            float robotSizeMM = 40.0f; // Taille du robot en millimètres
            float robotRadius = (robotSizeMM * scaleFactor * zoomLevel) / 2.0f; // Convertir en pixels avec le zoom
            float robotCenterX = centerX + (robotPosition.X * scaleFactor * zoomLevel) + panOffset.X;
            float robotCenterY = centerY + (robotPosition.Y * scaleFactor * zoomLevel) + panOffset.Y;
            g->FillEllipse(robotBrush, 
                           robotCenterX - robotRadius, 
                           robotCenterY - robotRadius, 
                           robotRadius * 2, robotRadius * 2);

            // Dessiner le contour du robot
            Pen^ robotOutline = gcnew Pen(Color::Black, 2.0f);
            g->DrawEllipse(robotOutline, 
                           robotCenterX - robotRadius, 
                           robotCenterY - robotRadius, 
                           robotRadius * 2, robotRadius * 2);

            // Dessiner la flèche (avant du robot)
            GraphicsPath^ arrowPath = gcnew GraphicsPath();
            float arrowLength = robotRadius * 1.2f;
            float arrowWidth = robotRadius * 0.7f;
            
            // Convertir l'angle en radians
            float angleRad = robotAngle * (float)Math::PI / 180.0f;
            
            // Points de base de la flèche (non tournee)
            cli::array<PointF>^ arrowPoints = gcnew cli::array<PointF>(7) {
                PointF(-arrowLength/2, -arrowWidth/2),
                PointF(0, -arrowWidth/2),
                PointF(0, -arrowWidth),
                PointF(arrowLength/2, 0),
                PointF(0, arrowWidth),
                PointF(0, arrowWidth/2),
                PointF(-arrowLength/2, arrowWidth/2)
            };
            
            // Appliquer la rotation à chaque point
            for (int i = 0; i < arrowPoints->Length; i++) {
                float x = arrowPoints[i].X;
                float y = arrowPoints[i].Y;
                arrowPoints[i].X = x * cos(angleRad) - y * sin(angleRad) + robotCenterX;
                arrowPoints[i].Y = x * sin(angleRad) + y * cos(angleRad) + robotCenterY;
            }
            
            arrowPath->AddPolygon(arrowPoints);
            g->FillPath(Brushes::Black, arrowPath);

            // Nettoyage
            delete robotBrush;
            delete robotOutline;
            delete arrowPath;

            // AJOUT : Dessiner le chemin du robot (en bleu clair epais)
            if (robotPath != nullptr && robotPath->Count > 1) {
                Pen^ pathPen = gcnew Pen(Color::Aqua, 3.0f);
                for (int i = 1; i < robotPath->Count; ++i) {
                    float x1 = centerX + (robotPath[i-1].X * scaleFactor * zoomLevel) + panOffset.X;
                    float y1 = centerY + (robotPath[i-1].Y * scaleFactor * zoomLevel) + panOffset.Y;
                    float x2 = centerX + (robotPath[i].X * scaleFactor * zoomLevel) + panOffset.X;
                    float y2 = centerY + (robotPath[i].Y * scaleFactor * zoomLevel) + panOffset.Y;
                    g->DrawLine(pathPen, x1, y1, x2, y2);
                }
                delete pathPen;
            }
        }

        List<LidarPoint^>^ ParseLidarData(String^ data) {
            List<LidarPoint^>^ points = gcnew List<LidarPoint^>();
            std::string rawData = msclr::interop::marshal_as<std::string>(data);

            if (rawData.size() < 4 || rawData.substr(0, 4) != "A55A") {
                return points;
            }

            const int packetSize = 10;
            for (size_t pos = 4; pos + packetSize <= rawData.size(); pos += packetSize) {
                std::string packet = rawData.substr(pos, packetSize);
                LidarPoint^ point = DecodePacket(packet);
                if (point != nullptr) {
                    points->Add(point);
                }
            }

            points->Sort(gcnew Comparison<LidarPoint^>(CompareLidarPoints));

            return points;
        }
        LidarPoint^ DecodePacket(const std::string& packet) {
            try {
                if (packet.size() < 10) return nullptr;

                int byte0 = std::stoi(packet.substr(0, 2), nullptr, 16);
                int byte1 = std::stoi(packet.substr(2, 2), nullptr, 16);
                int byte2 = std::stoi(packet.substr(4, 2), nullptr, 16);
                int byte3 = std::stoi(packet.substr(6, 2), nullptr, 16);
                int byte4 = std::stoi(packet.substr(8, 2), nullptr, 16);

                float angle = ((byte2 << 7) | (byte1 & 0x7F)) / 64.0f;
                float distance = ((byte4 << 8) | byte3) / 4.0f;

                return distance >= 0.1f ? gcnew LidarPoint(angle, distance) : nullptr;
            }
            catch (...) {
                return nullptr;
            }
        }
        void StartTimer() {
            timer = gcnew System::Windows::Forms::Timer();
            timer->Interval = 5000;
            timer->Tick += gcnew EventHandler(this, &MyForm::OnTimerTick);
            timer->Start();
        }

        void OnTimerTick(Object^ sender, EventArgs^ e) {
            LoadLidarData();
            panelLidar->Invalidate();
        }

        void OnPaint(Object^ sender, PaintEventArgs^ e) {
            if (persistentImage != nullptr) {
                e->Graphics->DrawImage(persistentImage, 0, 0);
            }
        }

        void OnMouseWheel(Object^ sender, MouseEventArgs^ e) {
            float zoomFactor = 1.1f;
            if (e->Delta < 0) {
                zoomFactor = 1.0f / zoomFactor;
            }

            // Calculer la position de la souris par rapport au centre
            float centerX = panelLidar->Width / 2.0f;
            float centerY = panelLidar->Height / 2.0f;
            float mouseX = e->X - centerX;
            float mouseY = e->Y - centerY;

            // Calculer le nouveau niveau de zoom
            float oldZoom = zoomLevel;
            zoomLevel *= zoomFactor;
            zoomLevel = Math::Max(minZoom, Math::Min(maxZoom, zoomLevel));

            // Ajuster le decalage pour zoomer vers la position de la souris
            panOffset.X += mouseX * (1 - zoomLevel/oldZoom);
            panOffset.Y += mouseY * (1 - zoomLevel/oldZoom);

            // Redessiner
            RedrawPersistentImage();
            panelLidar->Invalidate();
        }

        void OnMouseDown(Object^ sender, MouseEventArgs^ e) {
            if (e->Button == ::MouseButtons::Left) {
                isPanning = true;
                lastMousePos = e->Location;
                panelLidar->Cursor = Cursors::Hand;
            }
        }
        void OnMouseMove(Object^ sender, MouseEventArgs^ e) {
            if (isPanning) {
                panOffset.X += e->X - lastMousePos.X;
                panOffset.Y += e->Y - lastMousePos.Y;
                lastMousePos = e->Location;
                RedrawPersistentImage();
                panelLidar->Invalidate();
            }
        }
        void OnMouseUp(Object^ sender, MouseEventArgs^ e) {
            if (e->Button == ::MouseButtons::Left) {
                isPanning = false;
                panelLidar->Cursor = Cursors::Default;
            }
        }

    public:
        MyForm() {
            InitializeComponents();
            InitializeDrawingTools();
            lidarPoints = gcnew List<LidarPoint^>();
            UpdateScale();
            StartTimer();
            fullReceivedData = String::Empty;
            isConnected = false;
            isScanRunning = false;
            robotAngle = 0.0f; // Initialiser l'angle du robot à 0
            robotPosition = PointF(0, 0); // Initialiser la position du robot au centre

            mapPoints = gcnew List<PointF>();
            // AJOUT : Initialiser la liste du chemin et ajouter la position initiale
            robotPath = gcnew List<PointF>();
            robotPath->Add(robotPosition);
        }
    };
}