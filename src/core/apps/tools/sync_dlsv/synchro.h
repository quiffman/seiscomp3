
#ifndef __SEISCOMP_APPLICATIONS_SYNCHRO_H__
#define __SEISCOMP_APPLICATIONS_SYNCHRO_H__

#include <string>
#include <seiscomp3/client/application.h>
#include "define.h"

namespace Seiscomp {
namespace Applications {

class SynchroCallbacks {
	public:
		virtual ~SynchroCallbacks() {}
		virtual bool syncCallback()=0;
};

class Synchro : public Seiscomp::Client::Application, public SynchroCallbacks {
	public:
		Synchro(int argc, char **argv);
		~Synchro();


	protected:
		void createCommandLineDescription();
		bool initConfiguration();
		bool validateParameters();

		bool run();

	private:
		std::string _dcid;
		std::string _syncID;
		INIT_MAP _init;
		bool syncCallback();
};

} // namespace Seiscomp
} // namespace Applications

#endif

