///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 常用代码
#pragma once


template <typename T1, typename T2>
inline T1 __round_mask(T1 x, T2 y) { return (T1)((y)-1); }

// 以y为单位，向上对齐。但是不缩小x。y必须是2的整幂
template <typename T>
inline T round_up(T x, const T y) { return (((x - 1) | __round_mask(x, y)) + 1); }
//#define round_up(x, y)		((((x)-1) | __round_mask(x, y))+1)
/** round_down - round down to next specified power of 2
 * @x: the value to round
 * @y: multiple to round down to (must be a power of 2)
 *
 * Rounds @x down to next multiple of @y (which must be a power of 2). To perform arbitrary rounding down, use rounddown() below. */
#define round_down(x, y)	((x) & ~__round_mask(x, y))
