wsiris is a server that provides IRIS webservices from a SeisComP3 database
and an SDS waveform archive. The following services are implemented:

.. csv-table::
   :header: "Service", "Retrieves this...", "In this format"

   "`ws-event <http://www.iris.edu/ws/event>`_", "contributed earthquake origin and magnitude estimates", "`QuakeML <https://quake.ethz.ch/quakeml>`_, `SC3ML <http://geofon/ns/seiscomp3-schema/>`_"
   "`ws-station <http://www.iris.edu/ws/station>`_", "network, station, channel, response metadata", "`StationXML <http://www.data.scec.org/xml/station/>`_, `SC3ML <http://geofon/ns/seiscomp3-schema/>`_"
   "`ws-dataselect <http://www.iris.edu/ws/dataselect>`_", "single channel of time series data in miniSEED format. Use this to pass data to other workflow services", "`miniSEED <http://www.iris.edu/data/miniseed.htm>`_"
   "`ws-bulkdataselect <http://www.iris.edu/ws/bulkdataselect>`_", "multiple channels of time series data", "`miniSEED <http://www.iris.edu/data/miniseed.htm>`_"
   "`ws-availability <http://www.iris.edu/ws/availability>`_", "information about what time series data is available from the DMC", "XML, Query parameters for ws-bulkdataselect, ws-dataselect, or ws-timeseries"
