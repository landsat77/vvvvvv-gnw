# VVVVVV — installation

Requires Retro-Go SD **v2.0.0-rc1 or later** on the Game & Watch, and
**enough free room in its external flash** for the pack (44 MB by
default), which is copied there.

The game data is not included: you need the full game, VVVVVV 2.x, and its
`data.zip` file:

- Steam: right click VVVVVV → Manage → Browse local files: `data.zip`
- GOG / itch.io / Humble: in the folder of the game
- macOS: inside `VVVVVV.app` (right click → Show Package Contents →
  `Contents/Resources/data.zip`)

1. Build the data pack (Python 3, nothing else to install). Keep the
   `vv_render` and `assets` folders and `vv_levels.bin` next to
   `vv_pack.py`:

       python3 vv_pack.py VVVVVV.vvv /path/to/data.zip

   On Windows:

       py vv_pack.py VVVVVV.vvv "C:\Program Files (x86)\Steam\steamapps\common\VVVVVV\data.zip"

   The script asks which music to put in the pack (or give it with
   `--music`):

   | `--music` | Pack | Music |
   |---|---|---|
   | `ogg` | ~66 MB | original songs, not converted (best sound) |
   | `32000` | ~44 MB | mono 32 kHz (default) |
   | `22050` | ~31 MB | mono 22 kHz |
   | `11025` | ~17 MB | mono 11 kHz |
   | `none` | ~3.5 MB | no music |

   `ogg` only works with an external flash bigger than 64 MB (the pack
   and the firmware must fit in it).

   The pack goes with this version of `VVVVVV.bin` (the rooms of the game
   are in it): after an update, build it again.

2. Copy to the SD card, side by side, with the same name:

       /homebrews/VVVVVV.bin
       /homebrews/VVVVVV.vvv

3. Start "VVVVVV" from the Homebrew tab. The first start copies the pack to
   the external flash: it takes a few minutes.

Controls: D-pad = move, A = flip / action (select in the menus), B = flip
(back in the menus), TIME = map / teleporters / terminals, GAME = pause
menu, PAUSE/SET = Retro-Go menu (savestates; Options: music, sound, swap
A/B). A savestate keeps the player where he stands (not during a
cutscene). The game's own saves are in `/data/vvvvvv/VVVVVV/` on the SD
card.

---

# VVVVVV — installation (français)

Nécessite Retro-Go SD **v2.0.0-rc1 ou plus récent** sur la Game & Watch, et
**assez de place libre dans sa flash externe** pour le pack (44 Mo par
défaut), qui y est copié.

Les données du jeu ne sont pas fournies : il faut le jeu complet,
VVVVVV 2.x, et son fichier `data.zip` :

- Steam : clic droit sur VVVVVV → Gérer → Parcourir les fichiers locaux :
  `data.zip`
- GOG / itch.io / Humble : dans le dossier du jeu
- macOS : dans `VVVVVV.app` (clic droit → Afficher le contenu du paquet →
  `Contents/Resources/data.zip`)

1. Créer le pack de données (Python 3, rien d'autre à installer). Garder
   les dossiers `vv_render` et `assets` et `vv_levels.bin` à côté de
   `vv_pack.py` :

       python3 vv_pack.py VVVVVV.vvv /chemin/vers/data.zip

   Sous Windows :

       py vv_pack.py VVVVVV.vvv "C:\Program Files (x86)\Steam\steamapps\common\VVVVVV\data.zip"

   Le script demande quelle musique mettre dans le pack (ou la donner avec
   `--music`) :

   | `--music` | Pack | Musique |
   |---|---|---|
   | `ogg` | ~66 Mo | morceaux d'origine, sans conversion (meilleur son) |
   | `32000` | ~44 Mo | mono 32 kHz (défaut) |
   | `22050` | ~31 Mo | mono 22 kHz |
   | `11025` | ~17 Mo | mono 11 kHz |
   | `none` | ~3,5 Mo | pas de musique |

   `ogg` ne marche qu'avec une flash externe de plus de 64 Mo (le pack et
   le firmware doivent y tenir).

   Le pack va avec cette version de `VVVVVV.bin` (les salles du jeu sont
   dedans) : après une mise à jour, le refaire.

2. Copier sur la carte SD, côte à côte, avec le même nom :

       /homebrews/VVVVVV.bin
       /homebrews/VVVVVV.vvv

3. Lancer « VVVVVV » depuis l'onglet Homebrew. Au premier lancement, le pack
   est copié dans la flash externe : cela prend quelques minutes.

Commandes : croix = se déplacer, A = inverser la gravité / action (valider
dans les menus), B = inverser la gravité (retour dans les menus), TIME =
carte / téléporteurs / terminaux, GAME = menu pause, PAUSE/SET = menu
Retro-Go (sauvegardes ; Options : musique, son, inverser A/B). Une
sauvegarde Retro-Go garde le joueur là où il est (pas pendant une
cinématique). Les sauvegardes du jeu lui-même sont dans
`/data/vvvvvv/VVVVVV/` sur la carte SD.
