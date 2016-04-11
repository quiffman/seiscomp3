import os

'''
Plugin handler for the Earthworm export plugin.
'''
class SeedlinkPluginHandler:
  # Create defaults
  def __init__(self): pass

  def push(self, seedlink):
    # Check and set defaults
    host = seedlink._get('sources.scream.address')
    try: port = int(seedlink.param('sources.scream.port'))
    except: port = 1567

    try:
      map = seedlink.param('sources.scream.map')
      if not os.path.isabs(map):
        map = os.path.join(seedlink.config_dir, map)
    except: map = os.path.join(seedlink.config_dir, 'scream2sl.map')

    seedlink.setParam('sources.scream.mapFlag',map)
    seedlink.setParam('sources.scream.tcpFlag','')

    try:
      if seedlink.param('sources.scream.tcp').lower() in ("yes","true","1"):
        seedlink.setParam('sources.scream.tcpFlag',' -tcp')
    except: pass

    # The key is the complete command line
    return (host, port, map, seedlink.param('sources.scream.tcpFlag'))


  # Flush does nothing
  def flush(self, seedlink):
    pass
