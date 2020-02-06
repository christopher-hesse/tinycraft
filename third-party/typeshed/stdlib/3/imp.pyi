# Stubs for imp (Python 3.6)

import os
import sys
import types
from typing import Any, IO, List, Optional, Tuple, TypeVar, Union

from _imp import (
    lock_held as lock_held,
    acquire_lock as acquire_lock,
    release_lock as release_lock,
    get_frozen_object as get_frozen_object,
    is_frozen_package as is_frozen_package,
    init_frozen as init_frozen,
    is_builtin as is_builtin,
    is_frozen as is_frozen,
)

from _imp import create_dynamic as create_dynamic

_T = TypeVar("_T")

if sys.version_info >= (3, 6):
    _Path = Union[str, os.PathLike[str]]
else:
    _Path = str

SEARCH_ERROR: int
PY_SOURCE: int
PY_COMPILED: int
C_EXTENSION: int
PY_RESOURCE: int
PKG_DIRECTORY: int
C_BUILTIN: int
PY_FROZEN: int
PY_CODERESOURCE: int
IMP_HOOK: int

def new_module(name: str) -> types.ModuleType: ...
def get_magic() -> bytes: ...
def get_tag() -> str: ...
def cache_from_source(path: _Path, debug_override: Optional[bool] = ...) -> str: ...
def source_from_cache(path: _Path) -> str: ...
def get_suffixes() -> List[Tuple[str, str, int]]: ...

class NullImporter:
    def __init__(self, path: _Path) -> None: ...
    def find_module(self, fullname: Any) -> None: ...

# PathLike doesn't work for the pathname argument here
def load_source(
    name: str, pathname: str, file: Optional[IO[Any]] = ...
) -> types.ModuleType: ...
def load_compiled(
    name: str, pathname: str, file: Optional[IO[Any]] = ...
) -> types.ModuleType: ...
def load_package(name: str, path: _Path) -> types.ModuleType: ...
def load_module(
    name: str, file: IO[Any], filename: str, details: Tuple[str, str, int]
) -> types.ModuleType: ...

if sys.version_info >= (3, 6):
    def find_module(
        name: str,
        path: Union[None, List[str], List[os.PathLike[str]], List[_Path]] = ...,
    ) -> Tuple[IO[Any], str, Tuple[str, str, int]]: ...

else:
    def find_module(
        name: str, path: Optional[List[str]] = ...
    ) -> Tuple[IO[Any], str, Tuple[str, str, int]]: ...

def reload(module: types.ModuleType) -> types.ModuleType: ...
def init_builtin(name: str) -> Optional[types.ModuleType]: ...
def load_dynamic(
    name: str, path: str, file: Optional[IO[Any]] = ...
) -> types.ModuleType: ...
