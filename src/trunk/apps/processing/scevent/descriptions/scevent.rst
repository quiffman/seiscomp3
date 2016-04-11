As a consequence of a real-time system the SeisComP3 system creates several
origins (results of localization processes) for one earthquake because as time
goes by more seismic phases are available. scevent receives these origins and
associates the origins to events. It is also possible to import Origins from
other agencies.


Origin Matching
---------------

scevent associates Origins to Events by searching for the best match of the new
(incoming) Origin to other Origins for existing Events. If a match is not found
a new Event can be formed. The new Origin is matched to existing Origins
by comparing differences in the horizontal components of the locations, Origin
time difference, and matching Picks.
The new Origin is matched to an existing Origin which has the highest rank in
the following three groups (1, 2, 3):

1. Location and Time (lowest)

   The difference in horizontal location is less than
   :confval:`eventAssociation.maximumDistance` (degrees)
   and the difference in Origin times is less than
   :confval:`eventAssociation.maximumTimeSpan`.

2. Picks

   The two Origins have more than :confval:`eventAssociation.minimumMatchingArrivals`
   matching Picks.

3. Picks and Location and Time (highest)

   This is the best match, for which both the Location-and-Time and Picks
   criteria above are satisfied.

If more than one Origin is found in the highest ranking class, then the first
one of them is chosen.

.. note::

   For efficiency Events in the cache are scanned first and if no matches are found then the database
   is scanned for the time window :confval:`eventAssociation.eventTimeBefore` - :confval:`eventAssociation.eventTimeAfter`
   around the incoming Origin time.

The order of objects in the cache will affect the first best match - is the
time or insertion ordered?

**No Origin Match**

If no Event with an Origin that matches the incoming Origin is found then a
new Event is formed and the Origin is associated to that Event. The following
criteria are applied to allow the creation of the new Event:

The Agency for the Origin is not black listed (processing.blacklist.agencies).

and

If the Origin is an Automatic then it has more than eventAssociation.minimumDefiningPhases Picks.

.. figure:: media/scevent/Association_of_an_origin_by_matching_picks.jpg
    :scale: 50 %
    :alt: alternate Association of an origin by matching picks.


Associations of an origin to an event by matching picks.
