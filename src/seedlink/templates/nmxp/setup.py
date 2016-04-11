import os

'''
Plugin handler for the NRTS plugin.
'''
class SeedlinkPluginHandler:
  # Create defaults
  def __init__(self):
    pass

  def push(self, seedlink):
    # Check and set defaults
    address = "idahub.ucsd.edu"
    try: address = seedlink.param('sources.nmxp.address')
    except: seedlink.setParam('sources.nmxp.address', address)

    port = 28000
    try: port = int(seedlink.param('sources.nmxp.port'))
    except: seedlink.setParam('sources.nmxp.port', port)

    max_latency = 300
    try: max_latency = int(seedlink.param('sources.nmxp.max_latency'))
    except: seedlink.setParam('sources.nmxp.max_latency', max_latency)

    try: seedlink.param('sources.nmxp.proc')
    except: seedlink.setParam('sources.nmxp.proc', 'nmxp_bb40_sm100')

    # Key is address (one instance per address)
    return address + ":" + str(port)


  # Flush does nothing
  def flush(self, seedlink):
    pass