/*===========================================================================================================================
    Name:       dataless.h

    Purpose:    setting up synchronization between GDI and SeisComp

    Language:   C++, ANSI standard.

    Author:     Peter de Boer

    Revision:	2007-11-26	0.1	initial version

===========================================================================================================================*/
#ifndef DATALESS_H
#define DATALESS_H

#include <sstream>
#ifndef WIN32
#include <unistd.h>
#endif
#include <seiscomp3/communication/connection.h>
#include "inventory.h"
#include "define.h"
#include "tmanip.h"
#include "mystring.h"
#include "seed.h"

class Dataless
{
	public:
		Dataless(INIT_MAP &init) : init_values(init) {};
		bool SynchronizeDataless(Seiscomp::DataModel::Inventory *inv,
		                         const std::string &dataless);

	private:
		INIT_MAP init_values;
		Inventory *invent;
		bool ParseDataless(const std::string &file);
};
#endif /* DATALESS_H */
