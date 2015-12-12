import cortopy

print(dir(cortopy))

cortopy.start()


print("Address of root is", cortopy.root())

print("Address of lang is", cortopy.resolve("corto/lang"))
lang = cortopy.resolve("corto/lang")
print(lang)
print("Name of lang is", cortopy.nameof(lang))

# cortopy.resolve("dddd")

cortopy.stop()
