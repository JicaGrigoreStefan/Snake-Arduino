PROIECT ARDUINO - Jocul Snake matrice 8x8
🔧 Ce face proiectul:
1. Afișează jocul Snake pe o matrice de LED-uri 8x8

- Sarpele este reprezentat de LED-uri aprinse care se mișcă pe matrice.

- Fructele (sau "mâncarea") apar ca un LED aprins într-o poziție aleatoare.

- Când "șarpele" ajunge la fruct, crește în lungime.

2. Permite controlul direcției șarpelui

- De obicei cu butoane (sus/jos/stânga/dreapta) sau un joystick analogic.


Mod de funtionare :
 1. Matricea 8x8 afișează jocul Snake folosind LED-uri. Fiecare LED reprezintă o poziție pe care se poate afla șarpele sau mâncarea.
 2. Șarpele este reprezentat de o listă de coordonate (ex: (x, y)) care formează corpul. La fiecare pas, capul se mută într-o direcție, iar coada se șterge (dacă nu a mâncat).
 3. Controlul direcției se face cu 4 butoane sau un joystick, care trimit semnale către Arduino pentru a schimba direcția de mers a șarpelui.
 4. Mâncarea apare într-o poziție aleatoare. Dacă șarpele ajunge acolo, crește în lungime (nu se mai șterge coada) și se generează o nouă mâncare.
 5. Coliziunile (propriul corp) opreste jocul. Arduino detectează aceasta situație și poate reseta jocul automat sau la apăsarea unui buton.
 6. Totul este controlat prin cod C++ pe Arduino, care actualizează continuu matricea în funcție de pozițiile șarpelui și inputul primit.


🛠 Componente necesare :
Arduino NANO (clone)	/	UNO can be used as well
LED Matrix		(MAX7219 controlled 8x8 LED Matrix)
Joystick
Potentiometer	(any 1k ohm to 100k ohm should be fine)
Some wires	(12 wires needed)
Breadboard	
