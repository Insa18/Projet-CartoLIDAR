#pragma once
#include "LidarVisualization.h"
#include "Robot.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

namespace test1 {
    public ref class LidarPanel : public Panel {
    private:
        LidarVisualization^ visualization;
        bool isPanning;
        Point lastMousePos;
        Robot^ currentRobot;
        List<WallSegment^>^ detectedWalls;
        List<PointF>^ wallIntersections;

    public:
        LidarPanel() {
            this->DoubleBuffered = true;
            visualization = gcnew LidarVisualization();
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

        void SetWallSegments(List<WallSegment^>^ walls) {
            detectedWalls = walls;
            if (visualization != nullptr) {
                visualization->SetWallSegments(walls);
            }
            this->Invalidate();
        }

        void SetIntersections(List<PointF>^ pts) {
            wallIntersections = pts;
            if (visualization != nullptr) {
                visualization->SetIntersections(pts);
            }
            this->Invalidate();
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
            
            // Update the visualization scale while preserving all data
            if (visualization != nullptr) {
                // Store current state
                float currentZoom = visualization->GetCurrentZoom();
                PointF currentOffset = visualization->GetCurrentOffset();
                
                // Update scale
                visualization->UpdateScale(this->Width, this->Height);
                
                // Restore state
                visualization->SetZoom(currentZoom);
                visualization->SetOffset(currentOffset);
                
                // Ensure all data is preserved
                if (detectedWalls != nullptr) {
                    visualization->SetWallSegments(detectedWalls);
                }
                if (wallIntersections != nullptr) {
                    visualization->SetIntersections(wallIntersections);
                }
                if (currentRobot != nullptr) {
                    visualization->UpdateRobotPosition(currentRobot);
                }
            }
            
            this->Invalidate();
        }

    public:
        property LidarVisualization^ Visualization {
            LidarVisualization^ get() { return visualization; }
        }
    };
}