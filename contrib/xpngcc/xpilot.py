# setup.py
from distutils.core import setup
import py2exe

setup(console=["xpngcc.py"],
	options = {"py2exe": {"packages": ["encodings"]}},
)
