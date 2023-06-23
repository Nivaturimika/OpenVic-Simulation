#include "Types.hpp"

#include <cassert>
#include <iomanip>
#include <sstream>

using namespace OpenVic;

HasIdentifier::HasIdentifier(ovstring const& new_identifier) : identifier { new_identifier } {
	assert(!identifier.empty());
}

ovstring const& HasIdentifier::get_identifier() const {
	return identifier;
}

HasColour::HasColour(colour_t const new_colour, bool can_be_null) : colour(new_colour) {
	// assert(colour <= MAX_COLOUR_RGB);
	assert((can_be_null || colour != NULL_COLOUR));
}

colour_t HasColour::get_colour() const { return colour; }

ovstring HasColour::colour_to_hex_string(colour_t const colour) {
	std::wostringstream stream;
	stream << std::hex << std::setfill(L'0') << std::setw(6) << colour;
	return stream.str();
}

ovstring HasColour::colour_to_hex_string() const {
	return colour_to_hex_string(colour);
}
