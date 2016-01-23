from nose.tools import raises

import cortopy

@raises(Exception)
def test_cannot_resolve():
    cortopy.resolve("idontexist")

@raises(ValueError)
def test_declare_child_bad_parent():
    a = cortopy.declare_child("abc", "xyz", "int8")

@raises(ValueError)
def test_declare_child_bad_type():
    a = cortopy.declare_child("/corto", "xyz", "int88")

def test_serializeType():
    a = cortopy.gettype("bool")

def test_serializeClass():
    interfaceVector = cortopy.gettype("interfaceVector")
    # i = interfaceVector()

def test_deserialize_point_once():
    Point = cortopy.gettype("Point")

def test_deserialize_point_twice():
    Point = cortopy.gettype("Point")
    Point = cortopy.gettype("Point")
    pass

def test_eval():
    cortopy.eval("3 + 4")
    cortopy.eval(
        "struct Point3d::\n"
        "    x, y, z: int64\n"
    )
    cortopy.gettype("Point3d")

def test_declare_child_int8():
    cortopy.declare_child(None, "O1", "int8")

def test_declare_child_setval_int8():
    o = cortopy.declare_child(None, "O2", "int8")
    print(type(o).__mro__)
    o.setval(8)
