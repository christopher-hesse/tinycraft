import os
import subprocess as sp

sp.run("pip install -e .", shell=True, check=True)
# if the shared library is built in the mounted host dir, the mounted host dir is weird
# and either causes the shared library to be rebuilt or else causes cmake to not unlink
# it correctly, because cffi will segfault the next time the library is used
os.environ["TINYCRAFT_BUILD_ROOT"] = "/tmp/tinycraft-build"
sp.run("python -u -X faulthandler -m pytest tinycraft", shell=True, check=True)
