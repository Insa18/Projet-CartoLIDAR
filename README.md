# 🤖 Projet Robot Cartographe – Cartographie d’un bâtiment

## 📋 Description du projet
Ce projet a pour objectif de **concevoir et programmer un robot capable de cartographier un bâtiment** à l’aide d’un capteur **Lidar 360°** et de **moteurs pas à pas**.  
Le robot est capable de mesurer les distances, d’identifier les obstacles et de transmettre les données de mesure à un logiciel d’exploitation pour la création d’une carte de l’environnement.

Le projet repose sur plusieurs sous-systèmes :
- Pilotage des moteurs via un microcontrôleur
- Mesure des distances par Lidar
- Communication WiFi avec un logiciel de supervision
- Commande manuelle via une application Android
- Traitement et affichage des données de cartographie

---

## 🧠 Objectifs principaux
1. **Caractériser le robot cartographe**  
   Définir la structure matérielle et logicielle du robot, y compris les moteurs, capteurs et interfaces de communication.

2. **Positionner le robot sur un point de référence**  
   Assurer un positionnement initial précis pour garantir la cohérence des mesures.

3. **Mesurer et transmettre les distances**  
   Utiliser le Lidar pour collecter et transmettre des données de distance vers un logiciel d’exploitation.

4. **Commander les déplacements du robot**  
   Permettre à l’opérateur de déplacer le robot via une application mobile ou un programme prédéfini.

---

## ⚙️ Technologies et composants utilisés
- **Microcontrôleur** : [ESP32 / Arduino / autre selon ton matériel]  
- **Capteur Lidar** : Lidar 360° (données transmises en trames série)  
- **Moteurs** : Moteurs pas à pas avec codeurs incrémentaux  
- **Communication** : Module WiFi intégré  
- **Application mobile** : Android (interface de commande manuelle)  
- **Logiciel d’exploitation** : réception et affichage des données Lidar  

---

## 🗂️ Structure du projet
