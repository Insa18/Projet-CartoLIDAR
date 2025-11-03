#pragma once
#include "LidarPoint.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Drawing;
using namespace System::IO;

namespace test1 {
    // Structure pour représenter une droite (ax + by + c = 0)
    public ref struct Line {
        double a, b, c;
        
        Line() : a(0), b(0), c(0) {}
        Line(double a_, double b_, double c_) : a(a_), b(b_), c(c_) {}
        
        // Calculer la distance d'un point à la droite
        double DistanceToPoint(float x, float y) {
            return Math::Abs(a * x + b * y + c) / Math::Sqrt(a * a + b * b);
        }
    };

    public ref class WallSegment {
    public:
        property float StartX;
        property float StartY;
        property float EndX;
        property float EndY;
        property int PointCount;
        property bool IsHorizontal;
        property bool IsVertical;
        property List<LidarPoint^>^ Points;
        property PointF Start {
            PointF get() { return PointF(StartX, StartY); }
        }
        property PointF End {
            PointF get() { return PointF(EndX, EndY); }
        }

        WallSegment(float startX, float startY, float endX, float endY, int pointCount) {
            StartX = startX;
            StartY = startY;
            EndX = endX;
            EndY = endY;
            PointCount = pointCount;
            Points = gcnew List<LidarPoint^>();
            
            // Déterminer si le segment est horizontal ou vertical
            float deltaX = Math::Abs(endX - startX);
            float deltaY = Math::Abs(endY - startY);
            
            IsHorizontal = deltaY < deltaX * 0.1f; // Tolérance de 10%
            IsVertical = deltaX < deltaY * 0.1f;
        }

        double GetLength() {
            return Math::Sqrt(Math::Pow(EndX - StartX, 2) + Math::Pow(EndY - StartY, 2));
        }
    };

    public ref class LidarDroit {
    private:
        // Paramètres RANSAC
        float distanceThreshold;
        int maxIterations;
        int minPointsForLine;
        float minSegmentLength;
        Random^ rng;

        // Créer une droite à partir de deux points
        Line^ CreateLineFromPoints(LidarPoint^ p1, LidarPoint^ p2) {
            float x1 = p1->relativeX, y1 = p1->relativeY;
            float x2 = p2->relativeX, y2 = p2->relativeY;
            
            // Éviter les points identiques
            if (Math::Abs(x2 - x1) < 1e-6 && Math::Abs(y2 - y1) < 1e-6) {
                return gcnew Line(1, 0, -x1); // Droite verticale par défaut
            }
            
            // Équation de la droite : (y2-y1)x - (x2-x1)y + (x2-x1)y1 - (y2-y1)x1 = 0
            double a = y2 - y1;
            double b = x1 - x2;
            double c = (x2 - x1) * y1 - (y2 - y1) * x1;
            
            return gcnew Line(a, b, c);
        }

        // Trouver les points inliers pour une droite donnée
        List<LidarPoint^>^ FindInliers(List<LidarPoint^>^ points, Line^ line) {
            List<LidarPoint^>^ inliers = gcnew List<LidarPoint^>();
            
            for each(LidarPoint^ point in points) {
                if (line->DistanceToPoint(point->relativeX, point->relativeY) <= distanceThreshold) {
                    inliers->Add(point);
                }
            }
            
            return inliers;
        }

        // Créer un segment à partir d'un ensemble de points alignés
        WallSegment^ CreateSegmentFromPoints(List<LidarPoint^>^ points) {
            if (points->Count < 2) return nullptr;
            
            // Trouver les points extrêmes
            float minX = points[0]->relativeX, maxX = points[0]->relativeX;
            float minY = points[0]->relativeY, maxY = points[0]->relativeY;
            float minXY = minX, maxXY = maxX, minYX = minY, maxYX = maxY;
            
            for each(LidarPoint^ point in points) {
                if (point->relativeX < minX) { minX = point->relativeX; minYX = point->relativeY; }
                if (point->relativeX > maxX) { maxX = point->relativeX; maxYX = point->relativeY; }
                if (point->relativeY < minY) { minY = point->relativeY; minXY = point->relativeX; }
                if (point->relativeY > maxY) { maxY = point->relativeY; maxXY = point->relativeX; }
            }
            
            // Choisir les extrémités qui donnent le segment le plus long
            // Dans LidarDroit.h, modifiez ces lignes :
            float lengthX = (float)Math::Sqrt(Math::Pow(maxX - minX, 2) + Math::Pow(maxYX - minYX, 2));
            float lengthY = (float)Math::Sqrt(Math::Pow(maxXY - minXY, 2) + Math::Pow(maxY - minY, 2));
            
            WallSegment^ segment;
            if (lengthX >= lengthY) {
                segment = gcnew WallSegment(minX, minYX, maxX, maxYX, points->Count);
            } else {
                segment = gcnew WallSegment(minXY, minY, maxXY, maxY, points->Count);
            }
            
            // Copier les points dans le segment
            segment->Points->AddRange(points);
            return segment;
        }

        // Supprimer les points utilisés de la liste principale
        void RemoveUsedPoints(List<LidarPoint^>^ allPoints, List<LidarPoint^>^ usedPoints) {
            for each(LidarPoint^ usedPoint in usedPoints) {
                allPoints->Remove(usedPoint);
            }
        }

        // Fusionner les segments proches et alignés
        List<WallSegment^>^ MergeWalls(List<WallSegment^>^ wallSegments) {
            if (wallSegments == nullptr || wallSegments->Count == 0) 
                return gcnew List<WallSegment^>();

            List<WallSegment^>^ merged = gcnew List<WallSegment^>();
            wallSegments->Sort(gcnew Comparison<WallSegment^>(&LidarDroit::CompareWalls));

            WallSegment^ lastMerged = nullptr;
            for each(WallSegment^ current in wallSegments) {
                if (lastMerged == nullptr) {
                    lastMerged = current;
                    merged->Add(lastMerged);
                    continue;
                }

                    bool canMerge = false;
                if (lastMerged->IsHorizontal && current->IsHorizontal) {
                    float yDiff = Math::Abs(lastMerged->StartY - current->StartY);
                    if (yDiff < distanceThreshold) {
                                    canMerge = true;
                                }
                            }
                else if (lastMerged->IsVertical && current->IsVertical) {
                    float xDiff = Math::Abs(lastMerged->StartX - current->StartX);
                    if (xDiff < distanceThreshold) {
                                    canMerge = true;
                        }
                    }

                    if (canMerge) {
                    if (lastMerged->IsHorizontal) {
                        lastMerged->StartX = Math::Min(lastMerged->StartX, current->StartX);
                        lastMerged->EndX = Math::Max(lastMerged->EndX, current->EndX);
                        float avgY = (lastMerged->StartY + current->StartY) / 2.0f;
                        lastMerged->StartY = avgY;
                        lastMerged->EndY = avgY;
                    } else {
                        lastMerged->StartY = Math::Min(lastMerged->StartY, current->StartY);
                        lastMerged->EndY = Math::Max(lastMerged->EndY, current->EndY);
                        float avgX = (lastMerged->StartX + current->StartX) / 2.0f;
                        lastMerged->StartX = avgX;
                        lastMerged->EndX = avgX;
                    }
                    lastMerged->PointCount += current->PointCount;
                    lastMerged->Points->AddRange(current->Points);
                } else {
                    lastMerged = current;
                    merged->Add(lastMerged);
                }
            }

            return merged;
        }

        static int CompareWalls(WallSegment^ a, WallSegment^ b) {
            if (a->IsHorizontal != b->IsHorizontal)
                return a->IsHorizontal ? -1 : 1;
            
            if (a->IsHorizontal) {
                return a->StartY.CompareTo(b->StartY);
            } else {
                return a->StartX.CompareTo(b->StartX);
            }
        }

    public:
        LidarDroit() {
            distanceThreshold = 0.05f;    // 5 cm de tolérance
            maxIterations = 1000;         // Nombre max d'itérations RANSAC
            minPointsForLine = 5;         // Minimum de points pour considérer une droite
            minSegmentLength = 0.2f;      // Longueur minimale d'un segment (20 cm)
            rng = gcnew Random(static_cast<int>(DateTime::Now.Ticks));
        }

        List<LidarPoint^>^ LoadPointsFromCSV() {
            List<LidarPoint^>^ points = gcnew List<LidarPoint^>();
            String^ path = "lidar_data.csv";

            if (!File::Exists(path)) {
                Console::WriteLine("Le fichier lidar_data.csv n'existe pas");
                return points;
            }

            try {
                StreamReader^ reader = gcnew StreamReader(path);
                String^ header = reader->ReadLine();
                
                points->Capacity = 10000;  // Estimation de la taille

                while (!reader->EndOfStream) {
                    String^ line = reader->ReadLine();
                    array<String^>^ values = line->Split(',');
                    
                    if (values->Length >= 2) {
                        float x = float::Parse(values[0], System::Globalization::CultureInfo::InvariantCulture);
                        float y = float::Parse(values[1], System::Globalization::CultureInfo::InvariantCulture);
                        
                        LidarPoint^ point = gcnew LidarPoint();
                        point->relativeX = x;
                        point->relativeY = y;
                        points->Add(point);
                    }
                }
                reader->Close();
            }
            catch (Exception^ e) {
                Console::WriteLine("Erreur lors de la lecture du fichier CSV: " + e->Message);
            }

            return points;
        }

        List<WallSegment^>^ DetectWalls(List<LidarPoint^>^ points) {
            if (points == nullptr || points->Count == 0)
                return gcnew List<WallSegment^>();

            List<WallSegment^>^ segments = gcnew List<WallSegment^>();
            List<LidarPoint^>^ remainingPoints = gcnew List<LidarPoint^>(points);
            
            while (remainingPoints->Count >= minPointsForLine) {
                Line^ bestLine = nullptr;
                List<LidarPoint^>^ bestInliers = nullptr;
                int bestInlierCount = 0;
                
                // RANSAC : essayer plusieurs hypothèses de droites
                for (int iter = 0; iter < maxIterations && remainingPoints->Count >= 2; iter++) {
                    // Choisir deux points aléatoirement
                    int idx1 = rng->Next(remainingPoints->Count);
                    int idx2 = rng->Next(remainingPoints->Count);
                    
                    // S'assurer que les deux points sont différents
                    if (idx1 == idx2) {
                        if (remainingPoints->Count > 2) {
                            idx2 = (idx2 + 1) % remainingPoints->Count;
                        } else {
                            continue;
                        }
                    }
                    
                    // Créer une droite à partir de ces deux points
                    Line^ candidateLine = CreateLineFromPoints(remainingPoints[idx1], remainingPoints[idx2]);
                    
                    // Trouver tous les points inliers pour cette droite
                    List<LidarPoint^>^ inliers = FindInliers(remainingPoints, candidateLine);
                    
                    // Garder la meilleure droite (celle avec le plus d'inliers)
                    if (inliers->Count > bestInlierCount) {
                        bestInlierCount = inliers->Count;
                        bestLine = candidateLine;
                        bestInliers = inliers;
                    }
                }
                
                // Si on a trouvé une droite valide
                if (bestInliers != nullptr && bestInliers->Count >= minPointsForLine) {
                    // Créer un segment à partir des points inliers
                    WallSegment^ segment = CreateSegmentFromPoints(bestInliers);
                    
                    // Vérifier que le segment est assez long
                    if (segment != nullptr && segment->GetLength() >= minSegmentLength) {
                        segments->Add(segment);
                    }
                    
                    // Supprimer les points utilisés
                    RemoveUsedPoints(remainingPoints, bestInliers);
                } else {
                    // Si on ne trouve plus de droites valides, on arrête
                    break;
                }
            }
            
            // Fusionner les segments proches et alignés
            return MergeWalls(segments);
        }

        List<PointF>^ FindIntersections(List<WallSegment^>^ walls) {
            List<PointF>^ intersections = gcnew List<PointF>();
            float maxDistance = 0.2f; // 20 cm de tolérance pour les intersections

            for (int i = 0; i < walls->Count; i++) {
                for (int j = i + 1; j < walls->Count; j++) {
                    WallSegment^ wall1 = walls[i];
                    WallSegment^ wall2 = walls[j];

                    // Ne considérer que les intersections entre murs perpendiculaires
                    if (wall1->IsHorizontal == wall2->IsHorizontal)
                        continue;

                        PointF intersection;
                    if (wall1->IsHorizontal) {
                        intersection = PointF(wall2->StartX, wall1->StartY);
                    } else {
                        intersection = PointF(wall1->StartX, wall2->StartY);
                    }

                    // Vérifier si l'intersection est proche des deux segments
                    float dist1 = DistanceToSegment(intersection, wall1);
                    float dist2 = DistanceToSegment(intersection, wall2);

                    if (dist1 <= maxDistance && dist2 <= maxDistance) {
                        // Vérifier si cette intersection existe déjà
                        bool isNew = true;
                        for each(PointF existing in intersections) {
                            if (Math::Abs(existing.X - intersection.X) < maxDistance &&
                                Math::Abs(existing.Y - intersection.Y) < maxDistance) {
                                isNew = false;
                                break;
                            }
                        }
                        if (isNew) {
                            intersections->Add(intersection);
                        }
                    }
                }
            }

            return intersections;
        }

    private:
        float DistanceToSegment(PointF point, WallSegment^ segment) {
            float x = point.X;
            float y = point.Y;
            float x1 = segment->StartX;
            float y1 = segment->StartY;
            float x2 = segment->EndX;
            float y2 = segment->EndY;

            float A = x - x1;
            float B = y - y1;
            float C = x2 - x1;
            float D = y2 - y1;

            float dot = A * C + B * D;
            float len_sq = C * C + D * D;
            float param = -1;

            if (len_sq != 0)
                param = dot / len_sq;

            float xx, yy;

            if (param < 0) {
                xx = x1;
                yy = y1;
            }
            else if (param > 1) {
                xx = x2;
                yy = y2;
            }
            else {
                xx = x1 + param * C;
                yy = y1 + param * D;
            }

            float dx = x - xx;
            float dy = y - yy;

            return (float)Math::Sqrt(dx * dx + dy * dy);
        }

    public:
        // Méthodes pour ajuster les paramètres
        void SetDistanceThreshold(float threshold) { distanceThreshold = threshold; }
        void SetMaxIterations(int iterations) { maxIterations = iterations; }
        void SetMinPointsForLine(int minPoints) { minPointsForLine = minPoints; }
        void SetMinSegmentLength(float minLength) { minSegmentLength = minLength; }
        
        // Méthodes pour obtenir les paramètres actuels
        float GetDistanceThreshold() { return distanceThreshold; }
        int GetMaxIterations() { return maxIterations; }
        int GetMinPointsForLine() { return minPointsForLine; }
        float GetMinSegmentLength() { return minSegmentLength; }
    };
} 