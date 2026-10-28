# Mini Synthé — VST3 pour Ableton Live 11

Synthétiseur polyphonique léger (24 voix) avec arpégiateur intégré, réglé à partir de l'extrait audio fourni.
Format : **VST3** (Live 11 gère le VST3 sous Windows et macOS ; le VST2 n'est plus distribuable librement).

## Obtenir le plugin (aucune installation de compilateur)
1. Créez un dépôt GitHub vide et poussez-y le contenu de ce dossier (`git init`, `git add .`, `git commit`, `git push`).
2. Onglet **Actions** → workflow *Build VST3* → attendez la fin (≈ 10 min).
3. Téléchargez l'artefact `MiniSynth-VST3-windows-latest` ou `MiniSynth-VST3-macos-latest`.

## Compiler soi-même
Prérequis : CMake ≥ 3.22, Visual Studio 2022 (Windows) ou Xcode (macOS), accès à GitHub (JUCE 8.0.8 est téléchargé).
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Résultat : `build/MiniSynth_artefacts/Release/VST3/Mini Synthe.vst3`
(avec un JUCE local : `-DMS_JUCE_PATH=/chemin/JUCE`).

## Installation
- **Windows** : copier `Mini Synthe.vst3` dans `C:\Program Files\Common Files\VST3\`.
- **macOS** : copier dans `~/Library/Audio/Plug-Ins/VST3/`. Le plugin n'est pas signé : si macOS le bloque, lancer
  `xattr -cr "~/Library/Audio/Plug-Ins/VST3/Mini Synthe.vst3"` puis `codesign --force --deep -s - "~/Library/Audio/Plug-Ins/VST3/Mini Synthe.vst3"`.
- **Live 11** : *Préférences → Plug-ins* → activer « Utiliser les dossiers de plug-ins VST3 système » → *Rescan*.
  Ajouter « Mini Synthe » sur une piste MIDI (liste *Plug-ins*, catégorie Instruments).

## Utilisation
- Placer le plugin sur une **piste MIDI**. Par défaut l'arpège joue les notes MIDI du clip ou du clavier (source « Notes MIDI jouées ») ; choisissez « Accords du plugin » pour l'arpège automatique.
- **Arpège** : activé par défaut. Avec *Sync hôte* activé, il suit le tempo et la position de lecture de Live (il joue quand
  Live est en lecture). Désactivez *Sync hôte* pour qu'il tourne librement au **Tempo** du plugin (50 à 300 BPM).
- **Accords** : 4 accords de N mesures chacun ; pour chacun, choisissez la **note** (les 12) et le **mode** séparément
  (majeur, mineur, 7, maj7, min7, 9, dorien, phrygien, lydien, mixolydien, éolien, locrien, pentatoniques, blues…).
  Les accords défilent en boucle ; les modes (gammes) sont arpégés note par note.
- **Préréglages** (menu en haut du plugin ; ils ne sont plus appliqués par l'hôte, pour ne jamais écraser vos réglages au rechargement) : Extrait, Pluck, Cloche, Basse, Cyberpunk, Voix, Nappe.
- Tous les paramètres sont automatisables dans Live.

## Nouveautés
- **Source des notes** : « Accords du plugin » ou « Notes MIDI jouées ». En mode MIDI, l'arpège joue les notes que votre clip
  (piano roll d'Ableton) ou votre clavier envoie : les notes tenues sont arpégées, l'accord le plus grave sert de base.
  Placez le plugin sur une **piste MIDI** et dessinez vos notes dans le clip.
- **Piano roll intégré** (onglet « Piano roll ») : une mesure de 16 doubles croches, 37 lignes autour de la fondamentale
  de l'accord. Cliquez/glissez pour poser ou retirer des notes ; elles se transposent avec chaque accord.
- **Réglages mémorisés** : enregistrés dans le projet Live (réglages + piano roll). Boutons « Sauver… / Charger… » pour des
  presets `.msynth`, et « Définir par défaut » pour les retrouver à chaque nouvelle instance.
