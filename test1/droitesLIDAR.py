import numpy as np
import matplotlib.pyplot as plt
from scipy.spatial import ConvexHull
import sys
import io

# Donnees LIDAR
cube_data = 'lidar_data.csv'

# Charger les donnees avec toutes les colonnes
try:
    # Charger toutes les colonnes du CSV
    data = np.loadtxt(cube_data, delimiter=',', skiprows=1)  # skiprows=1 pour ignorer l'en-tête
    # Extraire les colonnes pertinentes
    points = data[:, 6:8]  # absoluteX, absoluteY
    robot_positions = data[:, 4:6]  # robotX, robotY
    angles = data[:, 0]  # angle
    distances = data[:, 1]  # distance
except Exception as e:
    print(f"Erreur lors du chargement des points : {e}")
    sys.exit()

if points.size == 0:
    print("Aucune donnee à traiter.")
    sys.exit()

if points.ndim == 1 or points.shape[1] != 2:
    print("Erreur : les donnees doivent contenir des coordonnees X, Y sur deux colonnes.")
    sys.exit()

# === Methode simple pour detecter les murs sans utiliser DBSCAN ===
def detect_walls_simple(points, bin_size=0.5, min_points=5, angle_threshold=15):
    """
    Detecte les murs à partir de points LIDAR en utilisant une approche de binning
    """
    # Plage des coordonnees
    x_min, y_min = np.min(points, axis=0)
    x_max, y_max = np.max(points, axis=0)
    
    # Ajouter une marge
    margin = 0.1
    x_min -= margin
    y_min -= margin
    x_max += margin
    y_max += margin
    
    # Creation des bins
    x_bins = np.arange(x_min, x_max + bin_size, bin_size)
    y_bins = np.arange(y_min, y_max + bin_size, bin_size)
    
    # Identifier les murs horizontaux
    h_walls = []
    for y_idx in range(len(y_bins) - 1):
        y_val = (y_bins[y_idx] + y_bins[y_idx + 1]) / 2
        mask = (points[:, 1] >= y_bins[y_idx]) & (points[:, 1] < y_bins[y_idx + 1])
        x_coords = points[mask, 0]
        
        if len(x_coords) >= min_points:
            # Trouver des segments contigus
            if len(x_coords) > 0:
                x_sorted = np.sort(x_coords)
                diffs = np.diff(x_sorted)
                breaks = np.where(diffs > bin_size * 2)[0]
                
                # Creer des segments à partir des points
                start_idx = 0
                for break_idx in breaks:
                    if break_idx - start_idx + 1 >= min_points:
                        segment_pts = points[mask][start_idx:break_idx + 1]
                        h_walls.append({
                            'type': 'h',
                            'start': np.array([np.min(segment_pts[:, 0]), y_val]),
                            'end': np.array([np.max(segment_pts[:, 0]), y_val]),
                            'points': segment_pts
                        })
                    start_idx = break_idx + 1
                
                # Dernier segment
                if len(x_sorted) - start_idx >= min_points:
                    segment_pts = points[mask][start_idx:]
                    h_walls.append({
                        'type': 'h',
                        'start': np.array([np.min(segment_pts[:, 0]), y_val]),
                        'end': np.array([np.max(segment_pts[:, 0]), y_val]),
                        'points': segment_pts
                    })
    
    # Identifier les murs verticaux
    v_walls = []
    for x_idx in range(len(x_bins) - 1):
        x_val = (x_bins[x_idx] + x_bins[x_idx + 1]) / 2
        mask = (points[:, 0] >= x_bins[x_idx]) & (points[:, 0] < x_bins[x_idx + 1])
        y_coords = points[mask, 1]
        
        if len(y_coords) >= min_points:
            # Trouver des segments contigus
            if len(y_coords) > 0:
                y_sorted = np.sort(y_coords)
                diffs = np.diff(y_sorted)
                breaks = np.where(diffs > bin_size * 2)[0]
                
                # Creer des segments à partir des points
                start_idx = 0
                for break_idx in breaks:
                    if break_idx - start_idx + 1 >= min_points:
                        segment_pts = points[mask][start_idx:break_idx + 1]
                        v_walls.append({
                            'type': 'v',
                            'start': np.array([x_val, np.min(segment_pts[:, 1])]),
                            'end': np.array([x_val, np.max(segment_pts[:, 1])]),
                            'points': segment_pts
                        })
                    start_idx = break_idx + 1
                
                # Dernier segment
                if len(y_sorted) - start_idx >= min_points:
                    segment_pts = points[mask][start_idx:]
                    v_walls.append({
                        'type': 'v',
                        'start': np.array([x_val, np.min(segment_pts[:, 1])]),
                        'end': np.array([x_val, np.max(segment_pts[:, 1])]),
                        'points': segment_pts
                    })
    
    # Combiner les murs
    wall_segments = h_walls + v_walls
    
    # Fusionner les segments de mur proches et alignes
    merged_walls = []
    for wall in wall_segments:
        merged = False
        for i, existing_wall in enumerate(merged_walls):
            if wall['type'] == existing_wall['type']:
                if wall['type'] == 'h' and abs(wall['start'][1] - existing_wall['start'][1]) < bin_size:
                    # Murs horizontaux proches en y
                    x_min = min(wall['start'][0], existing_wall['start'][0])
                    x_max = max(wall['end'][0], existing_wall['end'][0])
                    if (x_min == wall['start'][0] and x_max == existing_wall['end'][0]) or \
                       (x_min == existing_wall['start'][0] and x_max == wall['end'][0]) or \
                       (wall['start'][0] <= existing_wall['end'][0] and existing_wall['start'][0] <= wall['end'][0]):
                        # Les murs se chevauchent ou sont adjacents
                        y_avg = (wall['start'][1] + existing_wall['start'][1]) / 2
                        merged_walls[i] = {
                            'type': 'h',
                            'start': np.array([x_min, y_avg]),
                            'end': np.array([x_max, y_avg]),
                            'points': np.vstack((wall['points'], existing_wall['points']))
                        }
                        merged = True
                        break
                elif wall['type'] == 'v' and abs(wall['start'][0] - existing_wall['start'][0]) < bin_size:
                    # Murs verticaux proches en x
                    y_min = min(wall['start'][1], existing_wall['start'][1])
                    y_max = max(wall['end'][1], existing_wall['end'][1])
                    if (y_min == wall['start'][1] and y_max == existing_wall['end'][1]) or \
                       (y_min == existing_wall['start'][1] and y_max == wall['end'][1]) or \
                       (wall['start'][1] <= existing_wall['end'][1] and existing_wall['start'][1] <= wall['end'][1]):
                        # Les murs se chevauchent ou sont adjacents
                        x_avg = (wall['start'][0] + existing_wall['start'][0]) / 2
                        merged_walls[i] = {
                            'type': 'v',
                            'start': np.array([x_avg, y_min]),
                            'end': np.array([x_avg, y_max]),
                            'points': np.vstack((wall['points'], existing_wall['points']))
                        }
                        merged = True
                        break
        
        if not merged:
            merged_walls.append(wall)
    
    return merged_walls

# === Fonction pour trouver les intersections entre segments de murs ===
def find_intersections(wall_segments, max_distance=2.0):
    """
    Trouve les intersections entre les segments de murs
    Gère les cas où les murs ne se rejoignent pas parfaitement
    """
    intersections = []
    
    for i, wall1 in enumerate(wall_segments):
        start1, end1 = wall1['start'], wall1['end']
        
        for j, wall2 in enumerate(wall_segments):
            if i == j:
                continue
                
            start2, end2 = wall2['start'], wall2['end']
            
            # Verifier si les segments sont proches à leurs extremites
            distances = [
                np.linalg.norm(start1 - start2),
                np.linalg.norm(start1 - end2),
                np.linalg.norm(end1 - start2),
                np.linalg.norm(end1 - end2)
            ]
            
            if min(distances) <= max_distance:
                # Trouver quelle paire d'extremites est la plus proche
                pair_idx = np.argmin(distances)
                
                if pair_idx == 0:  # start1 - start2
                    intersection_point = (start1 + start2) / 2
                elif pair_idx == 1:  # start1 - end2
                    intersection_point = (start1 + end2) / 2
                elif pair_idx == 2:  # end1 - start2
                    intersection_point = (end1 + start2) / 2
                else:  # end1 - end2
                    intersection_point = (end1 + end2) / 2
                
                # Verifier si cette intersection existe dejà
                is_new = True
                for existing in intersections:
                    if np.linalg.norm(existing - intersection_point) < max_distance:
                        is_new = False
                        break
                
                if is_new:
                    intersections.append(intersection_point)
    
    return np.array(intersections) if intersections else np.array([])

# === Affichage ===
plt.figure(figsize=(10, 10))

# Afficher les points LIDAR
plt.scatter(points[:, 0], points[:, 1], s=10, color='gray', alpha=0.3, label='Points LIDAR')

# Afficher la position du robot pour chaque mesure
unique_robot_positions = np.unique(robot_positions, axis=0)
plt.scatter(unique_robot_positions[:, 0], unique_robot_positions[:, 1], 
           s=100, color='red', marker='o', label='Position du robot')

# Afficher les segments de murs detectes
wall_segments = detect_walls_simple(points, bin_size=0.3, min_points=5)
couleurs = ['b', 'g', 'm', 'c', 'y', 'k', 'orange']
for i, wall in enumerate(wall_segments):
    color = couleurs[i % len(couleurs)]
    plt.plot([wall['start'][0], wall['end'][0]], 
             [wall['start'][1], wall['end'][1]], 
             color=color, linewidth=2)
    
    # Afficher les points utilises pour chaque mur
    if 'points' in wall:
        plt.scatter(wall['points'][:, 0], wall['points'][:, 1], s=15, color=color, alpha=0.5)

# Afficher les intersections
intersections = find_intersections(wall_segments, max_distance=0.5)
if len(intersections) > 0:
    plt.scatter(intersections[:, 0], intersections[:, 1], s=100, color='black', marker='x', label='Intersections')

# Calculer et afficher l'enveloppe convexe des intersections si possible
if len(intersections) > 2:
    try:
        hull = ConvexHull(intersections)
        ordered_vertices = intersections[hull.vertices]
        polygon = np.vstack([ordered_vertices, ordered_vertices[0]])
        plt.plot(polygon[:, 0], polygon[:, 1], 'k--', linewidth=1, label='Contour')
    except:
        print("Impossible de calculer l'enveloppe convexe")

plt.axis('equal')
plt.grid(True)
plt.legend()
plt.title("Detection des murs et position du robot")
plt.show()

# Afficher des statistiques sur les données
print(f"Nombre total de points: {len(points)}")
print(f"Nombre de positions uniques du robot: {len(unique_robot_positions)}")
print(f"Nombre de murs detectes: {len(wall_segments)}")
if len(intersections) > 0:
    print(f"Nombre d'intersections detectees: {len(intersections)}")
else:
    print("Aucune intersection detectee")

# Afficher les informations sur chaque mur detecte
for i, wall in enumerate(wall_segments):
    print(f"Mur {i+1}: Type={wall['type']}, Debut=({wall['start'][0]:.2f}, {wall['start'][1]:.2f}), Fin=({wall['end'][0]:.2f}, {wall['end'][1]:.2f})")