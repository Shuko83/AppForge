#pragma once

#include "AfCore/Version.h"
#include "AfCore/IJsonSerializable.h"

#include <QString>
#include <QStringList>

namespace appforge::core {

    /**
     * @brief Représente la description (métadonnées) d'un composant fourni par un plugin.
     *    };
     * La classe encapsule les informations nécessaires pour identifier un composant
     * (pluginId + componentId), son affichage, sa catégorie
     * Elle fournit également des utilitaires de sérialisation JSON et des
     * validateurs d'identifiants.
     */
    class ComponentDescriptor : public IJsonSerializable
    {
    public:
        ComponentDescriptor() = default;

        /**
         * @brief Constructeur principal.
         * @param pluginId Identifiant du plugin qui fournit le composant.
         * @param componentId Identifiant unique du composant au sein du plugin.
         * @param displayName Nom lisible / affichable du composant.
         * @param version Version du composant (optionnelle, valeur par défaut si non fournie).
         */
        ComponentDescriptor(QString pluginId,
            QString componentId,
            QString displayName);

        /**
         * @brief Identifiant du plugin (fournisseur).
         * @return Chaîne représentant l'identifiant du plugin.
         */
        [[nodiscard]] QString pluginId() const;
        void setPluginId(const QString& pluginId);

        /**
         * @brief Identifiant du composant (local au plugin).
         * @return Chaîne représentant l'identifiant du composant.
         */
        [[nodiscard]] QString componentId() const;
        void setComponentId(const QString& componentId);

        /**
         * @brief Identifiant complet du composant.
         * @return Combinaison de `pluginId` et `componentId` (utilisé pour l'identification globale).
         */
        [[nodiscard]] QString fullId() const;

        /**
         * @brief Nom affichable du composant.
         */
        [[nodiscard]] QString displayName() const;
        void setDisplayName(const QString& displayName);

        /**
         * @brief Description textuelle du composant (usage, fonctionnalité, etc.).
         */
        [[nodiscard]] QString description() const;
        void setDescription(const QString& description);

        /**
         * @brief Version du composant.
         */
        [[nodiscard]] Version version() const;
        void setVersion(const Version& version);

        /**
         * @brief Catégorie fonctionnelle du composant (ex. "UI", "Data", "IO").
         */
        [[nodiscard]] QString category() const;
        void setCategory(const QString& category);

        /**
         * @brief Liste de tags libres associés au composant.
         */
        [[nodiscard]] QStringList tags() const;
        void setTags(const QStringList& tags);

        /**
         * @brief Vérifie la validité du descriptor.
         * @return true si le descriptor est considéré valide, false sinon.
         * @see errors() pour obtenir les messages détaillés en cas d'échec.
         */
        [[nodiscard]] bool isValid() const;
        /**
         * @brief Messages d'erreur collectés lors de la validation.
         * @return Map champ -> message d'erreur.
         */
        [[nodiscard]] QMap<QString, QString> errors() const;

        /**
         * @brief Compare l'identité (non pas l'entièreté des métadonnées) avec un autre descriptor.
          * @param other Autre descriptor à comparer.
          * @return true si `pluginId` et `componentId` correspondent.
          */
        [[nodiscard]] bool sameIdentity(const ComponentDescriptor& other) const;

        // Hérité via IJsonSerializable
        /**
         * @brief Sérialise le descriptor en JSON (format interne).
         * @return Objet JSON représentant ce descriptor.
         */
        [[nodiscard]] json toJson() const override;
        /**
         * @brief Désérialise à partir d'un objet JSON.
         * @param obj Objet JSON source.
         * @note Valide partiellement les champs et met à jour l'état interne.
         */
        void fromJson(const json& obj) override;

        /**
         * @brief Valide la syntaxe d'un pluginId.
         * @param pluginId Identifiant à valider.
         * @return true si le format est accepté par la politique d'ID du produit.
         */
        static bool isValidPluginId(const QString& pluginId);
        /**
         * @brief Valide la syntaxe d'un componentId.
         * @param componentId Identifiant à valider.
         * @return true si le format est accepté par la politique d'ID du produit.
         */
        static bool isValidComponentId(const QString& componentId);

        /**
         * @brief Opérateurs d'égalité/comparaison.
         * @return true si tous les champs pertinents sont égaux (ou différents).
         */
        bool operator==(const ComponentDescriptor& other) const;
        bool operator!=(const ComponentDescriptor& other) const;

    private:
        QString _pluginId;
        QString _componentId;

        QString _displayName;
        QString _description;
        Version _version;
        QString _category;
        QStringList _tags;
    };
}