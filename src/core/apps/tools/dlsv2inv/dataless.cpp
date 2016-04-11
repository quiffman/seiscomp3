/*===========================================================================================================================
    Name:       dataless.C

    Purpose:    synchronization of GDI and SeisComp

    Language:   C++, ANSI standard.

    Author:     Peter de Boer, KNMI

    Revision:	2007-11-26	0.1	initial version

===========================================================================================================================*/
#include <fstream>
#include "dataless.h"

#include <seiscomp3/core/exceptions.h>

#define SEISCOMP_COMPONENT sync_dlsv
#include <seiscomp3/logging/log.h>

using namespace std;

/****************************************************************************************************************************
* Function:     SynchronizeDataless                                                                                         *
* Parameters:   string name of the file to process                                                                          *
* Returns:      nothing                                                                                                     *
* Description:	this function calls CreateCommand, SendCommand and ProcessFiles						    *
****************************************************************************************************************************/
void Dataless::SynchronizeDataless(string dataless, Seiscomp::Communication::Connection* conn)
{
	SEISCOMP_INFO("START PROCESSING DATALESS");

	invent = new Inventory(init_values, conn);
	invent->vic = new VolumeIndexControl();
	invent->adc = new AbbreviationDictionaryControl();
	invent->sc = new StationControl();
	ParseDataless(dataless);
}

/****************************************************************************************************************************
* Function:     ParseDataless												    *
* Parameters:   file	- name of the file to process									    *
* Returns:      nothing													    *
* Description:  reads the file and processes every 4096 bytes of metadata						    *
****************************************************************************************************************************/
void Dataless::ParseDataless(string file)
{
	char buf[LRECL];
	int pos1, pos2;
	ifstream dataless(file.c_str());
	
	try
	{
		if(dataless.is_open())
		{
			while(dataless.read(buf, LRECL))
			{
				pos1 = pos2 = 0;
				string record(buf);
				string volume = SplitString(record, LINE_SEPARATOR, pos1, pos2);
				if(volume.size()!=7)
				{
					pos1 = pos2 = 0;
					volume = SplitString(record, '*', pos1, pos2);
				}
				pos2++;
				if(volume[volume.size()-1] == 'V')
				{
					invent->vic->ParseVolumeRecord(record.substr(pos2, (LRECL-pos2)));
				}
				else if(volume[volume.size()-1] == 'A')
				{ 
					invent->adc->ParseVolumeRecord(record.substr(pos2, (LRECL-pos2)));
				}
				else if(volume[volume.size()-1] == 'S')
				{
					invent->sc->ParseVolumeRecord(record.substr(pos2, (LRECL-pos2)));
				}
			}
		
			if(invent->SetConnection())
			{
				invent->SynchronizeInventory();
				invent->CloseConnection();
				invent->vic->EmptyVectors();
				invent->adc->EmptyVectors();
				invent->sc->EmptyVectors();
			}
		}
		else
		{
			SEISCOMP_ERROR("Cannot open %s", file.c_str());
			invent->CloseConnection();
		}
	}
	catch(BadConversion &o)
	{
		SEISCOMP_ERROR("Bad conversion: %s", o.what());
		invent->CloseConnection();
	}
	catch(out_of_range &o)
	{
		SEISCOMP_ERROR("Out of range: %s", o.what());
		invent->CloseConnection();
	}
	catch(length_error &o)
	{
		SEISCOMP_ERROR("Length error: %s", o.what());
		invent->CloseConnection();
	}
	catch(Seiscomp::Core::ValueException &o)
	{
		SEISCOMP_ERROR("Seiscomp::Core::ValueException: %s", o.what());
		invent->CloseConnection();
	}
	catch(Seiscomp::Core::GeneralException &o)
	{
		SEISCOMP_ERROR("Seiscomp::Core::GeneralException: %s", o.what());
		invent->CloseConnection();
	}
	catch(...)
	{
		SEISCOMP_ERROR("An unknown error has occurred");
		invent->CloseConnection();
	}
}  
