from typing import Dict, Iterable, Type

from .descriptor import EnumDescriptor, FileDescriptor
from .message import Message
from .message_factory import MessageFactory

class SymbolDatabase(MessageFactory):
    def RegisterMessage(self, message: Type[Message]) -> Type[Message]: ...
    def RegisterEnumDescriptor(
        self, enum_descriptor: Type[EnumDescriptor]
    ) -> EnumDescriptor: ...
    def RegisterFileDescriptor(
        self, file_descriptor: Type[FileDescriptor]
    ) -> FileDescriptor: ...
    def GetSymbol(self, symbol: bytes) -> Type[Message]: ...
    def GetMessages(self, files: Iterable[bytes]) -> Dict[bytes, Type[Message]]: ...

def Default(): ...
