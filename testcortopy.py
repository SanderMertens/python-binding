import cortopy

cortopy.start()

def Assert(b, *args):
    if b:
        print("=== Pass ===", *args)
    else:
        print("=== Fail ===", *args)

root = cortopy.root()
Assert(root, "root", root)

try:
    lang = cortopy.resolve("corto/lang")
    Assert(True, "lang is", lang)
    name = cortopy.nameof(lang) 
    Assert(name == "lang", "name of lang is", name)
except:
    Assert(False, "No lang")

try:
    cortopy.resolve("dddd")
    Assert(False, "Shouldn't have resolved dddd")
except:
    Assert(True, "Could not resolve dddd")

try:
    cortoint_type = cortopy.int
    Assert(isinstance(cortopy.int, type), "Corto int is instance of type")
except:
    Assert(False, "Couldn't get corto.int type")

try:
    a =  cortopy.int("Hey", "Hey", 1)
    Assert(a == 1, "a == 1")
except:
    Assert(False, "Couldn't create object")

a =  cortopy.int("Hey", "Hey", 1)

print(a)
print(a.name)

cortopy.stop()
