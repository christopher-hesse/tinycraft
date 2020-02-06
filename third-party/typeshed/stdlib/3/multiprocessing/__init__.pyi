# Stubs for multiprocessing

import sys
from typing import (
    Any,
    Callable,
    Iterable,
    Mapping,
    Optional,
    List,
    Union,
    Sequence,
    Tuple,
    Type,
    overload,
)

from ctypes import _CData
from logging import Logger
from multiprocessing import connection, pool, spawn, synchronize
from multiprocessing.context import (
    AuthenticationError as AuthenticationError,
    BaseContext,
    BufferTooShort as BufferTooShort,
    DefaultContext,
    Process as Process,
    ProcessError as ProcessError,
    SpawnContext,
    TimeoutError as TimeoutError,
)
from multiprocessing.managers import SyncManager
from multiprocessing.process import (
    active_children as active_children,
    current_process as current_process,
)
from multiprocessing.queues import (
    Queue as Queue,
    SimpleQueue as SimpleQueue,
    JoinableQueue as JoinableQueue,
)
from multiprocessing.spawn import freeze_support as freeze_support
from multiprocessing.spawn import set_executable as set_executable

if sys.version_info >= (3, 8):
    from multiprocessing.process import parent_process as parent_process
    from typing import Literal
else:
    from typing_extensions import Literal

if sys.platform != "win32":
    from multiprocessing.context import ForkContext, ForkServerContext

# N.B. The functions below are generated at runtime by partially applying
# multiprocessing.context.BaseContext's methods, so the two signatures should
# be identical (modulo self).

# Sychronization primitives
_LockLike = Union[synchronize.Lock, synchronize.RLock]

def Barrier(
    parties: int,
    action: Optional[Callable[..., Any]] = ...,
    timeout: Optional[float] = ...,
) -> synchronize.Barrier: ...
def BoundedSemaphore(value: int = ...) -> synchronize.BoundedSemaphore: ...
def Condition(lock: Optional[_LockLike] = ...) -> synchronize.Condition: ...
def Event(lock: Optional[_LockLike] = ...) -> synchronize.Event: ...
def Lock() -> synchronize.Lock: ...
def RLock() -> synchronize.RLock: ...
def Semaphore(value: int = ...) -> synchronize.Semaphore: ...
def Pipe(duplex: bool = ...) -> Tuple[connection.Connection, connection.Connection]: ...
def Pool(
    processes: Optional[int] = ...,
    initializer: Optional[Callable[..., Any]] = ...,
    initargs: Iterable[Any] = ...,
    maxtasksperchild: Optional[int] = ...,
) -> pool.Pool: ...

class Array:
    value: Any = ...
    def __init__(
        self,
        typecode_or_type: Union[str, Type[_CData]],
        size_or_initializer: Union[int, Sequence[Any]],
        *,
        lock: Union[bool, _LockLike] = ...,
    ) -> None: ...
    def acquire(self) -> bool: ...
    def release(self) -> bool: ...
    def get_lock(self) -> _LockLike: ...
    def get_obj(self) -> Any: ...
    @overload
    def __getitem__(self, key: int) -> Any: ...
    @overload
    def __getitem__(self, key: slice) -> List[Any]: ...
    def __getslice__(self, start: int, stop: int) -> Any: ...
    def __setitem__(self, key: int, value: Any) -> None: ...

class Value:
    value: Any = ...
    def __init__(
        self,
        typecode_or_type: Union[str, Type[_CData]],
        *args: Any,
        lock: Union[bool, _LockLike] = ...,
    ) -> None: ...
    def get_lock(self) -> _LockLike: ...
    def get_obj(self) -> Any: ...
    def acquire(self) -> bool: ...
    def release(self) -> bool: ...

# ----- multiprocessing function stubs -----
def allow_connection_pickling() -> None: ...
def cpu_count() -> int: ...
def get_logger() -> Logger: ...
def log_to_stderr(level: Optional[Union[str, int]] = ...) -> Logger: ...
def Manager() -> SyncManager: ...
def set_forkserver_preload(module_names: List[str]) -> None: ...
def get_all_start_methods() -> List[str]: ...
def get_start_method(allow_none: bool = ...) -> Optional[str]: ...
def set_start_method(method: str, force: Optional[bool] = ...) -> None: ...

if sys.platform != "win32":
    @overload
    def get_context(method: None = ...) -> DefaultContext: ...
    @overload
    def get_context(method: Literal["spawn"]) -> SpawnContext: ...
    @overload
    def get_context(method: Literal["fork"]) -> ForkContext: ...
    @overload
    def get_context(method: Literal["forkserver"]) -> ForkServerContext: ...
    @overload
    def get_context(method: str) -> BaseContext: ...

else:
    @overload
    def get_context(method: None = ...) -> DefaultContext: ...
    @overload
    def get_context(method: Literal["spawn"]) -> SpawnContext: ...
    @overload
    def get_context(method: str) -> BaseContext: ...
