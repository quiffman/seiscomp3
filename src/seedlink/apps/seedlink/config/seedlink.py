import os, string, time, re, glob, shutil, sys, imp
import seiscomp3.Kernel, seiscomp3.Config, seiscomp3.System


'''
NOTE:
The plugin to be used for a station of configured with:
plugin = [type]
All plugin specific parameters are stored in plugin.[type].*.

All parameters from seedlink.cfg are not prefixed with "seedlink.".
Local parameters that are created from seedlink.cfg parameters are
prefixed with "seedlink.".
'''


class TemplateModule(seiscomp3.Kernel.Module):
    def __init__(self, env):
        seiscomp3.Kernel.Module.__init__(self, env, env.moduleName(__file__))

        self.pkgroot = self.env.SEISCOMP_ROOT

        cfg = seiscomp3.Config.Config()

        # Defaults Global + App Cfg
        cfg.readConfig(os.path.join(self.pkgroot, "etc", "defaults", "global.cfg"))
        cfg.readConfig(os.path.join(self.pkgroot, "etc", "defaults", self.name + ".cfg"))

        # Config Global + App Cfg
        cfg.readConfig(os.path.join(self.pkgroot, "etc", "global.cfg"))
        cfg.readConfig(os.path.join(self.pkgroot, "etc", self.name + ".cfg"))

        # User Global + App Cfg
        cfg.readConfig(os.path.join(os.environ['HOME'], ".seiscomp3", "global.cfg"))
        cfg.readConfig(os.path.join(os.environ['HOME'], ".seiscomp3", self.name + ".cfg"))

        #seiscomp3.System.Environment.Instance().initConfig(cfg, self.name)
        self.global_params = dict([(x, ",".join(cfg.getStrings(x))) for x in cfg.names()])
        #self.global_params_ex = dict(filter(lambda s: s[1].find("$") != -1, [(x, ",".join(cfg.getStrings(x))) for x in cfg.names()]))
        self.station_params = dict()
        #self.station_params_ex = dict()
        self.plugin_dir = os.path.join(self.pkgroot, "share", "plugins", self.name)
        self.template_dir = os.path.join(self.pkgroot, "share", "templates", self.name)
        self.alt_template_dir = "" #os.path.join(self.env.home
        self.config_dir = os.path.join(self.pkgroot, "var", "lib", self.name)
        self.rc_dir = os.path.join(self.pkgroot, "var", "lib", "rc")
        self.run_dir = os.path.join(self.pkgroot, "var", "run", self.name)
        self.bindings_dir = os.path.join(self.pkgroot, "etc", "key")
        self.key_dir = os.path.join(self.bindings_dir, self.name)
        self.net = None
        self.sta = None

    def _read_station_config(self, cfg_file):
        cfg = seiscomp3.Config.Config()
        cfg.readConfig(os.path.join(self.key_dir, cfg_file))
        self.station_params = dict([(x, ",".join(cfg.getStrings(x))) for x in cfg.names()])
        #self.station_params_ex = dict(filter(lambda s: s[1].find("$") != -1, [(x, ",".join(cfg.getStrings(x))) for x in cfg.names()]))

    def _process_template(self, tpl_file, source=None, station_scope=True, print_error=False):
        tpl_paths = []

        if source:
            tpl_paths.append(os.path.join(self.alt_template_dir, source))
            tpl_paths.append(os.path.join(self.template_dir, source))

        tpl_paths.append(self.alt_template_dir)
        tpl_paths.append(self.template_dir)

        params = self.global_params.copy()
        #params_ex = self.global_params_ex.copy()

        if station_scope:
            params.update(self.station_params)
            #params_ex.update(self.station_params_ex)

        params['pkgroot'] = self.pkgroot

        #for (p,v) in params_ex.iteritems():
        #    try:
        #        t2 = seiscomp3.Kernel.Template(v)
        #        params[p] = t2.substitute(params)
        #
        #    except (KeyError, ValueError):
        #        pass

        return self.env.processTemplate(tpl_file, tpl_paths, params, print_error)

    def param(self, name, station_scope=True, print_warning=False):
        if station_scope:
            try:
                return self.station_params[name]

            except KeyError:
                pass

        try:
            return self.global_params[name]

        except KeyError:
            pass

        if print_warning:
            if station_scope:
                print "warning: parameter '%s' is not defined for station %s %s" % (name, self.net, self.sta)
            else:
                print "warning: parameter '%s' is not defined at global scope" % (name,)

        raise KeyError

    def setParam(self, name, value, station_scope=True):
        self._set(name, value, station_scope)

    def _get(self, name, station_scope=True):
        try: return self.param(name, station_scope)
        except KeyError: return ""

    def _set(self, name, value, station_scope=True):
        if station_scope:
            self.station_params[name] = value

        else:
            self.global_params[name] = value

class Module(TemplateModule):
    def __init__(self, env):
        TemplateModule.__init__(self, env)

    def _run(self):
        if self.env.syslog:
            daemon_opt = '-D '
        else:
            daemon_opt = ''

        daemon_opt += "-v -f " + os.path.join(self.config_dir, "seedlink.ini")

        return self.env.start(self.name, self.env.binaryFile(self.name), daemon_opt,\
                              not self.env.syslog)

    def _getPluginHandler(self, source_type):
        try:
            return self.plugins[source_type]
        except KeyError:
            path = os.path.join(self.template_dir, source_type, "setup.py")
            try: f = open(path, 'r')
            except: return None

            modname = '__seiscomp_seedlink_plugins_' + source_type
            if sys.modules.has_key(modname):
                mod = sys.modules[modname]
            else:
                # create a module
                mod = imp.new_module(modname)
                mod.__file__ = path

                # store it in sys.modules
                sys.modules[modname] = mod

            # our namespace is the module dictionary
            namespace = mod.__dict__

            # test whether this has been done already
            if not hasattr(mod, 'SeedlinkPluginHandler'):
                code = f.read()
                # compile and exec dynamic code in the module
                exec compile(code, '', 'exec') in namespace

            mod = namespace.get('SeedlinkPluginHandler')
            handler = mod()
            self.plugins[source_type] = handler
            return handler

    def _generateStationForIni(self):
        ini =  'station %s  description = "%s"\n' % \
               (self._get('seedlink.station.id'), self._get('seedlink.station.description'))
        ini += '             name = "%s"\n' % self._get('seedlink.station.code')
        ini += '             network = "%s"\n' % self._get('seedlink.station.network')
        if self._get('seedlink.station.sproc'):
            ini += '             proc = "%s"\n' % self._get('seedlink.station.sproc')
        if self._get('seedlink.station.access'):
            ini += '             access = "%s"\n' % self._get('seedlink.station.access').replace(',',' ')
        ini += '\n'
        return ini

    def __process_station(self, profile):
        try:
            station_dict = self.seedlink_station[self.sta]
            station_id = self.sta + str(len(station_dict))

        except KeyError:
            station_dict = {}
            station_id = self.sta
            self.seedlink_station[self.sta] = station_dict

        if profile:
            self.station_config_file = "profile_%s" % (profile,)
        else:
            self.station_config_file = "station_%s_%s" % (self.net, self.sta)

        self._read_station_config(self.station_config_file)

        # Generate plugin independent parameters
        self._set('seedlink.station.id', station_id)
        self._set('seedlink.station.code', self.sta)
        self._set('seedlink.station.network', self.net)
        self._set('seedlink.station.access', self._get('access'))
        self._set('seedlink.station.sproc', self._get('proc'))

        # read station description from var/lib/rc/station_NET_STA
        # if not set, use the station code
        try:
          rc = seiscomp3.Config.Config()
          rc.readConfig(os.path.join(self.rc_dir, "station_%s_%s" % (self.net, self.sta)))
          self._set('seedlink.station.description', rc.getString("description"))
        except:
          self._set('seedlink.station.description', self.sta)

        self.station_count += 1

        if self._last_net != self.net:
            print "+ network %s" % self.net
            self._last_net = self.net

        print "  + station %s" % self.sta

        # If real-time simulation is activated do not parse the sources
        # and force the usage of the mseedfifo_plugin
        if self.msrtsimul:
            self._set('seedlink.station.sproc', '')
            station_dict[(self.net, self.sta)] = self._generateStationForIni()
            return

        station_sproc = set()

        for source_type in self._get('sources').split(','):
            if not source_type: continue

            source_alias = source_type
            toks = source_type.split(':')
            if len(toks) > 2:
                print "Error: invalid source identifier '%s', expected '[alias:]type'"
                continue
            elif len(toks) == 2:
                source_alias = toks[0]
                source_type = toks[1]

            # Plugins are outsourced to external handlers
            # that can be added with new plugins.
            # This requires a handler file:
            # share/templates/seedlink/$type/setup.py
            pluginHandler = self._getPluginHandler(source_type)
            if pluginHandler is None:
                print "Error: no handler for plugin %s defined" % source_type
                continue

            stat = source_type
            if source_alias != source_type:
                stat += " as " + source_alias

            print "    + source %s" % stat

            # Backup original binding parameters
            station_params = self.station_params.copy()
            #station_params_ex = self.station_params_ex.copy()

            # Modify parameter set. Remove alias definition with type string
            if source_type != source_alias:
                tmp_dict = {}
                for x in self.station_params.keys():
                    if x.startswith('sources.%s.' % source_type): continue
                    if x.startswith('sources.%s.' % source_alias):
                        toks = x.split('.')
                        toks[1] = source_type
                        tmp_dict[".".join(toks)] = self.station_params[x]
                    else:
                        tmp_dict[x] = self.station_params[x]
                self.station_params = tmp_dict

                #tmp_dict = {}
                #for x in self.station_params_ex.keys():
                #    if x.startswith('sources.%s.' % source_type): continue
                #    if x.startswith('sources.%s.' % source_alias):
                #        toks = x.split('.')
                #        toks[1] = source_type
                #        tmp_dict[".".join(toks)] = self.station_params_ex[x]
                #    else:
                #        tmp_dict[x] = self.station_params_ex[x]
                #self.station_params_ex = tmp_dict

            # Create source entry that ends up in seedlink.ini as plugin
            try:
                source_dict = self.seedlink_source[source_type]

            except KeyError:
                source_dict = {}
                self.seedlink_source[source_type] = source_dict

            source_key = pluginHandler.push(self)
            if source_key is None:
                source_key = source_type
            else:
                source_key = (source_type, source_key)

            if not source_dict.has_key(source_key):
                source_id = source_type + str(len(source_dict))

            else:
                (source_type, source_id) = source_dict[source_key][:2]

            # Update internal parameters usable by a template
            self._set('seedlink.source.type', source_type)
            self._set('seedlink.source.id', source_id)
            source_dict[source_key] = (source_type, source_id, self.station_params.copy())
                
            # Create procs for this type for streams.xml
            sproc_name = self._get('sources.%s.proc' % (source_type)) or self._get('proc')
            if sproc_name:
                self.sproc_used = True
                sproc = self._process_template("streams_%s.tpl" % sproc_name, source_type)
                if sproc:
                    station_sproc.add(sproc_name)
                    self.sproc[sproc_name] = sproc

                else:
                    print "cannot find streams_%s.tpl" % sproc_name

            # Read plugins.ini template for this source and store content
            # under the provided key for this binding
            plugin_ini = self._process_template("plugins.ini.tpl", source_type)
            if plugin_ini:
                self.plugins_ini[source_key] = plugin_ini

            templates = self._get('sources.%s.templates' % (source_type))
            if templates:
                for t in templates.split(','):
                    self.templates.add((t, source_type, 0))

            # Set original parameters
            self.station_params = station_params
            #self.station_params_ex = station_params_ex

        if len(station_sproc) > 1:
            data = '  <proc name="%s">\n' % (",".join(station_sproc),)
            for name in station_sproc:
                data += '    <using proc="%s">\n' % (name,)
            
            data += '  </proc>\n'

            self._set('seedlink.station.sproc', ",".join(station_sproc))
            self.sproc[",".join(station_sproc)] = data

        elif station_sproc:
            self._set('seedlink.station.sproc', list(station_sproc)[0])

        # Create station section for seedlink.ini
        station_dict[(self.net, self.sta)] = self._generateStationForIni()

    def __load_stations(self):
        self.seedlink_source = {}
        self.seedlink_station = {}
        self.plugins_ini = {}
        self.sproc = {}
        self.plugins = {}
        self.sproc_used = False
        self.station_count = 0

        if self.env.syslog:
            self._set('seedlink._daemon_opt', ' -D', False)
        else:
            self._set('seedlink._daemon_opt', '', False)

        self._set('seedlink.plugin_dir', self.plugin_dir, False)
        self._set('seedlink.config_dir', self.config_dir, False)
        self._set('seedlink.run_dir', self.run_dir, False)
        self._set('seedlink.filters', os.path.join(self.config_dir, "filters.fir"), False)
        self._set('seedlink.streams', os.path.join(self.config_dir, "streams.xml"), False)

        self.templates = set()
        self.templates.add(("backup_seqfiles", None, 0755))

        rx_binding = re.compile(r'(?P<module>[A-Za-z0-9_\.-]+)(:(?P<profile>[A-Za-z0-9_-]+))?$')

        files = glob.glob(os.path.join(self.bindings_dir, "station_*"))
        files.sort()
        self._last_net = ""

        for f in files:
            try:
                (path, net, sta) = f.split('_')
                if not path.endswith("station"):
                    print "invalid path", f

            except ValueError:
                print "invalid path", f
                continue

            self.net = net
            self.sta = sta

            fd = open(f)
            line = fd.readline()
            while line:
                line = line.strip()
                if not line or line[0] == '#':
                    line = fd.readline()
                    continue

                m = rx_binding.match(line)
                if not m:
                    print "invalid binding in %s: %s" % (f, line)
                    line = fd.readline()
                    continue

                if m.group('module') != self.name:
                    line = fd.readline()
                    continue

                profile = m.group('profile')
                self.__process_station(profile)
                break

            fd.close()

    def _set_default(self, name, value, station_scope = True):
        try: self.param(name)
        except: self._set(name, value, station_scope)

    def updateConfig(self):
        # Set default values
        try: self._set_default("organization", self.env.getString("organization"), False)
        except: pass

        self._set_default("lockfile", os.path.join("@ROOTDIR@", self.env.lockFile(self.name)), False)
        self._set_default("filebase", os.path.join("@ROOTDIR@", "var", "lib", "seedlink", "buffers"), False)
        self._set_default("port", "18000", False)
        self._set_default("encoding", "steim2", False)
        self._set_default("trusted", "127.0.0.0/8", False)
        self._set_default("stream_check", "true", False)
        self._set_default("window_extraction", "true", False)
        self._set_default("window_extraction_trusted", "true", False)

        self._set_default("buffers", "100", False)
        self._set_default("segments", "50", False)
        self._set_default("segsize", "1000", False)

        self._set_default("gap_check_pattern", "", False)
        self._set_default("gap_treshold", "", False)

	self._set_default("info", "streams", False)
	self._set_default("info_trusted", "all", False)
	self._set_default("request_log", "true", False)
	self._set_default("proc_gap_warn", "10", False)
	self._set_default("proc_gap_flush", "100000", False)
	self._set_default("proc_gap_reset", "1000000", False)
	self._set_default("seq_gap_limit", "100000", False)
	self._set_default("connections", "500", False)
	self._set_default("connections_per_ip", "20", False)
	self._set_default("bytespersec", "0", False)

        ## Expand the @Variables@
        e = seiscomp3.System.Environment_Instance()
        self.setParam("filebase", e.absolutePath(self.param("filebase")), False)
        self.setParam("lockfile", e.absolutePath(self.param("lockfile")), False)
        
        if self._get("msrtsimul", False).lower() == "true":
          self.msrtsimul = True
        else:
          self.msrtsimul = False

        # Load custom stream processor definitions
        custom_procs = self._process_template("streams_custom.tpl", None)
        if custom_procs: self.sproc[""] = sproc

        self.__load_stations()

        if self.msrtsimul:
            self.seedlink_source['mseedfifo'] = {1:self._process_template("seedlink_plugin.tpl", "mseedfifo")}

        try: os.makedirs(self.config_dir)
        except: pass

        try: os.makedirs(self.run_dir)
        except: pass

        for p in self.plugins.itervalues():
            p.flush(self)

        if self._get("stream_check", False).lower() == "true":
            self._set("stream_check", "enabled", False)
        else:
            self._set("stream_check", "disabled", False)

        if self._get("window_extraction", False).lower() == "true":
            self._set("window_extraction", "enabled", False)
        else:
            self._set("window_extraction", "disabled", False)

        if self._get("window_extraction_trusted", False).lower() == "true":
            self._set("window_extraction_trusted", "enabled", False)
        else:
            self._set("window_extraction_trusted", "disabled", False)

        if self._get("request_log", False).lower() == "true":
            self._set("request_log", "enabled", False)
        else:
            self._set("request_log", "disabled", False)

        fd = open(os.path.join(self.config_dir, "seedlink.ini"), "w")
        fd.write(self._process_template("seedlink_head.tpl", None, False))

        if self.sproc_used:
            fd.write(self._process_template("seedlink_sproc.tpl", None, False))

        for i in self.seedlink_source.itervalues():
            for (source_type, source_id, self.station_params) in i.itervalues():
                source = self._process_template("seedlink_plugin.tpl", source_type)
                if source:
                    fd.write(source)

        fd.write(self._process_template("seedlink_station_head.tpl", None, False))

        for i in self.seedlink_station.itervalues():
            for j in i.itervalues():
                fd.write(j)

        fd.close()

        if self.plugins_ini:
            fd = open(os.path.join(self.config_dir, "plugins.ini"), "w")
            for i in self.plugins_ini.itervalues():
                fd.write(i)

            fd.close()
        else:
            # If no plugins.ini is not used remove it from previous runs
            try: os.remove(os.path.join(self.config_dir, "plugins.ini"))
            except: pass

        if self.sproc_used:
            fd = open(self._get('seedlink.streams', False), "w")
            fd.write('<streams>\n')

            for i in self.sproc.itervalues():
                fd.write(i)

            fd.write('</streams>\n')
            fd.close()

            fd = open(self._get('seedlink.filters', False), "w")
            fd.write(self._process_template("filters.fir.tpl", None, False))
            fd.close()

        # If no stream procs are used, remove the generated files of a
        # previous run
        else:
            try: os.remove(self._get('seedlink.streams', False))
            except: pass
            try: os.remove(self._get('seedlink.filters', False))
            except: pass

        for (f, s, perm) in self.templates:
            fd = open(os.path.join(self.config_dir, f), "w")
            fd.write(self._process_template(f + '.tpl', s, False))
            fd.close()
            if perm:
                os.chmod(os.path.join(self.config_dir, f), perm)

        return 0


    def printCrontab(self):
        print "55 23 * * * %s >/dev/null 2>&1" % (os.path.join(self.config_dir, "backup_seqfiles"),)
