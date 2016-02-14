import pytest

import cortopy


class NameGenerator:
    def __init__(self, prefix):
        self.prefix = prefix
        self.n = 0

    def next(self):
        self.n += 1
        return "{}{}".format(self.prefix, self.n)


names = NameGenerator("obj")


@pytest.fixture
def name():
    return names.next()


@pytest.fixture(scope='module')
def Point3d():
    cortopy.eval(
"""
struct Point3d::
    x, y, z: int64
"""
    )
    t = cortopy.gettype("Point3d")
    return t

@pytest.fixture(scope='module')
def Point4d(Point3d):
    cortopy.eval(
"""
struct Point4d: Point3d::
    w: int64
"""
    )
    t = cortopy.gettype("Point4d")
    return t

@pytest.fixture(scope='module')
def Line():
    cortopy.eval(
"""
struct Line::
    a, b: Point3d
"""
    )
    t = cortopy.gettype("Line")
    return t


@pytest.fixture(scope='module')
def Point3d():
    cortopy.eval(
"""
struct Point3d::
    x, y, z: int64
"""
    )
    p = cortopy.gettype("Point3d")
    return p


def test_declare_child_bad_parent():
    with pytest.raises(ValueError):
        a = cortopy.declare_child("abc", "xyz", "int8")


def test_declare_child_bad_type():
    with pytest.raises(ValueError):
        a = cortopy.declare_child("/corto", "xyz", "int88")


def test_serializeType():
    a = cortopy.gettype("bool")


def test_deserialize_point_once():
    cortopy.eval(
"""
struct PointA::
    x, y: int64
"""
    )
    Point = cortopy.gettype("PointA")


def test_deserialize_point_twice():
    cortopy.eval(
"""
struct PointB::
    x, y: int64
"""
    )
    Point1 = cortopy.gettype("PointB")
    Point2 = cortopy.gettype("PointB")
    assert Point1 == Point2


def test_eval_expr():
    cortopy.eval("3 + 4")


def test_declare_child_int8_string(name):
    cortopy.declare_child(None, name, "int8")


def test_declare_child_int8_type(name):
    int8 = cortopy.gettype("int8")
    cortopy.declare_child(None, name, int8)


def test_int8_type_and_superclass(name):
    int8 = cortopy.gettype("int8")
    assert isinstance(int8, type)
    assert issubclass(int8, cortopy.object)


@pytest.mark.parametrize(
    "type_, value",
    [
        ("int8", 2 ** 7 - 1),
        ("int8", 0),
        ("int8", - 2  ** 7),
        ("int16", 2 ** 15 - 1),
        ("int16", - 2 ** 15),
        # ("int32", 2 ** 31 - 1),
        # ("int32", - 2 ** 31),
        ("int64", 2 ** 63 - 1),
        ("int64", - 2 ** 63),
    ]
)
def test_int_setval(name, type_, value):
    o = cortopy.declare_child(None, name, type_)
    o.setval(value)
    assert o.val == value


@pytest.mark.parametrize(
    "type_, value",
    [
        ("int8", 128),
        ("int8", -129),
        ("int16", 2 ** 15),
        ("int16", - 2 ** 15 - 1),
        ("int32", 2 ** 31),
        ("int32", - 2 ** 31 - 1),
        ("int64", 2 ** 63),
        ("int64", - 2 ** 63 - 1),
    ]
)
def test_int_setval_value_error(name, type_, value):
    o = cortopy.declare_child(None, name, type_)
    with pytest.raises(ValueError):
        o.setval(value)


def test_int8_mro(name):
    int8 = cortopy.gettype("int8")
    assert int8.__mro__ == (cortopy.types["/corto/lang/int8"], cortopy.object, object)


def test_declare_point3d(name, Point3d):
    o = cortopy.declare_child(None, name, Point3d)
    o.setval({"x": 1, "y": 2, "z": 3})
    assert o.x == 1
    assert o.y == 2
    assert o.z == 3


def test_point4d_subtype_of_point3d(Point3d, Point4d):
    assert issubclass(Point4d, Point3d)


def test_declare_point4d(name, Point4d):
    o = cortopy.declare_child(None, name, Point4d)


def test_members_point4d(name, Point4d):
    o = cortopy.declare_child(None, name, Point4d)
    o.w = 11
    o.x = 22
    o.y = 33
    o.z = 44
    assert (o.w, o.x, o.y, o.z) == (11, 22, 33, 44)


def test_setval_point4d(name, Point4d):
    o = cortopy.declare_child(None, name, Point4d)
    o.setval({"w": 11, "x": 22, "y": 33, "z": 44})
    assert (o.w, o.x, o.y, o.z) == (11, 22, 33, 44)
