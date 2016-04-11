* Generated at #date# - Do not edit!
* template: #template#

[fs_mseed]

input_type = ddb
data_format = mseed
location = #pkgroot#/indata
station = #statid#

* Stations to process, separated by a comma. Default is all stations.
*station_list=

* "pattern" is a POSIX extended regular expression that must match
* input file names (useful for filtering out non-data files).  For
* example "BH[NEZ]" would match any files that contained "BHE",
* "BHN" or "BHZ".  If no pattern is specified all files will be
* processed.
*pattern = BH[NEZ]

* Look for data files at the 1st or 2nd directory level
scan_level = 2

* Move file to subdirectory "processed" before starting to read it
move_files = yes

* Delete processed files
delete_files = no

* Look only for files that are newer than the last file processed
use_timestamp = no

* Timestamp file is used to save the modification time of the last file
* processed
timestamp_file = #pkgroot#/status/fs_mseed.tim

* New files are searched for every "polltime" seconds
polltime = 10

* Wait until the file is at least 30 seconds old, before trying to read it
delay = 30

* "verbosity" tells how many debugging messages are printed
* verbosity = 1

* Maximum number of consecutive zeros in datastream before data gap will be
* declared (-1 = disabled)
* zero_sample_limit = 10

* If timing quality is not available, use this value as default
* (-1 = disabled)
default_timing_quality = -1

* Channel definitions (Mini-SEED streams are defined in streams.xml,
* look for <proc name="generic_3x50">)

channel Z source_id = "BHZ"
channel N source_id = "BHN"
channel E source_id = "BHE"

