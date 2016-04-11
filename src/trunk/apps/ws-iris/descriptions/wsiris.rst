wsiris is a server that provides IRIS Web services from a SeisComP3 database
and an SDS waveform archive. The following services are implemented:

.. csv-table::
   :header: "Service", "Retrieves this...", "In this format"

   "`ws-event <http://www.iris.edu/ws/event>`_", "contributed earthquake origin and magnitude estimates", "`QuakeML <https://quake.ethz.ch/quakeml>`_, `SC3ML <http://geofon/ns/seiscomp3-schema/>`_"
   "`ws-station <http://www.iris.edu/ws/station>`_", "network, station, channel, response metadata", "`StationXML <http://www.data.scec.org/xml/station/>`_, `SC3ML <http://geofon/ns/seiscomp3-schema/>`_"
   "`ws-availability <http://www.iris.edu/ws/availability>`_", "information about what time series data is available from the DMC", "XML, Query parameters for ws-bulkdataselect, ws-dataselect, or ws-timeseries"
   "`ws-dataselect <http://www.iris.edu/ws/dataselect>`_", "single channel of time series data in miniSEED format. Use this to pass data to other workflow services", "`miniSEED <http://www.iris.edu/data/miniseed.htm>`_"
   "`ws-bulkdataselect <http://www.iris.edu/ws/bulkdataselect>`_", "multiple channels of time series data", "`miniSEED <http://www.iris.edu/data/miniseed.htm>`_"

If wsiris is started, it accepts connections by default on port 8080 which
can be changed in the configuration.

Event
-----

* contributed earthquake origin and magnitude estimates
* request type: HTTP-GET
* events may be filtered e.g. by hypocenter, time and magnitude, see http://www.iris.edu/ws/event/

URL
^^^

* http://localhost:8080/event
* http://localhost:8080/event/query

Example
^^^^^^^

* http://localhost:8080/event?starttime=2012-09-12&minlat=30&maxlat=60&minlon=-10&maxlon=30

Feature Notes
^^^^^^^^^^^^^

* SeisComP does not distinguish between catalogs and contributors, but supports
  agencyIDs. Hence, if specified, the value of the catalog or contributor
  parameter is mapped to the agencyID. If both parameters are supplied, the
  value of the contributor is used.
* origin and magnitude filter parameters are always applied to preferred origin
  resp. preferred magnitude
* additional request parameters:

  * ``output: [qml, qml-rt, sc3ml]``, default: ``qml``
  * ``includecomments``: boolean, default: ``true``
  * ``formatted``: boolean, default: ``false``

Station
-------

* network, station, channel, response metadata
* request type: HTTP-GET
* stations may be filtered e.g. by geographic region and time, also the
  information depth level is selectable, see http://www.iris.edu/ws/station/

URL
^^^

* http://localhost:8080/station
* http://localhost:8080/station/query

Example
^^^^^^^

* http://localhost:8080/station?net=GE&level=sta

Feature Notes
^^^^^^^^^^^^^

* ``updatedafter`` request parameter not implemented: The last modification time
  in SeisComP is tracked on the object level. If a child of an object is updated
  the update time is not propagated to all parents. In order to check if a
  station was updated all children must be evaluated recursively. This operation
  would be much to expensive.
* additional request parameters:

  * ``output: [stationxml, sc3ml]``, default: ``stationxml``
  * ``formatted``: boolean, default: ``false``

.. _sec-availability:

Availability
------------

* information about what time series data is available from the DMC
* request type: HTTP-GET
* data may be filtered e.g. by geographic, time and channel information, see
  http://www.iris.edu/ws/availability/
* supports different output formats:

  * ``xml`` – StationXML like
  * ``query`` – HTTP-GET parameter list, used as input for `ref:sec-dataselect`
  * ``bulkdataselect`` – Lines of stream parameters, used for
    `ref:sec-bulkdataselect`

URL
^^^

* http://localhost:8080/availability
* http://localhost:8080/availability/query

Example
^^^^^^^

* http://localhost:8080/availability?start=2010&output=query

Feature Notes
^^^^^^^^^^^^^

* data availability is only based on SDS file names (not miniSEED content),
  hence date precision is limited to days

.. _sec-dataselect:

Data select
-----------

* single channel of time series data in miniSEED format
* request type: HTTP-GET

URL
^^^

* http://localhost:8080/dataselect
* http://localhost:8080/dataselect/query

Example
^^^^^^^

* http://localhost:8080/dataselect?net=AD&sta=DLV&loc=--&cha=BHE&start=2010-11-08T00:00:00&end=2010-11-09T23:59:59

Feature Notes
^^^^^^^^^^^^^

* ``quality`` parameter not implemented (information not available in SeisComP )
* ``ref`` parameter is limited to direct, no ICAB (IRIS Caching Artifact Builder) support

.. _sec-bulkdataselect:

Bulk data select
----------------

* multiple channels of time series data
* request type: HTTP-POST

URL
^^^

* http://localhost:8080/bulkdataselect
* http://localhost:8080/bulkdataselect/query

Example
^^^^^^^

In the following example the command line tool ``curl`` is used to first
retrieve a stream request list from the `ref:sec-availablity` service and then
to post the list to the `ref:sec-bulkdataselect` service.

.. code-block:: sh

   sysop@host:~$ curl -o request.txt "http://localhost:8080/availability?start=2011&output=bulkdataselect&net=ge&sta=M*&cha=BHE"
   sysop@host:~$ curl -o data.mseed --data-urlencode selection@request.txt "http://localhost:8080/bulkdataselect"

Feature Notes
^^^^^^^^^^^^^

* ``quality`` parameter not implemented (information not available in SeisComP )
* ``minimumlength`` parameter is not implemented
* ``longestonly`` parameter is not implemented

