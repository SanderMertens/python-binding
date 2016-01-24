import pytest

import cortopy


class NameGenerator:
    def __init__(self, prefix):
        self.prefix = prefix
        self.n = 0

    def next(self):
        self.n += 1
        return "{}{}".format(self.prefix, self.n)


names = NameGenerator("Obj")


# def test_cannot_resolve():
#     cortopy.resolve("idontexist")


# def test_declare_child_bad_parent():
#     a = cortopy.declare_child("abc", "xyz", "int8")

# @raises(ValueError)
# def test_declare_child_bad_type():
#     a = cortopy.declare_child("/corto", "xyz", "int88")

@pytest.fixture
def Point3d():
    cortopy.eval(
        "struct Point3d::\n"
        "    x, y, z: int64\n"
    )
    p = cortopy.gettype("Point3d")
    return p


def test_serializeType():
    a = cortopy.gettype("bool")


def test_serializeClass():
    interfaceVector = cortopy.gettype("interfaceVector")


def test_deserialize_point_once():
    Point = cortopy.gettype("Point")


def test_deserialize_point_twice():
    Point = cortopy.gettype("Point")
    Point = cortopy.gettype("Point")
    pass


def test_eval_expr():
    cortopy.eval("3 + 4")


def test_declare_child_int8_string():
    cortopy.declare_child(None, names.next(), "int8")


def test_declare_child_int8_type():
    int8 = cortopy.gettype("int8")
    print(int8.__mro__)
    print("type of it is {}".format(type(int8)))
    print("is instance of cortopyobject {}".format(isinstance(int8, cortopy.object)))
    print("is subclass of cortopyobject {}".format(issubclass(int8, cortopy.object)))
    print("type of cortopyobject {}".format(type(cortopy.object)));
    cortopy.declare_child(None, names.next(), int8)


@pytest.mark.parametrize(
    "type_, value",
    [
        ("int8", 127),
        ("int8", 0),
        ("int8", -128),
        ("int16", 2 ** 15 - 1),
        ("int16", - 2 ** 15),
        # ("int32", 2 ** 31 - 1),
        # ("int32", - 2 ** 31),
        ("int64", 2 ** 63 - 1),
        ("int64", - 2 ** 63),
    ]
)
def test_int_setval(type_, value):
    o = cortopy.declare_child(None, names.next(), type_)
    o.setval(value)


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
def test_int_setval_value_error(type_, value):
    o = cortopy.declare_child(None, names.next(), type_)
    with pytest.raises(ValueError):
        o.setval(value)


def test_declare_child_setval_int8():
    o = cortopy.declare_child(None, "O2", "int8")
    assert type(o).__mro__ == (cortopy.types()["/corto/lang/int8"], cortopy.object, object)

def test_declare_point3d(Point3d):
    o = cortopy.declare_child(None, names.next(), Point3d)
