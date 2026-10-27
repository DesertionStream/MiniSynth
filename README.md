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
- Joue au clavier MIDI normalement (24 voix).
- **Arpège** : activé par défaut. Avec *Sync hôte* activé, il suit le tempo et la position de lecture de Live (il joue quand
  Live est en lecture). Désactivez *Sync hôte* pour qu'il tourne librement au **Tempo** du plugin (50 à 300 BPM).
- **Accords** : 4 accords de N mesures chacun ; pour chacun, choisissez la **note** (les 12) et le **mode** séparément
  (majeur, mineur, 7, maj7, min7, 9, dorien, phrygien, lydien, mixolydien, éolien, locrien, pentatoniques, blues…).
  Les accords défilent en boucle ; les modes (gammes) sont arpégés note par note.
- **Préréglages** (liste de programmes de l'hôte et menu du plugin) : Extrait, Pluck, Cloche, Basse, Cyberpunk, Voix, Nappe.
- Tous les paramètres sont automatisables dans Live.
