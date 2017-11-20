
#include "SBTypes.h"

SBMacAddress::SBMacAddress(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5){
  this->Bytes[0] = a1;
  this->Bytes[1] = a2;
  this->Bytes[2] = a3;
  this->Bytes[3] = a4;
  this->Bytes[4] = a5;
}

bool SBMacAddress::isEquals(SBMacAddress otherAddress){
  if (this->Bytes[4] != otherAddress.Bytes[4]){
    return false;
  }
  if (this->Bytes[3] != otherAddress.Bytes[3]){
    return false;
  }
  if (this->Bytes[2] != otherAddress.Bytes[2]){
    return false;
  }
  if (this->Bytes[1] != otherAddress.Bytes[1]){
    return false;
  }
  if (this->Bytes[0] != otherAddress.Bytes[0]){
    return false;
  }
  return true;
}

