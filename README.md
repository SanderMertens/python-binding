Python Binding for Corto [![Build Status](https://travis-ci.org/cortoproject/python-binding.svg?branch=master)](https://travis-ci.org/cortoproject/python-binding)
========================

Use Corto from Python.

Example:

```python
cortopy.eval(
"""
struct Point3d::
    x, y, z: int64
struct Point4d: Point3d::
    w: int64
"""
)
a = cortopy.declare_child(None, "a", Point4d)
a.w = 11
a.x = 22
a.y = 33
a.z = 44
print("w = {}, x = {}, y = {}, z = {}".format(o.w, o.x, o.y, o.z))
```
