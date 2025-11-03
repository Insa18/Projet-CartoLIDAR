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


        TcpClient^ tcpClient;
        NetworkStream^ networkStream;
        Thread^ receiveThread;
        String^ buffer_receiv_ladar;
        String^ fullReceivedData;

        PointF CalculatePoint(LidarPoint^ point) {
            float rad = point->angle * (float)Math::PI / 180.0f;
            float scaledDist = point->distance * scaleFactor;
            return PointF(
                scaledDist * cos(rad) + origin.X,
                scaledDist * sin(rad) + origin.Y
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

                        btnConnect->Text = "Déconnecter";
                        btnStart->Enabled = true;
                        LogMessage("Connecté au serveur LIDAR");
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
                LogMessage("Déconnecté du serveur");
            }
        }

        void btnStart_Click(Object^ sender, EventArgs^ e) {
            if (isConnected) {
                SendCommand("AX"); 
                btnStop->Enabled = true;
                btnStart->Enabled = false;
                LogMessage("Scan démarré");
            }
        }

        void btnStop_Click(Object^ sender, EventArgs^ e) {
            if (isConnected) {
                SendCommand("AZ"); 
                btnStart->Enabled = true;
                btnStop->Enabled = false;
                LogMessage("Scan arrêté");
            }
        }

        void UpdateScale() {
            if (panelLidar->Width > 0 && panelLidar->Height > 0) {
                origin = PointF(panelLidar->Width / 2.0f, panelLidar->Height / 2.0f);
                scaleFactor = Math::Min(panelLidar->Width, panelLidar->Height) / 6000.0f;
            }
        }

        void InitializeComponents() {
            this->Text = "Affichage RPLIDAR A1";
            this->Size = Drawing::Size(800, 700);

            headerPanel = gcnew Panel();
            headerPanel->Dock = DockStyle::Top;
            headerPanel->Height = 40;
            headerPanel->BackColor = Color::FromArgb(240, 240, 240);

            btnConnect = gcnew Button();
            btnConnect->Text = "Connecter";
            btnConnect->Location = Point(10, 5);
            btnConnect->Click += gcnew EventHandler(this, &MyForm::btnConnect_Click);

            btnStart = gcnew Button();
            btnStart->Text = "Démarrer Scan";
            btnStart->Location = Point(120, 5);
            btnStart->Enabled = false;
            btnStart->Click += gcnew EventHandler(this, &MyForm::btnStart_Click);

            btnStop = gcnew Button();
            btnStop->Text = "Arrêter Scan";
            btnStop->Location = Point(230, 5);
            btnStop->Enabled = false;
            btnStop->Click += gcnew EventHandler(this, &MyForm::btnStop_Click);

            headerPanel->Controls->Add(btnConnect);
            headerPanel->Controls->Add(btnStart);
            headerPanel->Controls->Add(btnStop);

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

            if (lidarPoints != nullptr && lidarPoints->Count > 0) {
                DrawPointsOnPersistentImage(g, lidarPoints);
            }

            delete g;
        }

        void LoadLidarData() {
            String^ dataString = fullReceivedData;
            List<LidarPoint^>^ newPoints = ParseLidarData(dataString);

            if (newPoints->Count > 0) {
                lidarPoints = newPoints;
                RedrawPersistentImage();
            }
        }

        void ThreadSafeLog(String^ message) {
            textBoxLog->AppendText(DateTime::Now.ToString("[HH:mm:ss] ") + message + Environment::NewLine);
        }
        void DrawPointsOnPersistentImage(Graphics^ g, List<LidarPoint^>^ points) {
            if (points->Count < 2) return;

            array<bool>^ connected = gcnew array<bool>(points->Count);
            float angleThreshold = 2.0f;
            float distanceThreshold = 100.0f;

            List<PointF>^ linePoints = gcnew List<PointF>();
            for (int i = 0; i < points->Count - 1; i++) {
                LidarPoint^ current = points[i];
                LidarPoint^ next = points[i + 1];

                float angleDiff = Math::Abs(next->angle - current->angle);
                angleDiff = angleDiff > 180.0f ? 360.0f - angleDiff : angleDiff;
                float distanceDiff = Math::Abs(next->distance - current->distance);

                if (angleDiff <= angleThreshold && distanceDiff <= distanceThreshold) {
                    linePoints->Add(CalculatePoint(current));
                    linePoints->Add(CalculatePoint(next));
                    connected[i] = connected[i + 1] = true;
                }
            }

            if (linePoints->Count > 0) {
                g->DrawLines(linePen, linePoints->ToArray());
            }

            for (int i = 0; i < points->Count; i++) {
                if (!connected[i]) DrawPoint(g, points[i]);
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
                if (point != nullptr) points->Add(point);
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
        }
    };
}