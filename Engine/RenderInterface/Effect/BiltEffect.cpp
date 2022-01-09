#include "BiltEffect.h"

platform::Render::Effect::BiltEffect::BiltEffect(const std::string & name)
	:Effect(name), BlitLinear2D(GetTechnique("BlitLinear2D")),
	src_offset(GetParameterRef<white::math::float3>("src_offset")),
	src_scale(GetParameterRef<white::math::float3>("src_scale")),
	src_level(GetParameterRef<int>("src_level"))
{
}
