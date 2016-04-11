scimport is responsible to forward messages from one system to another. The
difference to :ref:`scimex` is that scimport does not handle the messages
event based. scimport supports two different modes. The relay mode does a
simple mapping from GROUP:SYSTEM_A to GROUP:SYSTEM_B. This mode is default.

In case GROUP is not defined in the second system the message is forwarded to
IMPORT_GROUP. The import mode supports custom mapping and filter functionality.
It is possible to forward GROUP1:SYSTEM_A to GROUP2:SYSTEM_B. In addition the
forwarded objects can be filtered by:

Pick
 - Status
 - Phase
 - AgencyID

Amplitude
 - Amplitude

Origin
 - Location
 - Depth
 - AgencyID
 - Status

Event
 - Type

StationMagnitude
 - Type

Magnitude
 - Type


Examples
========

Example scimport.cfg

.. code-block:: sh

   # The address of the importing system
   sink = sinkAddress

   # This option has to be set if the application runs in import mode.
   # The routing table has to be defined in the form of source_group:sink_group
   routingtable = PICK:PICK

   # List of sink groups to subscribe to. If this option is not set the message
   # groups will be determined automatically. If this option is set but not
   # needed for a setup it can be ignored with the option --ignore-groups
   msggroups = GROUP_ONE, GROUP_TWO

   # Available filter options
   # * means any value
   filter.pick.status   = *
   filter.pick.phase    = *
   filter.pick.agencyID = *

   # Values: eq (==), lt (<=) ,gt (>=), *
   filter.stationAmplitude.operator = gt
   filter.stationAmplitude.amplitude = *

   # Values: lat0:lat1 (range)
   filter.origin.latitude = *

   # Values: lon0:lon1 (range)
   filter.origin.longitude = *
   filter.origin.depth     = *
   filter.origin.agencyID  = *

   # Values: automatic, manual
   filter.origin.status = *

   # Values: earthquake, explosion, quarry blast, chemical explosion,
   # nuclear explosion, landslide, debris avalanche, rockslide,
   # mine collapse, volcanic eruption, meteor impact, plane crash,
   # building collapse, sonic boom, other
   filter.event.type = *

   # Values: Whatever your magnitudes are named
   filter.stationMagnitude.type = *

   # Values: Whatever your magnitudes are named
   filter.networkMagnitude.type = *

   # Values: latency, delay, timing quality, gaps interval, gaps length,
   # spikes interval, spikes amplitude, offset, rms
   filter.qc.type = *
