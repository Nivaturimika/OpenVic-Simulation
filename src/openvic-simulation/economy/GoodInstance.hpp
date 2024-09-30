#pragma once

#include "openvic-simulation/economy/GoodDefinition.hpp"
#include "openvic-simulation/types/HasIdentifier.hpp"
#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include "openvic-simulation/utility/Getters.hpp"

namespace OpenVic {
	struct GoodInstanceManager;

	struct GoodInstance : HasIdentifierAndColour {
		friend struct GoodInstanceManager;

	private:
		GoodDefinition const& PROPERTY(good_definition);
		fixed_point_t PROPERTY(price);
		bool PROPERTY(is_available);
		// TODO - supply, demand, actual bought

		GoodInstance(GoodDefinition const& new_good_definition);

	public:
		GoodInstance(GoodInstance&&) = default;
	};

	struct GoodInstanceManager {
	private:
		IdentifierRegistry<GoodInstance> IDENTIFIER_REGISTRY(good_instance);

	public:
		bool setup(GoodDefinitionManager const& good_definition_manager);
	};
}
