import os

'''
Plugin handler for the Reftek export plugin.
'''
class SeedlinkPluginHandler:
  # Create defaults
  def __init__(self): pass

  def push(self, seedlink):
    # Check and set defaults
    address = "127.0.0.1"
    try: address = seedlink.param('sources.reftek.address')
    except: seedlink.setParam('sources.reftek.address', address)

    port = 2543
    try: port = int(seedlink.param('sources.reftek.port'))
    except: seedlink.setParam('sources.reftek.port', port)

    unit = "91F3"
    try: unit = seedlink.param('sources.reftek.unit')
    except: seedlink.setParam('sources.reftek.unit', unit)

    proc = "reftek"
    try: port = int(seedlink.param('sources.reftek.proc'))
    except: seedlink.setParam('sources.reftek.proc', proc)

    # Key is address (one instance per address)
    return address + ":" + str(port)


  # Flush does nothing
  def flush(self, seedlink):
    pass
