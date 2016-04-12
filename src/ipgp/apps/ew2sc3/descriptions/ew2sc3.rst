Earthworm to SeisComP3 application.

This application connects to an Earthworm export module using IP protocol and
listens for messages thru a specified socket.

Communication
-------------

The communication between Earthworm and SeisComP3 is done by using Earthworm export_generic protocol.
It's a TCP/IP protocol over which text messages are sent by Earthworm.
An hearbeat is sent by both ends of the socket (Earthworm and SeisComP3), at a regular rate.
If for some reason the hearbeat is not received during a configurable amount of time (which should higher than the expected heartbeat rate), then the connection is re-initiated.


Message processing
------------------

The messages sent trough the socket are idenfied by their message ID, their institute ID and their module ID.
All those IDs are configured within the export_generic Earthworm module and Earthworm system.
Ew2sc3 only supports hypo2000_arc messages type from Earthworm. Any other message will lead to unknow behaviour of ew2sc3.

When a message is received, ew2sc3 will first assess whereas it is correct or not according to the IDs.
Then, the hypo2000_arc message is parsed and picks, arrivals, magnitudes and origin are created.
A new origin message with associated magnitudes, arrivals and picks is then sent to SeisComP3 messaging system.

In case the latitude and longitude in the hypo2000_arc message are null (space filled), a default location is used.


Pick uncertainties
------------------

Hypo2000_arc message uses weights for pick quality, 0 being the best picks and 4 the worst.
Those weight are translated into uncertainties for SeisComP3.
A list of uncertainties is configured into ew2sc3 configuration file (pickerUncertainties). Then weights from 0 to maxUncertainty are matched with the minimum and maximum uncertainties.
Finally, a linear formula allows to find the uncertainty corresponding to the assigned weight.


More
----

More informations on the content of Hypo2000_arc message can be found on Earthworm website.
http://www.earthwormcentral.org/documentation/PROGRAMMER/y2k-formats.html

More informations on export_generic configuration can be found on Earthworm website.
http://www.earthwormcentral.org/documentation/cmd/export_cmd.html

And finally, more informations on Earthworm can be found on central Earthworm website.
http://www.earthwormcentral.org/

