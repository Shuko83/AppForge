#pragma once

#include "CoreConstants.h"
#include "IJsonSerializable.h"

#include <QString>

namespace appforge::core {

    /**
     * @brief Représente une version sémantique simple (major.minor.patch).
     *
     * La classe fournit des accesseurs mutateurs pour les trois composants
     * de version, des utilitaires de validation et de conversion en chaîne,
     * ainsi que la sérialisation JSON via l'interface `IJsonSerializable`.
     */
    class AFCORE_EXPORT Version : public IJsonSerializable
    {
    public:

        /**
         * @brief Constructeur initialisant les composantes de la version.
         * @param major Numéro de version majeure.
         * @param minor Numéro de version mineure.
         * @param patch Numéro de correctif (patch).
         */
        explicit Version(unsigned int major = 0, unsigned int minor = 0 , unsigned int patch = 0);
		~Version() override = default;
        /**
         * @brief Retourne la composante majeure.
         * @return valeur entière >= 0.
         */
        unsigned int major() const;
        /**
         * @brief Définit la composante majeure.
         * @param value Valeur entière >= 0.
         */
        void setMajor(unsigned int value);

        /**
         * @brief Retourne la composante mineure.
         * @return valeur entière >= 0.
         */
        unsigned int minor() const;
        /**
         * @brief Définit la composante mineure.
         * @param value Valeur entière >= 0.
         */
        void setMinor(unsigned int value);

        /**
         * @brief Retourne la composante de correctif (patch).
         * @return valeur entière >= 0.
         */
        unsigned int patch() const;
        /**
         * @brief Définit la composante de correctif (patch).
         * @param value Valeur entière >= 0.
         */
        void setPatch(unsigned int value);

        /**
         * @brief Convertit la version en chaîne au format "major.minor.patch".
         * @return QString contenant la représentation textuelle.
         */
        QString toString() const;

        /**
         * @brief Indique si la version est valide.
         *
         * La validité est définie par des composantes non négatives.
         * (Comportement exact dépend de l'implémentation.)
         * @return true si valide, false sinon.
         */
        bool isValid() const;

        // ✅ JSON
        /**
         * @brief Sérialise l'objet en JSON.
         * @return Un objet `json` contenant les clés "major", "minor", "patch".
         */
        json toJson() const override;
        /**
         * @brief Désérialise l'objet depuis JSON.
         * @param j Objet `json` attendu contenant "major", "minor", "patch".
         */
        void fromJson(const json& j) override;

    private:
        unsigned int _major = 0; /**< Composante majeure. */
        unsigned int _minor = 0; /**< Composante mineure. */
        unsigned int _patch = 0; /**< Composante de correctif (patch). */
    };
}