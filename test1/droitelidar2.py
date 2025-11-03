import numpy as np
import matplotlib.pyplot as plt
import os
import sys

# === Paramètres ===
chemin_fichier = r"lidar_data.txt"
nb_murs = 8  # Nombre de murs de la pièce (à adapter)

# === Verification du fichier ===
if not os.path.exists(chemin_fichier):
    print(f"Erreur : le fichier '{chemin_fichier}' est introuvable.")
    sys.exit()

try:
    points = np.loadtxt(chemin_fichier, delimiter=',')
except Exception as e:
    print(f"Erreur lors du chargement des points : {e}")
    sys.exit()

if points.size == 0:
    print("Le fichier est vide. Aucune donnee à traiter.")
    sys.exit()

if points.ndim == 1 or points.shape[1] != 2:
    print("Erreur : le fichier doit contenir des coordonnees X, Y sur deux colonnes.")
    sys.exit()

# === Traitement ===
nb_points = len(points)
segments = [points[i * nb_points // nb_murs : (i + 1) * nb_points // nb_murs] for i in range(nb_murs)]

droites = []
for segment in segments:
    x = segment[:, 0]
    y = segment[:, 1]
    dx = x.max() - x.min()
    dy = y.max() - y.min()

    if dx > dy:
        y_moyen = np.mean(y)
        droites.append(('h', y_moyen))
    else:
        x_moyen = np.mean(x)
        droites.append(('v', x_moyen))

def intersection(d1, d2):
    if d1[0] == 'h' and d2[0] == 'v':
        return np.array([d2[1], d1[1]])
    elif d1[0] == 'v' and d2[0] == 'h':
        return np.array([d1[1], d2[1]])
    return None

coins = []
for i in range(nb_murs):
    coin = intersection(droites[i], droites[(i + 1) % nb_murs])
    if coin is not None:
        coins.append(coin)
coins = np.array(coins)

# === Affichage ===
plt.figure(figsize=(8, 8))
plt.scatter(points[:, 0], points[:, 1], s=10, color='gray', alpha=0.6, label='Points LIDAR')

couleurs = ['r', 'g', 'b', 'm', 'c', 'y', 'k', 'orange']
for i in range(len(coins)):
    d = coins[i]
    f = coins[(i + 1) % len(coins)]
    plt.plot([d[0], f[0]], [d[1], f[1]], color=couleurs[i % len(couleurs)], linewidth=3)

plt.axis('equal')
plt.grid(True)
plt.legend()
plt.title("Murs à partir des points LIDAR")
plt.show()
