/***************************************************************************
 *   Copyright (C) by GFZ Potsdam                                          *
 *                                                                         *
 *   You can redistribute and/or modify this program under the             *
 *   terms of the SeisComP Public License.                                 *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   SeisComP Public License for more details.                             *
 ***************************************************************************/


#ifndef __SEISCOMP_QC_QCLATENCY_H__
#define __SEISCOMP_QC_QCLATENCY_H__

#include "qcplugin.h"


namespace Seiscomp {
namespace Applications {
namespace Qc {


DEFINE_SMARTPOINTER(QcPluginLatency);

class QcPluginLatency : public QcPlugin {
    DECLARE_SC_CLASS(QcPluginLatency);

public:
    QcPluginLatency();
    std::string registeredName() const;
    std::vector<std::string> parameterNames() const;
	
private:
    Core::Time _lastArrivalTime;
    void timeoutTask();
};



}
}
}
#endif
