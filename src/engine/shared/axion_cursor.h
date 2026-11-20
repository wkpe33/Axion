#ifndef ENGINE_SHARED_AXION_CURSOR_H
#define ENGINE_SHARED_AXION_CURSOR_H
#include <base/vmath.h>

#include <algorithm>
inline vec2 AxionCursorForCanvas(vec2 Pixels, vec2 WindowSize, vec2 CanvasSize)
{
	WindowSize.x = std::max(1.0f, WindowSize.x);
	WindowSize.y = std::max(1.0f, WindowSize.y);
	Pixels.x = std::clamp(Pixels.x, 0.0f, WindowSize.x - 1);
	Pixels.y = std::clamp(Pixels.y, 0.0f, WindowSize.y - 1);
	return Pixels * CanvasSize / WindowSize;
}
#endif
