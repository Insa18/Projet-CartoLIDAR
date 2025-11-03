#pragma once
#include "LidarVisualization.h"
#include "LidarDroit.h"
#include "Robot.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace test1 {
    public ref class PanelLidar : public Panel {
    private:
        LidarVisualization^ visualization;
        LidarDroit^ lidarProcessor;
        bool isPanning;
        Point lastMousePos;
        Robot^ currentRobot;
        List<WallSegment^>^ detectedWalls;
        List<PointF>^ wallIntersections;

    public:
        PanelLidar() {
            this->DoubleBuffered = true;
            visualization = gcnew LidarVisualization();
            lidarProcessor = gcnew LidarDroit();
            isPanning = false;
            currentRobot = nullptr;
            detectedWalls = nullptr;
            wallIntersections = nullptr;
        }

        void AddPoints(List<LidarPoint^>^ points) {
            if (visualization != nullptr && points != nullptr && points->Count > 0) {
                visualization->AddPoints(points);
                this->Invalidate();
            }
        }

        void SetRobot(Robot^ robot) {
            currentRobot = robot;
            this->Invalidate();
        }

        void DetectWalls() {
            if (visualization != nullptr && visualization->StoredPoints != nullptr) {
                detectedWalls = lidarProcessor->DetectWalls(visualization->StoredPoints);
                wallIntersections = lidarProcessor->FindIntersections(detectedWalls);
                visualization->SetWallSegments(detectedWalls);
                visualization->SetIntersections(wallIntersections);
                this->Invalidate();
            }
        }

    protected:
        virtual void OnPaint(PaintEventArgs^ e) override {
            Panel::OnPaint(e);
            
            if (visualization != nullptr) {
                visualization->DrawMapAndRobot(e->Graphics, currentRobot);
            }
        }

        virtual void OnMouseWheel(MouseEventArgs^ e) override {
            Panel::OnMouseWheel(e);
            float zoomFactor = 1.1f;
            if (e->Delta < 0) {
                zoomFactor = 1.0f / zoomFactor;
            }

            float centerX = e->X - this->Width / 2.0f;
            float centerY = e->Y - this->Height / 2.0f;
            visualization->Zoom(zoomFactor, PointF(centerX, centerY));
            this->Invalidate();
        }

        virtual void OnMouseDown(MouseEventArgs^ e) override {
            Panel::OnMouseDown(e);
            if (e->Button == ::MouseButtons::Left) {
                isPanning = true;
                lastMousePos = e->Location;
                this->Cursor = Cursors::Hand;
            }
        }

        virtual void OnMouseMove(MouseEventArgs^ e) override {
            Panel::OnMouseMove(e);
            if (isPanning) {
                float deltaX = e->X - lastMousePos.X;
                float deltaY = e->Y - lastMousePos.Y;
                visualization->Pan(deltaX, deltaY);
                lastMousePos = e->Location;
                this->Invalidate();
            }
        }

        virtual void OnMouseUp(MouseEventArgs^ e) override {
            Panel::OnMouseUp(e);
            if (e->Button == ::MouseButtons::Left) {
                isPanning = false;
                this->Cursor = Cursors::Default;
            }
        }

        virtual void OnResize(EventArgs^ e) override {
            Panel::OnResize(e);
            visualization->UpdateScale(this->Width, this->Height);
            this->Invalidate();
        }

    public:
        property LidarVisualization^ Visualization {
            LidarVisualization^ get() { return visualization; }
        }
    };
} 