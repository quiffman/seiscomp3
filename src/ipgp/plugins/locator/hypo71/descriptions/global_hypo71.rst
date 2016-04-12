Hypo71 locator plugin which provides a way to use HYPO71 program to locate events in SeisComP3.

It uses a slightly modified Hypo71 version from Alexandre Nercessian (IPGP) which allows negative earthquake depth (above sea level)
and negative stations altitude (below sea level - OBS).


How it works
------------

When reveiving a list of arrivals to locate, the plugin builds a Hypo71 input file with informations from the station inventory and configured profile.
It then runs Hypo71, reads the output file and sends the results (location, uncertainties, RMS, pick residuals ...) to SeisComP3.

If several trial depths are configured, the plugin will runs as many Hypo71 as depths are configured.
Then all the results are read, and a decision is made on the best one, based on location RMS and uncertainty.
A final run is then made with the best result depth as trial depth.


Profiles
--------

The plugin allows the user to set up as many profiles as needed.
A profile contains all the information relative to the velocity model and Hypo71 iteration parameters.

This allows the user to tune the behaviour of Hypo71 to what he needs.
If no profiles are set-up, the plugin will use default Hypo71 profile, according to example shown in Hypo71 first publication.
Some of this default Hypo71 parameters have been altered to allow for more and finer iteration, since computer power is now far above what was available in the 1970's.


More informations
-----------------

Hypo71PC original manual and binary are available on JC Lahr website.
http://jclahr.com/science/software/hypo71/ 


