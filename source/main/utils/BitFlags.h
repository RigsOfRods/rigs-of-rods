#pragma once

#include <cstdint>

/** @file Bit operations */

// BITMASK(1) = 0x00000001 = 0b00....0001
// BITMASK(2) = 0x00000002 = 0b00....0010
#define BITMASK( OFFSET )           (          1  << ((OFFSET) - 1) )
#define BITMASK_64( OFFSET )        ( uint64_t(1) << ((OFFSET) - 1) )

#define BITMASK_SET_0( VAR, FLAGS ) ( (VAR) &= ~ (FLAGS) )

#define BITMASK_SET_1( VAR, FLAGS ) ( (VAR) |= (FLAGS) )

template<typename var_T, typename flags_T> void Bitmask_SetBool(bool val, var_T & _in_out_var, flags_T flags)
{
	// This code is supposed to compile only as conditional assignments, not as branches.

	var_T var_true  = _in_out_var;
	BITMASK_SET_1(var_true, flags);

	var_T var_false = _in_out_var;
	BITMASK_SET_0(var_false, flags);

	_in_out_var = (val) ? var_true : var_false;
}

#define BITMASK_IS_0( VAR, FLAGS )  ( ((VAR) & (FLAGS)) == 0 )

#define BITMASK_IS_1( VAR, FLAGS )  ( ((VAR) & (FLAGS)) == (FLAGS) )

#define BITMASK_FILL_1_EXCEPT( FLAGS_0 ) (( 0xFFFFFFFF & ~((FLAGS_0)) ))

/// Defines a bitmask constant with given bit index (1-indexed) and a getter function. 
#define BITMASK_PROPERTY_GET(VAR, BIT_INDEX, FLAG_NAME, GETTER_NAME) \
	static const unsigned int FLAG_NAME = BITMASK((BIT_INDEX)); \
	inline bool GETTER_NAME() const { return BITMASK_IS_1((VAR), FLAG_NAME); }

/// Defines a bitmask constant with given bit index (1-indexed) and a getter/setter functions.
#define BITMASK_PROPERTY(VAR, BIT_INDEX, FLAG_NAME, GETTER_NAME, SETTER_NAME) \
	BITMASK_PROPERTY_GET((VAR), (BIT_INDEX), FLAG_NAME, GETTER_NAME) \
	inline void SETTER_NAME(bool value) { Bitmask_SetBool(value, (VAR), FLAG_NAME); }

