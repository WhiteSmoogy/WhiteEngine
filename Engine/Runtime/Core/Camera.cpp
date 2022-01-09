#include "Camera.h"
#include <cmath>

using namespace WhiteEngine::Core;

void Camera::SetFrustum(white::uint16 _width, white::uint16 _height, const FrustumElement & frustum)
{
	width = _width;
	height = _height;

	frustum_elemnt = frustum;
}
