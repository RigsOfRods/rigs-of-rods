#pragma once

#include <cstdint>

/// @file
/// @brief Bit operations
typedef unsigned int BitMask_t;

#define BITMASK( OFFSET )           ( 1  << ((OFFSET) - 1) )

#define BITMASK_IS_0( VAR, FLAGS )  ( ((VAR) & (FLAGS)) == 0 )
#define BITMASK_IS_1( VAR, FLAGS )  ( ((VAR) & (FLAGS)) == (FLAGS) )

#define BITMASK_SET_0( VAR, FLAGS ) ( (VAR) &= ~ (FLAGS) )
#define BITMASK_SET_1( VAR, FLAGS ) ( (VAR) |= (FLAGS) )



// --------------- TO BE REMOVED --------------- //

/// Defines a bitmask constant with given bit index (1-indexed) and a getter function. 
#define BITMASK_PROPERTY_GET(VAR, BIT_INDEX, FLAG_NAME, GETTER_NAME) \
	static const BitMask_t FLAG_NAME = BITMASK((BIT_INDEX)); \
	inline bool GETTER_NAME() const { return BITMASK_IS_1((VAR), FLAG_NAME); }

