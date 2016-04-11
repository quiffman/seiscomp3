import os, time, seiscomp3.Kernel


# The kernel module which starts spread if enabled
class Module(seiscomp3.Kernel.CoreModule):
  def __init__(self, env):
    seiscomp3.Kernel.CoreModule.__init__(self, env, env.moduleName(__file__))
    # Highest priority
    self.order = -2

    # Default values
    self.messaging = True
    self.messagingPort = 4803

    try: self.messaging = self.env.getBool("messaging.enable")
    except: pass
    try: self.messagingPort = self.env.getInt("messaging.port")
    except: pass

  def _run(self):
    params = self.env.lockFile(self.name) + " spread -n localhost"
    spread_conf_dir = os.path.join(self.env.root, "var", "lib", "spread")

    # Create config directory in var/lib/spread
    if not os.path.exists(spread_conf_dir):
      try: os.makedirs(spread_conf_dir)
      except Exception, e:
        print "%s: %s" % (spread_conf_dir, str(e))
        return 1

    spread_conf = os.path.join(spread_conf_dir, "spread.conf")
    try: cfg = open(spread_conf, 'w')
    except:
      print "%s: failed to write: abort" % os.path.abspath(spread_conf)
      return 1

    # Write a default configuration file
    cfg.write("""\
Spread_Segment  127.0.0.255:%d {
    localhost               127.0.0.1
}

SocketPortReuse = ON
""" % self.messagingPort)
    cfg.close()

    params = params + " -c \"" + spread_conf + "\""
    res = self.env.start(self.name, "run_with_lock", params, nohup=True)
    time.sleep(2)
    return res

  def start(self):
    if not self.messaging:
      print "[kernel] %s is disabled by config" % self.name
      return 0

    return seiscomp3.Kernel.CoreModule.start(self)

  def stop(self):
    return seiscomp3.Kernel.CoreModule.stop(self)

  def check(self):
    if not self.messaging:
      print "[kernel] %s is disabled by config" % self.name
      return 0

    return seiscomp3.Kernel.CoreModule.check(self)

  def status(self, shouldRun):
    if not self.messaging: shouldRun = False
    seiscomp3.Kernel.CoreModule.status(self, shouldRun)
