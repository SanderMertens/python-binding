import corto
import unittest

class Test(unittest.TestCase):

    def setUp(self):
        corto.start()

    def tearDown(self):
        corto.stop()

    # def test_start_stop(self):
    #     pass

    # def test_root_o(self):
    #     self.assertIsNotNone(corto.ROOT_O)

    def test_name_of_root(self):
        name = corto.ROOT_O.name()
        self.assertEquals(name, 0)

    # def test_resolve(self):
    #     """
    #     Fails
    #     """
    #     lang = corto.resolve(None, b"lang")
    #     name = corto.nameof(lang)
