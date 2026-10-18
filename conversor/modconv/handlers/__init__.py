import importlib, pkgutil, os
for _m in sorted(m.name for m in pkgutil.iter_modules([os.path.dirname(__file__)])): importlib.import_module('handlers.' + _m)
