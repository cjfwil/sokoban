/* date = December 11th 2020 6:03 pm */

#ifndef NEWTYPES_H
#define NEWTYPES_H

typedef char unsigned u8;
typedef int unsigned u32;
typedef u32 b32;

#define true 1
#define false 0

#define Max(a, b) ((a > b) ? a : b)

#define i2(x, y, w) ((x) + ((y) * (w)))

#define Clamp(v, a, b) ((v > b) ? b : ((v < a) ? a : v))

#endif //NEWTYPES_H
