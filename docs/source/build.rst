Build Instructions
------------------

**jruntime** can be built in four ways for different purposes.

If you are under Windows, please activate the conda environment or virtual environments, as specified in the followed sections, in an `x64 Native Tools Command Prompt for VS 2019`_. Your Visual Studio should have installed the "C++ Clang Compiler for Windows" component. If you are under Linux, you should have installed the Clang toolchain. Jruntime recommends LLVM 11.

Building Conda packages
=======================

Make sure that you are in a conda environment that has ``conda-build``.

Execute the following command under the project root directory:

.. code-block:: bat

    conda build -c conda-forge .

This will build the ``jruntime`` conda packages for both Python 3.8 and 3.9. Upon succeeds, you may upload them to Anaconda if you have installed the ``anaconda-client`` package. The official channel is `simpleroseinc <https://anaconda.org/simpleroseinc>`_.

Building Wheels
===============

Make sure you are in a ``venv`` that has all the dependencies specified in ``requirements.txt``. A typical way to creating such an environment is by running

.. code-block:: bat

    python -m venv venv
    venv\Scripts\activate         # cmd.exe
    source venv/bin/activate.csh  # tcsh
    source venv/bin/activate      # bash
    pip install -r requirements.txt

In the rest of the section, let's assume that we are targeting Python 3.8.

If you are under Windows, execute the following command:

.. code-block:: bat

    python setup.py bdist_wheel

That's it.

If you are under Linux, make sure that you have installed the LLVM OpenMP runtime. It may be a package called ``libomp11-devel`` in your distribution's package manager. And then, execute the following command:

.. code-block:: bash

    python setup.py bdist_wheel --plat-name manylinux_2_24_x86_64

Now we need to fix the ``RUNPATH`` manually. Assume that ``/usr/lib64/libomp.so`` is where the LLVM OpenMP runtime is, execute the following commands:

.. code-block:: bash

    cd dist/
    wheel unpack jruntime-VERSION-cp38-cp38-manylinux_2_24_x86_64.whl
    cd jruntime-VERSION/
    cp /usr/lib64/libomp.so .
    patchelf --set-rpath '$ORIGIN' libj.so
    patchelf --set-rpath '$ORIGIN' jruntime.cpython-38-x86_64-linux-gnu.so
    cd ..
    wheel pack jruntime-VERSION/
    cd ..

Replace "VERSION" with the jruntime version in the above snippet.

You may upload the wheel using ``twine upload -s`` command followed by the path to the ``.whl`` file. Make sure you've configured GPG before doing that.

Building Documents
==================

Make sure you are in a ``venv`` that has all the dependencies specified in ``docs/requirements.txt``. A typical way to creating such an environment is by running

.. code-block:: bat

    python -m venv venv
    venv\Scripts\activate         # cmd.exe
    source venv/bin/activate.csh  # tcsh
    source venv/bin/activate      # bash
    pip install -r docs/requirements.txt

Sphinx needs to extract the docstrings by importing a ``jruntime`` module. As a result, you need to rerun the following commands every time you made a change to ``jsrc/jruntime.cc``:

.. code-block:: bat

    python setup.py install
    sphinx-build docs/source docs/build

Development
===========

Follow `CMake build instructions for J`_. Upon finishing the Debug build, if you are using Visual Studio, you will find the ``jruntime`` module under ``out\build\x64-Clang-Debug\jsrc``; if you are using the Ninja Multi-Config generator, the module is under ``build/jsrc/Debug``. Change directory, then you can ``import jruntime`` in Python REPL.

A typical way of debugging jruntime is running ``gdb python`` under that directory.

.. _x64 Native Tools Command Prompt for VS 2019: https://simpleroseinc.github.io/2020/11/13/visual-studio-command-prompt-in-windows-terminal.html
.. _CMake build instructions for J: https://github.com/jsoftware/jsource/pull/20#issue-518638949