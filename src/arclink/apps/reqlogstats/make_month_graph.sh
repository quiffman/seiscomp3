#!/bin/bash
#
# Prototype/demonstrator for making a graph of bytes per day or month.
#
# Begun by Peter L. Evans, December 2013/January 2014
#
# Input: reqlogstats-*.db SQLite database
# Parameters: network code [optional]
# Output: two PNG plots - total, and break-out by source.
#
# ----------------------------------------------------------------------
set -u

progname=`basename $0`
today=`date +%F`

start_year=`date +%Y`
start_month=`date +%m`
img_dir='/srv/www/webdc/eida/data'
db_dir='/home/sysop/reqlogstats/var'

if [ ! -d ${img_dir} ] ; then
    echo "${progname}: Images directory ${img_dir} does not exist. Bye."
    exit 1
fi

if [ ! -d ${db_dir} ] ; then
    echo "${progname}: SQLite DB directory ${db_dir} does not exist. Bye."
    exit 1
fi

function show_usage() {
  echo "Usage: ${progname} [--code {net} ] [ {month} [ {year} [ {db file} ]]]"
  echo
  echo "Create usage images from {db file} for the given date."
  echo "If {net} is not given, all networks are included."
  echo "If {month} or {year} are not given, use today's date."
  echo "If {db file} is not given, use a default."
}

code=
code_constr=""
table=

if [ $# -gt 0 ] ; then
    first=$1
    if [ "$first" == "--code" ] ; then
	code=$2
	code_constr="AND X.networkCode = '${code}'"
	shift 2;
    fi
fi

echo "Restricted to code=${code}; setting constraint: '${code_constr}'"
code2=${code/\//_}

if [ $# -ge 1 ] ; then
    start_month=$1  # Should be a two-digit number
fi
if [ $# -ge 2 ] ; then
    start_year=$2  # Should be a four-digit number
fi
if [ $# -ge 3 ] ; then
    dbfile=$3
else
    dbfile="${db_dir}/reqlogstats-${start_year}.db"
fi
echo "Looking in ${dbfile} for ${start_year} month ${start_month}" 
if [ ! -s ${dbfile} ] ; then
    echo "Error: ${dbfile} not found. Bye"
    exit 1
fi


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

# Rearrange to a table for histogram plotting, and summation by size:
cat > t2.py <<EOF
import sys
dcid_list = ['BGR', 'ETHZ', 'GFZ', 'INGV', 'IPGP', 'LMU', 'ODC', 'RESIF']
curday = None
row = {}

def flush_day(day, row):
	s = sum(float(row[x]) for x in row.keys())
	print "%s %10.1f" % (day, s),
	for dcid in dcid_list:
		if row.has_key(dcid):
			print "%8s" % row[dcid],
		else:
			print "%8d" % 0,
	print

print "# DAY    ", "      TOTAL", " ".join("%8s" % x for x in dcid_list)

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
		if (curday != None):
			flush_day(curday, row)
		curday = day
		row = {}
		row[dcid] = val
if (curday != None):
	flush_day(curday, row)
EOF

if [ -z "${code}" ] ; then
    cmd="SELECT start_day, dcid, total_size FROM ArcStatsSummary as X JOIN ArcStatsSource as Y where X.src = Y.id ${code_constr} AND X.start_day > '$start_year-$start_month-00' AND X.start_day < '$start_year-$start_month-99' ORDER BY start_day, dcid;"
else
    cmd="SELECT start_day, dcid, size/1024.0/1024.0 FROM ArcStatsNetwork as X JOIN ArcStatsSource as Y where X.src = Y.id ${code_constr} AND X.start_day > '$start_year-$start_month-00' AND X.start_day < '$start_year-$start_month-99' ORDER BY start_day, dcid;"
fi

echo ${cmd} \
    | sqlite3 ${dbfile} | sed -e 's/|/  /g' \
    | python t1.py \
    | python t2.py > days3.dat

if [ $(wc -l days3.dat | awk '{print $1}') -le 1 ] ; then
    echo "Nothing in db with '${code_constr}'."
    exit 0
fi

head -1 days3.dat
tail -7 days3.dat

start_month_name=$(date +%B -d "$start_year-$start_month-01")

gnuplot <<EOF
set terminal dumb
set xdata time
set timefmt "%Y-%m-%d"
set xlabel 'Day in $start_month_name $start_year'
set xrange ['$start_year-$start_month-01':]
set xtics 24*3600
set xtics format "%d"
set ylabel 'total_size, MiB'
set logscale y

set key top left
set grid
set style data linespoints

plot 'days3.dat' using 1:2 title 'All EIDA nodes'

set output 'out.svg'
set terminal svg font "arial,16" size 960,480   # even "giant" is not enough font.
replot
EOF

if [ -z "${code}" ] ; then
    outfile="${img_dir}/total-${start_year}-${start_month}.svg"
else
    outfile="${img_dir}/graph-${code2}-total.svg"
fi

if [ -s out.svg ] ; then
    mv out.svg $outfile
    echo "Wrote $outfile"
else
    echo "No output!"
    rm -f out.svg
    exit 0
fi

# ----------------------------------------------------------------------

gnuplot <<EOF
set terminal dumb

set xlabel 'Day in $start_month_name $start_year'

set ylabel 'total_size, MiB'
set yrange [0:]

set key top left
set grid 

set style data histograms
set style histogram rowstacked
set boxwidth 0.5 relative
set style fill solid 1.0 border -1

plot '<cut -c9- days3.dat' using 3:xtic(1) title 'BGR' ls 2, \
     '' using  4 title 'ETHZ' ls 1, \
     '' using  5 title 'GFZ' ls 3, \
     '' using  6 title 'INGV' ls 4, \
     '' using  7 title 'IPGP' ls 6, \
     '' using  8 title 'LMU' ls 7, \
     '' using  9 title 'ODC' ls 9, \
     '' using 10 title 'RESIF' ls 8

set output 'out.svg'
set terminal svg font "arial,16" size 960,480
replot
EOF

if [ -z "${code}" ] ; then
    outfile="${img_dir}/sources-${start_year}-${start_month}.svg"
    txtfile="${img_dir}/total-${start_year}-${start_month}.txt"
else
    outfile="${img_dir}/graph-${code2}-sources.svg"
    txtfile="${img_dir}/graph-${code2}-total.txt"
fi

if [ -s out.svg ] ; then
    mv out.svg $outfile
    echo "Wrote $outfile"
else
    rm -f out.svg
    echo "No SVG output!"
fi

if [ -s days3.dat ] ; then
    mv days3.dat $txtfile
    echo "Wrote $txtfile"
else
    echo "No text file output!"
fi

rm -f t1.py t2.py days3.dat
