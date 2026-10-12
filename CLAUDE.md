# AppForge — carte du code

Framework C++/Qt 6 (Core + Widgets) pour assembler des applications à partir de composants fournis par des plugins.
Le [README](README.md) documente l'API publique en détail ; ce fichier résume où sont les choses et les pièges.
À tenir à jour quand l'architecture change.

## Arborescence

| Dossier | Cible | Rôle |
| --- | --- | --- |
| `Core/` | `AppForge::Core` (DLL) | Composants, plugins, fabrique. En-têtes publics dans `include/AppForge/{Component,Plugin}/`, sources dans `src/` |
| `Core/cmake/` | — | `appforge_add_plugin()` et les gabarits `plugin.h.in` / `plugin.json.in` |
| `Forge/` | `Forge` (exe Widgets) | L'éditeur graphique ; tout est dans `Forge/src/`, sans namespace |
| `exemple/ExamplePlugin` | `.afdl` dans `bin/plugins` | Composant `Greeter` (fournit `greeter` : `IGreeter`) |
| `exemple/ExternalPlugin` | `.afdl` dans `<build>/external-plugins` | `Ticker` (fournit `display` : `QWidget`) et `Window` (consomme `content` : `QWidget`) |
| `exemple/ExampleApp` | exe console | Scanne, charge, liste et fait tourner un `Greeter` |
| `CMakeUtils/` | sous-module git | `cmu_add_target` / `cmu_add_executable` / `cmu_add_shared_library` |
| `CFmake/` | non suivi, dépôt à part | Ne fait pas partie du build : ne pas y toucher |

## Build

- `cmu_add_*` trouve les sources par `GLOB_RECURSE CONFIGURE_DEPENDS` dans `src/` et `include/` : aucun fichier à
  lister dans les `CMakeLists.txt`. Il génère `<NAME>_export.h` (`CORE_EXPORT`) et `<NAME>_info.h`
  (`CoreInfo::version`, `<Nom>Info::buildDate`).
- `cmu_organization` vaut `FabienDG` : les ids réels sont `FabienDG.AppForge.<Plugin>.<Composant>`. Le README écrit
  encore `Shuko83.…` dans ses exemples ; seul l'IID `Shuko83.AppForge.IPlugin/2.0` porte ce nom dans le code.
- Sorties : `<build>/bin[/<config>]`, plugins dans `plugins/` à côté. Extension `.afdl`, seule scannée.
- `add_dependencies(Forge ExamplePlugin ExternalPlugin)` : construire Forge reconstruit les plugins du dépôt.
- Pas de cible de test. CI : Windows, MSVC 2022, Qt 6.8, Release.
- C++ récent requis (concepts, `std::ranges`, initialiseurs désignés, `std::type_identity_t`).

## Core

**Plugin** (`Plugin.h`, `IPlugin.h`, `PluginManager.*`)
- `appforge_add_plugin(NAME …)` crée une bibliothèque MODULE et génère l'objet racine
  `AppForge::Generated::<Nom>Plugin` (QObject + `IPlugin`, `Q_PLUGIN_METADATA` avec le JSON id / name / description /
  version / coreVersion).
- Le plugin écrit une classe dérivant de `AppForge::Plugin`, `APPFORGE_PLUGIN(Classe);` une fois, et un
  `build(<Composant>&)` public par composant, où il crée la logique (un QObject enfant du composant).
- `APPFORGE_REGISTER_COMPONENT(ClassePlugin, Composant);` : initialisation statique qui appelle `Plugin::add<P, T>()`
  (série de `static_assert`), stocké dans `registered<P>()` puis copié dans `m_componentTypes` par `instantiate<P>()`.
- `Plugin::Instance<T>` est la seule classe instanciable dérivant d'un composant : elle implémente la virtuelle pure
  privée `Component::builtByPlugin()`, et son destructeur appelle `releaseInterfaces()` avant celui de `T`.
- `PluginManager` : `scan(dir)` / `addFile(path)` lisent les métadonnées sans charger (contrôle de l'IID, puis même
  majeure de Core et version ≤ celle de Core) ; `load(id)` charge, lit la date de build et appelle
  `ComponentFactory::instance().add()`. Rien n'est jamais déchargé. Erreurs dans `errorString()`.

**Composant** (`Component.*`, `IComponent.h`, `ComponentFactory.*`, `ComponentInfo.h`, `InterfaceInfo.h`)
- `ComponentFactory` (singleton `instance()`) : `components()`, `component(id)`, `create(id)` qui renvoie un
  `unique_ptr<Component>` et fixe `m_componentId`. `ComponentInfo` est lu depuis le `QMetaObject` : `Q_CLASSINFO`
  `description` / `category`, et les propriétés à partir de `Component::staticMetaObject.propertyCount()`.
- États : `Initializing` → `initialize()` → `Ready` → `start()` → `Running` → `stop()` → `Ready`. Crochets
  `onInitialize()` / `onStart()` (peuvent refuser) / `onStop()`. Signal `stateChanged`.
- Interfaces, typées par `QMetaType::fromType<T*>()`, pointeurs stockés en `void*` :
  - `provideInterface<T>(nom[, pointeur])` (protégé) : déclare, fournit, ou retire avec `nullptr`. Remplacer émet
    `interfaceUnavailable` puis `interfaceAvailable`.
  - `consumeInterface<T>(nom, membre, onAvailable, onUnavailable)` (protégé) : le membre est écrit par Core.
  - `bindInterface(nom, fournisseur, nomFourni)` / `unbindInterface(nom)` (publics) : même type exigé, un consommateur
    n'a qu'un fournisseur, un fournisseur peut avoir plusieurs consommateurs.
  - `providedInterfaces()` / `consumedInterfaces()` renvoient des `InterfaceInfo {name, type, available}` ;
    `interfacesChanged()` est émis à chaque déclaration ou changement de disponibilité.
- Journalisation : catégories `appforge.components` et `appforge.plugins`.

## Forge

`main.cpp` → `MainWindow` : trois panneaux dans un splitter horizontal.

| Classe | Rôle |
| --- | --- |
| `MainWindow` | Possède `PluginManager` et `Assembly` ; `load()`, `openPlugin()` ; rafraîchit les modèles après un chargement. Menu **File** et fichiers de projet (voir plus bas). Son destructeur détruit la scène avant l'`Assembly` |
| `PluginModel` / `PluginViewer` | Liste des plugins (Name, Version, State ; `IdRole`) et fiche avec bouton **Load** (signal `loadRequested`) |
| `ComponentModel` / `ComponentViewer` | Liste des composants (Name, Category, Plugin), glissables au format `application/x-appforge-component` (id en UTF-8), et fiche avec toutes les propriétés |
| `Assembly` | Possède les instances (`unique_ptr<Component>`) ; `instantiate(id[, nom])` les nomme `Greeter1`, `Greeter2`… dans `objectName`, ou du nom lu dans un projet ; `components()`, `clear()` |
| `EditionScene` | `QGraphicsScene` 4000×3000, grille de 20 : dépôt = instanciation, tracé des liens entre points d'interface, menu contextuel des zones, `selectedComponent()` ; `save()` / `load()` / `reset()` pour les projets |
| `ComponentItem` | `QGraphicsObject` déplaçable et sélectionnable ; reconstruit ses `InterfaceItem` sur `interfacesChanged` |
| `InterfaceItem` | Point sur le bord (consommée à gauche, fournie à droite), `Type = UserType + 1`, `canLinkTo()`, infobulle, estompage ; connaît ses `LinkItem` |
| `LinkItem` | Courbe entre deux points, Z = -1, se supprime avec le premier `InterfaceItem` détruit ; `curve()` statique sert aussi au lien en cours de tracé |
| `ZoneItem` | Zone colorée titrée, `Type = UserType + 2`, Z = -aire ; déplacement par le titre avec son contenu, redimensionnement par les bords, édition du titre sur place ; sa `shape()` ne couvre que titre et bords |
| `PropertyEditor` | `QTableWidget` des propriétés inscriptibles de type bool, int, uint, double, QString, QDate, QTime, QDateTime ; suit les signaux NOTIFY |

Les chaînes affichées passent par `tr()`, en anglais.

**Fichier de projet `.afp`** (JSON, version 1, format décrit dans le README)
- `EditionScene::save()` écrit `components` (nom, type, x, y, propriétés inscriptibles qui font l'aller-retour JSON),
  `bindings` (lus depuis les `LinkItem`, Core n'expose pas à quoi une interface est liée) et `zones`.
  `MainWindow::project()` y ajoute `version` et `plugins` (id + chemin absolu, seulement ceux des composants présents).
- `MainWindow::openProjectFile()` charge les plugins, puis `EditionScene::load()` vide la scène et la reconstruit ; les
  deux renvoient la liste de ce qui n'a pas pu être restauré, affichée dans une seule boîte. Après une ouverture
  partielle, le projet n'a plus de fichier associé pour que **Save** n'écrase pas ce qui manque.
- « Modifié » n'est pas suivi au fil de l'eau : `saveChanges()` compare `project()` à `m_savedProject`, mémorisé par
  `setProjectFile()`. Toute nouvelle donnée à sauvegarder doit donc passer par `project()`.
- `Forge fichier.afp` ouvre le projet au démarrage.
- Les dialogues partent de `scene/` à côté de `Forge.exe` (`defaultProjectDirectory()`, créé au premier
  **Save As...**), ou du dossier du fichier du projet quand il en a un.

## Pas encore fait

- Forge n'appelle jamais `initialize()` / `start()` : les instances restent en `Initializing`.
- Pas de suppression d'instance ni de lien dans Forge (`unbindInterface` n'y est pas utilisé) ; seules les zones se
  suppriment.
- Le fichier de projet n'est lu que par Forge : rien dans Core ne permet de lancer une application depuis un `.afp`.
- Pas de tests automatisés.

## Pièges

- Modifier `Component.h`, `IComponent.h` ou `Plugin.h` change l'agencement binaire : tous les plugins doivent être
  reconstruits, y compris ceux ouverts par **Open plugin...**, sinon plantage à la création d'un composant. Le contrôle
  de version ne compare que la majeure de Core.
- `MainWindow::~MainWindow` : les items et l'éditeur de propriétés référencent les composants de l'`Assembly` ; garder
  l'ordre de destruction actuel quand un membre est ajouté.
- Un composant ne peut pas être `final`, a besoin d'un constructeur par défaut et de `Q_OBJECT`.
- `Ticker` et `Window` ont besoin d'un `QApplication` (widgets) : pas utilisables dans `ExampleApp`.

## Conventions

- `.clang-format` : base LLVM, 4 espaces, 120 colonnes, accolades Allman, `if(` sans espace, `T* p`. `.clang-tidy`
  actif (bugprone, performance, readability, modernize…).
- Membres `m_camelCase`, `[[nodiscard]]` sur les accesseurs, `#pragma once`, `QStringLiteral` / `u"…"`.
- Commentaires en anglais, au-dessus de la déclaration, qui disent le pourquoi ou le contrat.
- Commits en français, préfixés : `feat(Forge): …`, `fix(Forge): …`, `refactor: …`.
- Toute nouvelle fonctionnalité visible est décrite dans le README (sections Plugin, Component, Forge).
