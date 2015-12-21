from nose.tools import raises

import cortopy

@raises(Exception)
def test_cannot_resolve():
    cortopy.resolve("idontexist")

def test_root():
    assert cortopy.root() is not None

def test_lang_with_slash():
    assert cortopy.resolve("/corto/lang") is not None

def test_lang_with_double_colon():
    assert cortopy.resolve("::corto::lang") is not None
