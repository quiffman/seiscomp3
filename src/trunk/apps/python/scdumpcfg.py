#!/usr/bin/env python

############################################################################
#    Copyright (C) by GFZ Potsdam                                          #
#                                                                          #
#    You can redistribute and/or modify this program under the             #
#    terms of the SeisComP Public License.                                 #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    SeisComP Public License for more details.                             #
############################################################################

import sys
import seiscomp3.Client, seiscomp3.Config


def readParams(sc_params):
  if sc_params.baseID():
    sc_params_base = seiscomp3.DataModel.ParameterSet.Find(sc_params.baseID())
    if sc_params_base is None:
      print >> sys.stderr, "Warning: %s: base parameter set for %s not found" % (sc_params.baseID(), sc_params.publicID())
      params = {}
    else:
      params = readParams(sc_params_base)
  else:
    params = {}

  for i in range(sc_params.parameterCount()):
    p = sc_params.parameter(i)
    params[p.name()] = p.value()

  return params


class DumpCfg(seiscomp3.Client.Application):
  def __init__(self, argc, argv):
    if argc < 2:
      print >> sys.stderr, "scdumpcfg {modname} [options]"
      raise RuntimeError

    # Remove first parameter to replace appname with passed module name
    argc = argc-1
    argv = argv[1:]

    seiscomp3.Client.Application.__init__(self, argc, argv)

    self.setMessagingEnabled(True)
    self.setMessagingUsername("")
    self.setDatabaseEnabled(True, True)
    self.setLoadConfigModuleEnabled(True)
    self.setDaemonEnabled(False)


  def createCommandLineDescription(self):
    self.commandline().addGroup("Dump")
    self.commandline().addStringOption("Dump", "param,P", "Specify parameter name to filter for")
    self.commandline().addOption("Dump", "bindings,B", "Dump bindings instead of module configuration")
    self.commandline().addOption("Dump", "allow-global,G", "Print global bindings if no module binding is avaible")


  def validateParameters(self):
    if not seiscomp3.Client.Application.validateParameters(self):
      return False

    self.dumpBindings = self.commandline().hasOption("bindings")

    try: self.param = self.commandline().optionString("param")
    except: self.param = None

    self.allowGlobal = self.commandline().hasOption("allow-global")

    if not self.dumpBindings:
      self.setMessagingEnabled(False)
      self.setDatabaseEnabled(False, False)
      self.setLoadConfigModuleEnabled(False)

    return True


  # Do nothing.
  def initSubscriptions(self):
     return True


  def run(self):
    cfg = self.configuration()

    if not self.dumpBindings:
      symtab = cfg.symbolTable()
      names = cfg.names()
      count = 0
      for name in names:
        if self.param and self.param != name: continue
        sym = symtab.get(name)
        print sym.name
        print '  content: %s' % sym.content
        print '  source:  %s' % sym.uri
        count = count + 1

      if self.param and count == 0:
        print >> sys.stderr, "%s: definition not found." % self.param
    else:
      cfg = self.configModule()
      if cfg is None:
        print >> sys.stderr, "No config module read"
        return False

      tmp = {}
      for i in range(cfg.configStationCount()):
        cfg_sta = cfg.configStation(i)
        tmp[(cfg_sta.networkCode(), cfg_sta.stationCode())] = cfg_sta

      name = self.name()
      # For backward compatibility rename global to default
      if name == "global": name = "default"

      for item in sorted(tmp.keys()):
        cfg_sta = tmp[item]
        sta_enabled = cfg_sta.enabled()
        cfg_setup = seiscomp3.DataModel.findSetup(cfg_sta, name, self.allowGlobal)
        if not cfg_setup is None:
          suffix = ""
          if sta_enabled and cfg_setup.enabled():
            out = "+ "
          else:
            suffix = " ("
            if not sta_enabled:
              suffix += "station disabled"
            if not cfg_setup.enabled():
              if suffix: suffix += ", "
              suffix += "setup disabled"
            suffix += ")"
            out = "- "
          out += "%s.%s%s\n" % (cfg_sta.networkCode(), cfg_sta.stationCode(), suffix)
          params = seiscomp3.DataModel.ParameterSet.Find(cfg_setup.parameterSetID())
          if params is None:
            print >> sys.stderr, "ERROR: %s: ParameterSet not found" % cfg_setup.parameterSetID()
            return False

          params = readParams(params)
          count = 0
          for name in sorted(params.keys()):
            if self.param and self.param != name: continue
            out += "  %s: %s\n" % (name, params[name])
            count = count + 1

          if count > 0:
            sys.stdout.write(out)

    return True


try:
  app = DumpCfg(len(sys.argv), sys.argv)
except:
  sys.exit(1)

sys.exit(app())
