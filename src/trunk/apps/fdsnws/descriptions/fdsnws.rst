fdsnws is a server that provides
`FDSN Web Services <http://www.fdsn.org/webservices>`_ from a SeisComP3 database
and record source. The following services are available:

.. csv-table::
   :header: "Service", "Retrieves this...", "In this format"

   "**fdsnws-dataselect**", "time series data in miniSEED format", "`miniSEED <http://www.iris.edu/data/miniseed.htm>`_"
   "**fdsnws-station**", "network, station, channel, response metadata", "`FDSN Station XML <http://www.fdsn.org/xml/station/>`_, `StationXML <http://www.data.scec.org/station/xml.html>`_, `SC3ML <http://geofon.gfz-potsdam.de/ns/seiscomp3-schema/>`_"
   "**fdsnws-event**", "contributed earthquake origin and magnitude estimates", "`QuakeML <https://quake.ethz.ch/quakeml>`_, `SC3ML <http://geofon.gfz-potsdam.de/ns/seiscomp3-schema/>`_"

If ``fdsnws`` is started, it accepts connections by default on port 8080 which
can be changed in the configuration. Also please read :ref:`sec-port` for
running the services on a privileged port, e.g. port 80 as requested by the
specification.


Data select
-----------

* time series data in miniSEED format
* request type: HTTP-GET, HTTP-POST

URL
^^^

* http://localhost:8080/fdsnws/dataselect/1/query
* http://localhost:8080/fdsnws/dataselect/1/queryauth
* http://localhost:8080/fdsnws/dataselect/1/version
* http://localhost:8080/fdsnws/dataselect/1/application.wadl

Example
^^^^^^^

http://localhost:8080/fdsnws/dataselect/1/query?net=GE&sta=BKNI&cha=BH?&start=2013-04-11T00:00:00&end=2013-04-11T12:00:00

To summit POST request the command line tool ``wget`` in combination with the
parameter ``post-data`` or ``post-file`` may be used.

.. code-block:: sh

   sysop@host:~$ wget --post-file=request.txt "http://localhost:8080/fdsnws/dataselect/1/query" -O output.mseed

Feature Notes
^^^^^^^^^^^^^

* ``quality`` parameter not implemented (information not available in SeisComP)
* ``minimumlength`` parameter is not implemented
* ``longestonly`` parameter is not implemented
* access to restricted networks and stations is only granted through the
  ``queryauth`` method
* additional request parameters:

  * ``nodata: [204, 404]``, default: ``204``, HTTP-GET only, defines the HTTP
    error code the server should respond with if no data was found

Station
-------

* network, station, channel, response metadata
* request type: HTTP-GET
* stations may be filtered e.g. by geographic region and time, also the
  information depth level is selectable

URL
^^^

* http://localhost:8080/fdsnws/station/1/query
* http://localhost:8080/fdsnws/station/1/version
* http://localhost:8080/fdsnws/station/1/application.wadl

Example
^^^^^^^

* http://localhost:8080/fdsnws/station/1/query?net=GE&level=sta

Feature Notes
^^^^^^^^^^^^^

* ``updatedafter`` request parameter not implemented: The last modification time
  in SeisComP is tracked on the object level. If a child of an object is updated
  the update time is not propagated to all parents. In order to check if a
  station was updated all children must be evaluated recursively. This operation
  would be much too expensive.
* to enable FDSNXML or StationXML support the plugins ``fdsnxml`` resp.
  ``staxml`` have to be loaded
* additional request parameters:

  * ``format: [fdsnxml, stationxml, sc3ml]``, default: ``fdsnxml``
  * ``formatted``: boolean, default: ``false``
  * ``nodata: [204, 404]``, default: ``204``, defines the HTTP error code the
    server should respond with if no data was found

Event
-----

* contributed earthquake origin and magnitude estimates
* request type: HTTP-GET
* events may be filtered e.g. by hypocenter, time and magnitude


URL
^^^

* http://localhost:8080/fdsnws/event/1/query
* http://localhost:8080/fdsnws/event/1/catalogs
* http://localhost:8080/fdsnws/event/1/contributors
* http://localhost:8080/fdsnws/event/1/version
* http://localhost:8080/fdsnws/event/1/application.wadl

Example
^^^^^^^

* http://localhost:8080/fdsnws/event/1/query?start=2013-04-18&lat=55&lon=11&maxradius=10

Feature Notes
^^^^^^^^^^^^^

* ``catalog`` request parameter is not implemented (information not available in
  SeisComP)
* ``contributor`` request parameter is mapped to the ``agencyID``, the file
  ``@DATADIR@/share/fdsn/contributors.xml`` has to be filled manually with all
  available agency ids
* additional request parameters:

  * ``format: [qml, qml-rt, sc3ml, csv]``, default: ``qml``
  * ``includecomments``: boolean, default: ``true``
  * ``includepicks``: boolean, default: ``false``, works only in combination
    with ``includearrivals`` set to ``true``
  * ``formatted``: boolean, default: ``false``
  * ``nodata: [204, 404]``, default: ``204``, defines the HTTP error code the
    server should respond with if no data was found


.. _sec-port:

Changing the service port
-------------------------

The FDSN Web service specification defines that the Service SHOULD be available
under port 80. Typically SeisComP3 runs under a user without root permissions
and therefore is not allowed to bind to privileged ports (<1024).
To serve on port 80 you may for instance

* run SeisComP3 with root privileged (not recommended)
* use a proxy Webserver, e.g. Apache with
  `mod-proxy <http://httpd.apache.org/docs/2.2/mod/mod_proxy.html>`_ module
* configure and use :ref:`sec-authbind`
* setup :ref:`sec-firewall` redirect rules


.. _sec-authbind:

Authbind
^^^^^^^^

``authbind`` allows a program which does not or should not run as root to bind
to low-numbered ports in a controlled way. Please refer to ``man authbind`` for
program descriptions. The following lines show how to install and setup authbind
for the user ``sysop`` under the Ubuntu OS.

.. code-block:: sh

   sysop@host:~$ sudo apt-get install authbind
   sysop@host:~$ sudo touch /etc/authbind/byport/80
   sysop@host:~$ sudo chown sysop /etc/authbind/byport/80
   sysop@host:~$ sudo chmod 500 /etc/authbind/byport/80

Once ``authbind`` is configured correctly the FDSN Web services may be started
as follows:

.. code-block:: sh

   sysop@host:~$ authbind --deep seiscomp exec fdsnws

In order use ``authbind`` when starting ``fdsnws`` as SeisComP service the last
line in the ``~/seiscomp3/etc/init/fdsnws.py`` have to be commented in.


.. _sec-firewall:

Firewall
^^^^^^^^

All major Linux distributions ship with their own firewall implementations which
are front-ends for the ``iptables`` kernel functions. The following line
temporary adds a firewall rule which redirects all incoming traffic on port 8080
to port 80.

.. code-block:: sh

   sysop@host:~$ sudo iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to 8080

Please refer to the documentation of your particular firewall solution on how to
set up this rule permanently.


