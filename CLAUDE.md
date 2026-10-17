# Nebula — carte du code

Framework C++/Qt 6 (Core + Widgets) pour assembler des applications à partir de composants fournis par des plugins.
Le [README](README.md) documente l'API publique en détail ; ce fichier résume où sont les choses et les pièges.
À tenir à jour quand l'architecture change.

## Arborescence

| Dossier | Cible | Rôle |
| --- | --- | --- |
| `Core/` | `Nebula::Core` (DLL) | Composants, plugins, fabrique, application. En-têtes publics dans `include/Nebula/{Application,Component,Plugin,Settings}/`, sources dans `src/` |
| `Core/cmake/` | — | `nebula_add_plugin()` et les gabarits `plugin.h.in` / `plugin.json.in`, livrés avec `Core` dans le paquet |
| `NebulaStudio/` | `NebulaStudio` (exe Widgets) | L'éditeur graphique ; le code est dans `NebulaStudio/src/`, sans namespace, celui des fichiers de réglages dans `NebulaStudio/src/Settings/`, le logo de Nebula, les icônes du glossaire et la police des titres dans `NebulaStudio/resources/` |
| `exemple/ExamplePlugin` | `.nbdl` dans `bin/plugins` | Composant `Greeter` (fournit `greeter` : `IGreeter`) |
| `exemple/ExternalPlugin` | `.nbdl` dans `<build>/external-plugins` | `Ticker` (fournit `display` : `QWidget`, dont il expose les propriétés dans le groupe `Display`), `ProfileSelector` (fournit `display` : la `QComboBox` de ses `profiles`, où l'utilisateur choisit `profile`) et `Window` (consomme `content` : `QWidget`, `Mandatory`) |
| `exemple/ExampleApp` | exe console | Scanne, charge, liste et fait tourner un `Greeter` |
| `exemple/ExampleUniverseApp` | exe console + Widgets | Le `main` type : options de `Universe::addCommandLineOptions()`, `applyCommandLine()`, `open()` du projet passé en argument, puis `start()` |
| `CMakeUtils/` | sous-module git | `cmu_add_target` / `cmu_add_executable` / `cmu_add_shared_library` |
| `docs/release-notes/` | — | Notes de chaque version, `<x.y.z>.md` : le texte de sa release GitHub |
| `docs/glossary.md` | — | Glossaire Nebula (Nebula, Univers, Galaxy, Astre, Comet), en anglais puis en français, référencé par le README : le vocabulaire à employer pour nommer code, documentation et interface ; Nebula et Univers (la classe `Universe`) sont déjà employés |
| `CFmake/` | non suivi, dépôt à part | Ne fait pas partie du build : ne pas y toucher |

## Build

- `cmu_add_*` trouve les sources par `GLOB_RECURSE CONFIGURE_DEPENDS` dans `src/` et `include/` : aucun fichier à
  lister dans les `CMakeLists.txt`. Il génère `<NAME>_export.h` (`CORE_EXPORT`) et `<NAME>_info.h`
  (`CoreInfo::version`, `<Nom>Info::buildDate`).
- `cmu_organization` vaut `FabienDG`, défini avant l'inclusion de CMakeUtils : les ids sont
  `FabienDG.Nebula.<Plugin>.<Composant>` et l'IID `FabienDG.Nebula.IPlugin/2.0`. `cmu_copyright`, défini au même
  endroit, vaut `Copyright (C) 2026 Fabien Di Grande`, celui de la licence, à la place de celui que CMakeUtils
  tirerait de l'organisation : il est écrit dans les fichiers d'infos (`<NAME>_info.h`, ressources Windows) et
  affiché par le splash de NebulaStudio. `Shuko83` reste le compte GitHub : adresse du dépôt, du sous-module
  CMakeUtils, et `organizationDomain` (valeur par défaut de CMakeUtils).
- Sorties : `<build>/bin[/<config>]`, plugins dans `plugins/` à côté. Extension `.nbdl`, seule scannée.
- `add_dependencies(NebulaStudio ExamplePlugin ExternalPlugin)`, de même pour `ExampleUniverseApp` : construire l'un
  reconstruit les plugins du dépôt.
- Pas de cible de test. CI : Windows, MSVC 2022, Qt 6.8, Release. `release.yml` fait la même compilation sur un tag
  de version (`0.2.0`) et publie la release (voir Process) ; sur une PR qui le modifie, il s'exécute sans rien publier et dépose le zip
  en artefact.
- C++ récent requis (concepts, `std::ranges`, initialiseurs désignés, `std::type_identity_t`).
- Paquet : CMakeUtils installe chaque target dans `<préfixe>/<target>/` (mode `TARGET`) et copie le résultat dans
  `<build>/package` après chaque build. `cmu_add_cmake_files(TARGET Core …)` y livre `NebulaPlugin.cmake`,
  `Template/` et CMakeUtils dans `Core/cmake/`, que `CoreConfig.cmake` inclut : un projet tiers n'a besoin que de
  `find_package(Nebula COMPONENTS Core)` pour appeler `nebula_add_plugin()`.

## Core

**Plugin** (`Plugin.h`, `IPlugin.h`, `PluginManager.*`)
- `nebula_add_plugin(NAME …)` crée une bibliothèque MODULE et génère l'objet racine
  `Nebula::Generated::<Nom>Plugin` (QObject + `IPlugin`, `Q_PLUGIN_METADATA` avec le JSON id / name / description /
  version / coreVersion).
- Le plugin écrit une classe dérivant de `Nebula::Plugin`, `NEBULA_PLUGIN(Classe);` une fois, et un
  `build(<Composant>&)` public par composant, où il crée la logique (un QObject enfant du composant).
- `NEBULA_REGISTER_COMPONENT(ClassePlugin, Composant);` : initialisation statique qui appelle `Plugin::add<P, T>()`
  (série de `static_assert`), stocké dans `registered<P>()` puis copié dans `m_componentTypes` par `instantiate<P>()`.
- `Plugin::Instance<T>` est la seule classe instanciable dérivant d'un composant : elle implémente la virtuelle pure
  privée `Component::builtByPlugin()`, et son destructeur appelle `releaseInterfaces()` avant celui de `T`.
- `PluginManager` : `scan(dir)` / `addFile(path)` lisent les métadonnées sans charger (contrôle de l'IID, puis même
  majeure de Core et version ≤ celle de Core) ; `load(id)` charge, lit la date de build et appelle
  `ComponentFactory::instance().add()` ; `unload(id)` appelle `ComponentFactory::remove()` puis
  `QPluginLoader::unload()`, qui détruit l'objet racine, donc la classe du plugin. La DLL, elle, reste en mémoire
  (voir Pièges) : recharger recrée l'objet racine sans relire le fichier. Erreurs dans `errorString()`.
- RAII : le destructeur décharge ce que ce gestionnaire a chargé (`Entry::loaded`). Il doit donc survivre aux
  composants de ses plugins : dans `Universe`, `m_plugins` est déclaré avant `m_components`. Chaque
  gestionnaire prend sa part du plugin par `QPluginLoader::load()`, que Qt compte : un plugin chargé par deux
  gestionnaires reste tant que l'un des deux le garde, et `unload()` renvoie alors `false`.

**Composant** (`Component.*`, `IComponent.h`, `ComponentFactory.*`, `ComponentInfo.h`, `InterfaceInfo.h`)
- `ComponentFactory` (singleton `instance()`) : `components()`, `component(id)`, `create(id)` qui renvoie un
  `unique_ptr<Component>` et fixe `m_componentId`, `remove(pluginId)` qui oublie les composants d'un plugin.
  `ComponentInfo` est lu depuis le `QMetaObject` : `Q_CLASSINFO` `description` / `category`, et les propriétés à partir de `Component::staticMetaObject.propertyCount()`.
- Deux modes : `Editing` (à la création : propriétés réglées, interfaces liées) → `start()` → `Running` →
  `stop()` → `Editing`. Crochets `onStart()` (peut refuser) / `onStop()`. Signal `stateChanged`.
- Interfaces, typées par `QMetaType::fromType<T*>()`, pointeurs stockés en `void*` :
  - `provideInterface<T>(nom[, pointeur])` (protégé) : déclare, fournit, ou retire avec `nullptr`. Remplacer émet
    `interfaceUnavailable` puis `interfaceAvailable`.
  - `consumeInterface<T>(nom, membre[, InterfaceRequirement], onAvailable, onUnavailable)` (protégé) : le membre est
    écrit par Core. `Mandatory` (`Optional` sinon) dit seulement que le composant ne fonctionne pas sans elle : rien
    ne le vérifie, NebulaStudio le montre.
  - `bindInterface(nom, fournisseur, nomFourni)` / `unbindInterface(nom)` (publics) : même type exigé, un consommateur
    n'a qu'un fournisseur, un fournisseur peut avoir plusieurs consommateurs.
  - `providedInterfaces()` / `consumedInterfaces()` renvoient des `InterfaceInfo {name, type, available, requirement}` ;
    `interfacesChanged()` est émis à chaque déclaration, liaison, déliaison ou changement de disponibilité.
  - `binding(nom)` dit à quoi une interface consommée est liée : `InterfaceBinding {provider, providedName}`.
  - `unboundMandatoryInterfaces()` : les consommées `Mandatory` liées à rien. Vide = composant opérationnel ; c'est ce
    que NebulaStudio signale (liée suffit, même si le fournisseur n'a pas encore de pointeur, comme `Greeter` en
    édition).
- Objets à propriétés (`PropertyObject.*`) : `addPropertiesForQObject(objet[, nom])` (protégé, appelé par le composant
  à sa construction) ajoute un `PropertyObject {object, name}` à `propertyObjects()` ; il en sort seul à sa
  destruction. Signal `propertyObjectsChanged`. `PropertyObject::properties()` : les propriétés déclarées, sans
  `objectName` ni celles `DESIGNABLE false` / `STORED false` ; c'est la liste qu'éditent NebulaStudio et qu'écrit un
  projet.
- Journalisation : catégories `nebula.components` et `nebula.plugins`.

**Application** (`Application/Universe.*`)
- `Universe` (QObject) : l'application assemblée, utilisée telle quelle par NebulaStudio et par le `main` d'une
  application. Possède un `PluginManager` (`plugins()`, scanne `plugins/` à la construction) et les instances
  (`unique_ptr<Component>`), nommées `Greeter1`, `Greeter2`… dans `objectName`. `rename(instance, nom)` renomme en
  gardant les noms uniques et non vides : c'est par son nom qu'un projet et `component(nom)` désignent une instance.
  `remove(instance)` la détruit ; ses liens tombent à sa destruction (`releaseInterfaces()` pour ce
  qu'elle consomme, signal `destroyed` pour ceux qui consomment ce qu'elle fournit). `remove()` et `clear()` la
  sortent de `components()` avant de la détruire, pour que ce que sa destruction fait émettre voie une liste cohérente.
  Signal `componentsChanged` après `instantiate()`, `remove()` et `clear()` (s'il y avait des instances).
- Projet : `read(fichier, erreur)` statique (lecture + contrôle de version), `load(json)` qui renvoie la liste de ce
  qui n'a pas pu être restauré, `open(fichier)` = les deux, strict (`false` au moindre problème), `save()` qui écrit
  `version`, `plugins`, `components` (nom, type, propriétés inscriptibles qui font l'aller-retour JSON, et `objects` : celles de ses
  `propertyObjects()`, retrouvés à l'ouverture par nom puis par ordre d'ajout) et `bindings`.
- Les chemins des plugins sont relatifs à `pluginRoot` (dossier de l'exécutable par défaut, `setPluginRoot()`) ; un
  chemin absolu dans un fichier reste lu tel quel. Seuls les plugins des composants présents sont écrits. À
  l'ouverture, un plugin est d'abord cherché par son id parmi ceux connus, son fichier n'est qu'un recours.
- Ligne de commande : `addCommandLineOptions(parser)` (statique) ajoute `--plugin-dir` (répétable) et `--plugin-root`,
  `applyCommandLine(parser)` scanne chaque dossier après `plugins/`, dans l'ordre (un id déjà connu est sauté par
  `PluginManager::scan()`), puis fixe la racine. NebulaStudio (par `MainWindow`, avant de remplir ses modèles) et
  `ExampleUniverseApp` s'en servent, avant toute ouverture de projet.
- Les liens ne sont pas mémorisés par l'application : `save()` les lit avec `Component::binding(nom)`.
- Exécution : `start()` mémorise les valeurs d'édition (`keepEditedValues()` : pour chaque composant et chacun de ses
  `propertyObjects()`, un `EditedValues {QPointer, propriétés, JSON}` de ce qu'écrit un projet), démarre les fichiers
  de réglages, puis les composants dans l'ordre d'instanciation ; au premier refus il arrête ce qu'il a démarré et
  revient à l'édition. Rappelé pendant l'exécution, il ne fait rien. `stop()` arrête dans l'ordre inverse, puis
  `returnToEdition()` arrête les fichiers de réglages et réécrit toutes les valeurs d'édition (`writeProperty()`).
  Pendant l'exécution, `save()` écrit ces valeurs-là (`savedValues()`), et `instantiate()` / `rename()` / `remove()`
  refusent ; les liens et les fichiers de réglages ne sont verrouillés que par NebulaStudio. `isRunning()` /
  `runningChanged`. `clear()`, `load()` et le destructeur arrêtent d'abord.
- Déchargement : `componentsOf(pluginId)` liste les instances d'un plugin ; `unloadPlugin(pluginId)` les supprime puis
  appelle `PluginManager::unload()`, et refuse (`errorString()`) pendant l'exécution.
- Fichiers de réglages (module `Settings` : `include/Nebula/Settings/SettingsFile.h`, `src/Settings/`) : ce que
  l'utilisateur de l'application change et retrouve au lancement suivant. `addSettingsFile()` / `removeSettingsFile()`
  / `settingsFiles()`, possédés par l'application (constructeur privé), écrits dans le projet (`settings`, seulement
  s'il y en a). Un `SettingsFile` a `fileName`, unique dans l'application sans tenir compte de la casse ni du dossier
  (`settingsFile(nom)`, `setFileName()` renvoie `false` sur un nom pris ; `addSettingsFile()` donne `settings.json`,
  puis `settings2.json`…, et `load()` signale un doublon, qui garde ce nom-là), `directory` (vide = `AppConfigLocation`
  de l'exécutable qui tourne, relatif = depuis son dossier)
  et des `SettingsEntry {component, object, property}`, `object` étant le composant ou un de ses `propertyObjects()` ;
  une propriété n'est que dans un fichier (`Universe::settingsFile(entry)`), seulement celles qu'un projet
  enregistre (`canHold()`), et une entrée part à la destruction de son objet (`destroyed`). `start()` (à la transition
  vers « en cours » seulement) lit chaque fichier avant de démarrer le premier composant, l'écrit avec les valeurs du projet
  s'il manque ou est incomplet, met de côté en `.bak` un fichier illisible, puis suit les NOTIFY (écriture regroupée
  par `QTimer::singleShot(0)`) ; `stop()` écrit une dernière fois et remet les valeurs du projet, mémorisées au
  `start()`. Le contenu inconnu d'un fichier est conservé. Problèmes : `SettingsFile::failed` →
  `Universe::settingsFileFailed`, journal `nebula.settings`. `src/Application/ProjectJson.*` (privé,
  inclus depuis `src/Settings/` par `"Application/ProjectJson.h"`) : l'écriture JSON des propriétés et la clé d'un
  objet (nom, puis rang parmi ceux de ce nom), partagées avec `Universe`.
- Messages d'erreur de Core en anglais, non traduits (`QStringLiteral`), comme ceux de `PluginManager`.

## NebulaStudio

`main.cpp` → `SplashScreen`, puis `MainWindow` : trois panneaux dans un splitter horizontal. `SettingsItem`,
`SettingsLinkItem` et `SettingsDialog` sont dans `src/Settings/`, inclus par `"Settings/SettingsItem.h"`.

| Classe | Rôle |
| --- | --- |
| `Theme` (`Theme.h`) | Fonctions libres, comme `Colors.h` : `ThemeColors`, les jetons de couleur nommés par usage (`space`, `field`, `surface`, `edge`, `text`, `mutedText`, `accent`, `onAccent`…), définis une fois par schéma dans `Theme.cpp` (`themeColors(Qt::ColorScheme)`, sombre si le système ne dit rien) ; `warning` / `onWarning` (ambre, ce qui ne fonctionne pas) n'ont pas de rôle dans la palette : on les lit par `currentThemeColors()` au moment de peindre ; `applyTheme()`, appelé par `main.cpp` juste après le `QApplication`, met le style Fusion (le seul qui peint tout aux couleurs de la palette), charge Space Grotesk Bold depuis les ressources et donne à l'application la palette du schéma du système, puis à chaque `QStyleHints::colorSchemeChanged` ; `titleFont(police)` : Space Grotesk gras, la famille de départ en repli, pour les noms des items, l'éditeur de nom, les titres de groupes du `PropertyEditor` et le splash ; `icon(GlossaryTerm)` et `logoPath()`, voir Logo et icônes. Chaque texte et son fond, et les bords de ce qui s'utilise, tiennent le niveau AA des WCAG (4,5 et 3) |
| `SplashScreen` | `QSplashScreen` dont l'image est peinte aux couleurs de la palette depuis `NebulaStudio_info.h` (produit, version, Qt, compilateur, date de build, copyright) et la licence ; `read()` est émis une fois, deux secondes après son premier affichage ou au premier clic. `main.cpp` le montre (`processEvents()` sans les entrées utilisateur, pour qu'un clic attende la connexion), construit `MainWindow`, puis sur `read()` montre la fenêtre, appelle `finish()` et ouvre le projet passé en argument |
| `MainWindow` | Possède l'`Universe` (donc le `PluginManager` et les instances) ; `load()`, `unload()` (confirmation quand des instances du plugin sont dans la scène, puis `EditionScene::remove()` et `Universe::unloadPlugin()`), `openPlugin()` ; rafraîchit les modèles après un chargement ou un déchargement. Menus **File** et **Run** (voir plus bas). Barre d'état : `refused` et `told` de la scène, `settingsFileFailed` de l'application. Au centre, splitter vertical : vue de la scène, `PropertyEditor`, `ProblemList` (son `componentActivated` → `EditionScene::select()` puis `centerOn()`). Son destructeur détruit la scène avant l'application |
| `ProblemList` | `QTreeWidget` (Component, Problem, How to fix it) d'une ligne par interface de `Component::unboundMandatoryInterfaces()`, l'aide nommant les interfaces fournies du même type (`Ticker1.display`), ou une phrase quand il n'y en a pas. Suit `Universe::componentsChanged`, et `interfacesChanged` / `objectNameChanged` de chaque composant ; reconstruit en différé (`scheduleRefresh()`, file d'événements), car ces signaux arrivent pendant une destruction. Activation d'une ligne = `componentActivated(nom)` |
| `PluginModel` / `PluginViewer` | Liste des plugins (Name, Version, State ; `IdRole`) et fiche avec boutons **Load** et **Unload** (signaux `loadRequested`, `unloadRequested`) ; `setUnloadAllowed(false)` désactive **Unload** pendant l'exécution |
| `ComponentModel` / `ComponentViewer` | Liste des composants (Name, Category, Plugin ; la description en infobulle), glissables au format `application/x-nebula-component` (id en UTF-8), et fiche avec toutes les propriétés ; `refresh()` insère les composants enregistrés depuis et retire ceux d'un plugin déchargé. La vue la montre à travers un `QSortFilterProxyModel` créé par `MainWindow` : barre de recherche (Ctrl+F), sans casse, sur la colonne et le rôle de la liste déroulante à côté (`SearchFields` dans `MainWindow.cpp`) : Name en `DisplayRole` (nom), Name en `ToolTipRole` (description), Category en `DisplayRole` (catégorie). Un rôle d'index de la vue se lit donc à travers le proxy (`data()`), pas de `row()` à passer au modèle |
| `EditionScene` | `QGraphicsScene` 4000×3000, grille de 20 : dépôt = `Universe::instantiate()`, tracé des liens entre points d'interface (`Component::bindInterface()`), menu contextuel des zones, des fichiers de réglages (**Add settings file** ouvre aussitôt sa fenêtre ; sur l'un d'eux : éditer, **Write default values**, supprimer) et du composant sous le curseur (renommer, couleur, supprimer), double-clic sur un fichier de réglages = `SettingsDialog` (ouvert par `open()`, pas `exec()`), touche Suppr = suppression des composants et fichiers sélectionnés sauf pendant l'édition d'un nom (`remove()` détruit l'item, donc ses liens, avant `Universe::remove()` / `removeSettingsFile()`), `selectedComponent()`, `select(composant)` (le sélectionne seul, renvoie son item) ; `remove(liste)` public pour le déchargement d'un plugin ; `save()` / `load()` / `reset()` pour les projets ; signaux `refused(why)` quand un nom est refusé ou un fichier non écrit, `told(what)` quand il l'est, affichés par `MainWindow` dans la barre d'état. Verrouillée pendant l'exécution (`isEditable()`, suit `runningChanged`) : `setEditable()` passe à chaque item (`ComponentItem::setEditable()`, `ItemIsMovable` d'un `SettingsItem`, `ZoneItem::setEditable()`), pas de dépôt ni de tracé de lien, double-clic sur un fichier de réglages ignoré, menu contextuel et Suppr remplacés par `refuseWhileRunning()` ; la sélection reste |
| `ComponentItem` | `QGraphicsObject` sélectionnable, déplaçable tant qu'il est éditable (`setEditable()`) ; reconstruit ses `InterfaceItem` sur `interfacesChanged` ; marque d'état dans le coin, redessinée sur `stateChanged` (rien = `Editing`, triangle = `Running`) ; dans le coin opposé, « ! » dans un disque `warning` tant que `unboundMandatoryInterfaces()` n'est pas vide (`m_unbound`, relu par `refreshInterfaces()`, avec l'infobulle et `InterfaceItem::setMissing()`) ; couleur optionnelle (`color()` invalide = couleurs du style), opacité comprise, peinte derrière ses noms seulement ; `editName()` édite le nom sur place (double-clic sur les noms, s'il est éditable) et émet `nameEdited`, que la scène applique par `Universe::rename()` |
| `NameEditor` | `QGraphicsTextItem` enfant d'un item, pour éditer son nom sur place (titre d'une zone, nom d'un composant) : Entrée ou un clic ailleurs valide, Échap abandonne. `Colors.h` : `over()`, une couleur translucide vue sur un fond, `luminance()` et `contrast()` au sens des WCAG, et `textColorOver()`, noir ou blanc, celui qui contraste le plus avec le fond (4,58 au moins) |
| `InterfaceItem` | Point sur le bord (consommée à gauche, fournie à droite), astérisque après le nom d'une consommée `Mandatory`, point et nom en `warning` quand elle n'est liée à rien (`setMissing()`), `typeName(QMetaType)` statique (sans l'`*`), `Type = UserType + 1`, `canLinkTo()`, infobulle, estompage ; connaît ses `LinkItem` |
| `LinkItem` | Courbe entre deux points, Z = -1, se supprime avec le premier `InterfaceItem` détruit ; `curve()` statique sert aussi au lien en cours de tracé |
| `ZoneItem` | Zone colorée titrée, `Type = UserType + 2`, Z = -aire ; déplacement par le titre avec son contenu (composants et fichiers de réglages par leur centre, zones entières), redimensionnement par les bords, édition du titre sur place ; sa `shape()` ne couvre que titre et bords ; `setEditable(false)` lui retire boutons et survol |
| `SettingsItem` / `SettingsLinkItem` | `QGraphicsObject` d'un `SettingsFile` : feuille au coin plié, nom du fichier, infobulle (chemin, propriétés tenues) ; refait ses `SettingsLinkItem` (traits pointillés de centre à centre, Z = -1, dans la scène et non enfants, détruits avec lui) sur `SettingsFile::changed` et à son ajout à la scène, et les suit par `xChanged` / `yChanged` des deux items |
| `SettingsDialog` | Nom, dossier (chemin résultant affiché, ou pourquoi le nom est refusé : vide, ou celui d'un autre fichier, OK alors grisé) et arbre composant → groupe d'objet → propriétés que `SettingsFile::canHold()` accepte, à cocher ; celles d'un autre fichier sont désactivées avec son nom ; les groupes d'objets ne sont ouverts que s'ils ont une propriété tenue ; appliqué au `accepted` |
| `PropertyEditor` | `QTableWidget` des propriétés inscriptibles de type bool, int, uint, double, QString, QDate, QTime, QDateTime : celles du composant, puis celles de ses `propertyObjects()` (sans `objectName`), sans nom à la suite, avec nom sous une ligne de titre par nom ; nom en italique pour une propriété tenue par un fichier de réglages ; suit les signaux NOTIFY, `propertyObjectsChanged`, `settingsFilesChanged` et `runningChanged` : en lecture seule pendant l'exécution (`NoEditTriggers`) |

Les chaînes affichées passent par `tr()`, en anglais.

**Logo et icônes** (`NebulaStudio/resources/`) : `nebula.svg`, le logo de Nebula (une nébuleuse traversée de trois
astres liés), et `icons/{universe,galaxy,astre,comet}.svg`, une icône par terme du glossaire, sont des SVG aux couleurs
du thème, compilés par `nebulastudio.qrc` (AUTORCC, nommé dans `SOURCES`). Les icônes n'emploient que les couleurs de
zones qui gardent 3 contre 1 sur les champs des deux schémas (`#6366F1`, `#C026D3`, `#3B82F6`, `#EC4899`). Qt Svg est
lié à NebulaStudio (`find_package` dans son `CMakeLists.txt`) : `QSvgRenderer` dessine le logo sur le splash, et son
moteur d'icônes, déployé par windeployqt (`iconengines/qsvgicon`), lit les `QIcon` des SVG : `logoPath()` pour
`QApplication::setWindowIcon()` au début de `main.cpp`, `icon(GlossaryTerm)` de `Theme.h` pour **New** (Univers),
**Add zone** (Galaxy), la liste des composants (Astre, `DecorationRole` de la colonne Name) et les fichiers de réglages
(Comet : **Add settings file**, **Edit settings file...**, `SettingsDialog`). `nebula.ico` (16 à 256 px, entrées PNG),
par `nebulastudio.rc` (`IDI_ICON1`, ajouté sous Windows seulement, à côté de la ressource de version de CMakeUtils), est
l'icône de `NebulaStudio.exe` : rendu du logo, à refaire quand il change, par un petit programme Qt qui le dessine avec
`QSvgRenderer` à chaque taille et assemble les PNG (pas d'outil dans le dépôt).

**Police** (`NebulaStudio/resources/fonts/`) : `SpaceGrotesk-Bold.ttf`, compilée dans l'exécutable par
`nebulastudio.qrc`, et sa licence `OFL.txt` (SIL Open Font License 1.1), que `release.yml` copie dans le paquet à côté
de NebulaStudio (`SpaceGrotesk-OFL.txt`). Seule la graisse Bold sert : en ajouter une autre, c'est l'ajouter au `.qrc`.

**Fichier de projet `.nbp`** (JSON, version 1, format décrit dans le README)
- Le contenu applicatif est lu et écrit par `Universe` (voir Core). `EditionScene::save()` y ajoute `x` / `y`
  et `color` (`#aarrggbb`, seulement si une couleur a été choisie) dans chaque composant (retrouvé par son nom),
  `x` / `y` dans chaque fichier de `settings` (par rang, dans l'ordre de `settingsFiles()`) et `zones` ;
  `EditionScene::load()` retire ses items, appelle `Universe::load()`, puis recrée un `ComponentItem` par
  composant, un `LinkItem` par `binding()` et un `SettingsItem` par fichier de réglages.
- `MainWindow::openProjectFile()` : `Universe::read()`, puis `m_scene->load()`, puis rafraîchit les modèles.
  Les problèmes sont affichés dans une seule boîte ; après une ouverture partielle, le projet n'a plus de fichier
  associé pour que **Save** n'écrase pas ce qui manque.
- « Modifié » n'est pas suivi au fil de l'eau : `saveChanges()` compare `m_scene->save()` à `m_savedProject`, mémorisé
  par `setProjectFile()`. Toute nouvelle donnée à sauvegarder doit donc passer par `EditionScene::save()`.
- `NebulaStudio [--plugin-dir <dossier>]... [--plugin-root <dossier>] fichier.nbp` ouvre le projet au démarrage (options
  déclarées par `Universe::addCommandLineOptions()` dans `main.cpp`, appliquées par `MainWindow`).
- Les dialogues partent de `scene/` à côté de `NebulaStudio.exe` (`defaultProjectDirectory()`, créé au premier
  **Save As...**), ou du dossier du fichier du projet quand il en a un.
- Menu **Run** : **Start** (F5) valide d'abord un nom en cours d'édition (`setFocusItem(nullptr)`), appelle
  `Universe::start()` et affiche `errorString()` en cas de refus. **Stop** (Maj+F5) appelle `stop()`. Leur
  activation suit `runningChanged`, comme celle de **New** et **Open...**, désactivés pendant l'exécution. Comme
  `stop()` remet les valeurs d'édition et que `save()` les écrit pendant l'exécution, faire tourner l'application ne
  rend pas le projet « modifié ».

## Pas encore fait

- Pas de suppression d'un lien seul dans NebulaStudio (`unbindInterface` n'y est pas utilisé) : un lien ne part qu'avec
  l'un de ses composants, ou remplacé par un autre.
- Pas d'annulation : un composant supprimé l'est sans confirmation, avec ses propriétés et ses liens.
- Aucun composant d'exemple ne refuse de démarrer : le chemin d'erreur de `start()` n'a pas été exercé.
- Le verrouillage pendant l'exécution n'est complet que dans NebulaStudio : Core refuse `instantiate()`, `rename()` et
  `remove()`, mais pas `Component::bindInterface()` ni les changements de fichiers de réglages.
- Pas de tests automatisés. Le paquet non plus n'est vérifié par rien : ni projet tiers d'exemple dans le dépôt, ni
  étape de CI qui en compile un.
- Composants non opérationnels : **Start** ne les signale pas et ne refuse rien ; seuls le « ! », l'ambre et
  `ProblemList` les montrent.
- Fichiers de réglages : pas de fenêtre « Préférences » générique où l'utilisateur final éditerait tout un fichier
  (chaque composant montre les siens), pas de partage d'une même valeur entre deux instances, et le cas de plusieurs
  objets de même nom dans un composant (`index` d'une entrée) n'a été exercé par aucun exemple.

## Pièges

- Modifier `Component.h`, `IComponent.h` ou `Plugin.h` change l'agencement binaire : tous les plugins doivent être
  reconstruits, y compris ceux ouverts par **Open plugin...**, sinon plantage à la création d'un composant. Le contrôle
  de version ne compare que la majeure de Core.
- `MainWindow::~MainWindow` : les items et l'éditeur de propriétés référencent les composants de `m_application` ;
  garder l'ordre de destruction actuel quand un membre est ajouté.
- Un `QGraphicsTextItem` qui a le focus fait défiler la vue pour montrer son curseur, et la vue rejoue alors le dernier
  événement souris comme un déplacement : un item encore tenu par le double-clic qui a lancé l'édition suivrait le
  défilement. `ComponentItem::mouseMoveEvent` ignore donc les déplacements tant que son nom est édité.
- `NebulaPlugin.cmake` s'exécute aussi chez un projet tiers, inclus par `find_package` depuis le paquet :
  `PROJECT_NAME` et `PROJECT_VERSION` y sont ceux de ce projet, `CMAKE_CURRENT_LIST_DIR` le dossier installé. La
  version de Core (`coreVersion`) y vient donc de `Nebula_VERSION`, et rien ne doit y lire l'arbre de sources. Un
  changement de ce fichier se vérifie avec un projet jetable qui fait `find_package` sur `<build>/package`.
- La palette de l'application change avec le schéma de couleurs du système, NebulaStudio ouvert : ce qui peint lit la
  palette au moment de peindre, et ce qui en garde une couleur (le fond des titres de groupes du `PropertyEditor`) la
  remet sur `QEvent::PaletteChange`. Une couleur en dur, hors des jetons de `Theme.cpp`, ne suit pas le schéma et n'est
  pas vérifiée contre les WCAG.
- Un composant ne peut pas être `final`, a besoin d'un constructeur par défaut et de `Q_OBJECT`.
- `Ticker` et `Window` ont besoin d'un `QApplication` (widgets) : pas utilisables dans `ExampleApp`.
- Tout ce qu'un objet ajouté par `addPropertiesForQObject()` a d'inscriptible est enregistré : pour l'afficheur du
  `Ticker`, `value` compris, qui change à chaque tic, et que `stop()` remet donc à sa valeur d'édition (`ticks`, en
  lecture seule, n'est pas remis). C'est aussi ce que propose la fenêtre d'un fichier de réglages : toutes les
  propriétés d'un `QWidget`.
- Un fichier de réglages range les valeurs sous le nom des instances : renommer une instance laisse ce que
  l'utilisateur lui avait donné sous l'ancien nom, et elle repart des valeurs du projet. Lancée depuis NebulaStudio, une
  application dont le fichier n'a pas de dossier écrit dans `AppConfigLocation` de NebulaStudio, pas dans celui de
  l'application livrée.
- Deux `Universe` (ou deux `MainWindow`) dans un même processus ont chacune leur `PluginManager` mais
  partagent `ComponentFactory::instance()` : la seconde instancie des composants dont elle ne connaît pas le plugin,
  qui manque alors dans le projet enregistré. Ces instances survivent aussi au plugin quand la première le décharge
  ou est détruite : un `PluginManager` ne garde en vie que ce qu'il a chargé lui-même, et ne décharge que cela.
- Un plugin déchargé garde sa DLL en mémoire : `QPluginLoader` charge avec `PreventUnloadHint` par défaut, et il faut
  le laisser. Sans lui, la DLL part vraiment, mais le registre des métatypes de Qt garde des pointeurs vers les types
  du plugin (`QMetaType::fromName("IGreeter*")` plante après coup) et un `QVariant` lu d'une de ses propriétés pointe
  aussi dans la DLL. Conséquences : le fichier `.nbdl` reste verrouillé jusqu'à la fermeture, et un plugin recompilé
  n'est relu qu'au lancement suivant.

## Conventions

Elles sont décrites dans [CONVENTIONS.md](CONVENTIONS.md), à lire avant d'écrire du code et à tenir à jour quand une
règle change. L'essentiel :

- Base : le style de Qt (`camelCase`) et les C++ Core Guidelines.
- `.clang-format` : base LLVM, 4 espaces, 120 colonnes, accolades Allman, `if(` sans espace, `T* p`. `.clang-tidy`
  actif (bugprone, performance, readability, modernize…).
- Membres `m_camelCase`, `[[nodiscard]]` sur les accesseurs, `#pragma once`, `QStringLiteral` / `u"…"`.
- Licence MPL-2.0 ([LICENSE](LICENSE)) : tout nouveau fichier source, en-tête, gabarit ou CMake commence par
  `// Copyright (C) <année> Fabien Di Grande` puis `// SPDX-License-Identifier: MPL-2.0` (`#` en CMake), et une
  ligne vide.
- Commentaires en anglais, au-dessus de la déclaration, qui disent le pourquoi ou le contrat.
- Commits en français, préfixés : `feat(NebulaStudio): …`, `fix(NebulaStudio): …`, `refactor: …`.
- Toute nouvelle fonctionnalité visible est décrite dans le README (sections Plugin, Component, NebulaStudio).

## Process

Chaque travail suit le cycle issue → branche → PR, sur `Shuko83/AppForge`, avec la CLI `gh`. Rien n'est commité ni
poussé directement sur `main`.

1. **Issue** : une par travail. Claude la rédige à partir de la demande (contexte, comportement attendu, critères
   d'acceptation) et la fait valider avant de la créer ; ou il part d'une issue existante.
2. **Branche** : depuis un `main` à jour (`git fetch`, puis fast-forward), nommée `feat/<n°>-<nom-court>`,
   `fix/<n°>-…`, `docs/<n°>-…` ou `refactor/<n°>-…`.
3. **Travail** : code, vérification, puis commits selon les conventions ci-dessus, qui citent l'issue (`#<n°>`) dans
   leur corps.
4. **PR** : push de la branche, puis PR vers `main` avec `Closes #<n°>`, un résumé, ce qui a été vérifié et ce qui ne
   l'a pas été. La CI compile chaque PR.
5. **Revue** : les corrections demandées vont sur la même branche.
6. **Fusion** : par l'utilisateur, en rebase. Ensuite, suppression de la branche et `main` local remis à jour.

Créer l'issue une fois validée, pousser la branche et ouvrir la PR font partie du cycle : Claude n'a pas à redemander.
Il ne fusionne jamais sans qu'on le lui demande.

Quand une demande sort du cycle, Claude le dit avant de continuer et propose d'y revenir : un changement demandé sans
issue, un commit ou un push sur `main`, une fusion sans PR, ou une demande sans rapport avec la branche en cours.

**Livraison d'une version** : le même cycle, puis un tag.

1. Une PR passe `project(Nebula VERSION x.y.z)` et les `find_package(Nebula x.y …)` du README à la nouvelle
   version, et ajoute `docs/release-notes/<x.y.z>.md`, en anglais puis en français.
2. Une fois la PR fusionnée, le tag `<x.y.z>`, les trois nombres de la version sans préfixe `v`, est posé sur ce
   commit de `main` et poussé, à la demande de l'utilisateur. `release.yml` compile alors en Release, refuse un tag
   qui n'est pas la version du projet, installe le paquet, y ajoute les licences et le README, et publie la release
   GitHub avec le zip et ces notes.
3. Un tag ne se déplace pas. Si c'est le workflow du tag qui est en cause, il se corrige par une PR puis se relance à
   la main depuis `main` (`workflow_dispatch`, entrée `tag`), qui compile le commit du tag avec le fichier corrigé.
