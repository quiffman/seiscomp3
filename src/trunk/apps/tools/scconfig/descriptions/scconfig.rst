scconfig is a graphical user interface which allows to configure all
SeisComP3 modules which provide :ref:`descriptions <contributing_documentation>`.

It supports furthermore to:

- start/stop/check all registered modules
- import station metadata from various sources
- configure modules
- configure module bindings

scconfig can be run in two different modes: user mode and system mode. In user
mode it will manage the configuration living at :file:`~/.seiscomp3` while
system mode manages the system wide configuration at :file:`etc`.
