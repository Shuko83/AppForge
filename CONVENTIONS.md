# Conventions de codage

Ce que suit le code d'AppForge aujourd'hui. `.clang-format` et `.clang-tidy`, à la racine, en appliquent une partie ;
le reste tient à la relecture. Une règle qui ne convient plus se change ici d'abord, puis dans le code.

La base est celle de Qt, son [Coding Style](https://wiki.qt.io/Qt_Coding_Style) et ses
[Coding Conventions](https://wiki.qt.io/Coding_Conventions) : du `camelCase`, pas de `snake_case`. Pour l'usage du
langage, ce sont les [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines). Ce document
dit ce qu'AppForge en retient et ce qu'il précise ; ce qu'il ne dit pas se décide d'après eux.

## Mise en forme

Elle est celle de `.clang-format` : formater un fichier avant de le commiter suffit.

- Base LLVM, 4 espaces, pas de tabulation, 120 colonnes.
- Accolades sur leur propre ligne (Allman), y compris pour un bloc d'une seule instruction : pas de `if` sans
  accolades.
- Pas d'espace avant une parenthèse ouvrante : `if(`, `for(`, `switch(`.
- `*` et `&` collés au type : `Component* component`, `const QString& name`.
- Une fonction vide peut tenir sur une ligne ; les autres non.

## Fichiers

- Un en-tête et une source par classe, nommés comme elle : `ComponentItem.h`, `ComponentItem.cpp`. Une interface
  porte le préfixe `I` : `IComponent.h`.
- Les en-têtes publics de Core sont dans `Core/include/AppForge/<Module>/` et s'incluent par ce chemin :
  `#include "AppForge/Component/Component.h"`. Ses sources sont dans `Core/src/<Module>/`.
- Forge et les exemples gardent en-têtes et sources côte à côte dans `src/`.
- `#pragma once` en tête de chaque en-tête, sans garde `#ifndef`.
- Les fichiers ne sont pas listés dans les `CMakeLists.txt` : `cmu_add_*` les trouve dans `src/` et `include/`.

## Inclusions

Triées par `.clang-format`, en deux groupes séparés par une ligne vide :

1. les en-têtes de Qt et de la bibliothèque standard, entre chevrons ;
2. ceux du projet, entre guillemets.

Une source inclut d'abord son propre en-tête, seul sur la première ligne.

Un en-tête déclare par avance (`class QLabel;`) ce dont il n'utilise qu'un pointeur ou une référence, et n'inclut
que ce dont il a besoin entier.

## Nommage

| Élément | Forme | Exemple |
| --- | --- | --- |
| Classe, structure, énumération | `PascalCase` | `ComponentFactory`, `InterfaceInfo` |
| Interface | `I` + `PascalCase` | `IComponent`, `IGreeter` |
| Fonction, méthode | `camelCase` | `bindInterface()`, `refreshInterfaces()` |
| Variable, paramètre | `camelCase` | `componentId`, `provider` |
| Membre | `m_` + `camelCase` | `m_component`, `m_linkSource` |
| Constante, valeur d'énumération | `PascalCase` | `GridStep`, `ComponentState::Ready` |
| Catégorie de journalisation | `lc` + `PascalCase` | `lcComponents` |
| Espace de noms | `PascalCase` | `AppForge`, `AppForge::Generated` |
| Macro | `APPFORGE_` + `MAJUSCULES` | `APPFORGE_REGISTER_COMPONENT` |
| Fonction CMake | `snake_case` préfixée | `appforge_add_plugin()` |

- Un accesseur porte le nom de ce qu'il renvoie, sans `get` : `color()`, `setColor()`. Un booléen se lit comme une
  question : `isRunning()`, `canLinkTo()`.
- Les noms sont en anglais et écrits en entier : `component`, pas `comp`.
- Core est dans l'espace de noms `AppForge`. Forge et les exemples n'en ont pas.
- Ce qui ne sert qu'à une source est dans un espace de noms anonyme, en tête de cette source, fermé par
  `} // namespace`.

## Langage

- C++ récent : concepts, `std::ranges`, initialiseurs désignés, `std::optional` pour ce qui peut manquer.
- `[[nodiscard]]` sur les accesseurs et sur toute fonction dont ignorer le résultat est une erreur.
- `const` partout où c'est possible : variables locales, méthodes, paramètres passés par référence.
- `constexpr` pour les constantes, plutôt que `#define` ou `const` ; pas de nombre sans nom dans le code quand il a
  un sens : `constexpr qreal Padding = 8;`.
- Une macro seulement là où le langage n'offre rien d'autre, comme l'enregistrement d'un composant dans son plugin.
- `enum class`, avec son type quand il est petit : `enum class Kind : std::uint8_t`.
- `auto` quand le type est écrit à droite ou sans intérêt : `auto* layout = new QVBoxLayout(this);`,
  `const auto found = std::ranges::find(…);`. Le `*` et le `const` restent écrits.
- `nullptr`, jamais `0` ni `NULL`. Un pointeur se compare explicitement, `if(item != nullptr)` ; un
  `std::optional` ou un booléen se teste directement, `if(!info)`.
- `override` sur toute méthode redéfinie, sans `virtual` devant ; `explicit` sur un constructeur à un argument.
- Une classe qui ne se copie pas le dit : constructeur de copie et affectation `= delete`.
- Un paramètre inutilisé garde son nom en commentaire : `QWidget* /*widget*/`.
- Une référence pour ce qui ne peut pas être nul, un pointeur pour ce qui peut l'être ou qui change.
- La propriété s'exprime par `std::unique_ptr` ; les objets Qt appartiennent à leur parent.
- Les algorithmes de `std::ranges` plutôt qu'une boucle écrite à la main quand ils disent la même chose.

## Qt

- Connexions par pointeurs de fonction ou lambda, avec un objet de contexte : jamais `SIGNAL()` / `SLOT()`.
- `signals:` et `private slots:`, pas `Q_SIGNALS` / `Q_SLOTS`. Une méthode n'est un slot que si elle doit l'être.
- Chaînes littérales : `QStringLiteral("…")`, ou `u"…"` quand une vue suffit (`json.value(u"name")`).
- Tout texte affiché passe par `tr()`, en anglais. Une classe qui n'est pas un `QObject` le permet avec
  `Q_DECLARE_TR_FUNCTIONS`.
- Les messages de Core (`errorString()`, journal) sont en anglais et ne sont pas traduits.
- Journal : `qCWarning(lcComponents)` avec une catégorie `appforge.<module>`, jamais `qDebug()` laissé dans le code.
- `qobject_cast` pour les `QObject`, `qgraphicsitem_cast` pour les items qui déclarent leur `Type`.
- Un item ou un widget prend ses couleurs dans la palette, pas en dur.

## Commentaires

- En anglais.
- Au-dessus de la déclaration, dans l'en-tête : ce que fait la fonction pour qui l'appelle, ses cas limites et ce
  qu'elle renvoie alors. Pas de Doxygen, pas de `@param`.
- Une classe commence par quelques lignes qui disent ce qu'elle est et comment elle se comporte.
- Dans le code, un commentaire dit pourquoi, pas ce que la ligne fait déjà. Court, il suit l'instruction :
  `update(); // Its title is painted again`.
- Un membre dont le rôle ne se lit pas dans son nom a son commentaire en fin de ligne.
- Ni code commenté ni `TODO` : ce qui reste à faire est une issue.

## CMake

- Une cible par dossier, déclarée avec `cmu_add_executable`, `cmu_add_shared_library` ou `appforge_add_plugin`.
- Un commentaire au-dessus de ce qui n'est pas évident, comme dans le code.
- Les cibles se lient par leur alias : `AppForge::Core`, `Qt6::Widgets`.

## Commits, branches et PR

- Commits en français, préfixés : `feat(Forge): …`, `fix(Forge): …`, `refactor: …`, `docs: …`, `build: …`,
  `test: …`. Le titre commence par une majuscule et dit ce qui change ; le corps dit pourquoi et cite l'issue.
- Une branche par issue, `feat/<n°>-<nom-court>` et de même pour `fix`, `docs`, `refactor` ; une PR vers `main`,
  fusionnée en rebase. Rien ne va directement sur `main`.
- Toute fonctionnalité visible est décrite dans le README dans la même PR.
