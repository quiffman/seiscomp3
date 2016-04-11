Basics
======

All SeisComP3 graphical user interfaces are based on common libraries.
This chapter describes what configuration and styling options are available
for all GUI applications.

The GUI specific configuration options are additional to the standard application
options. Setup e.g. the messaging connection and database is equal to the
CLI (command line interface) applications.

To adjust the look-and-feel to the host desktop system and the personal taste
several styling options are available. Since all GUI applications are using the
Qt4 library the tool :program:`qtconfig-qt4` can be used to adjust the widget
theme and the colors. If  KDE is used as desktop system the same style is used
since KDE bases on Qt as well. 

The style options supported by SC3 (and not covered by the general Qt setup)
have to be given in the applications (or global) configuration file.
The parameters are prefixed with ``scheme.`` and are documented in :ref:`global/GUI`.


Configuration syntax
--------------------

Beside the usual integer, float, boolean and string parameters GUI applications
support also color, color gradient and font parameters.
The syntax is explained below.

Colors
^^^^^^

.. code-block:: sh

   color = RRGGBBAA | "rgb(red,green,blue)" | "rgba(red,green,blue,alpha)"

Colors are specified either in a hexadecimal notation or in a rgb(a) notation.
When using the rgb(a) notation it should be quoted. Otherwise the configuration
parser would tokenize the value into 3 or 4 strings due to the comma.


Color gradients
^^^^^^^^^^^^^^^

.. code-block:: sh

   gradient = 1:FF0000, 2:00FF00, 3:0000FF

This defines a gradient from red through green to blue for the nodes 1, 2 and 3.
Values out of range are clipped to the lower or upper bound.

Fonts
^^^^^

.. code-block:: sh

   # The font family
   font.family = "Arial"

   # Point size
   font.size = 12

   # Bold?
   # Default: false
   font.bold = false

   # Italic?
   # Default: false
   font.italic = false

   # Underline?
   # Default: false
   font.underline = false

   # Overline?
   # Default: false
   font.overline = false

An example to configure the SC3 base font:

.. code-block:: sh

   scheme.fonts.base.family = "Arial"
   scheme.fonts.base.size = 10
