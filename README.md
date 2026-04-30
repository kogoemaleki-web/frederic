# Projet Balance Électronique (PIC16F876A + HX711 + LCD 16x2)

Ce dépôt contient un exemple de programme en C (XC8) pour réaliser une balance électronique avec:
- PIC16F876A
- Module HX711
- Load cell (pont de jauges)
- LCD 16x2 en mode 4 bits
- Bouton de tare

## Fichier principal
- `main.c`

## Câblage utilisé
- HX711 DOUT -> RB0
- HX711 SCK  -> RB1
- Tare bouton -> RB2 (actif bas, pull-up)
- LCD RS -> RC0
- LCD EN -> RC1
- LCD D4 -> RC2
- LCD D5 -> RC3
- LCD D6 -> RC4
- LCD D7 -> RC5

## Calibration
Ajuster la variable suivante dans `main.c` selon votre capteur et votre montage:

```c
static float g_calibration_factor = 2100.0f;
```

Procédure typique:
1. Mettre un poids de référence connu (ex: 100 g).
2. Observer la valeur affichée.
3. Ajuster `g_calibration_factor` jusqu'à obtenir la bonne mesure.

## Compilation
- Ouvrir le projet dans MPLAB X
- Compilateur XC8
- Cible: PIC16F876A

