#!/usr/bin/python

import sys

if (len(sys.argv) > 1):
    base=sys.argv[1]
else:
    base='msg'

# Usage: split_mail {basename} < mailfile

months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

count = 0
count_lines = 0

fname = '/dev/null'
fid = open(fname, 'w')

for line in sys.stdin.readlines():
  count_lines += 1
  if line.startswith("From "):
      count += 1

      words = line.split();
      date = "%d %s %02d" % ( int(words[6]), words[3], int(words[4]) )
      print date
      try:
          mm = months.index(words[3])+1
      except ValueError:
          mm = 0
      day = "%02d-%02d" % (mm, int(words[4]))

      fid.close()
      print "Wrote", count_lines, "lines to", fname
      fname = base + "." + day
      fid = open(fname, "w")

  print >>fid, line,

print count, "messages written to %s.nn" % base
