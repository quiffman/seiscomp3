import os

'''
Plugin handler for the WIN plugin.
'''
class SeedlinkPluginHandler:
  # Create defaults
  def __init__(self):
    self.instances = {}
    self.channelMap = {}

  def push(self, seedlink):
    # Check and set defaults
    udpport = 18000
    try: udpport = seedlink.param('sources.win.udpport')
    except: seedlink.setParam('sources.win.udpport', udpport)

    try: seedlink.param('sources.win.proc')
    except: seedlink.setParam('sources.win.proc', 'win')

    try:
      winId = self.instances[udpport]

    except KeyError:
      winId = len(self.instances)
      self.instances[udpport] = winId
      self.channelMap[winId] = []
    
    try:
      channelItems = [ x.strip() for x in seedlink.param('sources.win.channels').split(',') ]

    except KeyError:
      raise Exception("Error: sources.win.channels not defined")

    for item in channelItems:
      mapping = [x.strip() for x in item.split(':')]
      if len(mapping) != 2:
        raise Exception("Error: invalid rename mapping '%s' in %s" % (item, seedlink.station_config_file))
      if not mapping[0] or not mapping[1]:
        raise Exception("Error: invalid rename mapping '%s' in %s" % (item, seedlink.station_config_file))
      self.channelMap[winId].append(mapping)
      
    seedlink.setParam('seedlink.win.id', winId)
    
    return udpport

  def flush(self, seedlink):
    for x in self.channelMap.keys():
      win2slmap = os.path.join(seedlink.config_dir, "win2sl%d.map" % x)

      fd = open(win2slmap, "w")

      for c in self.channelMap[x]:
        fd.write("%s %s\n" % tuple(c))

      fd.close()

