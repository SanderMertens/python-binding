Development guide
=================

Setting up
----------

To set up the environment, use pyvenv.

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
pip uninstall cortopy
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
