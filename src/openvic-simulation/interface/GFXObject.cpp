#include "GFXObject.hpp"

using namespace OpenVic;
using namespace OpenVic::GFX;
using namespace OpenVic::NodeTools;

node_callback_t Object::expect_objects(length_callback_t length_callback, callback_t<std::unique_ptr<Object>&&> callback) {
	return expect_dictionary_keys_and_length(
		length_callback,

		"EMFXActorType", ZERO_OR_MORE, _expect_instance<Object, Actor>(callback),

		/* arrows.gfx */
		"arrowType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"bodytexture", ONE_EXACTLY, success_callback,
			"color", ONE_EXACTLY, success_callback,
			"colortwo", ONE_EXACTLY, success_callback,
			"endAt", ONE_EXACTLY, success_callback,
			"height", ONE_EXACTLY, success_callback,
			"type", ONE_EXACTLY, success_callback,
			"heading", ONE_EXACTLY, success_callback,
			"effect", ONE_EXACTLY, success_callback
		),

		/* battlearrow.gfx */
		"battlearrow", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"textureFile1", ONE_EXACTLY, success_callback,
			"start", ONE_EXACTLY, success_callback,
			"stop", ONE_EXACTLY, success_callback,
			"x", ONE_EXACTLY, success_callback,
			"y", ONE_EXACTLY, success_callback,
			"font", ONE_EXACTLY, success_callback,
			"scale", ONE_EXACTLY, success_callback,
			"nofade", ZERO_OR_ONE, success_callback,
			"textureloop", ZERO_OR_ONE, success_callback
		),
		"mapinfo", ZERO_OR_MORE, _expect_instance<Object, MapInfo>(callback),

		/* mapitems.gfx */
		"projectionType", ZERO_OR_MORE, _expect_instance<Object, Projection>(callback),
		"progressbar3dType", ZERO_OR_MORE, _expect_instance<Object, ProgressBar3d>(callback),
		"billboardType", ZERO_OR_MORE, _expect_instance<Object, Billboard>(callback),
		"unitstatsBillboardType", ZERO_OR_MORE, _expect_instance<Object, UnitStatsBillboard>(callback),

		/* core.gfx */
		"animatedmaptext", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"speed", ONE_EXACTLY, success_callback,
			"position", ZERO_OR_ONE, success_callback,
			"scale", ZERO_OR_ONE, success_callback,
			"textblock", ONE_EXACTLY, expect_dictionary_keys(
				"text", ONE_EXACTLY, success_callback,
				"color", ONE_EXACTLY, success_callback,
				"font", ONE_EXACTLY, success_callback,
				"position", ONE_EXACTLY, success_callback,
				"size", ONE_EXACTLY, success_callback,
				"format", ONE_EXACTLY, success_callback
			)
		),
		"flagType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback
		),
		"provinceType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback
		),
		"provinceWaterType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback
		),
		"mapTextType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback
		),
		"meshType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"xfile", ONE_EXACTLY, success_callback
		)
	);
}

Actor::Attachment::Attachment(std::string_view new_actor_name, std::string_view new_attach_node, attach_id_t new_attach_id)
  : actor_name { new_actor_name }, attach_node { new_attach_node }, attach_id { new_attach_id } {}

Actor::Animation::Animation(std::string_view new_file, fixed_point_t new_scroll_time)
  : file { new_file }, scroll_time { new_scroll_time } {}

Actor::Actor() : model_file {}, scale { 1 }, idle_animation {}, move_animation {}, attack_animation {} {}

bool Actor::_set_animation(std::string_view name, std::string_view file, fixed_point_t scroll_time) {
	std::optional<Animation>* animation = nullptr;

	if (name == "idle") {
		animation = &idle_animation;
	} else if (name == "move") {
		animation = &move_animation;
	} else if (name == "attack") {
		animation = &attack_animation;
	} else {
		Logger::error(
			"Unknown animation type \"", name, "\" for actor ", get_name(), " (with file ", file, " and scroll time ",
			scroll_time, ")"
		);
		return false;
	}

	if (animation->has_value()) {
		Logger::error(
			"Duplicate ", name, " animation for actor ", get_name(), ": ", file, " with scroll time ",
			scroll_time, " (already set to ", (*animation)->file, " with scroll time ", (*animation)->scroll_time, ")"
		);
		return false;
	}

	/* Don't set static/non-moving animation, to avoid needing an AnimationPlayer for the generated Godot Skeleton3D. */
	static constexpr std::string_view null_animation = "static.xsm";
	if (file.ends_with(null_animation)) {
		return true;
	}

	animation->emplace(Animation { file, scroll_time });
	return true;
}

bool Actor::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"actorfile", ONE_EXACTLY, expect_string(assign_variable_callback_string(model_file)),
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),

		"attach", ZERO_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			std::string_view actor_name {}, attach_node {};
			Attachment::attach_id_t attach_id = 0;

			if (!expect_dictionary_keys<StringMapCaseInsensitive>(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(actor_name)),
				"node", ONE_EXACTLY, expect_string(assign_variable_callback(attach_node)),
				"attachId", ONE_EXACTLY, expect_uint(assign_variable_callback(attach_id))
			)(node)) {
				Logger::error("Failed to load attachment for actor ", get_name());
				return false;
			}

			attachments.push_back({ actor_name, attach_node, attach_id });
			return true;
		},

		"idle", ZERO_OR_ONE, expect_string(std::bind(&Actor::_set_animation, this, "idle", std::placeholders::_1, 0)),
		"move", ZERO_OR_ONE, expect_string(std::bind(&Actor::_set_animation, this, "move", std::placeholders::_1, 0)),
		"attack", ZERO_OR_ONE, expect_string(std::bind(&Actor::_set_animation, this, "attack", std::placeholders::_1, 0)),
		"animation", ZERO_OR_MORE, [this](ast::NodeCPtr node) -> bool {
			std::string_view name {}, file {};
			fixed_point_t scroll_time = 0;

			if (!expect_dictionary_keys<StringMapCaseInsensitive>(
				"name", ONE_EXACTLY, expect_string(assign_variable_callback(name)),
				"file", ONE_EXACTLY, expect_string(assign_variable_callback(file)),
				"defaultAnimationTime", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scroll_time))
			)(node)) {
				Logger::error("Failed to load animation for actor ", get_name());
				return false;
			}

			return _set_animation(name, file, scroll_time);
		}
	);

	return ret;
}

		/* arrows.gfx */
		/*"arrowType", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"size", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"bodytexture", ONE_EXACTLY, success_callback,
			"color", ONE_EXACTLY, success_callback,
			"colortwo", ONE_EXACTLY, success_callback,
			"endAt", ONE_EXACTLY, success_callback,
			"height", ONE_EXACTLY, success_callback,
			"type", ONE_EXACTLY, success_callback,
			"heading", ONE_EXACTLY, success_callback,
			"effect", ONE_EXACTLY, success_callback
		),*/

BattleArrow::BattleArrow() : texture_file {}, scale { 1 } {}

bool BattleArrow::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	/*ret &= add_key_map_entries(key_map,
		"textureFile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file)),
		"textureFile2", ZERO_OR_ONE, expect_string(assign_variable_callback_string(back_texture_file)),
		"scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(scale))
	);*/

	return ret;
}

/*
		battlearrow.gfx
		"battlearrow", ZERO_OR_MORE, expect_dictionary_keys(
			"name", ONE_EXACTLY, success_callback,
			"textureFile", ONE_EXACTLY, success_callback,
			"textureFile1", ONE_EXACTLY, success_callback,
			"start", ONE_EXACTLY, success_callback,
			"stop", ONE_EXACTLY, success_callback,
			"x", ONE_EXACTLY, success_callback,
			"y", ONE_EXACTLY, success_callback,
			"font", ONE_EXACTLY, success_callback,
			"scale", ONE_EXACTLY, success_callback,
			"nofade", ZERO_OR_ONE, success_callback,
			"textureloop", ZERO_OR_ONE, success_callback
		),
*/

MapInfo::MapInfo() : texture_file {}, scale { 1 } {}

bool MapInfo::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"textureFile", ZERO_OR_ONE, expect_string(assign_variable_callback_string(texture_file)),
		"scale", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(scale))
	);

	return ret;
}

/* MapItems.gfx */
Projection::Projection() : 
	texture_file {}, size { 1 }, spin { 1 },
	pulsating { false }, pulse_lowest { 1 }, pulse_speed { 1 },
	additative { false }, expanding { 1 }, duration {}, fadeout {} {}

//TODO: Verify whether size, pulseSpeed, duration, fadeout are fixedpoint_t or int
//TODO: Verify there aren't more, unused properties?
bool Projection::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"textureFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_file)),
		"size", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(size)),
		"spin", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(spin)),
		"pulsating", ONE_EXACTLY, expect_bool(assign_variable_callback(pulsating)),
		"pulseLowest", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pulse_lowest)),
		"pulseSpeed", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(pulse_speed)),
		"additative", ONE_EXACTLY, expect_bool(assign_variable_callback(additative)),
		"expanding", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(expanding)),
		"duration", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(duration)),
		"fadeout", ZERO_OR_ONE, expect_fixed_point(assign_variable_callback(fadeout))
	);

	return ret;
}

Billboard::Offset::Offset(fixed_point_t x, fixed_point_t y, fixed_point_t z) :
	x { 0 }, y { 0 }, z { 0 } {}

Billboard::Billboard() : 
	texture_file {}, scale { 1 }, no_of_frames { 1 }, font_size { 7 },
	offset {0,0,0},
	font {} {}

//TODO: billboard was a <StringMapCaseInsensitive> on its dictionnary, how do we preserve this?
bool Billboard::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	int components = 0;
	fixed_point_t x = 0;
	fixed_point_t y = 0;
	fixed_point_t z = 0;

	ret &= add_key_map_entries(key_map,
		"texturefile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_file)),
		"noOfFrames", ZERO_OR_ONE, expect_int64(assign_variable_callback(no_of_frames)),
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),
		"font_size", ZERO_OR_ONE, expect_int64(assign_variable_callback(font_size)),
		"offset2", ZERO_OR_ONE,expect_list_of_length(3, expect_fixed_point(
			[&components,&x,&y,&z](fixed_point_t val) -> bool {
				if(components == 0) x = val;
				else if(components == 1) y = val;
				else z =  val;
				components++;
				return true;
			})
		),
		"font", ZERO_OR_ONE, expect_string(assign_variable_callback_string(font))
	);

	offset.x = x;
	offset.y = y;
	offset.z = z;

	return ret;
}

// Projection type
UnitStatsBillboard::UnitStatsBillboard() : 
	texture_file {}, mask_file {}, effect_file {}, scale { 1 },
	no_of_frames { 1 }, font_size { 7 }, font {} {}

//TODO: Verify font_size is int
//TODO: Verify there aren't more, unused properties?
bool UnitStatsBillboard::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"textureFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(texture_file)),
		"mask", ONE_EXACTLY, expect_string(assign_variable_callback_string(mask_file)),
		"effectFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(effect_file)),		
		"scale", ONE_EXACTLY, expect_fixed_point(assign_variable_callback(scale)),
		"noOfFrames", ONE_EXACTLY, expect_int64(assign_variable_callback(no_of_frames)),
		"font_size", ONE_EXACTLY, expect_int64(assign_variable_callback(font_size)),
		"font", ONE_EXACTLY, expect_string(assign_variable_callback_string(font))
	);

	return ret;
}


// Projection type
ProgressBar3d::ProgressBar3d() : back_colour {}, progress_colour {}, size {}, effect_file {} {}

//TODO: Verify there aren't more, unused properties?
bool ProgressBar3d::_fill_key_map(NodeTools::case_insensitive_key_map_t& key_map) {
	bool ret = Object::_fill_key_map(key_map);

	ret &= add_key_map_entries(key_map,
		"color", ONE_EXACTLY, expect_colour(assign_variable_callback(progress_colour)),
		"colortwo", ONE_EXACTLY, expect_colour(assign_variable_callback(back_colour)),
		"size", ONE_EXACTLY, expect_ivec2(assign_variable_callback(size)),
		"effectFile", ONE_EXACTLY, expect_string(assign_variable_callback_string(effect_file))
	);

	return ret;
}
