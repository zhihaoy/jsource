.. image:: _static/jpyd.png

|

**jruntime** is a modern Python extension that interacts with the J language.  It provides deep integration between Python and J in the following ways:

+---------------------------------------------------------------------+
| Takes over J engine I/O with Python I/O                             |
+---------------------------------------------------------------------+
| Maps between J arrays and NumPy ndarrays, J boxes and Python tuples |
+---------------------------------------------------------------------+
| Binds J nouns to lexical scopes using Python ``with`` statements    |
+---------------------------------------------------------------------+
| Handles J errors using Python exceptions                            |
+---------------------------------------------------------------------+
| Emits the complete API document in ``help(jruntime)``               |
+---------------------------------------------------------------------+

Getting Started
===============

Install jruntime and NumPy from PyPI:

.. code-block:: bash

    pip install jruntime numpy

Now you can run J code and read data in NumPy arrays:

    >>> import jruntime
    >>> j = jruntime.Session()
    >>> j.runsource('v =: 3 4 $ i.12')
    >>> j['v']
    array([[ 0,  1,  2,  3],
           [ 4,  5,  6,  7],
           [ 8,  9, 10, 11]], dtype=int64)

Or put Python data in J and run code that uses it:

    >>> with j.let(title='Price', v=[.99, 1.48, 0.26]):
    ...     j.runsource('title;+/ v')
    +-----+----+
    |Price|2.73|
    +-----+----+
