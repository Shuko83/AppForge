#pragma once


namespace appforge::core {
	class ComponentDescriptor;
	/**
	 * @brief Interface représentant un composant fourni par un plugin.
	 *
	 * La classe IComponent définit l'interface de base pour tous les composants
	 * fournis par des plugins. Chaque composant doit fournir un identifiant unique
	 * et une description (métadonnées) via le ComponentDescriptor associé.
	 */
	class IComponent
	{
	public:
		virtual ~IComponent() = default;

		/**
		 * @brief Obtient la description (métadonnées) du composant.
		 * @return Référence constante vers le ComponentDescriptor associé.
		 */
		virtual const ComponentDescriptor& getDescriptor() const = 0;
	};
}