  <!-- template: #template# -->
  <group
    address="#srcaddr#:#srcport#"
    seqfile="#pkgroot#/status/#statid#.dial.seq"
    lockfile="#pkgroot#/status/dial.pid"
    uptime="#dial_uptime#"
    schedule="#dial_schedule#"
    ifup=""
    ifdown="">

    <station id="#statid#" name="#station#" network="#netid#" selectors="#selectors#"/>
  </group>
