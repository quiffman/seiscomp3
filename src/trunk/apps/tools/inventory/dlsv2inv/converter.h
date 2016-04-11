
#ifndef __SEISCOMP_APPLICATIONS_SYNCHRO_H__
#define __SEISCOMP_APPLICATIONS_SYNCHRO_H__

#include <string>
#include <seiscomp3/client/application.h>
#include "define.h"

namespace Seiscomp {
namespace Applications {


class Converter : public Seiscomp::Client::Application {
	public:
		Converter(int argc, char **argv);
		~Converter();


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

