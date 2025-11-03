#pragma once
#include "LidarPoint.h"
#include "Robot.h"
#include "LidarDroit.h"
#include <algorithm>
#include <vector>
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::Drawing;
using namespace System::Drawing::Drawing2D;
using namespace System::Collections::Generic;
using namespace System::Windows::Forms;

namespace test1 {
    public ref class LidarVisualization : public System::Windows::Forms::UserControl {    private:
        float zoomLevel;
        float scaleFactor;
        PointF origin;
        PointF panOffset;
        bool needsRedraw;
        List<LidarPoint^>^ storedPoints;
        LidarDroit^ lidarDroit;
        List<WallSegment^>^ detectedWalls;
        List<PointF>^ wallIntersections;
        bool showWalls;
        bool showIntersections;
        float scale;
        PointF offset;
        bool isDragging;
        Point lastMousePos;

    public:
        Bitmap^ persistentImage;
        Graphics^ persistentGraphics;

        LidarVisualization() {
            InitializeComponent();
            zoomLevel = 1.0f;
            scaleFactor = 1.0f;
            origin = PointF(0, 0);
            panOffset = PointF(0, 0);
            needsRedraw = true;
            persistentImage = nullptr;
            persistentGraphics = nullptr;
            storedPoints = gcnew List<LidarPoint^>();
            lidarDroit = gcnew LidarDroit();
            detectedWalls = nullptr;
            wallIntersections = nullptr;
            showWalls = false;
            showIntersections = false;
            scale = 1.0f;
            offset = PointF(0, 0);
            isDragging = false;
            this->DoubleBuffered = true;
        }

        property List<LidarPoint^>^ StoredPoints {
            List<LidarPoint^>^ get() { return storedPoints; }
        }

        void SetLidarPoints(List<LidarPoint^>^ points) {
            storedPoints = points;
            this->Invalidate();
        }

        void SetWallSegments(List<WallSegment^>^ walls) {
            detectedWalls = walls;
            showWalls = true;
            UpdateVisualization();
            this->Invalidate();
        }

        void SetIntersections(List<PointF>^ points) {
            wallIntersections = points;
            showIntersections = true;
            UpdateVisualization();
            this->Invalidate();
        }

        void UpdateScale(int width, int height) {
            if (width > 0 && height > 0) {
                // Sauvegarder l'etat actuel
                float oldZoom = zoomLevel;
                PointF oldPanOffset = panOffset;
                List<LidarPoint^>^ oldPoints = gcnew List<LidarPoint^>(storedPoints);
                List<WallSegment^>^ oldWalls = detectedWalls;
                List<PointF>^ oldIntersections = wallIntersections;
                bool oldShowWalls = showWalls;
                bool oldShowIntersections = showIntersections;

                // Mettre à jour les dimensions
                origin = PointF(width / 2.0f, height / 2.0f);
                scaleFactor = Math::Min(width, height) / 2000.0f;
                
                // Recreer l'image persistante
                if (persistentImage != nullptr) {
                    delete persistentImage;
                    delete persistentGraphics;
                }
                
                persistentImage = gcnew Bitmap(width, height);
                persistentGraphics = Graphics::FromImage(persistentImage);
                persistentGraphics->SmoothingMode = SmoothingMode::AntiAlias;
                
                // Restaurer l'etat
                zoomLevel = oldZoom;
                panOffset = oldPanOffset;
                storedPoints = oldPoints;
                detectedWalls = oldWalls;
                wallIntersections = oldIntersections;
                showWalls = oldShowWalls;
                showIntersections = oldShowIntersections;

                // Redessiner tout
                persistentGraphics->Clear(Color::Black);
                DrawGrid(persistentGraphics);
                
                // Redessiner les points
                if (storedPoints != nullptr && storedPoints->Count > 0) {
                    SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
                    for each (LidarPoint^ point in storedPoints) {
                        float x = origin.X + (point->relativeX * scaleFactor * zoomLevel) + panOffset.X;
                        float y = origin.Y + (point->relativeY * scaleFactor * zoomLevel) + panOffset.Y;
                        persistentGraphics->FillEllipse(pointBrush, x - 2.0f, y - 2.0f, 4.0f, 4.0f);
                    }
                    delete pointBrush;
                }

                // Redessiner les murs et intersections
                if (showWalls && detectedWalls != nullptr) {
                    DrawWalls(persistentGraphics);
                }
            }
        }

        void AddPoints(List<LidarPoint^>^ points) {
            if (persistentImage == nullptr || points == nullptr || points->Count == 0) 
                return;

            // Ajouter les points à notre collection
            storedPoints->AddRange(points);
            // Limiter à un nombre raisonnable pour eviter les problèmes de performance
            if (storedPoints->Count > 10000) {
                storedPoints = storedPoints->GetRange(storedPoints->Count - 5000, 5000);
            }
            // Dessiner les nouveaux points sur l'image persistante
            SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
            for each (LidarPoint^ point in points) {
                // Utiliser les coordonnees relatives au robot pour l'affichage
                float x = origin.X + (point->relativeX * scaleFactor * zoomLevel) + panOffset.X;
                float y = origin.Y + (point->relativeY * scaleFactor * zoomLevel) + panOffset.Y;
                persistentGraphics->FillEllipse(pointBrush, x - 2.0f, y - 2.0f, 4.0f, 4.0f);
            }
            delete pointBrush;
        }

        void DrawMapAndRobot(Graphics^ g, Robot^ robot) {
            if (persistentImage == nullptr) return;
            
            // Dessiner l'image persistante
            g->DrawImage(persistentImage, 0, 0);
            
            // Dessiner le robot par dessus
            if (robot != nullptr) {
                DrawRobot(g, robot);
                DrawRobotPath(g, robot);
            }

            if (showWalls && detectedWalls != nullptr) {
                DrawWalls(g);
            }
        }

        void Zoom(float factor, PointF center) {
            float oldZoom = zoomLevel;
            zoomLevel *= factor;
            zoomLevel = Math::Max(0.1f, Math::Min(5.0f, zoomLevel));
            panOffset.X += center.X * (1 - zoomLevel/oldZoom);
            panOffset.Y += center.Y * (1 - zoomLevel/oldZoom);
            
            // Recreer l'image persistante avec la grille
            if (persistentImage != nullptr) {
                persistentGraphics->Clear(Color::Black);
                DrawGrid(persistentGraphics);
                
                // Redessiner tous les points stockes avec le nouveau zoom
                SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
                for each (LidarPoint^ point in storedPoints) {
                    // Utiliser les coordonnees relatives au robot pour l'affichage
                    float x = origin.X + (point->relativeX * scaleFactor * zoomLevel) + panOffset.X;
                    float y = origin.Y + (point->relativeY * scaleFactor * zoomLevel) + panOffset.Y;
                    persistentGraphics->FillEllipse(pointBrush, x - 2.0f, y - 2.0f, 4.0f, 4.0f);
                }
                delete pointBrush;
            }
        }

        void Pan(float deltaX, float deltaY) {
            panOffset.X += deltaX;
            panOffset.Y += deltaY;
            
            // Recreer l'image persistante avec la grille
            if (persistentImage != nullptr) {
                persistentGraphics->Clear(Color::Black);
                DrawGrid(persistentGraphics);
                
                // Redessiner tous les points stockes avec le nouveau decalage
                SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
                for each (LidarPoint^ point in storedPoints) {
                    // Utiliser les coordonnees relatives au robot pour l'affichage
                    float x = origin.X + (point->relativeX * scaleFactor * zoomLevel) + panOffset.X;
                    float y = origin.Y + (point->relativeY * scaleFactor * zoomLevel) + panOffset.Y;
                    persistentGraphics->FillEllipse(pointBrush, x - 2.0f, y - 2.0f, 4.0f, 4.0f);
                }
                delete pointBrush;
            }
        }

        void ClearMap() {
            if (persistentImage != nullptr) {
                persistentGraphics->Clear(Color::Black);
                DrawGrid(persistentGraphics);
                storedPoints->Clear();  // Vider la collection de points
            }
        }

        void DetectAndShowWalls() {
            if (storedPoints == nullptr || storedPoints->Count == 0) {
                // Si pas de points stockes, essayer de charger depuis le CSV
                storedPoints = lidarDroit->LoadPointsFromCSV();
                if (storedPoints->Count == 0) {
                    return;
                }
            }

            detectedWalls = lidarDroit->DetectWalls(storedPoints);
            wallIntersections = lidarDroit->FindIntersections(detectedWalls);
            showWalls = true;
            UpdateVisualization();
        }

        void HideWalls() {
            showWalls = false;
            UpdateVisualization();
        }

        // Add these new methods to get and set the current state
        float GetCurrentZoom() { return zoomLevel; }
        void SetZoom(float zoom) { 
            zoomLevel = Math::Max(0.1f, Math::Min(5.0f, zoom));
            UpdateVisualization();
        }
        
        PointF GetCurrentOffset() { return panOffset; }
        void SetOffset(PointF offset) { 
            panOffset = offset;
            UpdateVisualization();
        }

        void UpdateRobotPosition(Robot^ robot) {
            if (robot != nullptr) {
                UpdateVisualization();
            }
        }

    protected:
        virtual void OnPaint(PaintEventArgs^ e) override {
            Graphics^ g = e->Graphics;
            g->SmoothingMode = System::Drawing::Drawing2D::SmoothingMode::AntiAlias;

            // Calculer les dimensions du contrôle
            float width = this->ClientSize.Width;
            float height = this->ClientSize.Height;
            float centerX = width / 2.0f;
            float centerY = height / 2.0f;

            // Dessiner la grille et le robot (dejà fait dans DrawMapAndRobot ou autre)

            // Dessiner les points LIDAR en orange
            SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
            for each (LidarPoint^ point in storedPoints) {
                float x = centerX + (point->relativeX + offset.X) * scale;
                float y = centerY + (point->relativeY + offset.Y) * scale;
                g->FillEllipse(pointBrush, x - 2.0f, y - 2.0f, 4.0f, 4.0f);
            }
            delete pointBrush;

            // Dessiner les murs
            if (showWalls && detectedWalls != nullptr) {
              Pen^ wallPen = gcnew Pen(Color::Red, 2.0f);
                for each (WallSegment^ wall in detectedWalls) {
                    float x1 = centerX + (wall->StartX + offset.X) * scale;
                    float y1 = centerY + (wall->StartY + offset.Y) * scale;
                    float x2 = centerX + (wall->EndX + offset.X) * scale;
                    float y2 = centerY + (wall->EndY + offset.Y) * scale;
                    g->DrawLine(wallPen, x1, y1, x2, y2);
                }
            delete wallPen;
            }

            // Dessiner les intersections
            if (showIntersections && wallIntersections != nullptr) {
                for each (PointF intersection in wallIntersections) {
                    float x = centerX + (intersection.X + offset.X) * scale;
                    float y = centerY + (intersection.Y + offset.Y) * scale;
                    g->FillEllipse(Brushes::Green, x - 3.0f, y - 3.0f, 6.0f, 6.0f);
                }
            }
        }

        virtual void OnMouseWheel(MouseEventArgs^ e) override {
            if (e->Delta > 0) {
                scale *= 1.1f;
            }
            else {
                scale /= 1.1f;
            }
            this->Invalidate();
        }

        virtual void OnMouseDown(MouseEventArgs^ e) override {
            if (e->Button == System::Windows::Forms::MouseButtons::Left) {
                isDragging = true;
                lastMousePos = e->Location;
            }
        }

        virtual void OnMouseUp(MouseEventArgs^ e) override {
            if (e->Button == System::Windows::Forms::MouseButtons::Left) {
                isDragging = false;
            }
        }

        virtual void OnMouseMove(MouseEventArgs^ e) override {
            if (isDragging) {
                float dx = (e->X - lastMousePos.X) / scale;
                float dy = (e->Y - lastMousePos.Y) / scale;
                offset.X += dx;
                offset.Y += dy;
                lastMousePos = e->Location;
                this->Invalidate();
            }
        }

    private:
        System::ComponentModel::Container^ components;
#pragma region Windows Form Designer generated code
        void InitializeComponent(void) {
            this->components = gcnew System::ComponentModel::Container();
            this->Size = System::Drawing::Size(800, 600);
            this->Text = L"Visualisation LIDAR";
            this->Padding = System::Windows::Forms::Padding(0);
            this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
        }
#pragma endregion

        void DrawGrid(Graphics^ g) {
            Pen^ gridPen = gcnew Pen(Color::FromArgb(50, 128, 128, 128));
            float gridSpacing = 10.0f * scaleFactor * zoomLevel;
            int numLines = 1000;

            float gridMinX = origin.X + (-numLines * gridSpacing) + panOffset.X;
            float gridMaxX = origin.X + (numLines * gridSpacing) + panOffset.X;
            float gridMinY = origin.Y + (-numLines * gridSpacing) + panOffset.Y;
            float gridMaxY = origin.Y + (numLines * gridSpacing) + panOffset.Y;
            
            for (int i = -numLines; i <= numLines; i++) {
                float x = origin.X + (i * gridSpacing) + panOffset.X;
                g->DrawLine(gridPen, x, gridMinY, x, gridMaxY);
            }

            for (int i = -numLines; i <= numLines; i++) {
                float y = origin.Y + (i * gridSpacing) + panOffset.Y;
                g->DrawLine(gridPen, gridMinX, y, gridMaxX, y);
            }

            delete gridPen;
        }

        void DrawRobot(Graphics^ g, Robot^ robot) {
            float robotWidthMM = 170.0f; 
            float robotHeightMM = 175.0f; 
            float robotRadius = (Math::Max(robotWidthMM, robotHeightMM) * scaleFactor * zoomLevel) / 2.0f;
            float robotCenterX = origin.X + (robot->Position.X * scaleFactor * zoomLevel) + panOffset.X;
            float robotCenterY = origin.Y + (robot->Position.Y * scaleFactor * zoomLevel) + panOffset.Y;
            SolidBrush^ robotBrush = gcnew SolidBrush(Color::Yellow);
            g->FillEllipse(robotBrush, robotCenterX - robotRadius, robotCenterY - robotRadius, robotRadius * 2, robotRadius * 2);
            DrawRobotDirection(g, robot, robotCenterX, robotCenterY, robotRadius);
            delete robotBrush;
        }

        void DrawRobotDirection(Graphics^ g, Robot^ robot, float centerX, float centerY, float radius) {
            GraphicsPath^ arrowPath = gcnew GraphicsPath();
            float arrowLength = radius * 1.2f;
            float arrowWidth = radius * 0.7f;
            float angleRad = (robot->Angle + 180.0f) * (float)Math::PI / 180.0f;

            array<PointF>^ arrowPoints = gcnew array<PointF>(7) {
                PointF(-arrowLength/2, -arrowWidth/2),
                PointF(0, -arrowWidth/2),
                PointF(0, -arrowWidth),
                PointF(arrowLength/2, 0),
                PointF(0, arrowWidth),
                PointF(0, arrowWidth/2),
                PointF(-arrowLength/2, arrowWidth/2)
            };

            for (int i = 0; i < arrowPoints->Length; i++) {
                float x = arrowPoints[i].X;
                float y = arrowPoints[i].Y;
                arrowPoints[i].X = x * cos(angleRad) - y * sin(angleRad) + centerX;
                arrowPoints[i].Y = x * sin(angleRad) + y * cos(angleRad) + centerY;
            }

            arrowPath->AddPolygon(arrowPoints);
            g->FillPath(Brushes::Black, arrowPath);
            delete arrowPath;
        }

        void DrawRobotPath(Graphics^ g, Robot^ robot) {
            if (robot->Path->Count < 2) return;

            Pen^ pathPen = gcnew Pen(Color::Aqua, 3.0f);
            for (int i = 1; i < robot->Path->Count; i++) {
                float x1 = origin.X + (robot->Path[i-1].X * scaleFactor * zoomLevel) + panOffset.X;
                float y1 = origin.Y + (robot->Path[i-1].Y * scaleFactor * zoomLevel) + panOffset.Y;
                float x2 = origin.X + (robot->Path[i].X * scaleFactor * zoomLevel) + panOffset.X;
                float y2 = origin.Y + (robot->Path[i].Y * scaleFactor * zoomLevel) + panOffset.Y;
                g->DrawLine(pathPen, x1, y1, x2, y2);
            }
            delete pathPen;
        }

        void DrawWalls(Graphics^ g) {
            if (!showWalls || detectedWalls == nullptr)
                return;

            // Dessiner les murs
            Pen^ wallPen = gcnew Pen(Color::Red, 2.0f);
            for each (WallSegment^ wall in detectedWalls) {
                float x1 = origin.X + (wall->StartX * scaleFactor * zoomLevel) + panOffset.X;
                float y1 = origin.Y + (wall->StartY * scaleFactor * zoomLevel) + panOffset.Y;
                float x2 = origin.X + (wall->EndX * scaleFactor * zoomLevel) + panOffset.X;
                float y2 = origin.Y + (wall->EndY * scaleFactor * zoomLevel) + panOffset.Y;
                g->DrawLine(wallPen, x1, y1, x2, y2);
            }
            delete wallPen;

            // Dessiner les intersections
            if (showIntersections && wallIntersections != nullptr) {
                SolidBrush^ intersectionBrush = gcnew SolidBrush(Color::Green);
                for each (PointF intersection in wallIntersections) {
                    float x = origin.X + (intersection.X * scaleFactor * zoomLevel) + panOffset.X;
                    float y = origin.Y + (intersection.Y * scaleFactor * zoomLevel) + panOffset.Y;
                    g->FillEllipse(intersectionBrush, x - 5.0f, y - 5.0f, 10.0f, 10.0f);
                }
                delete intersectionBrush;
            }
        }

        void UpdateVisualization() {
            if (persistentImage != nullptr) {
                persistentGraphics->Clear(Color::Black);
                DrawGrid(persistentGraphics);
                
                // Redessiner les points stockes
                SolidBrush^ pointBrush = gcnew SolidBrush(Color::Orange);
                for each (LidarPoint^ point in storedPoints) {
                    float x = origin.X + (point->relativeX * scaleFactor * zoomLevel) + panOffset.X;
                    float y = origin.Y + (point->relativeY * scaleFactor * zoomLevel) + panOffset.Y;
                    persistentGraphics->FillEllipse(pointBrush, x - 2.0f, y - 2.0f, 4.0f, 4.0f);
                }
                delete pointBrush;

                // Dessiner les murs et intersections
                DrawWalls(persistentGraphics);
            }
        }
    };
}