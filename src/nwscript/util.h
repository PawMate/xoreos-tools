/* xoreos - A reimplementation of BioWare's Aurora engine
 *
 * xoreos is the legal property of its developers, whose names
 * can be found in the AUTHORS file distributed with this source
 * distribution.
 *
 * xoreos is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * xoreos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xoreos. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  NWScript utility functions.
 */

#ifndef NWSCRIPT_UTIL_H
#define NWSCRIPT_UTIL_H

#include "src/common/ustring.h"

#include "src/nwscript/types.h"

namespace NWScript {

/** Return the textual name of the opcode. */
Common::UString getOpcodeName(Opcode op);

/** Return the textual suffix of the opcode's type. */
Common::UString getInstTypeName(InstructionType type);

/** Return the direct arguments this opcode takes.
 *
 *  Please note that there are 3 exceptions that require special handling:
 *  - kOpcodeCONST has one argument of a variable type
 *  - kOpcodeEQ has no direct arguments, except if type is kInstTypeStructStruct,
 *    then it has one of type kOpcodeArgUint16
 *  - kOpcodeNEQ has no direct arguments, except if type is kInstTypeStructStruct,
 *    then it has one of type kOpcodeArgUint16
 */
const OpcodeArgument *getDirectArguments(Opcode op);

/** Return the number of direct arguments this opcode takes.
 *
 *  Please note that there are 3 exceptions that require special handling:
 *  - kOpcodeCONST has one argument of a variable type
 *  - kOpcodeEQ has no direct arguments, except if type is kInstTypeStructStruct,
 *    then it has one of type kOpcodeArgUint16
 *  - kOpcodeNEQ has no direct arguments, except if type is kInstTypeStructStruct,
 *    then it has one of type kOpcodeArgUint16
 */
size_t getDirectArgumentCount(Opcode op);

/** Format the bytes compromising this instruction into a string.
 *
 *  This includes the opcode, the instruction type and the direct
 *  arguments. However, for the CONST instruction with a string
 *  direct argument, the literal text "str" is printed instead
 *  of the actual string.
 *
 *  Examples:
 *  01 01 FFFFFFFC 04
 *  04 05 str
 *
 *  The final formatted string will not exceed 26 characters.
 */
Common::UString formatBytes(const Instruction &instr);

/** Format the instruction into an assembly-like mnemonic string.
 *
 *  This includes the opcode, the instruction type and the direct
 *  arguments.
 *
 *  Examples:
 *  CPDOWNSP -4 4
 *  CONSTS "Foobar"
 */
Common::UString formatInstruction(const Instruction &instr);

} // End of namespace NWScript

#endif // NWSCRIPT_UTIL_H
