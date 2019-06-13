IRDA Joy V2.1 - HW

Connecteurs
===========
* J1 : Joystick numérique atari ou paddle analogiques
* J2 : extension port USART
* J3 : alimentation 3V (ou 5V)
* J4 : ICSP

Brochage J1
===========
Pin SUB9 (Joystick)
-------------------		
#1 : Haut
#2 : Bas
#3 : Gauche
#4 : Droite
#5 : NC
#6 : Bouton 
#7 : NC
#8 : Gnd
#9 : NC

Pin SUB9 (Paddle)
-----------------		
#1 : NC
#2 : NC
#3 : Bouton paddle 1
#4 : Bouton paddle 0 
#5 : Paddle 0
#6 : NC 
#7 : Vcc
#8 : Gnd
#9 : Paddle 1

Switchs et boutons
==================
* S1 : reset
* S2 : Non utilisé
* SW1 : 2-3 (RB2) : Mode USART (ouvert) / IRDA (fermé)
* SW1 : 1-4 (RB3) : Non utilisé

Nomenclature
============
* J1 : SUB9 M
* J2 : Connecteur 4 pins
* J3 : Connecteur 2 pins
* J4 : Connecteur 6 pins
* SW1 : Switch 2 circuits
* S1, S2 : Poussoir
* T1 : BC547 ou équivalent
* R1 : 10K
* R2 : 120
* R3 : 6.8K
* R4 : 330
* C1, C2 : 33pF
* LED1 : CQY89 ou équivalente
* LED2 : Led 3mm verte (non utilisée)
* IC1 : PIC18LF452TQFP (pour alimentation 2V à 5.5V) ou PIC18F452TQFP (pour alimentation 5V)
* Qz : 10MHz

* Boitier SA123 (dim ext. 100 x 65 x 23, dim int. 92 x 57 x 19)
* Led multicolore + résistance 56 Ohms
* 1 interrupteur 1T
* 1 bloc piles (2xAAA)