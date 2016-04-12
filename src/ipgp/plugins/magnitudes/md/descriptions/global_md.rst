The duration magnitude is based on coda duration measurement.
It's usually valid for small earthquakes up to magnitude 4 to 5.

First used in 1972 by Lee et al., Duration magnitude (Md) or Coda duration
magnitude plugin estimates Richter magnitude of local earthquakes by using
signal duration on vertical components of seismographs.

Estimations are quite stable for local earthquakes ranging from magnitude
Md 0.0 to 5.0.

Processing description
----------------------

Duration magnitude is usually computed on short period seismometers by searching the time at which
the amplitude of the signal is close to pre-earthquake amplitude.

Since it's mainly used for small earthquake whose signal is at rather high frequency, it's usefull to highpass filter broadband seismomters.
A good idea is to use a Wood-Anderson conversion as pre-processing.

The plugin then searches for the maximum amplitude of the signal, which should be the S-wave and then computed mean amplitude of one-second time windows.
As soon as a one-second time window mean amplitude vs pre-earthquake amplitude reaches the configured SNR ratio, the process is stopped.
The middle of the one-second time window is assumed to be the end of the Coda and the time difference between Coda time and P arrival time is stored as Coda duration.
Finally, the generic formula is applied and the duration magnitude is computed for a given station, if it fits the criteria (max depth, max distance).

More information
----------------

Description of the formula can be found in Hypo2000 manual from USGS website.
http://geopubs.wr.usgs.gov/open-file/of02-171/
