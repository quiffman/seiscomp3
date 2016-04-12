#
# Populate an SQLite database from Arclink webreqlog statistics HTML pages. 
#
import glob
import os
import sqlite3
import xml.etree.ElementTree as ET
import xml

DEBUG = False

# ----------- SQLite stuff ---------

def new_table(con):
    """Establish new SQLite tables."""
    with con:
        cur = con.cursor()
        linef = ""
        with open('reqlogstats.sql', 'r') as fid:
            for line in fid.readlines():
                line1 = line.strip()
                if line1.startswith('/*') or line1.startswith('*'):
                    continue
                linef += ' ' + line1
                if line1.endswith(')') or line1.endswith(';'):
                    #print linef
                    cur.execute(linef)
                    linef = ""


# ---------- HTML parsing/scraping stuff --------
def parse_table(table):
    """Parse <table> and return data as list of tuples.

    table - ET.Element

    """
    if table is None:
        return

    # First/any row:
    tr = table.find("thead").find("tr")
    headers = []
    for e in tr:
        if e.tag == "th":
            headers.append(e.text)
    result = [tuple(headers)]
    
    tbody = table.find("tbody")
    for tr in tbody:
        rowdata = []
        for td in tr:
            if td.text is None:
                link = td.find("a")
                if link is not None:
                    content = link.text
                else:
                    content = "??"
            else:
                if td.attrib.has_key("sorttable_customkey"):
                    content = td.attrib["sorttable_customkey"]
                else:
                    content = td.text
            rowdata.append(str(content).strip())
            #print '|'.join([td.tag, str(td.attrib), str(content).strip(), str(td.tail)])
        result.append(tuple(rowdata))
    return result

def parse_intro_text(text, attribs):
    """Handles all text between <h2>Arclink...Report</h2> and the first <table>."""
    
    for line in text.splitlines():
        parts = line.split(":", 1)
        if len(parts) > 1:
            tag = parts[0].strip().replace(" ", "")
            rest = parts[1].strip()
            if tag == "Start" or tag == "End":
                rest = " ".join(rest.split(" ", 2)[0:2])
            attribs[tag] = rest
            
def parse_html(data):
    """Extract everything of value from a report.

    data - a strung, not a file!

    result - a list of table dumps, plus any extra non-table stuff as a dictionary.
    
    """
    attribs = {}
    result = [attribs]
    
    parser = ET.XMLParser(html=1)
    try:
        parser.feed(data)
    except (xml.parsers.expat.ExpatError, xml.etree.ElementTree.ParseError) as e:
	print "Parser ExpatError while reading the file."
	print e
	return None

    except:  ###  ParseError as e:
        print "Error while reading the file."
        raise
    elem = parser.close()
    body = elem.find("body")
    if body is None:
        print "Got no body!"
        
    parent = body
    table = parent.find("table")
    if table is None:
        bodydiv = parent.find("div")
        print "bodydiv:",  bodydiv, bodydiv.attrib
        #for e in bodydiv:
        #    print '('+e.tag+')'
        table = bodydiv.find("table")

        if table is None:
            prediv = bodydiv.find("pre")
            table = prediv.find("table")
            parent = prediv

    heading = parent.find("h2")
    if heading is not None:
        # Preamble contains mark-up. This would be too easy:
        #preamble = heading.tail

        # Instead slurp all until first table:
        # What's the right way to do this?
        for t in parent:
            if t.tag == "table":
                break
            if t.tag == "h2":
                preamble = t.tail
            else:
                if t.tag == "a":
                    preamble += t.text + t.tail
                else:
                    preamble += ET.tostring(t)  # Includes t's tail. + t.tail

        if (DEBUG):
            print 50*"="
            print preamble
            print 50*"="
        
        parse_intro_text(preamble, attribs)
        print "Preamble: got", attribs

    verbose = False
    things = parent.findall("table")
    print "Num tables:", len(things)
    for table in things:
	if (verbose):
	    print table.tag, table.attrib,
        stuff = parse_table(table)
        if (verbose):
		print '\t|'.join(stuff[0])
        #print '\t+'.join(len(headers)*['-------'])
        #for row in stuff:
        #    print '\t|'.join(row)

        result.append(stuff)
    return result

# ---------- Connecting stuff -----------
class ReportData(object):
    """Holds the contents of a report, but not its key."""
    
    summary = {}
    user = {}
    request = {}
    volume = {}
    station = {}  # Not included in current reports, Dec 2013?
    network = {}
    messages = {}
    clientIP = {}
    userIP = {}
    
    def __init__(self, text):
        content = parse_html(text)
	if content == None:
		return  ## an empty object

        attribs = content[0]
        tables = content[1:]

        self.summary = {'requests': attribs['Requests'],
                   'request_with_errors': attribs['withErrors'],
                   'error_count': attribs['ErrorCount'],
                   'users': attribs['Users'],
                   'lines': attribs['TotalLines'],
                   'size': attribs['TotalSize'],
                   'start': attribs['Start'],
                   'end': attribs['End']
                   }
        if attribs.has_key("Stations"):
            self.summary["stations"] = attribs["Stations"]

        table = tables[0]
        assert table[0][0] == "User"
        self.user = table

        for table in tables:
            if table[0][0] == "Station":
                self.station = table

            if table[0][0] == "Network":
                self.network = table

            if table[0][0] == "Request Type":
                self.request = table
        
                # Three of these, type = { WAVEFORM | ROUTING | INVENTORY }
                #request = {'type': t,
                #    'requests': r,
                #    'lines': l,
                #    'errors': e,
                #    'size': s
                #}

            if table[0][0] == "Volume":
                self.volume = table
                #volume = {'type': t,
                #    'requests': r,
                #    'lines': l,
                #    'errors': e,
                #    'size': s
                #    }

            if table[0][1] == "Message":
                self.messages = table

            if table[0][0] == "UserIP":
                self.userIP = table

            if table[0][0] == "ClientIP":
                self.clientIP = table

        
                
def lookup_source(con, host, port, dcid):
    """Return a source id - either an existing one, or a new one.

        con - Database connection cursor?
        host - varchar
        port - int
        dcid - varchar

    Returns an int, index into db tables.
    
    """

    constraints = " WHERE " + " AND ".join(["host = '%s'" % host,
                                               "port = %i" % port,
                                               "dcid = '%s'" % dcid])

    cursor = con.cursor()
    q = "SELECT COUNT(*) FROM `ArcStatsSource`" + constraints
    #print "SQLITE: %s" % q
    cursor.execute(q)
    result = cursor.fetchall()  # Not present: get [(0,)]
    found = (result[0][0] != 0)
    #print result, 'found=', found
    if found == False:
        q = "INSERT INTO ArcStatsSource VALUES (NULL, ?, ?, ?, 'sometext')"
        v = (host, port, dcid)
        print "SQLITE: %s" % q
        cursor.execute(q, v)
        result = cursor.fetchall()
        con.commit()
        
    q = "SELECT id FROM `ArcStatsSource`" + constraints
    #print "SQLITE: %s" % q
    cursor.execute(q)
    result = cursor.fetchall()
    print result
    assert len(result) == 1

    cursor.close()
    
    return int(result[0][0])


def insert_summary(con, k, summary):
    cursor = con.cursor()
    try:
        r = summary['requests']
        rwe = summary['request_with_errors']
        e = summary['error_count']
        u = summary['users']
        tl = summary['lines']
        ts = summary['size']

        if summary.has_key('stations'):
            st = summary['stations']
            q = "INSERT INTO ArcStatsSummary VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
            v = (k[0], k[1], r, rwe, e, u, st, tl, ts)
        else:
            q = "INSERT INTO ArcStatsSummary VALUES (?, ?, ?, ?, ?, ?, NULL, ?, ?)"
            v = (k[0], k[1], r, rwe, e, u, tl, ts)

    except KeyError:
        print "Couldn't find some needed value(s)"
        for k, v in summary.iteritems():
            print k,':', v
        return

    cursor.execute(q, v)
    con.commit()

def insert_user(con, k, user):
    cursor = con.cursor()
    q = "INSERT INTO ArcStatsUser VALUES (?, ?, ?, ?, ?, ?, ?)"
    heads = user[0]
    print "User table: got", "|".join(heads)

    for row in user[1:]:
        items = row[0:5]  # User  Requests  Lines  Nodata/Errors  Size
        v = (k[0], k[1], items[0], items[1], items[2], items[3], items[4])
        cursor.execute(q, v)
    con.commit()

def insert_request(con, k, table):
    cursor = con.cursor()
    q = "INSERT INTO ArcStatsRequest VALUES (?, ?, ?, ?, ?, NULL, ?, NULL)"
    heads = table[0]
    print "Request Type table: got", "|".join(heads)
    for row in table[1:]:
        items = row[0:4]  # Request Type    Requests    Lines    Nodata/Errors
        v = (k[0], k[1], items[0], items[1], items[2], items[3])
        cursor.execute(q, v)
    con.commit()

def insert_volume(con, k, table):
    cursor = con.cursor()
    q = "INSERT INTO ArcStatsVolume VALUES (?, ?, ?, ?, NULL, ?, ?)"
    heads = table[0]
    print "Volume table: got", "|".join(heads)

    for row in table[1:]:
        items = row[0:4]  # Volume    Count    Nodata/Errors    Size
        v = (k[0], k[1], items[0], items[1], items[2], items[3])
        cursor.execute(q, v)
    con.commit()

def insert_station(con, k, table):
    heads = table[0]
    print "Station table: got", "|".join(heads), ' ...IGNORED'


def insert_network(con, k, table):
    cursor = con.cursor()
    q = "INSERT INTO ArcStatsNetwork VALUES (?, ?, ?, ?, ?, ?, ?, ?, NULL)"
    heads = table[0]
    print "Network table: got", "|".join(heads)

    merged_errors_nodata = False;
    if heads[3] == "Errors/Nodata":
        # Old-style report from before 2013-12-03 or so,
        # store this column's data under 'Errors' in the db.
        merged_errors_nodata = True;

    for row in table[1:]:
        if len(row) == 6:
            items = row[0:6]  # Network    Requests    Lines    Nodata    Errors    Size
        elif (len(row) == 5) and merged_errors_nodata:
            items = [row[0], row[1], row[2], 0, row[3], row[4]]
        else:
            print "Funny row, skipping it:", row
            continue

        v = (k[0], k[1], items[0], items[1], items[2], items[3], items[4], items[5])
        cursor.execute(q, v)
    con.commit()

def insert_messages(con, k, table):
    cursor = con.cursor()
    q = "INSERT INTO ArcStatsMessages VALUES (?, ?, ?, ?)"
    heads = table[0]
    print "Messages table: got", "|".join(heads)

    for row in table[1:]:
        items = row[0:2]  # Count    Message
        v = (k[0], k[1], items[1], items[0])
    cursor.execute(q, v)
    con.commit()

def insert_clientIP(con, k, table):
    cursor = con.cursor()
    heads = table[0]
    print "ClientIP table: got", "|".join(heads)
    q = "INSERT INTO ArcStatsClientIP VALUES (?, ?, ?, ?, ?, ?, NULL)"

    for row in table[1:]:
        items = row[0:4]  # ClientIP    Requests    Lines    Nodata/Errors
        v = (k[0], k[1], items[0], items[1], items[2], items[3])
        cursor.execute(q, v)
    con.commit()

def insert_userIP(con, k, table):
    cursor = con.cursor()
    heads = table[0]
    print "UserIP table: got", "|".join(heads)
    q = "INSERT INTO ArcStatsUserIP VALUES (?, ?, ?, ?, ?, ?, NULL)"

    for row in table[1:]:
        items = row[0:4]  # UserIP    Requests    Lines    Nodata/Errors
        v = (k[0], k[1], items[0], items[1], items[2], items[3])
        cursor.execute(q, v)
    con.commit()

def insert_data(db, rd, host, port, dcid, start_day):
    """Insert the information found in a report into a database.

    db - database object to insert into
    rd - ReportData object

    """
    con = sqlite3.connect(db)
    source_id = lookup_source(con, host, port, dcid)
    k = (start_day, source_id)  ## SOME_KEY_IDENTIFYING_A_REPORT
    insert_summary(con, k, rd.summary)
    insert_user(con, k, rd.user)
    insert_request(con, k, rd.request)
    insert_volume(con, k, rd.volume)
    if rd.station: insert_station(con, k, rd.station)
    if rd.network: insert_network(con, k, rd.network)
    if len(rd.messages) > 0:
        insert_messages(con, k, rd.messages)
    insert_clientIP(con, k, rd.clientIP)
    insert_userIP(con, k, rd.userIP)
    con.close()
    
def summary_data(db):
    """Quick overview of what's in a database.

    db - string, file name of the SQLite database to examine.

    """
    summary = {}

    tables = ['Source', 'Messages', 'Report', 'Request', 'Network', 'Station', 'Summary', 'User', 'UserIP', 'Volume']
    con = sqlite3.connect(db)
    cur = con.cursor()

    for tableName in tables:
        cur.execute("SELECT COUNT(*) FROM `ArcStats%s`;" % tableName)
        result = cur.fetchall()
        summary[tableName] = result[0][0]

    con.close()
    return summary

def process_file(f, host, port, dcid):

    text = open(f).read().replace("<hr>", "<hr/>")
    rd = ReportData(text)

    if len(rd.summary) == 0:
	print "%s: empty summary" % (f)
	return

    print rd.summary

    start_day = rd.summary['start'].split(' ', 1)[0]
    insert_data(db, rd, host, port, dcid, start_day)

   
db = 'test4.db'
con = sqlite3.connect(db)
# Only works if the tables exist:
#print summary_data(db)

print "Wiping old tables."
new_table(con)

# Now look at the report file(s),
# and import their contents into our db.
if (DEBUG):
    count = 0
    for line in con.iterdump():
        count += 1
        print 'DUMP %03i:' % count, line


# Given an input file, insert it:
#myfile=os.path.join('eida_stats', 'sample-resif.html')
#filelist = glob.glob("./eida_stats/sample-[f-z]*.html")
filelist = [];
filelist.extend(glob.glob("./eida_stats/[e-z]*.[0-9][0-9]-[0-9][0-9].html"))
filelist.extend(glob.glob("./eida_stats/gfz.seq00[0-9][0-9].html"))

for myfile in filelist:
    name = os.path.basename(myfile)
    host = name.split('.', 1)[0]
    port = -1
    dcid = host.upper()  ## 'SOMEDCID'

    print
    print 70*'-'
    print "Processing %s : %s (%s:%i)" % (myfile, dcid, host, port)
    
    process_file(myfile, host, port, dcid)
    
print 70*'-'
print "Done with %i file(s)." % len(filelist)
summary = summary_data(db)
print summary
print "Database %s contains  %i source(s), and %i day summaries." % (db, summary['Source'], summary['Summary'])
