#if not defined(VERSION_H)
#define VERSION_H

#include "CoreConstants.h"
#include <nlohmann/json.hpp>

#include <QString>

#include <compare>

namespace appforge::core {

    /**
     * @brief Représente une version sémantique simple (major.minor.patch).
     *
     * La classe fournit des accesseurs mutateurs pour les trois composants
     * de version, des utilitaires de validation et de conversion en chaîne,
     * ainsi que la sérialisation JSON via l'interface `IJsonSerializable`.
     */
    class CORE_EXPORT Version
    {
    public:

        /**
         * @brief Constructeur initialisant les composantes de la version.
         * @param major Numéro de version majeure.
         * @param minor Numéro de version mineure.
         * @param patch Numéro de correctif (patch).
         */
        explicit Version(unsigned int major = 0, unsigned int minor = 0 , unsigned int patch = 0);
		~Version() = default;
        /**
         * @brief Retourne la composante majeure.
         * @return valeur entière >= 0.
         */
        [[nodiscard]] unsigned int major() const;
        /**
         * @brief Définit la composante majeure.
         * @param value Valeur entière >= 0.
         */
        void setMajor(unsigned int value);

        /**
         * @brief Retourne la composante mineure.
         * @return valeur entière >= 0.
         */
        [[nodiscard]] unsigned int minor() const;
        /**
         * @brief Définit la composante mineure.
         * @param value Valeur entière >= 0.
         */
        void setMinor(unsigned int value);

        /**
         * @brief Retourne la composante de correctif (patch).
         * @return valeur entière >= 0.
         */
        [[nodiscard]] unsigned int patch() const;
        /**
         * @brief Définit la composante de correctif (patch).
         * @param value Valeur entière >= 0.
         */
        void setPatch(unsigned int value);

        /**
         * @brief Convertit la version en chaîne au format "major.minor.patch".
         * @return QString contenant la représentation textuelle.
         */
        [[nodiscard]] QString toString() const;

        /**
         * @brief Indique si la version est valide.
         *
         * La validité est définie par des composantes non négatives.
         * (Comportement exact dépend de l'implémentation.)
         * @return true si valide, false sinon.
         */
        [[nodiscard]] bool isValid() const;

        [[nodiscard]] auto operator<=>(const Version& other) const = default;

    private:
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Version,_major,_minor,_patch)

        unsigned int _major = 0; /**< Composante majeure. */
        unsigned int _minor = 0; /**< Composante mineure. */
        unsigned int _patch = 0; /**< Composante de correctif (patch). */
    };
}
#endif