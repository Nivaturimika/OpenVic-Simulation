#pragma once

#include "openvic-simulation/types/IdentifierRegistry.hpp"
#include <openvic-simulation/dataloader/NodeTools.hpp>
//#include "openvic-simulation/interface/GFXSprite.hpp"

using namespace OpenVic;
using namespace OpenVic::NodeTools;

struct AlignedElement {
public:
    enum class format_t {
        left, centre, right, justified
    };

private:
    format_t PROPERTY(format);

protected:
    AlignedElement();

    //bool _fill_key_map(NodeTools::case_insensitive_key_map_t& key_map, UIManager const& ui_manager) override;

    bool _fill_key_map(case_insensitive_key_map_t& key_map) {
        using enum format_t;
        static const string_map_t<format_t> format_map = {
            { "left", left }, { "right", right }, { "centre", centre }, { "center", centre }, { "justified", justified }
        };
        return add_key_map_entries(key_map,
            "format", ZERO_OR_ONE, expect_identifier(expect_mapped_string(format_map, assign_variable_callback(format))
        ));
    }


public:
    AlignedElement(AlignedElement&&) = default;
    virtual ~AlignedElement() = default;

    //OV_DETAIL_GET_TYPE
};
