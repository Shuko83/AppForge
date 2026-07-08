#pragma once

#include "Core/IJsonSerializable.h"

#include <QString>
#include <QStringList>

namespace appforge::core {

    /**
     * @brief Représente la description (métadonnées) d'un composant fourni par un plugin.
     *    };
     * La classe encapsule les informations nécessaires pour identifier un composant
     * (pluginId + componentId), son affichage, sa catégorie et sa description.
     * Elle fournit également des utilitaires de sérialisation JSON.
     */
    class IComponentDescriptor : public IJsonSerializable
    {
    public:
        ComponentDescriptor() = default;

        /**
         * @brief Constructeur principal.
         * @param componentId Identifiant unique du composant au sein du plugin.
         * @param displayName Nom lisible / affichable du composant.
         * @param description Description du composant.
         */
        ComponentDescriptor(QString componentId,
            QString displayName,
            QString description = QString(),
            QString category = QString());

        /**
         * @brief Identifiant du plugin (fournisseur).
         * @return Chaîne représentant l'identifiant du plugin.
         */
        QString pluginId() const;
        void setPluginId(const QString& pluginId);

        /**
         * @brief Identifiant du composant (local au plugin).
         * @return Chaîne représentant l'identifiant du composant.
         */
        QString componentId() const;
        void setComponentId(const QString& componentId);

        /**
         * @brief Identifiant complet du composant.
         * @return Combinaison de `pluginId` et `componentId` (utilisé pour l'identification globale).
         */
        QString fullId() const;

        /**
         * @brief Nom affichable du composant.
         */
        QString displayName() const;
        void setDisplayName(const QString& displayName);

        /**
         * @brief Description textuelle du composant (usage, fonctionnalité, etc.).
         */
        QString description() const;
        void setDescription(const QString& description);

        /**
         * @brief Catégorie fonctionnelle du composant (ex. "UI", "Data", "IO").
         */
        QString category() const;
        void setCategory(const QString& category);

        /**
         * @brief Compare l'identité (non pas l'entièreté des métadonnées) avec un autre descriptor.
          * @param other Autre descriptor à comparer.
          * @return true si `pluginId` et `componentId` correspondent.
          */
        bool sameIdentity(const ComponentDescriptor& other) const;

        // Hérité via IJsonSerializable
        /**
         * @brief Sérialise le descriptor en JSON (format interne).
         * @return Objet JSON représentant ce descriptor.
         */
        json toJson() const;
        /**
         * @brief Désérialise à partir d'un objet JSON.
         * @param obj Objet JSON source.
         * @note Valide partiellement les champs et met à jour l'état interne.
         */
        void fromJson(const json& obj) override;

        /**
         * @brief Opérateurs d'égalité/comparaison.
         * @return true si tous les champs pertinents sont égaux (ou différents).
         */
        bool operator==(const ComponentDescriptor& other) const;
        bool operator!=(const ComponentDescriptor& other) const;

    };
}