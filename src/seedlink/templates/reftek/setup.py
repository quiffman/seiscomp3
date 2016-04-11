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

    timeout = 60
    try: timeout = int(seedlink.param('sources.reftek.timeout'))
    except: seedlink.setParam('sources.reftek.timeout', timeout)

    default_tq = 40
    try: default_tq = int(seedlink.param('sources.reftek.default_tq'))
    except: seedlink.setParam('sources.reftek.default_tq', default_tq)

    unlock_tq = 10
    try: unlock_tq = int(seedlink.param('sources.reftek.unlock_tq'))
    except: seedlink.setParam('sources.reftek.unlock_tq', unlock_tq)

    # Overwrite Seedlink station id with unit id
    print "      + change station id from %s to %s" % (seedlink.param('seedlink.station.id'), unit)
    seedlink.setParam('seedlink.station.id', unit)

    proc = "reftek"
    try: port = int(seedlink.param('sources.reftek.proc'))
    except: seedlink.setParam('sources.reftek.proc', proc)

    log_soh = "true"
    try:
      if seedlink.param('sources.reftek.log_soh').lower() in ("yes", "true", "1"):
        log_soh = "true"
      else:
        log_soh = "false"
    except: seedlink.setParam('sources.reftek.log_soh', log_soh)

    # Key is address (one instance per address)
    return address + ":" + str(port)


  # Flush does nothing
  def flush(self, seedlink):
    pass
