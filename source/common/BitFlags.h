#pragma once

/** @file Bit operations */

// BITMASK(1) = 0x00000001 = 0b00....0001
// BITMASK(2) = 0x00000002 = 0b00....0010
#define BITMASK( OFFSET )           (1 << ( (OFFSET) - 1))

#define BITMASK_SET_0( VAR, FLAGS ) ( (VAR) &= ~ (FLAGS) )

#define BITMASK_SET_1( VAR, FLAGS ) ( (VAR) |= (FLAGS) )

#define BITMASK_IS_0( VAR, FLAGS )  ( ((VAR) & (FLAGS)) == 0 )

#define BITMASK_IS_1( VAR, FLAGS )  ( ((VAR) & (FLAGS)) == (FLAGS) )