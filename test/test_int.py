import cortopy

class TestCortopyInt:

    def setup(self):
        self.a = cortopy.int("parent", "a", 1)

    def test_value(self):
        assert self.a == 1

    def test_name(self):
        assert self.a.name == "a"

    def test_nameof(self):
        assert cortopy.nameof(self.a) == "a"

    def test_type(self):
        assert type(self.a) == cortopy.int

    def test_isinstance_of_builtin_int(self):
        assert isinstance(self.a, int)

