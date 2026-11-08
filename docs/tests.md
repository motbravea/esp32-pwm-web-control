# Plan de tests — ESP32 PWM et serveur web

## Préparation
- DEL câblée sur GPIO18, avec résistance en série de 220 Ω et GND.
- Arduino-ESP32 3.x ; Moniteur série à **115200 bauds**.
- Navigateur et ESP32 sur le même réseau Wi-Fi.

| Test | Requête / action | Attendu |
|---|---|---|
| Démarrage | Réinitialiser l'ESP32 | DEL éteinte ; IP affichée |
| 0 % | `/pwm?duty=0` | PWM 0, DEL éteinte |
| 25 % | `/pwm?duty=25` | PWM 64, env. 0,83 V |
| 50 % | `/pwm?duty=50` | PWM 128, env. 1,65 V |
| 75 % | `/pwm?duty=75` | PWM 191, env. 2,48 V |
| 100 % | `/pwm?duty=100` | PWM 255, env. 3,30 V |
| Augmenter | `/plus` depuis 50 % | 60 %, PWM 153 |
| Diminuer | `/moins` depuis 60 % | 50 %, PWM 128 |
| Limite haute | `/plus` depuis 100 % | Reste à 100 % |
| Limite basse | `/moins` depuis 0 % | Reste à 0 % |
| Texte invalide | `/pwm?duty=abc` | HTTP 400, intensité inchangée |
| Dépassement | `/pwm?duty=150` | HTTP 400, intensité inchangée |
| Paramètre absent | `/pwm` | HTTP 400, intensité inchangée |
| Route inexistante | `/inconnu` | HTTP 404 |

### Reconnexion Wi-Fi
1. Choisir 75 %.
2. Désactiver temporairement le point d'accès et le réactiver.
3. Vérifier que la DEL garde 75 % pendant la déconnexion.
4. Vérifier le retour de l'IP et l'accès HTTP.

Le logiciel tente une reconnexion toutes les dix secondes. **La simulation d'une panne prolongée doit être vérifiée sur le matériel** avant de déclarer ce cas complètement testé.

### Captures / preuves
Conserver des photos du montage et des captures de la page web sur **ordinateur et téléphone**, ainsi que les journaux du moniteur série. Ces images peuvent être ajoutées dans `media/`.
