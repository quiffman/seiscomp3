
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
		std::string _net_description;
		std::string _net_type;
		std::string _net_start_str;
		std::string _net_end_str;
		Core::Time _net_start;
		Core::Time _net_end;
		std::string _syncID;
		bool syncCallback();
};

} // namespace Seiscomp
} // namespace Applications

#endif

