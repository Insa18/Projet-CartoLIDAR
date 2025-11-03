#pragma once
#include <cmath>
#include <fstream>
#include <sys/stat.h> 

using namespace System;
using namespace System::IO;
using namespace System::Globalization;

namespace test1 {
    public ref class LidarPoint {
    public:
        float angle;
        float distance;
        float relativeX;
        float relativeY;

        LidarPoint() : angle(0), distance(0), relativeX(0), relativeY(0) {}

        LidarPoint(float a, float d, float robotX, float robotY) : angle(a), distance(d) {
            float rad = angle * (float)Math::PI / 180.0f;
            relativeX = distance * cos(rad);
            relativeY = distance * sin(rad);

            try {
                String^ path = "lidar_data.csv";
                bool fileExists = File::Exists(path);
                bool writeHeader = !fileExists || (File::ReadAllText(path)->Length == 0);

                StreamWriter^ writer = gcnew StreamWriter(path, true);
                CultureInfo^ culture = CultureInfo::InvariantCulture;

                float absoluteX = robotX + relativeX;
                float absoluteY = robotY + relativeY;
                writer->WriteLine(String::Format(culture, "{0},{1}", absoluteX, absoluteY));
                writer->Close();
            }
            catch (Exception^ e) {
                Console::WriteLine("Erreur lors de l'ecriture dans le fichier CSV: " + e->Message);
            }
        }
    };
}
