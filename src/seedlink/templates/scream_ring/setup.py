import os

'''
Plugin handler for the Earthworm export plugin.
'''
class SeedlinkPluginHandler:
  # Create defaults
  def __init__(self): pass

  def push(self, seedlink):
    # Check and set defaults
    host = seedlink._get('sources.scream_ring.address')
    try: tcpport = int(seedlink.param('sources.scream_ring.tcpport'))
    except: tcpport = 1567
    try: udpport = int(seedlink.param('sources.scream_ring.udpport'))
    except: udpport = 1567

    try:
      map = seedlink.param('sources.scream_ring.map')
      if not os.path.isabs(map):
        map = os.path.join(seedlink.config_dir, map)
    except: map = os.path.join(seedlink.config_dir, 'scream2sl.map')

    seedlink.setParam('sources.scream_ring.mapFlag',map)

    # The key is the complete command line
    return (host, tcpport, udpport, map)


  # Flush does nothing
  def flush(self, seedlink):
    pass
