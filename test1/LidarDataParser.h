#pragma once
#include "LidarPoint.h"
#include <msclr/marshal_cppstd.h>
#include <string>

using namespace System;
using namespace System::Collections::Generic;

namespace test1 {
    public ref class LidarDataParser {
    public:
        static List<LidarPoint^>^ ParseData(String^ data) {
            List<LidarPoint^>^ points = gcnew List<LidarPoint^>();
            std::string rawData = msclr::interop::marshal_as<std::string>(data);

            if (rawData.size() < 4 || rawData.substr(0, 4) != "A55A") {
                return points;
            }

            const int packetSize = 10;
            for (size_t pos = 4; pos + packetSize <= rawData.size(); pos += packetSize) {
                std::string packet = rawData.substr(pos, packetSize);
                LidarPoint^ point = DecodePacket(packet, 0.0f, 0.0f);
                if (point != nullptr) {
                    points->Add(point);
                }
            }

            points->Sort(gcnew Comparison<LidarPoint^>(CompareLidarPoints));
            return points;
        }

        static int CompareLidarPoints(LidarPoint^ a, LidarPoint^ b) {
            return a->angle.CompareTo(b->angle);
        }

        static LidarPoint^ DecodePacket(const std::string& packet, float robotX, float robotY) {
            try {
                if (packet.length() < 10) return nullptr;

                int byte1 = std::stoi(packet.substr(2, 2), nullptr, 16);
                int byte2 = std::stoi(packet.substr(4, 2), nullptr, 16);
                int byte3 = std::stoi(packet.substr(6, 2), nullptr, 16);
                int byte4 = std::stoi(packet.substr(8, 2), nullptr, 16);

                float angle = ((byte2 << 7) | (byte1 & 0x7F)) / 64.0f;
                float distance = ((byte4 << 8) | byte3) / 4.0f;

                return distance >= 0.1f ? gcnew LidarPoint(angle, distance, robotX, robotY) : nullptr;
            }
            catch (...) {
                return nullptr;
            }
        }
    };
}