#############################################################################
# sds.py                                                                    #
#                                                                           #
# ArcLink archive structure handler                                         #
#                                                                           #
# (c) 2004 Joachim Saul, GFZ Potsdam                                        #
# (c) 2006 Doreen Pahlke, GFZ Potsdam                                       #
#                                                                           #
# This program is free software; you can redistribute it and/or modify it   #
# under the terms of the GNU General Public License as published by the     #
# Free Software Foundation; either version 2, or (at your option) any later #
# version. For more information, see http://www.gnu.org/                    #
#                                                                           #
#############################################################################

import os, time, datetime, subprocess
from seiscomp import logs
from seiscomp import mseedlite as mseed
from optparse import OptionParser

MOUNT_WRAPPER = "/home/sysop/seiscomp/arclink/bin/isomount.sh"
LOCK_WAIT  = 1
LOCK_RETRY = 10

####################### local functions ##########################
def _get_timespan(fname):
	"""
	Returns the start and end time of the given Mini SEED file.
	
	@arguments: fname,   a string giving the file name
	@return: a tuple containing the start time and the end time
			raises IOError otherwise
	"""
	fh = file(fname)
	try:
		rec = mseed.Record(fh)
		begtime = rec.begin_time
		fh.seek(-rec.size,2)
		rec = mseed.Record(fh)
		endtime = rec.end_time
	finally:
		fh.close()
		
	return begtime, endtime

	
def _findtime(fname, ftime, kind=0):
	"""
	Searches within the given file for the record which contains the specified time.

	@arguments: fname,   a string giving the file name
				ftime,   a datetime object defining the time to be searched for
				kind,    a flag specifying the kind of offset (0 = begoffset, 1 = endoffset)
	@return: an integer storing the offset of the record which contains the specified time
			raises IOError in case of file access problems
			raises LookupError in case of time mismatch
			raises OSError in case of system access problems
	"""
	begtime, endtime = _get_timespan(fname)

	if ftime < begtime or ftime > endtime:
		raise LookupError

	fsize = os.path.getsize(fname)
	fh = file(fname)
	rec = mseed.Record(fh)
	bufsize = rec.size
	start = half = 0
	end = int(fsize/bufsize)

	while end-start > 1:
		half = start + (end-start)/2
		fh.seek(half*bufsize)
		rec = mseed.Record(fh)
		if ftime > rec.end_time:
			start = half
		if ftime < rec.begin_time:
			end = half
		if rec.begin_time <= ftime <= rec.end_time:
			logs.debug("found: %s <= %s <= %s" % (str(rec.begin_time),str(ftime),str(rec.end_time)))
			if kind or rec.end_time == ftime:
				logs.debug("+1")
				half += 1
			break
		
	### special case: for the first record only ###
	if half == end == 1 and ftime < rec.begin_time:
		logs.debug("special case!")
		fh.seek(0)
		rec = mseed.Record(fh)
		if rec.begin_time <= ftime <= rec.end_time:
			half = 0
			if kind:
				half = 1
	### if ftime is located in a gap ###
	if half == start and ftime > rec.end_time:
		half += 1

	fh.close()
	return half*bufsize
##################################################################


class DataSource(object):
	"""
	This class encapsulates the archive organisation.
	"""

	def __init__(self, nrtdir, archdir, isodir, mntdir):
		"""
		Constructor of class DataSource.

		@instance vars: nrtdir,  a string specifying the path to the at most 40 days old near real time data
						archdir, a string storing the path to the SDS archive of current year
						isodir,  a string defining the path to the ISO image archive
						mntdir,  a string specifying the path to the mountable symlinks
		"""
		self.nrtdir = nrtdir
		self.archdir = archdir
		self.isodir = isodir
		self.mntdir = mntdir

		
	def __get_iso_name(self, year, net, sta):
		"""
		Constructs a file name corresponding to the archive name convention.

		@arguments: year, a four digit year
					net,  a string defining the network code
					sta,  a string defining the station code
		@return: a string storing the path to the ISO archive file if exists
				None otherwise
		"""
		isoname = os.path.join(self.isodir,year,net,"%s.%s.%s.%s" % (sta,net,year,"iso"))
		if os.path.exists(isoname):
			return isoname

		return None


	def __get_iso_root(self, isoname, retry=0):
		"""
		Returns the path to the SeisComP data structure respectively the mount point of the ISO image.

		@arguments: isoname, a string specifying the absolute ISO image file name
					retry,   an integer giving the previous lock trials
		@return: a string, giving the path to the SeisComP data structure - mount point resp. - if it exists
				None otherwise
		"""       
		command = "%s -f %s -m" % (MOUNT_WRAPPER,isoname)
		try:
			p = subprocess.Popen(command,shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,close_fds=True)
			(child_out,child_err) = p.communicate()
			ex_code = p.wait()
		except OSError, e:
			raise ArclinkError("Executing the mount wrapper failed: %s!" % str(e))

		if ex_code == 255:
			if retry < LOCK_RETRY:
				time.sleep(LOCK_WAIT)
				return self.__get_iso_root(isoname,retry+1)
			else:
				logs.warning("Mount process is still locked after %d trials: %s" % (LOCK_RETRY,child_err))
				return None
		elif ex_code != 0:
			logs.error("Locking the mount process failed: %s" % child_err)          
			return None

		return child_out


	def get_sds_path(self, fname):
		"""
		Returns the full path to the Mini SEED file in the corresponding archive structure.
		
		@arguments: fname, a string defining the SDS path name
		@return: a string storing the full path to the Mini SEED file
				None if the path doesn't exist
		"""
		nrtname = os.path.join(self.nrtdir,fname)
		if os.path.exists(nrtname):
			return nrtname
		
		archname = os.path.join(self.archdir,fname)
		if os.path.exists(archname):
			return archname
		
		fname_split = fname.split("/")
		isoname = self.__get_iso_name(fname_split[0],fname_split[1],fname_split[2])
		if isoname:
			isoroot = self.__get_iso_root(isoname)
			if isoroot:
				seedfile = os.path.join(isoroot,fname)
				if os.path.exists(seedfile):
					return seedfile
				else:
					self.free_sds_path(seedfile)
	
		return None        
		

	def free_sds_path(self, fname, retry=0):
		"""
		Frees the system resource corresponding to the given file name.

		@arguments: fname, a string defining the full SDS path to a SEED file
					retry, an integer giving the previous lock trials
		"""
		if fname and not fname.startswith(self.archdir) and not fname.startswith(self.nrtdir):            
			fname_split = os.path.basename(fname).split(".")
			isoname = self.__get_iso_name(fname_split[-2],fname_split[0],fname_split[1])
			if isoname:
				command = "%s -f %s -u" % (MOUNT_WRAPPER,isoname)
				try:
					p = subprocess.Popen(command,shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,close_fds=True)
					(child_out,child_err) = p.communicate()
					ex_code = p.wait()
				except OSError, e:
					raise ArclinkError("Mount wrapper is not executable: %s!" % str(e))                    
				
				if ex_code == 255:
					if retry < LOCK_RETRY:
						time.sleep(LOCK_WAIT)
						self.free_sds_path(fname,retry+1)
					else:
						logs.warning("Umount process ist still locked: %s" % child_err)
				elif ex_code != 0:
					logs.error("Locking the umount process failed: %s" % child_err)
			else:
				logs.error("File %s could not be found. Freeing of corresponding mounted ressource fails!" % fname)
		


class SDS(DataSource):
	"""
	This class implements the access to SEED data using the SeisComP Directory Structure.
	
	"""

	def __init__(self, nrtdir, archdir, isodir, mntdir):
		"""
		Constructor of class SDS derived from class DataSource.

		@instance vars: dtmax,   the maximal amount of delta time
		"""        
		DataSource.__init__(self,nrtdir,archdir,isodir,mntdir)
		self.dtmax   = datetime.timedelta(hours=24)

	def __get_sds_name(self, timeobj, net, sta, cha, loc):
		"""
		Constructs a SDS conform file name from the given parameters.

		@arguments: timeobj, a datetime object
					net,     a string defining the network code
					sta,     a string defining the station code
					cha,     a string defining the channel code
					loc,     a string defining the location ID
		@return: a string, giving the path to the SeisComP data structure;
				the path doesn't need to exist
		"""
		postfix = time.strftime("%Y.%j", timeobj.timetuple())
		name = "%d/%s/%s/%s.D/%s.%s.%s.%s.D.%s" % (timeobj.year, net, sta, cha, net, sta, loc, cha, postfix)

		return name
	
			
	def getwin(self, time1, time2, net, sta, cha, loc):
		"""
		Returns the Mini SEED data records according to the given parameters.
		@arguments: time1, a datetime object specifying the start of time window
					time2, a datetime object specifying the end of time window
					net,   a string defining the network code
					sta,   a string defining the station code
					cha,   a string defining the channel code
					loc,   a string defining the location ID
		@return: Mini SEED data records according to the given parameters
				None otherwise
				raises AssertionError in case of requesting data for more than 24 hours
				raises IOError in case of file access problems
				raises OSError in case of system access problems
		"""
		logs.debug("----------------------- \n -->SDS::getwin: t1: %s  t2: %s" % (str(time1), str(time2)))

		assert time1 <= time2 and time2-time1 <= self.dtmax
		data = None
		free = 0     # flag: 0 = fname1 must be freed, 1 = free the ressource fname2
		sdsname1 = self.__get_sds_name(time1, net, sta, cha, loc)
		sdsname2 = self.__get_sds_name(time2, net, sta, cha, loc)
		fname1 = self.get_sds_path(sdsname1)
		fname2 = self.get_sds_path(sdsname2)

		logs.debug("SDS::files: %s %s" % (sdsname1, sdsname2))
		
		try:
			### get the possible time span from Mini SEED file(s) ###
			if fname1 != None or fname2 != None:
				if fname1 == fname2:
					self.free_sds_path(fname2)
				if fname1 != None:
					begtime, endtime = _get_timespan(fname1)
				else:
					fname1 = fname2
					free = 1
				if fname2 != None:
					dummy, endtime = _get_timespan(fname2)
				else:
					fname2 = fname1
				if fname1 == fname2 and free:
					begtime = dummy
					
				logs.debug("SDS::possible time span %s - %s" % (begtime, endtime))
				
				### set the offsets to the data available for the time span to be searched for ###
				begoffset = 0                        # default
				endoffset = os.path.getsize(fname2)  # default
				
				if time1 < endtime and time2 > begtime:	
					if begtime < time1:             # begoffset has to be set
						logs.debug("1")
						try:
							begoffset = _findtime(fname1,time1)
						except LookupError:         # in case of time1 is lying after the end time of file1 ...
							logs.debug("2")
							try:
								begoffset = _findtime(fname2,time1) # ... look into file2
							except LookupError:     # in case of lying before the beginning of file2's time span
								logs.debug("3")
								begoffset = 0
							self.free_sds_path(fname1)
							fname1 = fname2
							free = 1
							
					if time2 < endtime:             # endoffset has to be set
						logs.debug("4")
						try:
							endoffset = _findtime(fname2,time2,1)
						except LookupError:         # if time2 is lying before the beginning of the file2 ...
							logs.debug("5")
							if free and begoffset == 0: # if the searched time span is lying in a gap between file1 end and file2 start
								logs.debug("6")
								endoffset = begoffset
							else:
								logs.debug("7")
								try:
									endoffset = _findtime(fname1,time2,1) # ... look into file1
								except LookupError:     # if time2 is lying after the end time of file1
									logs.debug("8")
									endoffset = os.path.getsize(fname1)
								self.free_sds_path(fname2)
								fname2 = fname1
				else:
					logs.debug("9")
					endoffset = begoffset
				
				logs.debug("SDS::beg/end-offset: %s %s" % (str(begoffset),str(endoffset)))
					
					
				### get the data from the file(s) according to the offsets ###
				if fname1 == fname2 and begoffset != endoffset:
					logs.debug("10")
					fh = file(fname1)
					try:
						fh.seek(begoffset)
						data = fh.read(endoffset-begoffset)
					finally:
						logs.debug("11")
						fh.close()
				elif fname1 != fname2: # and begoffset != endoffset:
					logs.debug("12")
					fh = file(fname1)
					try:
						fh.seek(begoffset)
						data = fh.read(os.path.getsize(fname1)-begoffset)
						fh.close()
						fh = file(fname2)
						data += fh.read(endoffset)
					finally:
						logs.debug("13")
						fh.close()

		### free the system ressources ###
		finally:
			if free:
				self.free_sds_path(fname2)
			else:
				self.free_sds_path(fname1)
				if fname1 != fname2:
					self.free_sds_path(fname2)

		logs.debug("14")
		return data


def main():
	parser = OptionParser(usage="usage: %prog -n <nrt_directory> -a <cur_arch_directory> -i <iso_directory> -m <mount_directory>")
	parser.add_option("-n", "--nrtdir", type="string", dest="nrtdir", help="nrt directory")   
	parser.add_option("-a", "--archdir", type="string", dest="archdir", help="current archive directory")
	parser.add_option("-i", "--isodir", type="string", dest="isodir", help="iso archive directory")
	parser.add_option("-m", "--mntdir", type="string", dest="mntdir", help="directory for the mountable symlinks")
	(options, args) = parser.parse_args()

	if options.nrtdir == None:
		parser.error("Error: nrt directory not specified!")
	if options.archdir == None:
		parser.error("Error: current archive directory not specified!")
	if options.isodir == None:
		parser.error("Error: iso archive directory not specified!")
	if options.mntdir == None:
		parser.error("Error: directory to mountable symlinks not specified!")

	sdsobj = SDS(options.nrtdir,options.archdir,options.isodir,options.mntdir)
	reqlist = [(datetime.datetime(2003,12,30,12,15,34),datetime.datetime(2003,12,31,12,15,30),"GE","APE","BHE",""),(datetime.datetime(2003,12,31,0,0,34),datetime.datetime(2004,1,1,0,0,0),"GE","APE","VHN",""),(datetime.datetime(2003,12,31,10,0,34),datetime.datetime(2004,1,1,6,0,0),"GE","APE","VHZ",""),(datetime.datetime(1993,12,30,23,59,59),datetime.datetime(1993,12,31,0,0,6),"GE","PMG","VHN","")]
	count = 0
	for req in reqlist:
		count += 1
		print count," request"
		data = sdsobj.getwin(*req)
		if data != None:
			print "data_%d" % count
			fh = file("data_%d" % count,"w")
			fh.write(data)
			fh.close()
		else:
			print "No data available"
	

if __name__=="__main__":
	main()                

