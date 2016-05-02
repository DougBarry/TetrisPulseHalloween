/*
  TQED.cpp - Quadrature decoder library for Arduino using I2C
  Revision 1.2
  Copyright (c) 2012 Adriaan Swanepoel.  All right reserved.
  Revised 2014, 2016 by Douglas Barry.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "TQED.h"
#include "TQED_I2CCMD.h"

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Wire.h>
#include <inttypes.h>

union quadruplebyte
{
  int32_t value;
  unsigned char bytes[4];
};

TQED::TQED(uint8_t address)
{
  deviceaddress = address;
  Wire.begin();
}

long TQED::getCount()
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_READ_COUNTER);
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, (uint8_t)4);
  int retry = 0;
  while ((Wire.available() < 4) && (retry < RETRYCOUNT))
    retry++;

  if (Wire.available() >= 4)
  {
    union quadruplebyte count;
    //read 4 bytes and turn them into long
    count.bytes[0] = Wire.read();
    count.bytes[1] = Wire.read();
    count.bytes[2] = Wire.read();
    count.bytes[3] = Wire.read();


    return count.value;
  } else return 0;
}

long TQED::getCounterMin()
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_GET_COUNTERMIN);
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, (uint8_t)4);
  int retry = 0;
  while ((Wire.available() < 4) && (retry < RETRYCOUNT))
    retry++;

  if (Wire.available() >= 4)
  {
    union quadruplebyte count;
    //read 4 bytes and turn them into long
    count.bytes[0] = Wire.read();
    count.bytes[1] = Wire.read();
    count.bytes[2] = Wire.read();
    count.bytes[3] = Wire.read();


    return count.value;
  } else return 0;
}

long TQED::getCounterMax()
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_GET_COUNTERMAX);
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, (uint8_t)4);
  int retry = 0;
  while ((Wire.available() < 4) && (retry < RETRYCOUNT))
    retry++;

  if (Wire.available() >= 4)
  {
    union quadruplebyte count;
    //read 4 bytes and turn them into long
    count.bytes[0] = Wire.read();
    count.bytes[1] = Wire.read();
    count.bytes[2] = Wire.read();
    count.bytes[3] = Wire.read();


    return count.value;
  } else return 0;
}

bool TQED::centerCount()
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_CENTER_COUNTER);
  Wire.endTransmission();
  return true;
}

bool TQED::resetCount()
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_RESET_COUNTER);
  Wire.endTransmission();
  return true;
}

uint8_t TQED::getAddress()
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_GET_ADDRESS);
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, (uint8_t)1);
  int retry = 0;
  while ((Wire.available() < 1) && (retry < RETRYCOUNT))
    retry++;

  uint8_t addr = 0;

  if (Wire.available() >= 1)
  {
    addr = Wire.read();
    return addr;
  } else return 0;

}

bool TQED::setAddress(uint8_t newaddress)
{
  Wire.beginTransmission(deviceaddress);
  Wire.write(TQED_SET_ADDRESS);
  Wire.write(newaddress);
  Wire.endTransmission();
  deviceaddress = newaddress;
  delay(10);
  return true;
}
