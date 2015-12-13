Development guide
=================

Setting up
----------

Have Python 3.4.

Have corto installed as directed in www.corto.io.

Install pyvenv if you don't have it. I had trouble in Cloud 9's Ubuntu, so I use https://gist.github.com/jleeothon/ccd798591c2cff1dc555.

```
pyvenv env
```

Building and testing
--------------------

Activate the environment to start working:

```
. env/bin/activate
```

Build with

```
python setup.py build
```

Make the package with

```
python setup.py sdist
```

If cortopy has been already installed, uninstall it:

```
pip uninstall --yes cortopy
```

Install the package to the virtual environment

```
pip install dist/cortopy-0.1.tar.gz
```

Test your changes!


If you need to work on something else, deactivate the environment with

```
deactivate
```
