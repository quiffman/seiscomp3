#!/bin/bash
#
# Prototype/demonstrator for making a graph of bytes per day.
#
# Begun by Peter L. Evans, December 2013/January 2014
#
# Input: test4.db SQLite database
# Output: two PNG plots
#
# ----------------------------------------------------------------------
set -u

start_year=2013
start_month=12

echo "SELECT start_day, dcid, total_size FROM ArcStatsSummary as X JOIN ArcStatsSource as Y where X.source = Y.id AND X.start_day > '$start_year-$start_month-00' AND X.start_day < '$start_year-$start_month-99' ORDER BY start_day, source;" \
	| sqlite3 test4.db | sed -e 's/|/  /g' > days.dat

# Normalise GiB,MiB etc to MiB ... Python, awk, or what?
cat > t1.py <<EOF
import sys
for x in sys.stdin.readlines():
        line = x.strip()
        if line.endswith('GiB'):
                words = line.split();
                val = words[-2];
                print '\t'.join(words[0:-2]), '\t', float(val) * 1024.0, 'MiB'
        else:
                print line

EOF
python t1.py < days.dat > days2.dat
rm days.dat

# Rearrange to a table for histogram plotting, and summation by size:
cat > t2.py <<EOF
import sys
dcid_list = ['ETHZ', 'GFZ', 'INGV', 'RESIF']
curday = None
row = {}

def flush_day(day, row):
	print day,
	for dcid in dcid_list:
		if row.has_key(dcid):
			print "%8s" % row[dcid],
		else:
			print "%8d" % -1,
	s = sum(float(row[x]) for x in row.keys())
	print s

print "# DAY    ", " ".join("%8s" % x for x in dcid_list)

for x in sys.stdin.readlines():
	line = x.strip()
	words = line.split()
	day = words[0]
	dcid = words[1]
	val = words[2]

	if day == curday:
		row[dcid] = val
	else:
		# new day, flush and reload
		if curday != None:
			flush_day(curday, row)
		curday = day
		row = {}
		row[dcid] = val
flush_day(curday, row)
EOF
python t2.py < days2.dat > days3.dat
rm days2.dat


gnuplot <<EOF
set terminal dumb
set xdata time
set timefmt "%Y-%m-%d"
set xlabel 'Date'
set xrange ['$start_year-$start_month-01':]
set ylabel 'total_size, MiB'
set logscale y

set key top left
set grid
set style data linespoints

plot 'days3.dat' using 1:6 title 'All EIDA nodes'

set output 'test.png'
set term png medium size 960 
replot
EOF

mv test.png /srv/www/geofon

gnuplot <<EOF
set terminal dumb

set xlabel 'Day in $start_year-$start_month'

set ylabel 'total_size, MiB'
set yrange [0:]

set key top left
set grid 

set style data histograms
set style histogram rowstacked
set boxwidth 0.5 relative
set style fill solid 1.0 border -1

plot '<cut -c9- days3.dat' using 2:xtic(1) title 'ETHZ', \
     '' using 3 title 'GFZ', \
     '' using 4 title 'INGV', \
     '' using 5 title 'RESIF'

set output 'testh.png'
set term png medium size 960 
replot
EOF

mv testh.png /srv/www/geofon
rm t1.py t2.py days3.dat

