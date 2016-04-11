
#ifndef __SEISCOMP_APPLICATIONS_SYNCHRO_H__
#define __SEISCOMP_APPLICATIONS_SYNCHRO_H__

#include <string>
#include <seiscomp3/client/application.h>
#include "define.h"

namespace Seiscomp {
namespace Applications {

class Synchro : public Seiscomp::Client::Application {
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
		INIT_MAP _init;
};

} // namespace Seiscomp
} // namespace Applications

#endif

