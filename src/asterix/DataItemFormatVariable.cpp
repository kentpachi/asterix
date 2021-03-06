/*
 *  Copyright (c) 2013 Croatia Control Ltd. (www.crocontrol.hr)
 *
 *  This file is part of Asterix.
 *
 *  Asterix is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Asterix is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Asterix.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * AUTHORS: Damir Salantic, Croatia Control Ltd.
 *
 */

#include "DataItemFormatVariable.h"
#include "Tracer.h"
#include "asterixformat.hxx"

DataItemFormatVariable::DataItemFormatVariable()
{
}

DataItemFormatVariable::~DataItemFormatVariable()
{
  // destroy list items
  std::list<DataItemFormat*>::iterator it = m_lSubItems.begin();
  while(it != m_lSubItems.end())
  {
    delete (DataItemFormatFixed*)(*it);
    it = m_lSubItems.erase(it);
  }
}

long DataItemFormatVariable::getLength(const unsigned char* pData)
{
  long length = 0;
  std::list<DataItemFormat*>::iterator it;
  bool lastPart = false;
  it=m_lSubItems.begin();

  DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);

  do
  {
    lastPart = dip->isLastPart(pData);
    long partlen = dip->getLength();

    length += partlen;
    pData += partlen;

    if (it != m_lSubItems.end())
    {
      it++;
      if (it != m_lSubItems.end())
      {
        dip = (DataItemFormatFixed*)(*it);
      }
    }
  }
  while(!lastPart);

  return length;
}

void DataItemFormatVariable::addBits(DataItemBits* pBits)
{
  std::list<DataItemFormat*>::reverse_iterator rit;
  rit = m_lSubItems.rbegin();
  if (rit != m_lSubItems.rend())
  {
    DataItemFormatFixed* dip = (DataItemFormatFixed*)(*rit);
    if (dip)
    {
      dip->addBits(pBits);
      return;
    }
  }
  Tracer::Error("Adding bits to Variable failed");
}

bool DataItemFormatVariable::getText(std::string& strResult, std::string& strHeader, const unsigned int formatType, unsigned char* pData, long)
{
  bool ret = false;

  std::list<DataItemFormat*>::iterator it;
  bool lastPart = false;
  it=m_lSubItems.begin();
  std::string tmpResult;

  DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);

  switch(formatType)
    {
   	  case CAsterixFormat::EJSON:
   	  case CAsterixFormat::EJSONH:
      {
   		strResult += '{';
      }
   	  break;
    }

  do
  {
    lastPart = dip->isLastPart(pData);

    switch(formatType)
    {
  	  case CAsterixFormat::EJSON:
  	  case CAsterixFormat::EJSONH:
      {
  		  tmpResult = "";
  		  ret |= dip->getText(tmpResult, strHeader, formatType, pData, dip->getLength());
  		  strResult += tmpResult.substr(1, tmpResult.length()-2); // trim {}
  		 if (!lastPart)
  			strResult += ',';
  }
  	  break;
  	  default:
  		ret |= dip->getText(strResult, strHeader, formatType, pData, dip->getLength());
  	  break;
}

    pData += dip->getLength();

    if (it != m_lSubItems.end())
    {
      it++;
      if (it != m_lSubItems.end())
      {
        dip = (DataItemFormatFixed*)(*it);
      }
    }
  }
  while(!lastPart);

  switch(formatType)
    {
   	  case CAsterixFormat::EJSON:
   	  case CAsterixFormat::EJSONH:
      {
   		strResult += '}';
    }
   	  break;
  }

  return ret;
}

std::string DataItemFormatVariable::printDescriptors(std::string header)
{
	std::string strDef = "";

	std::list<DataItemFormat*>::iterator it;
	for ( it=m_lSubItems.begin() ; it != m_lSubItems.end(); it++ )
  {
		DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);
		strDef += dip->printDescriptors(header);
	}
	return strDef;
}

bool DataItemFormatVariable::filterOutItem(const char* name)
    {
	std::list<DataItemFormat*>::iterator it;
	for ( it=m_lSubItems.begin() ; it != m_lSubItems.end(); it++ )
      {
		DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);
		if (true == dip->filterOutItem(name))
			return true;
  }
  return false;
}

bool DataItemFormatVariable::isFiltered(const char* name)
{
	std::list<DataItemFormat*>::iterator it;
	for ( it=m_lSubItems.begin() ; it != m_lSubItems.end(); it++ )
  {
		DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);
		if (true == dip->isFiltered(name))
      return true;
  }
  return false;
}

#if defined(WIRESHARK_WRAPPER) || defined(ETHEREAL_WRAPPER)
fulliautomatix_definitions* DataItemFormatVariable::getWiresharkDefinitions()
{
  fulliautomatix_definitions *def = NULL, *startDef = NULL;

  std::list<DataItemFormat*>::iterator it;
  for ( it=m_lSubItems.begin() ; it != m_lSubItems.end(); it++ )
  {
    DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);
    if (def)
    {
      def->next = dip->getWiresharkDefinitions();
    }
    else
    {
      startDef = def = dip->getWiresharkDefinitions();
    }
    while(def->next)
      def = def->next;
  }
  return startDef;
}

fulliautomatix_data* DataItemFormatVariable::getData(unsigned char* pData, long len, int byteoffset)
{
  fulliautomatix_data *lastData = NULL, *firstData = NULL;
  std::list<DataItemFormat*>::iterator it;
  bool lastPart = false;

  it=m_lSubItems.begin();

  DataItemFormatFixed* dip = (DataItemFormatFixed*)(*it);

  do
  {
    lastPart = dip->isLastPart(pData);

    if (lastData)
    {
      lastData->next = dip->getData(pData, dip->getLength(), byteoffset);
    }
    else
    {
      firstData = lastData = dip->getData(pData, dip->getLength(), byteoffset);
    }
    while(lastData->next)
      lastData = lastData->next;

    byteoffset += dip->getLength();
    pData += dip->getLength();
    len -= dip->getLength();

    if (it != m_lSubItems.end())
    {
      it++;
      if (it != m_lSubItems.end())
      {
        dip = (DataItemFormatFixed*)(*it);
      }
    }
  }
  while(!lastPart && len > 0);

  return firstData;
}
#endif
