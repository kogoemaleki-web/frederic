# Balance électronique PIC16F876A (28 broches) + HX711 + LCD

Version **mikroC PRO for PIC** (sans `xc.h`).

## Pourquoi cette version est plus stable
- Lecture HX711 avec **timeout** (évite blocage infini).
- Moyennage avec validation des échantillons valides.
- Anti-rebond du bouton TARE.
- Messages d'erreur LCD si HX711 non détecté.

## Câblage
- HX711 DOUT -> RB0
- HX711 SCK  -> RB1
- Bouton TARE -> RB2 (actif bas)
- LCD RS -> RC0
- LCD EN -> RC1
- LCD D4 -> RC2
- LCD D5 -> RC3
- LCD D6 -> RC4
- LCD D7 -> RC5

## Paramètre important
Dans `main.c`:
```c
float calibration_factor = 2100.0;
```

## Si le programme "ne marche pas toujours"
1. Vérifier masse commune PIC + HX711 + LCD.
2. Ajouter condensateur 100nF près du HX711.
3. Câble load cell court et blindé si possible.
4. Vérifier alimentation stable (5V régulée).
5. Refaire la calibration avec un poids connu.

