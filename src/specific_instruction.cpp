#include "specific_instruction.hpp"

#include <vector>

#include "address.hpp"
#include "const.hpp"
#include "ram.hpp"
#include "util.hpp"

using namespace std;

static void increasePc(vector<bool> &pc);
static Address getThreeBitAddress(const vector<bool> &val);
static void addOrSubtract(const Address &adr, vector<bool> &reg, Ram &ram, bool add);
static void shift(vector<bool> &pc, vector<bool> &reg, int delta);
static bool getRegBit(vector<bool> &reg, int index);
static void andOrOr(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram,
                    bool isAnd);
static void incOrDec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram,
                     bool isInc);

// READ

/*
 * Copies value at the passed address to the register.
 */
void Read::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  reg = ram.get(adr);
  increasePc(pc);
}

vector<Address> Read::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address Read::getAddress(Address &firstOrderAdr, const vector<bool> &reg, 
                        const Ram &ram) {
  return firstOrderAdr;
}

string Read::getLabel() {
  return "READ  ";
}

string Read::getCode(const vector<bool> &val) {
  int intAdr = Util::getInt(Util::getSecondNibble(val));
  if (intAdr == RAM_SIZE) {
    return "reg = predecesor();";
  } else {
    return "reg = data["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
  }
}

// WRITE

/*
 * Copies value of the register to the passed address.
 */
void Write::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  ram.set(adr, reg);
  increasePc(pc);
}

vector<Address> Write::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address Write::getAddress(Address &firstOrderAdr, const vector<bool> &reg, 
                          const Ram &ram) {
  return firstOrderAdr;  
}

string Write::getLabel() {
  return "WRITE  ";
}

string Write::getCode(const vector<bool> &val) {
  int intAdr = Util::getInt(Util::getSecondNibble(val));
  if (intAdr == RAM_SIZE) {
    return "return reg;";
  } else {
    return "data["+to_string(intAdr)+"] = reg;";
  }
}

// ADD

/*
 * Adds value at the passed address to the register, and
 * stores result in the register.
 */
void Add::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  addOrSubtract(adr, reg, ram, true);
  increasePc(pc);
}

vector<Address> Add::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address Add::getAddress(Address &firstOrderAdr, const vector<bool> &reg, 
                        const Ram &ram) {
  return firstOrderAdr;  
}

string Add::getLabel() {
  return "ADD";
}

string Add::getCode(const vector<bool> &val) {
  return "reg = sadd(reg, data["+to_string(Util::getInt(Util::getSecondNibble(val)))+"]);";
}

// SUB

/*
 * Subtracts value at the passed address from the register, and
 * stores result in the register.
 */
void Sub::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  addOrSubtract(adr, reg, ram, false);
  increasePc(pc);
}

vector<Address> Sub::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address Sub::getAddress(Address &firstOrderAdr, const vector<bool> &reg, 
                        const Ram &ram) {
  return firstOrderAdr;  
}

string Sub::getLabel() {
  return "SUB";
}

string Sub::getCode(const vector<bool> &val) {
  return "reg = ssub(reg, data["+to_string(Util::getInt(Util::getSecondNibble(val)))+"]);";
}

// JUMP

/*
 * Jumps to the passed address.
 */
void Jump::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  pc = adr.val;
}

vector<Address> Jump::getFirstOrderAdr(vector<bool> &val) {
  return { Address(CODE, Util::getSecondNibble(val)) };
}

Address Jump::getAddress(Address &firstOrderAdr, const vector<bool> &reg, 
                         const Ram &ram) {
  return firstOrderAdr;  
}

string Jump::getLabel() {
  return "JUMP";
}

string Jump::getCode(const vector<bool> &val) {
  return "goto *labels["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
}

// IF MAX

/*
 * Jumps to passed address if value of the register is 'max'.
 */
void IfMax::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  if (Util::getInt(reg) >= MAX_VALUE) {
    pc = adr.val;
  } else {
    increasePc(pc);
  }
}

vector<Address> IfMax::getFirstOrderAdr(vector<bool> &val) {
  return { Address(CODE, Util::getSecondNibble(val)) };
}

Address IfMax::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                          const Ram &ram) {
  return firstOrderAdr;  
}

string IfMax::getLabel() {
  return "IF MAX";
}

string IfMax::getCode(const vector<bool> &val) {
  return "if (reg == "+to_string(MAX_VALUE)+") goto *labels["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
}

// IF MIN

/*
 * Jumps to passed address if value of the register is 'min'.
 */
void IfMin::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  if (Util::getInt(reg) <= 0) {
    pc = adr.val;
  } else {
    increasePc(pc);
  }
}

vector<Address> IfMin::getFirstOrderAdr(vector<bool> &val) {
  return { Address(CODE, Util::getSecondNibble(val)) };
}

Address IfMin::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                          const Ram &ram) {
  return firstOrderAdr;  
}

string IfMin::getLabel() {
  return "IF MIN";
}

string IfMin::getCode(const vector<bool> &val) {
  return "if (reg == 0) goto *labels["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
}

// JUMP REG

/*
 * Jumps to the address stored in register.
 */
void JumpReg::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  pc = adr.val;
}

vector<Address> JumpReg::getFirstOrderAdr(vector<bool> &val) {
  return { Address(NONE, FIRST_ADDRESS) };
}

Address JumpReg::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                            const Ram &ram) {
  return Address(CODE, Util::getSecondNibble(reg));
}

string JumpReg::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string JumpReg::getCode(const vector<bool> &val) {
  return "goto *labels[reg&"+ to_string(RAM_SIZE) +"];";
}

// READ REG

/*
 * Copies value at the address that is stored in register
 * to the register.
 */
void ReadReg::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  reg = ram.get(adr);
  increasePc(pc);
}

vector<Address> ReadReg::getFirstOrderAdr(vector<bool> &val) {
  return { Address(NONE, FIRST_ADDRESS) };
}

Address ReadReg::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                            const Ram &ram) {
  return Address(DATA, Util::getSecondNibble(reg));
}

string ReadReg::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string ReadReg::getCode(const vector<bool> &val) {
  return "reg = data[reg&"+ to_string(RAM_SIZE) +"];";
}

// INITIALIZE FIRST ADDRESS

/*
 * Copies value at the ningth address to the first address.
 */
void InitializeFirstAddress::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  vector<bool> value = ram.get(Address(DATA, Util::getBoolNibb(1)));
  ram.set(adr, value);
  reg = value;
  increasePc(pc);
}

vector<Address> InitializeFirstAddress::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, FIRST_ADDRESS), Address(DATA, Util::getBoolNibb(1)) };
}

Address InitializeFirstAddress::getAddress(Address &firstOrderAdr,
                                           const vector<bool> &reg,
                                           const Ram &ram) {
  return firstOrderAdr;  
}

string InitializeFirstAddress::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string InitializeFirstAddress::getCode(const vector<bool> &val) {
  return "data[0] = data[1]; reg = data[0];";
}

// NOT

/*
 * Executes 'not' operation on the value of the register.
 */
void Not::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  reg = Util::bitwiseNot(reg);
  increasePc(pc);
}

vector<Address> Not::getFirstOrderAdr(vector<bool> &val) {
  return { Address(NONE, FIRST_ADDRESS) };
}

Address Not::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                        const Ram &ram) {
  return firstOrderAdr;  
}

string Not::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string Not::getCode(const vector<bool> &val) {
  return "reg = ~reg;";
}

// SHIFT LEFT

/*
 * Shifts bits in the register one spot to the left.
 */
void ShiftLeft::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                     Ram &ram) {
  shift(pc, reg, 1);
}

vector<Address> ShiftLeft::getFirstOrderAdr(vector<bool> &val) {
  return { Address(NONE, FIRST_ADDRESS) };
}

Address ShiftLeft::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                              const Ram &ram) {
  return firstOrderAdr;  
}

string ShiftLeft::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string ShiftLeft::getCode(const vector<bool> &val) {
  return "reg <<= 1;";
}

// SHIFT RIGHT

/*
 * Shifts bits in the register one spot to the right.
 */
void ShiftRight::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                      Ram &ram) {
  shift(pc, reg, -1);
}

vector<Address> ShiftRight::getFirstOrderAdr(vector<bool> &val) {
  return { Address(NONE, FIRST_ADDRESS) };
}

Address ShiftRight::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                               const Ram &ram) {
  return firstOrderAdr;  
}

string ShiftRight::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string ShiftRight::getCode(const vector<bool> &val) {
  return "reg >>= 1;";
}

// AND

/*
 * Executes 'and' operation between register value, and
 * value at first address and writes the result to register.
 */
void And::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  andOrOr(adr, pc, reg, ram, true);
}

vector<Address> And::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getBoolNibb(2)) };
}

Address And::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                        const Ram &ram) {
  return firstOrderAdr;  
}

string And::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string And::getCode(const vector<bool> &val) {
  return "reg &= data[2];";
}

// OR

/*
 * Executes 'or' operation between register value, and
 * value at first address and writes the result to register.
 */
void Or::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  andOrOr(adr, pc, reg, ram, false);
}

vector<Address> Or::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getBoolNibb(3)) };
}

Address Or::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                       const Ram &ram) {
  return firstOrderAdr;  
}

string Or::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string Or::getCode(const vector<bool> &val) {
  return "reg |= data[3];";
}

// XOR

/*
 * Executes 'xor' operation between register value, and
 * value at the specified address (0-7) and writes the result to register.
 */
void Xor::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  vector<bool> ramValue = ram.get(adr);
  reg = Util::bitwiseXor(reg, ramValue);
  increasePc(pc);
}

vector<Address> Xor::getFirstOrderAdr(vector<bool> &val) {
  return { getThreeBitAddress(val) };
}

Address Xor::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                        const Ram &ram) {
  return firstOrderAdr;  
}

string Xor::getLabel() {
  return LOGIC_OPS_INDICATOR;
}

string Xor::getCode(const vector<bool> &val) {
  return "reg ^= data[" + to_string(Util::getInt(getThreeBitAddress(val).val)) + "];";
}

// READ POINTER

/*
 * Reads from the address that is stored at passed address.
 */
void ReadPointer::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                       Ram &ram) {
  reg = ram.get(adr);
  increasePc(pc);
}

vector<Address> ReadPointer::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address ReadPointer::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                                const Ram &ram) {
  vector<bool> pointer = ram.get(firstOrderAdr);
  return Address(DATA, Util::getSecondNibble(pointer));
}

string ReadPointer::getLabel() {
  return "READ *";
}

string ReadPointer::getCode(const vector<bool> &val) {
  return "reg = data[data["+ to_string(Util::getInt(Util::getSecondNibble(val))) +"]&"+to_string(RAM_SIZE)+"];";
}

// WRITE POINTER

/*
 * Writes 'reg' to the address that is stored at passed address.
 */
void WritePointer::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                        Ram &ram) {
  ram.set(adr, reg);
  increasePc(pc);
}

vector<Address> WritePointer::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address WritePointer::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                                 const Ram &ram) {
  vector<bool> pointer = ram.get(firstOrderAdr);
  return Address(DATA, Util::getSecondNibble(pointer));
}

string WritePointer::getLabel() {
  return "WRITE *";
}

string WritePointer::getCode(const vector<bool> &val) {
  return "data[data["+ to_string(Util::getInt(Util::getSecondNibble(val))) +"]&"+to_string(RAM_SIZE)+"] = reg;";
}

// INCREASE

/*
 * Increases value at the passed address, and copies
 * it to the register.
 */
void Increase::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                    Ram &ram) {
  incOrDec(adr, pc, reg, ram, true);
}

vector<Address> Increase::getFirstOrderAdr(vector<bool> &val) {
  return { getThreeBitAddress(val) };
}

Address Increase::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                             const Ram &ram) {
  return firstOrderAdr;  
}

string Increase::getLabel() {
  return "INC/DEC";
}

string Increase::getCode(const vector<bool> &val) {
  string strAdr = to_string(Util::getInt(getThreeBitAddress(val).val));
  return "data[" +strAdr+ "] = sadd(data[" +strAdr+ "], 1); reg = data["+strAdr+"];";
}

// DECREASE

/*
 * Decreases value at the passed address, and copies
 * it to the register.
 */
void Decrease::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                    Ram &ram) {
  incOrDec(adr, pc, reg, ram, false);
}

vector<Address> Decrease::getFirstOrderAdr(vector<bool> &val) {
  return { getThreeBitAddress(val) };
}

Address Decrease::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                             const Ram &ram) {
  return firstOrderAdr;  
}

string Decrease::getLabel() {
  return "INC/DEC";
}

string Decrease::getCode(const vector<bool> &val) {
  string strAdr = to_string(Util::getInt(getThreeBitAddress(val).val));
  return "data[" +strAdr+ "] = ssub(data[" +strAdr+ "], 1); reg = data["+strAdr+"];";
}

// PRINT

/*
 * Copies value at the passed address to the last address and thus
 * sends it to the printer.
 */
void Print::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram) {
  vector<bool> val = ram.get(adr);
  ram.set(Address(DATA, LAST_ADDRESS), val);
  increasePc(pc);
}

vector<Address> Print::getFirstOrderAdr(vector<bool> &val) {
  return { Address(DATA, Util::getSecondNibble(val)) };
}

Address Print::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                          const Ram &ram) {
  return firstOrderAdr;  
}

string Print::getLabel() {
  return "PRINT";
}

string Print::getCode(const vector<bool> &val) {
  return "return data["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
}

// IF NOT MAX

/*
 * Jumps to passed address if value of the register is not 'max'.
 */
void IfNotMax::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                    Ram &ram) {
  if (Util::getInt(reg) >= MAX_VALUE) {
    increasePc(pc);
  } else {
    pc = adr.val;
  }
}

vector<Address> IfNotMax::getFirstOrderAdr(vector<bool> &val) {
  return { Address(CODE, Util::getSecondNibble(val)) };
}

Address IfNotMax::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                             const Ram &ram) {
  return firstOrderAdr;  
}

string IfNotMax::getLabel() {
  return "IF NOT MAX";
}

string IfNotMax::getCode(const vector<bool> &val) {
  return "if (reg != "+to_string(MAX_VALUE)+") goto *labels["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
}

// IF NOT MIN

/*
 * Jumps to passed address if value of the register is not 'min'.
 */
void IfNotMin::exec(const Address &adr, vector<bool> &pc, vector<bool> &reg,
                    Ram &ram) {
  if (Util::getInt(reg) <= 0) {
    increasePc(pc);
  } else {
    pc = adr.val;
  }
}

vector<Address> IfNotMin::getFirstOrderAdr(vector<bool> &val) {
  return { Address(CODE, Util::getSecondNibble(val)) };
}

Address IfNotMin::getAddress(Address &firstOrderAdr, const vector<bool> &reg,
                             const Ram &ram) {
  return firstOrderAdr;  
}

string IfNotMin::getLabel() {
  return "IF NOT MIN";
}

string IfNotMin::getCode(const vector<bool> &val) {
  return "if (reg != 0) goto *labels["+to_string(Util::getInt(Util::getSecondNibble(val)))+"];";
}

//////////
// UTIL //
//////////

void increasePc(vector<bool> &pc) {
  pc = Util::getBoolNibb(Util::getInt(pc) + 1);
}

Address getThreeBitAddress(const vector<bool> &val) {
  vector<bool> adr = Util::getSecondNibble(val);
  adr[0] = false;
  return Address(DATA, adr);
}

/*
 * Adds or subtracts value at passed address from register,
 * and stores result in the register.
 */
void addOrSubtract(const Address &adr, vector<bool> &reg, Ram &ram, bool add) {
  int regValue = Util::getInt(reg);
  int ramValue = Util::getInt(ram.get(adr));
  if (add) {
    reg = Util::getBoolByte(regValue + ramValue);
  } else {
    reg = Util::getBoolByte(regValue - ramValue);
  }
}

/*
 * Shifts bits in the register for 'delta' spots.
 */
void shift(vector<bool> &pc, vector<bool> &reg, int delta) {
  vector<bool> tmp = vector<bool>(WORD_SIZE);
  for(int i = 0; i < WORD_SIZE; i++) {
    tmp[i] = getRegBit(reg, i + delta);
  }
  reg = tmp;
  increasePc(pc);
}

/*
 * Returns register bit at passed position, or 'false' if position 
 * is out of bounds.
 */
bool getRegBit(vector<bool> &reg, int index) {
  bool indexOutOfBounds = index < 0 || index >= WORD_SIZE;
  if (indexOutOfBounds) {
    return false;
  }
  return reg.at(index);
}

/*
 * Executes 'and' operation between register value, and
 * value at passed address and writes the result to register.
 */
void andOrOr(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram,
             bool isAnd) {
  vector<bool> ramValue = ram.get(adr);
  reg = Util::bitwiseAndOrOr(reg, ramValue, isAnd);
  increasePc(pc);
}

/*
 * Increases or decreases value at the passed address, and copies
 * it to register.
 */
void incOrDec(const Address &adr, vector<bool> &pc, vector<bool> &reg, Ram &ram,
              bool isInc) {
  vector<bool> value = ram.get(adr);
  int intValue = Util::getInt(value);
  if (isInc) {
    if (intValue == MAX_VALUE) {
      intValue = 0;
    } else {
      intValue++;
    }
  } else {
    if (intValue == 0) {
      intValue = MAX_VALUE;
    } else {
      intValue--;
    }
  }
  ram.set(adr, Util::getBoolByte(intValue));
  reg = Util::getBoolByte(intValue);
  increasePc(pc);
}
