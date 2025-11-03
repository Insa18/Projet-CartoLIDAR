#pragma once
#include <msclr/marshal_cppstd.h>
#include <string>

using namespace System;
using namespace System::Net::Sockets;
using namespace System::Text;
using namespace System::Threading;

namespace test1 {
    // Declaration du delegue pour l'evenement
    public delegate void ScanCompleteEventHandler(Object^ sender, EventArgs^ e);

    public ref class LidarScanner {
    private:
        TcpClient^ tcpClient;
        NetworkStream^ networkStream;
        Thread^ receiveThread;
        Thread^ scanCycleThread; // Thread pour la boucle de scan
        bool isConnected;
        bool isScanRunning;
        bool stopScanCycle; // Contrôle de la boucle de scan
        Form^ parentForm;
        Action<String^>^ logCallback;
        Object^ dataLock; // Verrou pour la synchronisation des donnees
        
        // Declaration de l'evenement comme membre prive
        ScanCompleteEventHandler^ scanCompletedEvent;

    public:
        // Proprietes pour l'evenement
        event ScanCompleteEventHandler^ ScanCompleted {
            void add(ScanCompleteEventHandler^ handler) {
                scanCompletedEvent = static_cast<ScanCompleteEventHandler^>(
                    Delegate::Combine(scanCompletedEvent, handler));
            }
            void remove(ScanCompleteEventHandler^ handler) {
                scanCompletedEvent = static_cast<ScanCompleteEventHandler^>(
                    Delegate::Remove(scanCompletedEvent, handler));
            }
        }

        property bool IsConnected { 
            bool get() { return isConnected; }
        }

        property bool IsScanRunning {
            bool get() { return isScanRunning; }
        }

        String^ buffer_receiv_ladar;
        String^ fullReceivedData;
        String^ accumulatedData; // Donnees accumulees avant traitement

        LidarScanner() {
            isConnected = false;
            isScanRunning = false;
            stopScanCycle = false;
            buffer_receiv_ladar = String::Empty;
            fullReceivedData = String::Empty;
            accumulatedData = String::Empty;
            dataLock = gcnew Object();
            scanCompletedEvent = nullptr;
        }

        void SetLogCallback(Action<String^>^ callback) {
            logCallback = callback;
        }

        void SetParentForm(Form^ form) {
            parentForm = form;
        }

        void Connect(String^ ip, int port) {
            if (!isConnected) {
                try {
                    tcpClient = gcnew TcpClient();
                    tcpClient->Connect(ip, port);
                    if (tcpClient->Connected) {
                        networkStream = tcpClient->GetStream();
                        isConnected = true;
                        receiveThread = gcnew Thread(gcnew ThreadStart(this, &LidarScanner::ReceiveData));
                        receiveThread->IsBackground = true;
                        receiveThread->Start();
                    }
                }
                catch (Exception^ ex) {
                    throw gcnew Exception("Erreur de connexion: " + ex->Message);
                }
            }
        }

        void Disconnect() {
            if (isConnected) {
                isConnected = false;
                stopScanCycle = true;
                
                if (scanCycleThread != nullptr && scanCycleThread->IsAlive) {
                    scanCycleThread->Join(1000); // Attendre au maximum 1 seconde
                    if (scanCycleThread->IsAlive) {
                        scanCycleThread->Abort(); // Forcer l'arrêt si necessaire
                    }
                }
                
                if (networkStream != nullptr) networkStream->Close();
                if (tcpClient != nullptr) tcpClient->Close();
                if (receiveThread != nullptr && receiveThread->IsAlive) receiveThread->Join();
                
                isScanRunning = false;
            }
        }

        void StartScan() {
            if (isConnected) {
                isScanRunning = true;
                stopScanCycle = false;
                
                // Vider les buffers
                Monitor::Enter(dataLock);
                try {
                    accumulatedData = String::Empty;
                    buffer_receiv_ladar = String::Empty;
                }
                finally {
                    Monitor::Exit(dataLock);
                }
                
                if (logCallback != nullptr && parentForm != nullptr) {
                    parentForm->Invoke(logCallback, "Commande de demarrage du scan envoyee");
                    parentForm->Invoke(logCallback, "Scan demarre");
                }
                
                // Demarrer le thread de boucle de scan
                scanCycleThread = gcnew Thread(gcnew ThreadStart(this, &LidarScanner::ScanCycle));
                scanCycleThread->IsBackground = true;
                scanCycleThread->Start();
            }
        }

        void StopScan() {
            if (isConnected) {
                stopScanCycle = true;
                isScanRunning = false;
                
                // Arrêter le thread de la boucle de scan s'il est en cours d'execution
                if (scanCycleThread != nullptr && scanCycleThread->IsAlive) {
                    scanCycleThread->Join(1000); // Attendre au maximum 1 seconde
                    if (scanCycleThread->IsAlive) {
                        scanCycleThread->Abort(); // Forcer l'arrêt si necessaire
                    }
                }
                
                // Envoyer la commande d'arrêt plusieurs fois pour s'assurer qu'elle est bien reçue
                for (int i = 0; i < 4; i++) {
                    SendCommand("AZ");
                    Thread::Sleep(250); // Attendre 250ms entre chaque envoi
                }
            }
        }

        void SendCommand(String^ command) {
            if (isConnected && networkStream != nullptr) {
                // Ajouter un retour chariot et un saut de ligne à la commande
                command = command + "\r\n";
                array<Byte>^ bytes = Encoding::ASCII->GetBytes(command);
                networkStream->Write(bytes, 0, bytes->Length);
            }
        }

        // Methode pour forcer le traitement des donnees accumulees
        bool ProcessData() {
            bool dataProcessed = false;
            
            Monitor::Enter(dataLock);
            try {
                // Verifier si nous avons des donnees accumulees contenant un paquet complet
                if (!String::IsNullOrEmpty(accumulatedData) && accumulatedData->Contains("A55A")) {
                    // Transferer vers le buffer de traitement
                    buffer_receiv_ladar = accumulatedData;
                    fullReceivedData += accumulatedData;
                    
                    // Loguer l'information
                    if (logCallback != nullptr && parentForm != nullptr) {
                        parentForm->Invoke(logCallback, "Trame complète prête à traiter: " + accumulatedData->Length + " octets");
                    }
                    
                    // Vider le buffer d'accumulation
                    accumulatedData = String::Empty;
                    dataProcessed = true;
                }
            }
            finally {
                Monitor::Exit(dataLock);
            }
            
            // Declencher l'evenement de scan complete si des donnees ont ete traitees
            if (dataProcessed && scanCompletedEvent != nullptr) {
                scanCompletedEvent(this, EventArgs::Empty);
            }
            
            return dataProcessed;
        }

    private:
        // Methode pour la boucle de scan
        void ScanCycle() {
            while (isConnected && isScanRunning && !stopScanCycle) {
                try {
                    // Envoyer la commande AX pour demarrer le scan
                    SendCommand("AX");
                    
                    if (logCallback != nullptr && parentForm != nullptr) {
                        parentForm->Invoke(logCallback, "Demarrage d'un cycle de scan (AX)");
                    }
                    
                    // Attendre que des donnees soient collectees (4 secondes)
                    DateTime startTime = DateTime::Now;
                    
                    // Attendre pendant la periode de collecte (4 secondes)
                    while (DateTime::Now.Subtract(startTime).TotalSeconds < 4.0 && !stopScanCycle) {
                        Thread::Sleep(100); // Petit delai pour ne pas bloquer la CPU
                    }
                    
                    // Envoyer la commande AZ pour arrêter le scan
                    SendCommand("AZ");
                    
                    if (logCallback != nullptr && parentForm != nullptr) {
                        parentForm->Invoke(logCallback, "Fin du cycle de scan (AZ)");
                    }
                    
                    // Attendre que les dernières donnees soient reçues (100ms)
                    Thread::Sleep(100);
                    
                    // Forcer le traitement des donnees accumulees
                    bool processedData = ProcessData();
                    
                    if (logCallback != nullptr && parentForm != nullptr) {
                        if (processedData) {
                            parentForm->Invoke(logCallback, "Donnees du cycle traitees avec succès");
                        } else {
                            parentForm->Invoke(logCallback, "Pas de donnees valides dans ce cycle");
                        }
                    }
                    
                    // Petit delai avant de recommencer la boucle
                    Thread::Sleep(100);
                }
                catch (ThreadAbortException^) {
                    // Ignorer si le thread est interrompu
                    break;
                }
                catch (Exception^ ex) {
                    // Log de l'erreur
                    if (logCallback != nullptr && parentForm != nullptr) {
                        try {
                            parentForm->Invoke(logCallback, "Erreur dans ScanCycle: " + ex->Message);
                        }
                        catch (Exception^) {
                            // Ignorer les erreurs d'invocation
                        }
                    }
                }
            }
        }

        void ReceiveData() {
            array<Byte>^ buffer = gcnew array<Byte>(8096);
            DateTime lastTransferTime = DateTime::Now;
            
            while (isConnected) {
                try {
                    if (networkStream->DataAvailable) {
                        int bytesRead = networkStream->Read(buffer, 0, buffer->Length);
                        if (bytesRead > 0) {
                            String^ receivedData = Encoding::ASCII->GetString(buffer, 0, bytesRead);
                            
                            Monitor::Enter(dataLock);
                            try {
                                accumulatedData += receivedData;
                                
                            }
                            finally {
                                Monitor::Exit(dataLock);
                            }
                            lastTransferTime = DateTime::Now;
                        }
                    }
                    else {
                        Thread::Sleep(5);
                    }
                }
                catch (Exception^ ex) {
                    if (logCallback != nullptr && parentForm != nullptr) {
                        try {
                            parentForm->Invoke(logCallback, "Erreur dans ReceiveData: " + ex->Message);
                        }
                        catch (Exception^) {
                        }
                    }
                }
            }
        }
    };
}