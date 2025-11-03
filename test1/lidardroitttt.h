#pragma once
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>

using namespace System;
using namespace System::Collections::Generic;

namespace test1 {
    // Structure pour représenter un segment de mur
    public ref class WallSegment {
    public:
        property float StartX;
        property float StartY;
        property float EndX;
        property float EndY;
        property int PointCount;
        property bool IsHorizontal;
        property bool IsVertical;

        WallSegment(float startX, float startY, float endX, float endY, int pointCount) {
            StartX = startX;
            StartY = startY;
            EndX = endX;
            EndY = endY;
            PointCount = pointCount;
            
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

    // Classe pour la détection de droites avec RANSAC
    public ref class LidarRANSAC {
    private:
        // Paramètres RANSAC
        float distanceThreshold;
        int maxIterations;
        int minPointsForLine;
        float minSegmentLength;
        std::mt19937 rng;

        // Structure pour représenter une droite (ax + by + c = 0)
        struct Line {
            double a, b, c;
            
            Line() : a(0), b(0), c(0) {}
            Line(double a_, double b_, double c_) : a(a_), b(b_), c(c_) {}
            
            // Calculer la distance d'un point à la droite
            double DistanceToPoint(float x, float y) {
                return std::abs(a * x + b * y + c) / std::sqrt(a * a + b * b);
            }
        };

        // Créer une droite à partir de deux points
        Line CreateLineFromPoints(LidarPoint^ p1, LidarPoint^ p2) {
            float x1 = p1->relativeX, y1 = p1->relativeY;
            float x2 = p2->relativeX, y2 = p2->relativeY;
            
            // Éviter les points identiques
            if (Math::Abs(x2 - x1) < 1e-6 && Math::Abs(y2 - y1) < 1e-6) {
                return Line(1, 0, -x1); // Droite verticale par défaut
            }
            
            // Équation de la droite : (y2-y1)x - (x2-x1)y + (x2-x1)y1 - (y2-y1)x1 = 0
            double a = y2 - y1;
            double b = x1 - x2;
            double c = (x2 - x1) * y1 - (y2 - y1) * x1;
            
            return Line(a, b, c);
        }

        // Trouver les points inliers pour une droite donnée
        List<LidarPoint^>^ FindInliers(List<LidarPoint^>^ points, Line line) {
            List<LidarPoint^>^ inliers = gcnew List<LidarPoint^>();
            
            for each(LidarPoint^ point in points) {
                if (line.DistanceToPoint(point->relativeX, point->relativeY) <= distanceThreshold) {
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
            float lengthX = Math::Sqrt(Math::Pow(maxX - minX, 2) + Math::Pow(maxYX - minYX, 2));
            float lengthY = Math::Sqrt(Math::Pow(maxXY - minXY, 2) + Math::Pow(maxY - minY, 2));
            
            if (lengthX >= lengthY) {
                return gcnew WallSegment(minX, minYX, maxX, maxYX, points->Count);
            } else {
                return gcnew WallSegment(minXY, minY, maxXY, maxY, points->Count);
            }
        }

        // Supprimer les points utilisés de la liste principale
        void RemoveUsedPoints(List<LidarPoint^>^ allPoints, List<LidarPoint^>^ usedPoints) {
            for each(LidarPoint^ usedPoint in usedPoints) {
                allPoints->Remove(usedPoint);
            }
        }

    public:
        // Constructeur avec paramètres par défaut
        LidarRANSAC() {
            distanceThreshold = 0.05f;    // 5 cm de tolérance
            maxIterations = 1000;         // Nombre max d'itérations RANSAC
            minPointsForLine = 5;         // Minimum de points pour considérer une droite
            minSegmentLength = 0.2f;      // Longueur minimale d'un segment (20 cm)
            rng.seed(static_cast<unsigned int>(System::DateTime::Now.Ticks));
        }

        // Constructeur avec paramètres personnalisés
        LidarRANSAC(float threshold, int iterations, int minPoints, float minLength) {
            distanceThreshold = threshold;
            maxIterations = iterations;
            minPointsForLine = minPoints;
            minSegmentLength = minLength;
            rng.seed(static_cast<unsigned int>(System::DateTime::Now.Ticks));
        }

        // Méthode principale pour détecter les segments
        List<WallSegment^>^ DetectWallSegments(List<LidarPoint^>^ points) {
            List<WallSegment^>^ segments = gcnew List<WallSegment^>();
            
            // Créer une copie de la liste des points pour pouvoir la modifier
            List<LidarPoint^>^ remainingPoints = gcnew List<LidarPoint^>(points);
            
            while (remainingPoints->Count >= minPointsForLine) {
                Line bestLine;
                List<LidarPoint^>^ bestInliers = nullptr;
                int bestInlierCount = 0;
                
                // RANSAC : essayer plusieurs hypothèses de droites
                for (int iter = 0; iter < maxIterations && remainingPoints->Count >= 2; iter++) {
                    // Choisir deux points aléatoirement
                    std::uniform_int_distribution<int> dist(0, remainingPoints->Count - 1);
                    int idx1 = dist(rng);
                    int idx2 = dist(rng);
                    
                    // S'assurer que les deux points sont différents
                    if (idx1 == idx2) {
                        if (remainingPoints->Count > 2) {
                            idx2 = (idx2 + 1) % remainingPoints->Count;
                        } else {
                            continue;
                        }
                    }
                    
                    // Créer une droite à partir de ces deux points
                    Line candidateLine = CreateLineFromPoints(remainingPoints[idx1], remainingPoints[idx2]);
                    
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
            
            return segments;
        }

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