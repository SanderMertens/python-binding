from nose.tools import raises

import cortopy

@raises(Exception)
def test_cannot_resolve():
    cortopy.resolve("idontexist")

def test_root():
    assert cortopy.root() is not None

def test_declare_child():
    pass
    # a = cortopy.declare_child(None, "testDeclareChildA", "int8");
    # b = cortopy.resolve("::testDeclareChildA")
    # assert b is not None

# @raises(TypeError)
# def test_declare_child_bad_arguments():
#     a = cortopy.declare_child(4, 5, 6)

@raises(ValueError)
def test_declare_child_bad_parent():
    a = cortopy.declare_child("abc", "xyz", "int8")

@raises(ValueError)
def test_declare_child_bad_type():
    a = cortopy.declare_child("/corto", "xyz", "int88")
