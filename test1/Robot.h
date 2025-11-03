#pragma once
#include <cmath>

using namespace System;
using namespace System::Drawing;
using namespace System::Collections::Generic;

namespace test1 {
    public ref class Robot {
    private:
        PointF position;
        float angle;
        List<PointF>^ path;
        List<String^>^ deplacements;

    public:
        Robot() {
            position = PointF(0, 0);
            angle = 0.0f;
            path = gcnew List<PointF>();
            path->Add(position);
            deplacements = gcnew List<String^>();
        }

        property PointF Position {
            PointF get() { return position; }
        }

        property float Angle {
            float get() { return angle; }
        }

        property List<PointF>^ Path {
            List<PointF>^ get() { return path; }
        }

        property List<String^>^ Deplacements {
            List<String^>^ get() { return deplacements; }
        }

        void MoveForward(float distance) {
            float angleRad = angle * (float)Math::PI / 180.0f;
            position.X += distance * cos(angleRad);
            position.Y += distance * sin(angleRad);
            path->Add(position);
        }

        void MoveBackward(float distance) {
            float angleRad = angle * (float)Math::PI / 180.0f;
            position.X -= distance * cos(angleRad);
            position.Y -= distance * sin(angleRad);
            path->Add(position);
        }

        void RotateLeft(float degrees) {
            angle -= degrees;
            path->Add(position);
        }

        void RotateRight(float degrees) {
            angle += degrees;
            path->Add(position);
        }

        void EnregistrerDeplacement(String^ deplacement) {
            deplacements->Add(deplacement);
        }
    };
}