# Nebula glossary

*Version française [plus bas](#français).*

The common vocabulary of the framework, the reference for naming the code, the documentation and the interface.
The code and the documentation do not use it yet: the last column gives the name each notion has in them today.

| Term    | Definition | Today |
| ------- | ---------- | ----- |
| Nebula  | The framework | AppForge: Core, Forge and the plugins written for them |
| Univers | The global scene, the root container of the application | The application assembled from instances, `ForgeApplication`, shown in the edition zone of Forge (`EditionScene`) |
| Galaxy  | A group of components | A colored zone of the edition zone (`ZoneItem`), known to Forge only: an application runs without them |
| Astre   | A component, with no hierarchy: no Astre is the parent or the child of another | An instance of a component (`Component`), named in its application (`Greeter1`) |
| Comet   | A configuration component, loaded at startup | A settings file (`SettingsFile`), read by `ForgeApplication::start()` before the first component starts |

The Astres of an Univers are side by side: they are tied to one another only by the bindings of their interfaces, a
Galaxy groups them without owning them, and a Comet holds properties of some of them.

The glossary does not rename the other notions: plugin, interface, binding, property, project, and Forge, the editor.

---

## Français

Le vocabulaire commun du framework, la référence pour nommer le code, la documentation et l'interface. Le code et la
documentation ne l'emploient pas encore : la dernière colonne donne le nom que chaque notion y porte aujourd'hui.

| Terme   | Définition | Aujourd'hui |
| ------- | ---------- | ----------- |
| Nebula  | Le framework | AppForge : Core, Forge et les plugins écrits pour eux |
| Univers | La scène globale, conteneur racine de l'application | L'application assemblée à partir d'instances, `ForgeApplication`, montrée dans la zone d'édition de Forge (`EditionScene`) |
| Galaxy  | Un regroupement de composants | Une zone colorée de la zone d'édition (`ZoneItem`), connue de Forge seulement : une application tourne sans elles |
| Astre   | Un composant, sans hiérarchie : aucun Astre n'est le parent ni l'enfant d'un autre | Une instance d'un composant (`Component`), nommée dans son application (`Greeter1`) |
| Comet   | Composant de configuration chargé au démarrage | Un fichier de réglages (`SettingsFile`), lu par `ForgeApplication::start()` avant que le premier composant démarre |

Les Astres d'un Univers sont côte à côte : ils ne sont liés entre eux que par les liens de leurs interfaces, une Galaxy
les regroupe sans les posséder, et une Comet tient des propriétés de certains d'entre eux.

Le glossaire ne renomme pas les autres notions : plugin, interface, lien, propriété, projet, et Forge, l'éditeur.
