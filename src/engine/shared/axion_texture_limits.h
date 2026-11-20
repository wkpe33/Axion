#ifndef ENGINE_SHARED_AXION_TEXTURE_LIMITS_H
#define ENGINE_SHARED_AXION_TEXTURE_LIMITS_H
#include <algorithm>
#include <cstdint>
#include <utility>

// Bound decoded images before creating GPU textures; keep sprite-sheet grids.
inline std::pair<int, int> AxionBoundImage(int Width, int Height, int MaxWidth, int MaxHeight, int GridX = 1, int GridY = 1)
{
	if(Width <= 0 || Height <= 0 || GridX <= 0 || GridY <= 0)
		return {0, 0};
	const double Scale = std::min({1.0, (double)MaxWidth / Width, (double)MaxHeight / Height});
	return {std::max(GridX, (int)(Width * Scale / GridX) * GridX), std::max(GridY, (int)(Height * Scale / GridY) * GridY)};
}
constexpr uint64_t AXION_SKIN_TEXTURE_HIGH_WATER = 768ull * 1024 * 1024;
#endif
