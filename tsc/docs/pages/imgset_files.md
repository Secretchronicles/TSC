Image Set files formats
=======================

Animated sequences can be defined by files containing information about
the set of images that the animation uses as well as timing and branching
information for the set of images.  Each line is a space separated list of
terms, where the first term is either a filename of the image for a frame
or an action term.  A filename can be followed by a set of informational
terms.  By convention, these image set files have the extension ".imgset"

Here is an example of an image set file:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
time 180
1.png
2.png
3.png
4.png
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible for a frame to have a random display time.  To do this,
the time term must specify a minimum and maximum time value:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
time 180 360
1.png
2.png
3.png
4.png
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is also possible to specify the time of the image directly.  This only
affects the specified image, and no others.  When this format is used, both
the minimum and maximum time value must be specified:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
time 180 360
1.png
2.png time 360 480
3.png
4.png time 360 480
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

It is possible for a branch to occur after a frame has be displayed.  To
do this, a branch term must be added to the frame.  The first value after
the branch term is the number of the destination frame in the same image
set, and the second term is the chance of a branch specified as a percentage:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
time 180 
1.png
2.png branch 0 20 branch 3 20 
3.png
4.png
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the example above, the second frame has a 20% chance to branch to the
first frame, and a 20% chance to branch to the forth frame.

It is possible to mix the terms within a frame:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
time 180 
1.png
2.png branch 0 20 time 80 180 branch 3 20
3.png
4.png
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

